
#pragma once

#include "Teide/BasicTypes.h"
#include "Teide/Renderer.h"
#include "Teide/Surface.h"
#include "Teide/VulkanBuffer.h"
#include "Teide/VulkanTexture.h"

#include <string>
#include <vector>

namespace Teide
{
class VulkanDevice;
class Queue;

enum class ResourceType : uint8
{
    Texture,
    TextureData,
};
std::string to_string(ResourceType type);

enum class CommandType : uint8
{
    Write,
    Read,
    Render,
    Dispatch,
    Present,
};
std::string to_string(CommandType type);

struct VulkanGraph
{
    struct ResourceNodeRef
    {
        ResourceType type;
        usize index;

        bool operator==(const ResourceNodeRef&) const = default;
    };

    struct CommandNodeRef
    {
        CommandType type;
        usize index;

        bool operator==(const CommandNodeRef&) const = default;
    };

    static auto WriteRef(usize i) -> CommandNodeRef { return {.type = CommandType::Write, .index = i}; }
    static auto ReadRef(usize i) -> CommandNodeRef { return {.type = CommandType::Read, .index = i}; }
    static auto RenderRef(usize i) -> CommandNodeRef { return {.type = CommandType::Render, .index = i}; }
    static auto DispatchRef(usize i) -> CommandNodeRef { return {.type = CommandType::Dispatch, .index = i}; }
    static auto PresentRef(usize i) -> CommandNodeRef { return {.type = CommandType::Present, .index = i}; }
    static auto TextureRef(usize i) -> ResourceNodeRef { return {.type = ResourceType::Texture, .index = i}; }
    static auto TextureDataRef(usize i) -> ResourceNodeRef { return {.type = ResourceType::TextureData, .index = i}; }

    struct CommandNode
    {
        uint32 index;
    };

    struct WriteNode : CommandNode
    {
        static constexpr auto NodeType = CommandType::Write;

        std::string name;
        ResourceNodeRef source;
        ResourceNodeRef target;
        VulkanBufferData stagingBuffer;
    };

    struct ReadNode : CommandNode
    {
        static constexpr auto NodeType = CommandType::Read;

        std::string name;
        ResourceNodeRef source;
        ResourceNodeRef target;
        VulkanBufferData stagingBuffer;
    };

    struct RenderNode : CommandNode
    {
        static constexpr auto NodeType = CommandType::Render;

        RenderList renderList;
        RenderTargetInfo renderTargetInfo;
        std::optional<ResourceNodeRef> colorTarget;
        std::optional<ResourceNodeRef> depthStencilTarget;
        std::vector<ResourceNodeRef> dependencies;
    };

    struct DispatchNode : CommandNode
    {
        std::string name;
        Kernel kernel;
        std::vector<ResourceNodeRef> dependencies;
        std::vector<ResourceNodeRef> outputs;
    };

    struct PresentNode : CommandNode
    {
        static constexpr auto NodeType = CommandType::Present;

        std::string name;
        ResourceNodeRef source;
    };

    struct TextureNode
    {
        static constexpr auto NodeType = ResourceType::Texture;

        Texture texture;
        CommandNodeRef source;
        vk::ImageUsageFlags usage;

        struct StateChange
        {
            uint32 nodeIndex;
            TextureState expectedState;
            TextureState ensuredState;
        };

        std::vector<StateChange> stateChanges;

        void AddStateChange(uint32 nodeIndex, TextureState expectedState, TextureState ensuredState);
        auto GetStateTransition(uint32 nodeIndex) const -> std::optional<TextureStateTransition>;

        std::string_view GetName() const { return texture.GetName(); }
    };

    struct TextureDataNode
    {
        static constexpr auto NodeType = ResourceType::TextureData;

        std::string name;
        TextureData data;
        std::optional<CommandNodeRef> source;

        std::string_view GetName() const { return name; }
    };

    std::vector<WriteNode> writeNodes;
    std::vector<ReadNode> readNodes;
    std::vector<RenderNode> renderNodes;
    std::vector<DispatchNode> dispatchNodes;
    std::vector<PresentNode> presentNodes;
    uint32 commandNodeCount = 0;

    std::vector<TextureNode> textureNodes;
    std::vector<TextureDataNode> textureDataNodes;

    static constexpr auto NodeLists = std::tuple{
        &VulkanGraph::writeNodes,   &VulkanGraph::readNodes,        &VulkanGraph::renderNodes,
        &VulkanGraph::textureNodes, &VulkanGraph::textureDataNodes,
    };

    auto NewCommandNode() { return CommandNode{.index = commandNodeCount++}; }

    auto AddWriteNode(ResourceNodeRef source, ResourceNodeRef target)
    {
        auto name = fmt::format("write{}", writeNodes.size() + 1);
        AddUsage(target, vk::ImageUsageFlagBits::eTransferDst);
        writeNodes.emplace_back(NewCommandNode(), std::move(name), source, target);
        const auto r = WriteRef(writeNodes.size() - 1);
        SetSource(target, r);
        return r;
    }

    auto AddReadNode(ResourceNodeRef source, ResourceNodeRef target)
    {
        auto name = fmt::format("read{}", readNodes.size() + 1);
        AddUsage(source, vk::ImageUsageFlagBits::eTransferSrc);
        readNodes.emplace_back(NewCommandNode(), std::move(name), source, target);
        const auto r = ReadRef(readNodes.size() - 1);
        SetSource(target, r);
        return r;
    }

