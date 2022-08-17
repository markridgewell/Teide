
#pragma once

#include "Teide/Shader.h"
#include "Vulkan.h"

struct DescriptorSetInfo
{
	vk::UniqueDescriptorSetLayout layout;
	vk::ShaderStageFlags uniformsStages;
};

struct VulkanShaderBase
{
	vk::UniqueShaderModule vertexShader;
	vk::UniqueShaderModule pixelShader;
	DescriptorSetInfo sceneDescriptorSet;
	DescriptorSetInfo viewDescriptorSet;
	DescriptorSetInfo materialDescriptorSet;
	DescriptorSetInfo objectDescriptorSet;
	vk::UniquePipelineLayout pipelineLayout;
};

struct VulkanShader : VulkanShaderBase, public Shader
{
	explicit VulkanShader(VulkanShaderBase base) : VulkanShaderBase{std::move(base)} {}

	vk::DescriptorSetLayout GetDescriptorSetLayout(ParameterBlockType type) const
	{
		switch (type)
		{
			using enum ParameterBlockType;
			case Scene:
				return sceneDescriptorSet.layout.get();
			case View:
				return viewDescriptorSet.layout.get();
			case Material:
				return materialDescriptorSet.layout.get();
			case Object:
				return objectDescriptorSet.layout.get();
		}
		return {};
	}
};

template <>
struct VulkanImpl<Shader>
{
	using type = VulkanShader;
};
