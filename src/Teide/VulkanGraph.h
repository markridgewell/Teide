
#pragma once

#include "Teide/BasicTypes.h"
#include "Teide/Renderer.h"

#include <string>
#include <vector>

namespace Teide
{
class VulkanDevice;
class VulkanRenderer;

enum class ResourceType : uint8
{
    Texture,
    TextureData,
};
std::string to_string(ResourceType type);

enum class CommandType : uint8
{
    Copy,
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

    static auto CopyRef(usize i) -> CommandNodeRef { return {.type = CommandType::Copy, .index = i}; }
    static auto RenderRef(usize i) -> CommandNodeRef { return {.type = CommandType::Render, .index = i}; }
    static auto DispatchRef(usize i) -> CommandNodeRef { return {.type = CommandType::Dispatch, .index = i}; }
    static auto TextureRef(usize i) -> ResourceNodeRef { return {.type = ResourceType::Texture, .index = i}; }
    static auto TextureDataRef(usize i) -> ResourceNodeRef { return {.type = ResourceType::TextureData, .index = i}; }

    struct CopyNode
    {
        ResourceNodeRef source;
    };

    struct RenderNode
    {
        RenderList renderList;
        std::vector<ResourceNodeRef> dependencies;
    };

    struct DispatchNode
    {
        Kernel kernel;
        std::vector<ResourceNodeRef> dependencies;
    };

    struct TextureNode
    {
        Texture texture;
        CommandNodeRef source;
    };

    struct TextureDataNode
    {
        std::string name;
        TextureData data;
        std::optional<CommandNodeRef> source;
    };

    std::vector<CopyNode> copyNodes;
    std::vector<RenderNode> renderNodes;
    std::vector<DispatchNode> dispatchNodes;
    std::vector<TextureNode> textureNodes;
    std::vector<TextureDataNode> textureDataNodes;

    auto AddCopyNode(ResourceNodeRef source)
    {
        copyNodes.emplace_back(source);
        return CopyRef(copyNodes.size() - 1);
    }

    auto AddRenderNode(RenderList renderList)
    {
        renderNodes.emplace_back(std::move(renderList));
        return RenderRef(renderNodes.size() - 1);
    }

    auto AddDispatchNode(Kernel kernel)
    {
        dispatchNodes.emplace_back(std::move(kernel));
        return DispatchRef(dispatchNodes.size() - 1);
    }

    auto AddTextureNode(Texture texture, CommandNodeRef source)
    {
        textureNodes.emplace_back(std::move(texture), source);
        return TextureRef(textureNodes.size() - 1);
    }

    auto AddTextureDataNode(std::string name, TextureData texture, std::optional<CommandNodeRef> source = {})
    {
        textureDataNodes.emplace_back(std::move(name), std::move(texture), source);
        return TextureDataRef(textureDataNodes.size() - 1);
    }

    std::string_view GetNodeName(ResourceNodeRef ref);
    std::optional<std::string_view> FindNodeWithSource(CommandNodeRef source);

    void ForEachCopyNode(auto f)
    {
        for (auto i = VulkanGraph::CopyRef(0); i.index < copyNodes.size(); i.index++)
        {
            const auto& node = copyNodes[i.index];
            f(i, fmt::format("copy{}", i.index + 1), std::span(&node.source, 1));
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
        ForEachCopyNode(f);
        ForEachRenderNode(f);
        ForEachDispatchNode(f);
    }
};

void BuildGraph(VulkanGraph& graph, VulkanDevice& device);
std::string VisualizeGraph(VulkanGraph& graph);
void ExecuteGraph(VulkanGraph& graph, VulkanDevice& device, VulkanRenderer& renderer);

} // namespace Teide
