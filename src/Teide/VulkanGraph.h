
#pragma once

#include "Teide/BasicTypes.h"
#include "Teide/Renderer.h"

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

        bool operator==(const RenderNode&) const noexcept = default;
    };

    struct TextureNode
    {
        Texture texture;
        usize source;

        explicit TextureNode(Texture texture, CommandNodeRef source) : texture{texture}, source{source} {}

        bool operator==(const TextureNode&) const noexcept = default;
    };

    std::vector<RenderNode> renderNodes;
    std::vector<TextureNode> textureNodes;
};

void BuildGraph(VulkanGraph& graph, VulkanDevice& device);
} // namespace Teide
