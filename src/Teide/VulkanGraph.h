
#pragma once

#include "Teide/BasicTypes.h"
#include "Teide/Renderer.h"
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
    static auto TextureRef(usize i) -> ResourceNodeRef { return {.type = ResourceType::Texture, .index = i}; }
    static auto TextureDataRef(usize i) -> ResourceNodeRef { return {.type = ResourceType::TextureData, .index = i}; }

    struct WriteNode
    {
        static constexpr auto NodeType = CommandType::Write;

        ResourceNodeRef source;
        ResourceNodeRef target;
        VulkanBufferData stagingBuffer;
    };

    struct ReadNode
    {
        static constexpr auto NodeType = CommandType::Read;

        ResourceNodeRef source;
        ResourceNodeRef target;
        VulkanBufferData stagingBuffer;
    };

    struct RenderNode
    {
        static constexpr auto NodeType = CommandType::Render;

        RenderList renderList;
        RenderTargetInfo renderTargetInfo;
        std::optional<ResourceNodeRef> colourTarget;
        std::optional<ResourceNodeRef> depthStencilTarget;
        std::vector<ResourceNodeRef> dependencies;
    };

    struct DispatchNode
    {
        Kernel kernel;
        std::vector<ResourceNodeRef> dependencies;
        std::vector<ResourceNodeRef> outputs;
    };

    struct TextureNode
    {
        static constexpr auto NodeType = ResourceType::Texture;

        Texture texture;
        CommandNodeRef source;
        TextureState state;

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
    std::vector<TextureNode> textureNodes;
    std::vector<TextureDataNode> textureDataNodes;

    static constexpr auto NodeLists = std::tuple{
        &VulkanGraph::writeNodes,   &VulkanGraph::readNodes,        &VulkanGraph::renderNodes,
        &VulkanGraph::textureNodes, &VulkanGraph::textureDataNodes,
    };

    auto AddWriteNode(ResourceNodeRef source, ResourceNodeRef target)
    {
        writeNodes.emplace_back(source, target);
        const auto r = WriteRef(writeNodes.size() - 1);
        SetSource(target, r);
        return r;
    }

    auto AddReadNode(ResourceNodeRef source, ResourceNodeRef target)
    {
        readNodes.emplace_back(source, target);
        const auto r = ReadRef(readNodes.size() - 1);
        SetSource(target, r);
        return r;
    }

    auto AddDispatchNode(Kernel kernel, std::vector<ResourceNodeRef> inputs, std::vector<ResourceNodeRef> outputs)
    {
        const auto& node = dispatchNodes.emplace_back(std::move(kernel), std::move(inputs), std::move(outputs));
        const auto r = DispatchRef(dispatchNodes.size() - 1);
        for (const auto output : node.outputs)
        {
            SetSource(output, r);
        }
        return r;
    }

    auto AddRenderNode(RenderList renderList, std::optional<ResourceNodeRef> colorTarget, std::optional<ResourceNodeRef> depthStencilTarget)
    {
        auto renderTargetInfo = RenderTargetInfo{};
        const auto r = RenderRef(renderNodes.size());
        if (colorTarget)
        {
            SetSource(*colorTarget, r);
            const auto& tex = Get<TextureNode>(colorTarget->index);
            renderTargetInfo.size = tex.texture.GetSize();
            renderTargetInfo.framebufferLayout.colorFormat = tex.texture.GetFormat();
        }
        if (depthStencilTarget)
        {
            SetSource(*depthStencilTarget, r);
            const auto& tex = Get<TextureNode>(depthStencilTarget->index);
            renderTargetInfo.size = tex.texture.GetSize();
            renderTargetInfo.framebufferLayout.depthStencilFormat = tex.texture.GetFormat();
        }

        renderNodes.emplace_back(std::move(renderList), renderTargetInfo, colorTarget, depthStencilTarget);
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

    std::string_view GetNodeName(ResourceNodeRef ref);
    std::optional<std::string_view> FindNodeWithSource(CommandNodeRef source);

    void ForEachWriteNode(auto f)
    {
        for (auto i = VulkanGraph::WriteRef(0); i.index < writeNodes.size(); i.index++)
        {
            const auto& node = writeNodes[i.index];
            f(i, fmt::format("write{}", i.index + 1), std::span(&node.source, 1));
        }
    };

    void ForEachReadNode(auto f)
    {
        for (auto i = VulkanGraph::ReadRef(0); i.index < readNodes.size(); i.index++)
        {
            const auto& node = readNodes[i.index];
            f(i, fmt::format("read{}", i.index + 1), std::span(&node.source, 1));
        }
    };

    void ForEachRenderNode(auto f)
    {
        for (auto i = VulkanGraph::RenderRef(0); i.index < renderNodes.size(); i.index++)
        {
            const auto& node = renderNodes[i.index];
            f(i, node.renderList.name, node.dependencies);
        }
    };

    void ForEachDispatchNode(auto f)
    {
        for (auto i = VulkanGraph::DispatchRef(0); i.index < dispatchNodes.size(); i.index++)
        {
            const auto& node = dispatchNodes[i.index];
            f(i, fmt::format("dispatch{}", i.index + 1), node.dependencies);
        }
    };

    void ForEachCommandNode(auto f)
    {
        ForEachWriteNode(f);
        ForEachReadNode(f);
        ForEachRenderNode(f);
        ForEachDispatchNode(f);
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
