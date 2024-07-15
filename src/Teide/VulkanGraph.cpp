
#include "Teide/VulkanGraph.h"

#include "Teide/VulkanDevice.h"

#include <algorithm>
#include <format>
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
            return static_cast<usize>(std::distance(graph.textureNodes.begin(), it));
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
    std::string ret;
    auto out = std::back_inserter(ret);
    (void)graph;
    (void)out;
    return ret;
}
} // namespace Teide
