
#include "Teide/VulkanGraph.h"

#include "Teide/Definitions.h"
#include "Teide/Format.h"
#include "Teide/Util/ThreadUtils.h"
#include "Teide/Vulkan.h"
#include "Teide/VulkanDevice.h"
#include "Teide/VulkanRenderer.h"

#include <fmt/core.h>
#include <fmt/format.h>
#include <vkex/vkex.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>

#include <algorithm>
#include <functional>
#include <ranges>

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

void VulkanGraph::WriteNode::Process(VulkanGraph& graph, VulkanDevice& device, vk::CommandBuffer cmdBuffer)
{
    const auto& sourceNode = graph.Get<VulkanGraph::TextureDataNode>(source.index);
    auto& targetNode = graph.Get<VulkanGraph::TextureNode>(target.index);

    VulkanTexture& texture = device.GetImpl(targetNode.texture);

    if (auto transition = targetNode.GetStateTransition(index))
    {
        texture.Transition(cmdBuffer, *transition);
    }

    spdlog::info("Copy texture data to texture: {} -> {}", sourceNode.GetName(), targetNode.GetName());
    spdlog::info("Size: {}", sourceNode.data.pixels.size());
    const auto bufferSize = GetByteSize(sourceNode.data);
    stagingBuffer = device.CreateBufferUninitialized(
        bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
        vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);
    std::ranges::fill(stagingBuffer.mappedData, std::byte{0xab});

    const auto& data = stagingBuffer.mappedData;
    std::ranges::copy(sourceNode.data.pixels, data.data());

    // Copy staging buffer to image
    const auto imageExtent = vk::Extent3D{.width = sourceNode.data.size.x, .height = sourceNode.data.size.y, .depth = 1};
    CopyBufferToImage(cmdBuffer, stagingBuffer.buffer.get(), texture.image.get(), sourceNode.data.format, imageExtent);
}

void VulkanGraph::ReadNode::Process(VulkanGraph& graph, VulkanDevice& device, vk::CommandBuffer cmdBuffer)
{
    if (completion.expired())
    {
        // If the weak pointer to the completion state has expired, that means the sender was destroyed, so there's no need to do any work for this node since the result will be lost anyway.
        return;
    }

    auto& sourceNode = graph.Get<VulkanGraph::TextureNode>(source.index);

    spdlog::info("Copy texture to texture data: {}", sourceNode.GetName());

    const VulkanTexture& texture = device.GetImpl(sourceNode.texture);

    if (const auto transition = sourceNode.GetStateTransition(index))
    {
        texture.Transition(cmdBuffer, *transition);
    }

    const auto data = TextureData{
        .size = texture.properties.size,
        .format = texture.properties.format,
        .mipLevelCount = texture.properties.mipLevelCount,
        .sampleCount = texture.properties.sampleCount,
    };

    const auto bufferSize = GetByteSize(data);
    spdlog::info("Size: {}", bufferSize);
    stagingBuffer = device.CreateBufferUninitialized(
        bufferSize, vk::BufferUsageFlagBits::eTransferDst,
        vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessRandom);
    std::ranges::fill(stagingBuffer.mappedData, std::byte{0xef});

    // Copy image to staging buffer
    const vk::Extent3D extent = {
        .width = texture.properties.size.x,
        .height = texture.properties.size.y,
        .depth = 1,
    };
    CopyImageToBuffer(
        cmdBuffer, texture.image.get(), stagingBuffer.buffer.get(), texture.properties.format, extent,
        texture.properties.mipLevelCount);
}

void VulkanGraph::RenderNode::Process(VulkanGraph& graph, VulkanDevice& device, vk::CommandBuffer cmdBuffer)
{
    if (colorTarget && depthStencilTarget)
    {
        spdlog::info(
            "Render to color texture: {}, depth/stencil texture: {}", graph.GetNodeName(*colorTarget),
            graph.GetNodeName(*depthStencilTarget));
    }
    else if (colorTarget)
    {
        spdlog::info("Render to color texture: {}", graph.GetNodeName(*colorTarget));
    }
    else if (depthStencilTarget)
    {
        spdlog::info("Render to depth/stencil texture: {}", graph.GetNodeName(*depthStencilTarget));
    }

    // Transition image dependencies
    for (const auto dep : dependencies)
    {
        if (const auto* depNode = graph.GetIf<VulkanGraph::TextureNode>(dep))
        {
            if (auto transition = depNode->GetStateTransition(index))
            {
                device.GetImpl(depNode->texture).Transition(cmdBuffer, *transition);
            }
        }
    }

    const auto attachments = vkex::Join(colorTarget, depthStencilTarget)
        | std::views::transform([&graph, &device](auto ref) {
                                 return device.GetImpl(graph.Get<VulkanGraph::TextureNode>(ref).texture).imageView.get();
                             })
        | std::ranges::to<std::vector<vk::ImageView>>();

    const auto& renderTarget = renderTargetInfo;
    const RenderPassDesc renderPassDesc = {
        .framebufferLayout = renderTarget.framebufferLayout,
        .renderOverrides = renderList.renderOverrides,
    };

    const auto renderPass = device.CreateRenderPass(renderTarget.framebufferLayout, renderList.clearState);
    const auto framebuffer
        = device.CreateFramebuffer(renderPass, renderTarget.framebufferLayout, renderTarget.size, attachments);

    VulkanRenderer::RecordRenderListCommands(device, cmdBuffer, renderList, renderPass, renderPassDesc, framebuffer);
}

