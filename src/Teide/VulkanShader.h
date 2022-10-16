
#pragma once

#include "Teide/Shader.h"
#include "Teide/ShaderData.h"
#include "Vulkan.h"
#include "VulkanParameterBlock.h"

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

	std::uint32_t GetAttributeLocation(std::string_view attributeName) const
	{
		const auto pos = std::ranges::find(vertexShaderInputs, attributeName, &ShaderVariable::name);
		assert(pos != vertexShaderInputs.end());
		return static_cast<std::uint32_t>(std::ranges::distance(vertexShaderInputs.begin(), pos));
	}
};

template <>
struct VulkanImpl<Shader>
{
	using type = VulkanShader;
};
