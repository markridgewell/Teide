
#pragma once

#include "Vulkan.h"
#include "VulkanParameterBlock.h"

#include "Teide/Shader.h"
#include "Teide/ShaderData.h"

namespace Teide
{

struct VulkanShaderBase
{
    vk::UniqueShaderModule vertexShader;
    vk::UniqueShaderModule pixelShader;
    std::vector<ShaderVariable> vertexShaderInputs;
    VulkanParameterBlockLayoutPtr scenePblockLayout;
    VulkanParameterBlockLayoutPtr viewPblockLayout;
    VulkanParameterBlockLayoutPtr materialPblockLayout;
    VulkanParameterBlockLayoutPtr objectPblockLayout;
    vk::UniquePipelineLayout pipelineLayout;
};

struct VulkanShader : VulkanShaderBase, public Shader
{
    explicit VulkanShader(VulkanShaderBase base) : VulkanShaderBase{std::move(base)} {}

    ParameterBlockLayoutPtr GetMaterialPblockLayout() const override
    {
        return static_pointer_cast<const ParameterBlockLayout>(materialPblockLayout);
    }

    ParameterBlockLayoutPtr GetObjectPblockLayout() const override
    {
        return static_pointer_cast<const ParameterBlockLayout>(objectPblockLayout);
    }

    uint32 GetAttributeLocation(std::string_view attributeName) const
    {
        const auto pos = std::ranges::find(vertexShaderInputs, attributeName, &ShaderVariable::name);
        TEIDE_ASSERT(pos != vertexShaderInputs.end());
        return static_cast<uint32>(std::ranges::distance(vertexShaderInputs.begin(), pos));
    }
};

using VulkanShaderPtr = std::shared_ptr<const VulkanShader>;

template <>
struct VulkanImpl<Shader>
{
    using type = VulkanShader;
};

} // namespace Teide
