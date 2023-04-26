
#pragma once

#include "Vulkan.h"
#include "VulkanParameterBlock.h"

#include "Teide/Shader.h"
#include "Teide/ShaderEnvironment.h"

namespace Teide
{
struct VulkanShaderEnvironmentBase
{
    VulkanParameterBlockLayoutPtr scenePblockLayout;
    VulkanParameterBlockLayoutPtr viewPblockLayout;
};

struct VulkanShaderEnvironment : VulkanShaderEnvironmentBase, public ShaderEnvironment
{
    explicit VulkanShaderEnvironment(VulkanShaderEnvironmentBase base) : VulkanShaderEnvironmentBase{std::move(base)} {}

    ParameterBlockLayoutPtr GetScenePblockLayout() const override
    {
        return static_pointer_cast<const ParameterBlockLayout>(scenePblockLayout);
    }

    ParameterBlockLayoutPtr GetViewPblockLayout() const override
    {
        return static_pointer_cast<const ParameterBlockLayout>(viewPblockLayout);
    }
};

template <>
struct VulkanImpl<ShaderEnvironment>
{
    using type = VulkanShaderEnvironment;
};

} // namespace Teide
