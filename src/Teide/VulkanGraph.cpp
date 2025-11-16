
#include "Teide/VulkanGraph.h"

#include "Teide/Vulkan.h"
#include "Teide/VulkanDevice.h"

#include <fmt/core.h>
#include <stdexec/__detail/__sync_wait.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>

#include <algorithm>
#include <functional>

namespace Teide
{
namespace
{
    auto FindTexture(const VulkanGraph& graph, const Texture& texture) -> std::optional<VulkanGraph::ResourceNodeRef>
    {
        const auto it = std::ranges::find(graph.textureNodes, texture, &VulkanGraph::TextureNode::texture);
        if (it != graph.textureNodes.end())
        {
            return VulkanGraph::TextureRef(static_cast<usize>(std::distance(graph.textureNodes.begin(), it)));
        }
        return std::nullopt;
    }

    void AddTextureDependencies(VulkanGraph::RenderNode& node, std::span<const Texture> textures, const VulkanGraph& graph)
    {
        for (const Texture& texture : textures)
        {
            if (const auto ref = FindTexture(graph, texture))
            {
                node.dependencies.push_back(*ref);
            }
        }
    }
} // namespace


std::string to_string(ResourceType type)
{
    switch (type)
    {
        case ResourceType::Texture: return "Texture";
        case ResourceType::TextureData: return "TextureData";
    }
    return "";
}
std::string to_string(CommandType type)
{
    switch (type)
    {
        case CommandType::Write: return "Write";
        case CommandType::Read: return "Read";
        case CommandType::Render: return "Render";
    }
    return "";
}


void BuildGraph(VulkanGraph& graph, VulkanDevice& device)
{
    for (VulkanGraph::RenderNode& node : graph.renderNodes)
    {
        AddTextureDependencies(node, node.renderList.viewParameters.textures, graph);

        for (const RenderObject& obj : node.renderList.objects)
        {
            AddTextureDependencies(node, obj.objectParameters.textures, graph);
            const auto& matParams = device.GetImpl(obj.materialParameters);
            AddTextureDependencies(node, matParams.textures, graph);
        }
    }
}

std::string VisualizeGraph(VulkanGraph& graph)
{
    std::string ret = "strict digraph {\n"
                      "    rankdir=LR\n"
                      "    node [shape=box]\n";
    auto out = std::back_inserter(ret);
    for (const auto& node : graph.textureNodes)
    {
        fmt::format_to(out, "    {}\n", node.texture.GetName());
    }
    for (const auto& node : graph.textureDataNodes)
    {
        fmt::format_to(out, "    {}\n", node.name);
    }
    fmt::format_to(out, "    node [shape=box, margin=0.5]\n");

    for (auto i = VulkanGraph::RenderRef(0); i.index < graph.renderNodes.size(); i.index++)
    {
        const auto& node = graph.renderNodes[i.index];

        if (const auto it = std::ranges::find(graph.textureNodes, i, &VulkanGraph::TextureNode::source);
            it != graph.textureNodes.end())
        {
            fmt::format_to(out, "    {} -> {}\n", node.renderList.name, it->texture.GetName());
        }
        else
        {
            fmt::format_to(out, "    {}\n", node.renderList.name);
        }

        for (const auto& dep : node.dependencies)
        {
            switch (dep.type)
            {
                case ResourceType::Texture:
                    fmt::format_to(
                        out, "    {} -> {}\n", graph.textureNodes.at(dep.index).texture.GetName(), node.renderList.name);
                    break;
                case ResourceType::TextureData:
                    fmt::format_to(out, "    {} -> {}\n", graph.textureDataNodes.at(dep.index).name, node.renderList.name);
                    break;
            }
        }
    }

    int copyIndex = 0;
    for (const auto& node : graph.writeNodes)
    {
        std::string nodeName = fmt::format("copy{}", ++copyIndex);

        const auto& src = graph.Get<VulkanGraph::TextureDataNode>(node.source.index);
        const auto& tgt = graph.Get<VulkanGraph::TextureNode>(node.target.index);
        fmt::format_to(out, "    {} -> {}\n", nodeName, tgt.texture.GetName());
        fmt::format_to(out, "    {} -> {}\n", src.name, nodeName);
    }
    for (const auto& node : graph.readNodes)
    {
        std::string nodeName = fmt::format("copy{}", ++copyIndex);

        const auto& src = graph.Get<VulkanGraph::TextureNode>(node.source.index);
        const auto& tgt = graph.Get<VulkanGraph::TextureDataNode>(node.target.index);
        fmt::format_to(out, "    {} -> {}\n", nodeName, tgt.name);
        fmt::format_to(out, "    {} -> {}\n", src.texture.GetName(), nodeName);
    }
    ret += '}';
    return ret;
}

struct Commands
{
    vk::UniqueCommandPool pool;
    std::vector<vk::CommandBuffer> cmdBuffers;
    std::vector<std::function<void()>> completionFuncs;
};

namespace
{
    void ProcessCommandNode(VulkanGraph& graph, VulkanGraph::WriteNode& writeNode, VulkanDevice& device, vk::CommandBuffer cmdBuffer)
    {
        const auto& sourceNode = graph.Get<VulkanGraph::TextureDataNode>(writeNode.source.index);
        auto& targetNode = graph.Get<VulkanGraph::TextureNode>(writeNode.target.index);

        spdlog::info("Copy texture data to texture: {} -> {}", sourceNode.GetName(), targetNode.GetName());
        spdlog::info("Size: {}", sourceNode.data.pixels.size());
        const auto bufferSize = GetByteSize(sourceNode.data);
        writeNode.stagingBuffer = device.CreateBufferUninitialized(
            bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
            vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);
        std::ranges::fill(writeNode.stagingBuffer.mappedData, std::byte{0xab});

        const auto& data = writeNode.stagingBuffer.mappedData;
        std::ranges::copy(sourceNode.data.pixels, data.data());

        VulkanTexture& texture = device.GetImpl(targetNode.texture);

        // Copy staging buffer to image
        texture.TransitionToTransferDst(targetNode.state, cmdBuffer);
        const auto imageExtent
            = vk::Extent3D{.width = sourceNode.data.size.x, .height = sourceNode.data.size.y, .depth = 1};
        CopyBufferToImage(
            cmdBuffer, writeNode.stagingBuffer.buffer.get(), texture.image.get(), sourceNode.data.format, imageExtent);
    }

