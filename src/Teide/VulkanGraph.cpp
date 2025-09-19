
#include "Teide/VulkanGraph.h"

#include "Teide/Definitions.h"
#include "Teide/VulkanDevice.h"

#include <fmt/core.h>

#include <algorithm>
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

std::string_view VulkanGraph::GetNodeName(ResourceNodeRef ref)
{
    switch (ref.type)
    {
        case ResourceType::Texture: return textureNodes.at(ref.index).texture.GetName();
        case ResourceType::TextureData: return textureDataNodes.at(ref.index).name;
    }
    Unreachable();
}

std::optional<std::string_view> VulkanGraph::FindNodeWithSource(CommandNodeRef source)
{
    if (const auto it = std::ranges::find(textureNodes, source, &TextureNode::source); it != textureNodes.end())
    {
        return it->texture.GetName();
    }

    if (const auto it = std::ranges::find(textureDataNodes, source, &TextureDataNode::source); it != textureDataNodes.end())
    {
        return it->name;
    }

    return std::nullopt;
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
        case CommandType::Copy: return "Copy";
        case CommandType::Render: return "Render";
        case CommandType::Dispatch: return "Dispatch";
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

    graph.ForEachCommandNode([&](VulkanGraph::CommandNodeRef ref, std::string_view name, const auto& dependencies) {
        if (const auto resourceName = graph.FindNodeWithSource(ref))
        {
            fmt::format_to(out, "    {} -> {}\n", name, *resourceName);
        }
        else
        {
            fmt::format_to(out, "    {}\n", name);
        }

        for (const auto& dep : dependencies)
        {
            fmt::format_to(out, "    {} -> {}\n", graph.GetNodeName(dep), name);
        }
    });
    ret += '}';
    return ret;
}

void ExecuteGraph(VulkanGraph& graph, VulkanDevice& device, VulkanRenderer& renderer)
{
    (void)device;
    (void)renderer;
    (void)graph;
}

} // namespace Teide
