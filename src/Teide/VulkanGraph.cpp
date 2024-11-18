
#include "Teide/VulkanGraph.h"

#include "Teide/Vulkan.h"
#include "Teide/VulkanDevice.h"

#include <fmt/core.h>

#include <algorithm>

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
        case CommandType::Copy: return "Copy";
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

    for (auto i = VulkanGraph::CopyRef(0); i.index < graph.copyNodes.size(); i.index++)
    {
        const auto& node = graph.copyNodes[i.index];
        std::string nodeName = fmt::format("copy{}", i.index + 1);

        if (const auto it1 = std::ranges::find(graph.textureNodes, i, &VulkanGraph::TextureNode::source);
            it1 != graph.textureNodes.end())
        {
            fmt::format_to(out, "    {} -> {}\n", nodeName, it1->texture.GetName());
        }
        else if (const auto it2 = std::ranges::find(graph.textureDataNodes, i, &VulkanGraph::TextureDataNode::source);
                 it2 != graph.textureDataNodes.end())
        {
            fmt::format_to(out, "    {} -> {}\n", nodeName, it2->name);
        }
        else
        {
            fmt::format_to(out, "    {}\n", nodeName);
        }

        switch (node.source.type)
        {
            case ResourceType::Texture:
                fmt::format_to(out, "    {} -> {}\n", graph.textureNodes.at(node.source.index).texture.GetName(), nodeName);
                break;
            case ResourceType::TextureData:
                fmt::format_to(out, "    {} -> {}\n", graph.textureDataNodes.at(node.source.index).name, nodeName);
                break;
        }
    }
    ret += '}';
    return ret;
}

void ExecuteGraph(VulkanGraph& graph, VulkanDevice& device, VulkanRenderer& renderer)
{
    // 1. Create transient textures
    for (VulkanGraph::TextureNode& node : graph.textureNodes)
    {
        VulkanTexture& texture = device.GetImpl(node.texture);
        using enum vk::ImageUsageFlagBits;
        device.CreateTextureImpl(texture, eTransferSrc | eTransferDst);
    }

    // 2. Create staging buffers
    usize inputStagingBufferSize = 0;
    for (const auto& node : graph.textureNodes)
    {
        if (const auto* copyNode = graph.GetIf<VulkanGraph::CopyNode>(node.source))
        {
            if (const auto* sourceNode = graph.GetIf<VulkanGraph::TextureDataNode>(copyNode->source))
            {
                spdlog::info("Copy texture data to texture: {} -> {}", sourceNode->GetName(), node.GetName());
                spdlog::info("Size: {}", sourceNode->data.pixels.size());
                inputStagingBufferSize += sourceNode->data.pixels.size();
            }
        }
    }

    usize outputStagingBufferSize = 0;
    for (const auto& node : graph.textureDataNodes)
    {
        if (const auto* copyNode = node.source ? graph.GetIf<VulkanGraph::CopyNode>(*node.source) : nullptr)
        {
            if (const auto* sourceNode = graph.GetIf<VulkanGraph::TextureNode>(copyNode->source))
            {
                spdlog::info("Copy texture to texture data: {} -> {}", sourceNode->GetName(), node.GetName());

                const VulkanTexture& textureImpl = device.GetImpl(sourceNode->texture);

                const TextureData textureData = {
                    .size = textureImpl.properties.size,
                    .format = textureImpl.properties.format,
                    .mipLevelCount = textureImpl.properties.mipLevelCount,
                    .sampleCount = textureImpl.properties.sampleCount,
                };

                const auto bufferSize = GetByteSize(textureData);
                outputStagingBufferSize += bufferSize;
                spdlog::info("Size: {}", bufferSize);
            }
        }
    }
    auto inputStagingBuffer = device.CreateBufferUninitialized(
        inputStagingBufferSize, vk::BufferUsageFlagBits::eTransferSrc,
        vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);

    auto outputStagingBuffer = device.CreateBufferUninitialized(
        outputStagingBufferSize, vk::BufferUsageFlagBits::eTransferDst,
        vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessRandom);

    // 3. Record command buffers
    auto vkdevice = device.GetVulkanDevice();
    auto commandPool = vkdevice.createCommandPoolUnique({.queueFamilyIndex = device.GetQueueFamilies().graphicsFamily});

    auto cmdBuffers = vkdevice.allocateCommandBuffersUnique({.commandPool = commandPool.get(), .commandBufferCount = 1});
    auto cmdBuffer = std::move(cmdBuffers.front());
    for (const VulkanGraph::TextureNode& node : graph.textureNodes)
    {
        if (const auto* copyNode = graph.GetIf<VulkanGraph::CopyNode>(node.source))
        {
            if (const auto* sourceNode = graph.GetIf<VulkanGraph::TextureDataNode>(copyNode->source))
            {
                const TextureData& data = sourceNode->data;
                VulkanTexture& texture = device.GetImpl(node.texture);
                using enum vk::ImageUsageFlagBits;
                auto state = device.CreateTextureImpl(texture, eTransferSrc | eTransferDst);

                // Copy staging buffer to image
                TransitionImageLayout(
                    cmdBuffer.get(), texture.image.get(), data.format, data.mipLevelCount, state.layout,
                    vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTopOfPipe,
                    vk::PipelineStageFlagBits::eTransfer);
                state = {
                    .layout = vk::ImageLayout::eTransferDstOptimal,
                    .lastPipelineStageUsage = vk::PipelineStageFlagBits::eTransfer,
                };
                const auto imageExtent = vk::Extent3D{data.size.x, data.size.y, 1};
                CopyBufferToImage(
                    cmdBuffer.get(), inputStagingBuffer.buffer.get(), texture.image.get(), data.format, imageExtent);
            }
        }
    }

    // 4. Submit to GPU executor
    (void)renderer;
}
} // namespace Teide
