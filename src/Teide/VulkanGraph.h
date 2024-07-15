
#pragma once

#include "Teide/BasicTypes.h"
#include "Teide/Renderer.h"

#include <string>
#include <vector>

namespace Teide
{
class VulkanDevice;

struct VulkanGraph
{
    using ResourceNodeRef = usize;
    using CommandNodeRef = usize;

    struct RenderNode
    {
        RenderList renderList;
        std::vector<ResourceNodeRef> dependencies;
    };

    struct TextureNode
    {
        Texture texture;
        usize source;

        explicit TextureNode(Texture texture, CommandNodeRef source) : texture{texture}, source{source} {}
    };

    std::vector<RenderNode> renderNodes;
    std::vector<TextureNode> textureNodes;
};

void BuildGraph(VulkanGraph& graph, VulkanDevice& device);
std::string VisualizeGraph(VulkanGraph& graph);

} // namespace Teide