void VulkanGraph::DispatchNode::Process(VulkanGraph& graph, VulkanDevice& device, vk::CommandBuffer cmdBuffer)
{
    // TODO
    static_cast<void>(this);
    static_cast<void>(graph);
    static_cast<void>(device);
    static_cast<void>(cmdBuffer);
}

void VulkanGraph::PresentNode::Process(VulkanGraph& graph, VulkanDevice& device, vk::CommandBuffer cmdBuffer)
{
    // TODO
    static_cast<void>(this);
    static_cast<void>(graph);
    static_cast<void>(device);
    static_cast<void>(cmdBuffer);
}

void VulkanGraph::TextureNode::AddStateChange(uint32 nodeIndex, TextureState expectedState, TextureState ensuredState)
{
    stateChanges.insert(
        std::ranges::lower_bound(stateChanges, nodeIndex, std::less(), &StateChange::nodeIndex),
        {nodeIndex, expectedState, ensuredState});
}

auto VulkanGraph::TextureNode::GetStateTransition(uint32 nodeIndex) const -> std::optional<TextureStateTransition>
{
    const auto it = std::ranges::lower_bound(stateChanges, nodeIndex, std::less(), &StateChange::nodeIndex);
    if (it != stateChanges.end())
    {
        const auto oldState = it == stateChanges.begin() ? TextureState{} : (it - 1)->ensuredState;
        const auto newState = it->expectedState;
        return TextureStateTransition{.from = oldState, .to = newState};
    }
    return std::nullopt;
}

void VulkanGraph::AddStateChange(ResourceNodeRef ref, uint32 nodeIndex, TextureState expectedState, TextureState ensuredState)
{
    if (auto* node = GetIf<TextureNode>(ref))
    {
        node->AddStateChange(nodeIndex, expectedState, ensuredState);
    }
}

std::string_view VulkanGraph::GetNodeName(ResourceNodeRef ref)
{
    switch (ref.type)
    {
        case ResourceType::Texture: return textureNodes.at(ref.index).texture.GetName();
        case ResourceType::TextureData: return textureDataNodes.at(ref.index).name;
    }
    Unreachable();
}

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
        case CommandType::Dispatch: return "Dispatch";
        case CommandType::Present: return "Present";
    }
    return "";
}

