
#pragma once

#include "Teide/BasicTypes.h"
#include "Teide/Renderer.h"

#include <string>
#include <vector>

namespace Teide
{
class VulkanDevice;

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
        ResourceNodeRef source;
    };

    struct RenderNode
    {
        RenderList renderList;
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
};

void BuildGraph(VulkanGraph& graph, VulkanDevice& device);
std::string VisualizeGraph(VulkanGraph& graph);

} // namespace Teide