    auto AddDispatchNode(Kernel kernel, std::vector<ResourceNodeRef> inputs, std::vector<ResourceNodeRef> outputs)
    {
        auto name = fmt::format("dispatch{}", dispatchNodes.size() + 1);
        const auto& node = dispatchNodes.emplace_back(
            NewCommandNode(), std::move(name), std::move(kernel), std::move(inputs), std::move(outputs));
        const auto r = DispatchRef(dispatchNodes.size() - 1);

        for (const auto input : node.dependencies)
        {
            AddUsage(input, vk::ImageUsageFlagBits::eSampled);
        }
        for (const auto output : node.outputs)
        {
            AddUsage(output, vk::ImageUsageFlagBits::eStorage);
            SetSource(output, r);
        }
        return r;
    }

    auto AddPresentNode(ResourceNodeRef source)
    {
        auto name = fmt::format("present{}", presentNodes.size() + 1);
        presentNodes.emplace_back(NewCommandNode(), std::move(name), source);
        return PresentRef(presentNodes.size() - 1);
    }

    auto AddRenderNode(RenderList renderList, std::optional<ResourceNodeRef> colorTarget, std::optional<ResourceNodeRef> depthStencilTarget)
    {
        auto renderTargetInfo = RenderTargetInfo{};
        const auto r = RenderRef(renderNodes.size());

        if (colorTarget)
        {
            AddUsage(*colorTarget, vk::ImageUsageFlagBits::eColorAttachment);
            SetSource(*colorTarget, r);
            const auto& tex = Get<TextureNode>(colorTarget->index);
            renderTargetInfo.size = tex.texture.GetSize();
            renderTargetInfo.framebufferLayout.colorFormat = tex.texture.GetFormat();
        }

        if (depthStencilTarget)
        {
            AddUsage(*depthStencilTarget, vk::ImageUsageFlagBits::eDepthStencilAttachment);
            SetSource(*depthStencilTarget, r);
            const auto& tex = Get<TextureNode>(depthStencilTarget->index);
            renderTargetInfo.size = tex.texture.GetSize();
            renderTargetInfo.framebufferLayout.depthStencilFormat = tex.texture.GetFormat();
        }

        renderNodes.emplace_back(NewCommandNode(), std::move(renderList), renderTargetInfo, colorTarget, depthStencilTarget);
        return r;
    }

    auto AddTextureNode(Texture texture)
    {
        textureNodes.emplace_back(std::move(texture));
        return TextureRef(textureNodes.size() - 1);
    }

    auto AddTextureDataNode(std::string name, TextureData texture)
    {
        textureDataNodes.emplace_back(std::move(name), std::move(texture));
        return TextureDataRef(textureDataNodes.size() - 1);
    }

    void AddStateChange(ResourceNodeRef ref, uint32 nodeIndex, TextureState expectedState, TextureState ensuredState);

    std::string_view GetNodeName(ResourceNodeRef ref);

    void ForEachPresentNode(auto f)
    {
        for (auto i = VulkanGraph::PresentRef(0); i.index < presentNodes.size(); i.index++)
        {
            const auto& node = presentNodes[i.index];
            f(i, fmt::format("present{}", i.index + 1), std::span(&node.source, 1));
        }
    }

    void ForEachCommandNode(auto f)
    {
        for (auto& node : writeNodes)
        {
            f(node);
        }
        for (auto& node : readNodes)
        {
            f(node);
        }
        for (auto& node : renderNodes)
        {
            f(node);
        }
        for (auto& node : dispatchNodes)
        {
            f(node);
        }
        for (auto& node : presentNodes)
        {
            f(node);
        }
    }

    template <class T>
    T& Get(usize index)
    {
        using List = std::vector<T>;
        using MemPtrType = List VulkanGraph::*;
        constexpr auto MemPtr = std::get<MemPtrType>(NodeLists);
        auto& list = this->*MemPtr;
        TEIDE_ASSERT(index < list.size());
        return list[index];
    }

    template <class T>
    T& Get(ResourceNodeRef ref)
    {
        return Get<T>(ref.index);
    }

    template <class T>
    T* GetIf(CommandNodeRef ref)
    {
        return (ref.type == T::NodeType) ? &Get<T>(ref.index) : nullptr;
    }

    template <class T>
    T* GetIf(ResourceNodeRef ref)
    {
        return (ref.type == T::NodeType) ? &Get<T>(ref.index) : nullptr;
    }

private:
    void AddUsage(ResourceNodeRef ref, vk::ImageUsageFlags usage)
    {
        if (auto* texture = GetIf<TextureNode>(ref))
        {
            texture->usage |= usage;
        }
    }

    void SetSource(ResourceNodeRef ref, CommandNodeRef source)
    {
        switch (ref.type)
        {
            case ResourceType::Texture: Get<TextureNode>(ref.index).source = source; break;
            case ResourceType::TextureData: Get<TextureDataNode>(ref.index).source = source; break;
        }
    }
};

void BuildGraph(VulkanGraph& graph, VulkanDevice& device);
std::string VisualizeGraph(VulkanGraph& graph);
void ExecuteGraph(VulkanGraph& graph, VulkanDevice& device, Queue& queue);

} // namespace Teide
