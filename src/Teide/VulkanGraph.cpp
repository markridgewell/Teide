
#include "Teide/VulkanGraph.h"

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
            if (const auto matParams = device.GetImpl(obj.materialParameters))
            {
                AddTextureDependencies(node, matParams->textures, graph);
            }
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
} // namespace Teide
