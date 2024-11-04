
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
    static auto TextureRef(usize i) -> ResourceNodeRef { return {.type = ResourceType::Texture, .index = i}; }
    static auto TextureDataRef(usize i) -> ResourceNodeRef { return {.type = ResourceType::TextureData, .index = i}; }

    struct CopyNode
    {
        static constexpr auto NodeType = CommandType::Copy;

        ResourceNodeRef source;
    };

    struct RenderNode
    {
        static constexpr auto NodeType = CommandType::Render;

        RenderList renderList;
        std::vector<ResourceNodeRef> dependencies;
    };

    struct TextureNode
    {
        static constexpr auto NodeType = ResourceType::Texture;

        Texture texture;
        CommandNodeRef source;

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

    std::vector<CopyNode> copyNodes;
    std::vector<RenderNode> renderNodes;
    std::vector<TextureNode> textureNodes;
    std::vector<TextureDataNode> textureDataNodes;

    static constexpr auto NodeLists = std::tuple{
        &VulkanGraph::copyNodes,
        &VulkanGraph::renderNodes,
        &VulkanGraph::textureNodes,
        &VulkanGraph::textureDataNodes,
    };

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
    T* GetIf(CommandNodeRef ref)
    {
        return (ref.type == T::NodeType) ? &Get<T>(ref.index) : nullptr;
    }

    template <class T>
    T* GetIf(ResourceNodeRef ref)
    {
        return (ref.type == T::NodeType) ? &Get<T>(ref.index) : nullptr;
    }
};

void BuildGraph(VulkanGraph& graph, VulkanDevice& device);
std::string VisualizeGraph(VulkanGraph& graph);
void ExecuteGraph(VulkanGraph& graph, VulkanDevice& device, VulkanRenderer& renderer);

} // namespace Teide