    void ProcessCommandNode(
        VulkanGraph& graph, VulkanGraph::ReadNode& readNode, VulkanDevice& device, vk::CommandBuffer cmdBuffer,
        std::vector<std::function<void()>>& completionFuncs)
    {
        auto& sourceNode = graph.Get<VulkanGraph::TextureNode>(readNode.source.index);
        auto& targetNode = graph.Get<VulkanGraph::TextureDataNode>(readNode.target.index);

        spdlog::info("Copy texture to texture data: {} -> {}", sourceNode.GetName(), targetNode.GetName());

        VulkanTexture& texture = device.GetImpl(sourceNode.texture);

        spdlog::info("Copy texture to texture data: {} -> {}", sourceNode.GetName(), targetNode.GetName());

        const VulkanTexture& textureImpl = device.GetImpl(sourceNode.texture);

        targetNode.data = {
            .size = textureImpl.properties.size,
            .format = textureImpl.properties.format,
            .mipLevelCount = textureImpl.properties.mipLevelCount,
            .sampleCount = textureImpl.properties.sampleCount,
        };

        const auto bufferSize = GetByteSize(targetNode.data);
        spdlog::info("Size: {}", bufferSize);
        readNode.stagingBuffer = device.CreateBufferUninitialized(
            bufferSize, vk::BufferUsageFlagBits::eTransferDst,
            vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessRandom);
        std::ranges::fill(readNode.stagingBuffer.mappedData, std::byte{0xef});

        completionFuncs.emplace_back([&readNode, &targetNode] {
            const auto& data = readNode.stagingBuffer.mappedData;

            targetNode.data.pixels.resize(data.size());
            std::ranges::copy(data, targetNode.data.pixels.data());
        });

        // Copy image to staging buffer
        texture.TransitionToTransferSrc(sourceNode.state, cmdBuffer);
        const vk::Extent3D extent = {
            .width = texture.properties.size.x,
            .height = texture.properties.size.y,
            .depth = 1,
        };
        CopyImageToBuffer(
            cmdBuffer, texture.image.get(), readNode.stagingBuffer.buffer.get(), texture.properties.format, extent,
            texture.properties.mipLevelCount);
    }

    auto RecordCommands(VulkanGraph& graph, VulkanDevice& device) -> Commands
    {
        Commands ret;

        // Create transient textures
        for (VulkanGraph::TextureNode& node : graph.textureNodes)
        {
            VulkanTexture& texture = device.GetImpl(node.texture);
            using enum vk::ImageUsageFlagBits;
            node.state = device.CreateTextureImpl(texture, eTransferSrc | eTransferDst | eSampled);
        }

        // Record command buffers
        auto vkdevice = device.GetVulkanDevice();
        ret.pool = vkdevice.createCommandPoolUnique({
            .flags = vk::CommandPoolCreateFlagBits::eTransient,
            .queueFamilyIndex = device.GetQueueFamilies().graphicsFamily,
        });

        ret.cmdBuffers = vkdevice.allocateCommandBuffers({.commandPool = ret.pool.get(), .commandBufferCount = 1});
        auto cmdBuffer = ret.cmdBuffers.front();

        cmdBuffer.begin({.flags = {vk::CommandBufferUsageFlagBits::eOneTimeSubmit}});

        for (auto& writeNode : graph.writeNodes)
        {
            ProcessCommandNode(graph, writeNode, device, cmdBuffer);
        }

        for (auto& readNode : graph.readNodes)
        {
            ProcessCommandNode(graph, readNode, device, cmdBuffer, ret.completionFuncs);
        }

        cmdBuffer.end();

        return ret;
    }
} // namespace

void ExecuteGraph(VulkanGraph& graph, VulkanDevice& device, Queue& queue)
{
    auto commands = RecordCommands(graph, device);
    ex::sync_wait(queue.LazySubmit(commands.cmdBuffers));

    for (const auto& callback : commands.completionFuncs)
    {
        callback();
    }
}

} // namespace Teide