void BuildGraph(VulkanGraph& graph, VulkanDevice& device)
{
    for (const auto& node : graph.writeNodes)
    {
        const auto state = TextureState{
            .layout = vk::ImageLayout::eTransferDstOptimal,
            .lastPipelineStageUsage = vk::PipelineStageFlagBits::eTransfer,
        };
        graph.AddStateChange(node.target, node.index, state, state);
    }

    for (const auto& node : graph.readNodes)
    {
        const auto state = TextureState{
            .layout = vk::ImageLayout::eTransferSrcOptimal,
            .lastPipelineStageUsage = vk::PipelineStageFlagBits::eTransfer,
        };
        graph.AddStateChange(node.source, node.index, state, state);
    }

    for (auto& node : graph.renderNodes)
    {
        AddTextureDependencies(node, node.renderList.viewParameters.textures, graph);

        for (const RenderObject& obj : node.renderList.objects)
        {
            AddTextureDependencies(node, obj.objectParameters.textures, graph);
            const auto& matParams = device.GetImpl(obj.materialParameters);
            AddTextureDependencies(node, matParams.textures, graph);
        }

        for (const auto dep : node.dependencies)
        {
            const auto state = TextureState{
                .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
                .lastPipelineStageUsage = vk::PipelineStageFlagBits::eFragmentShader,
            };
            graph.AddStateChange(dep, node.index, state, state);
        }

        if (node.colorTarget)
        {
            graph.AddStateChange(
                *node.colorTarget, node.index, {},
                {
                    .layout = vk::ImageLayout::eColorAttachmentOptimal,
                    .lastPipelineStageUsage = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                });
        }

        if (node.depthStencilTarget)
        {
            graph.AddStateChange(
                *node.depthStencilTarget, node.index, {},
                {
                    .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
                    .lastPipelineStageUsage
                    = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
                });
        }
    }

    for (const auto& node : graph.dispatchNodes)
    {
        for (const auto dep : node.dependencies)
        {
            const auto state = TextureState{
                .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
                .lastPipelineStageUsage = vk::PipelineStageFlagBits::eComputeShader,
            };
            graph.AddStateChange(dep, node.index, state, state);
        }

        for (const auto dep : node.outputs)
        {
            const auto state = TextureState{
                .layout = vk::ImageLayout::eGeneral,
                .lastPipelineStageUsage = vk::PipelineStageFlagBits::eComputeShader,
            };
            graph.AddStateChange(dep, node.index, state, state);
        }
    }

    for (const auto& node : graph.presentNodes)
    {
        const auto state = TextureState{
            .layout = vk::ImageLayout::ePresentSrcKHR,
            .lastPipelineStageUsage = vk::PipelineStageFlagBits::eTopOfPipe,
        };
        graph.AddStateChange(node.source, node.index, state, state);
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

    for (const auto& node : graph.writeNodes)
    {
        fmt::format_to(out, "    {} -> {}\n", node.name, graph.GetNodeName(node.target));
        fmt::format_to(out, "    {} -> {}\n", graph.GetNodeName(node.source), node.name);
    }
    for (const auto& node : graph.readNodes)
    {
        fmt::format_to(out, "    {} -> {}\n", graph.GetNodeName(node.source), node.name);
    }
    for (const auto& node : graph.renderNodes)
    {
        const auto& name = node.renderList.name;
        if (node.colorTarget)
        {
            fmt::format_to(out, "    {} -> {}\n", name, graph.GetNodeName(*node.colorTarget));
        }
        if (node.depthStencilTarget)
        {
            fmt::format_to(out, "    {} -> {}\n", name, graph.GetNodeName(*node.depthStencilTarget));
        }
        for (const auto& dep : node.dependencies)
        {
            fmt::format_to(out, "    {} -> {}\n", graph.GetNodeName(dep), name);
        }
    }
    for (const auto& node : graph.dispatchNodes)
    {
        for (const auto& input : node.outputs)
        {
            fmt::format_to(out, "    {} -> {}\n", node.name, graph.GetNodeName(input));
        }
        for (const auto& dep : node.dependencies)
        {
            fmt::format_to(out, "    {} -> {}\n", graph.GetNodeName(dep), node.name);
        }
    }
    for (const auto& node : graph.presentNodes)
    {
        // fmt::format_to(out, "    {}\n", node.name);
        fmt::format_to(out, "    {} -> {}\n", graph.GetNodeName(node.source), node.name);
    }

    ret += '}';
    return ret;
}

struct Commands
{
    vk::UniqueCommandPool pool;
    std::vector<vk::CommandBuffer> cmdBuffers;
};

namespace
{
    auto RecordCommands(VulkanGraph& graph, VulkanDevice& device) -> Commands
    {
        Commands ret;

        // Set correct texture properties for write node targets
        for (const auto& writeNode : graph.writeNodes)
        {
            auto& targetNode = graph.Get<VulkanGraph::TextureNode>(writeNode.target);
            auto& targetProps = device.GetImpl(targetNode.texture).properties;
            const auto& sourceData = graph.Get<VulkanGraph::TextureDataNode>(writeNode.source).data;
            targetProps.size = sourceData.size;
            targetProps.format = sourceData.format;
            targetProps.mipLevelCount = sourceData.mipLevelCount;
            targetProps.sampleCount = sourceData.sampleCount;
        }

        // Create transient textures
        for (VulkanGraph::TextureNode& node : graph.textureNodes)
        {
            VulkanTexture& texture = device.GetImpl(node.texture);
            texture.usage = node.usage;
            device.CreateTextureImpl(texture);
        }

        // Record command buffers
        auto vkdevice = device.GetVulkanDevice();
        ret.pool = vkdevice.createCommandPoolUnique({
            .flags = vk::CommandPoolCreateFlagBits::eTransient,
            .queueFamilyIndex = device.GetQueueFamilies().graphicsFamily,
        });

        ret.cmdBuffers = vkdevice.allocateCommandBuffers(
            {.commandPool = ret.pool.get(), .commandBufferCount = graph.commandNodeCount});

        graph.ForEachCommandNode([&](auto& node) {
            auto& cmdBuffer = ret.cmdBuffers[node.index];
            cmdBuffer.begin({.flags = {vk::CommandBufferUsageFlagBits::eOneTimeSubmit}});
            node.Process(graph, device, cmdBuffer);
            cmdBuffer.end();
        });

        return ret;
    }
} // namespace

void ExecuteGraph(VulkanGraph& graph, VulkanDevice& device, Queue& queue)
{
    BuildGraph(graph, device);

    auto commands = RecordCommands(graph, device);

    const auto snd = queue.LazySubmit(commands.cmdBuffers);
    ex::sync_wait(snd);

    for (auto& readNode : graph.readNodes)
    {
        auto& sourceNode = graph.Get<VulkanGraph::TextureNode>(readNode.source.index);
        const VulkanTexture& texture = device.GetImpl(sourceNode.texture);

        const auto& data = readNode.stagingBuffer.mappedData;

        const auto result = TextureData{
            .size = texture.properties.size,
            .format = texture.properties.format,
            .mipLevelCount = texture.properties.mipLevelCount,
            .sampleCount = texture.properties.sampleCount,
            .pixels = data | std::ranges::to<std::vector>(),
        };

        if (const auto comp = readNode.completion.lock())
        {
            comp->SetValue(result);
        }
    }
}

} // namespace Teide
