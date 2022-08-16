
#pragma once

#include "Teide/Shader.h"
#include "Vulkan.h"

struct VulkanShaderBase
{
	vk::UniqueShaderModule vertexShader;
	vk::UniqueShaderModule pixelShader;
	vk::UniqueDescriptorSetLayout sceneDescriptorSetLayout;
	vk::UniqueDescriptorSetLayout viewDescriptorSetLayout;
	vk::UniqueDescriptorSetLayout materialDescriptorSetLayout;
	vk::UniqueDescriptorSetLayout objectDescriptorSetLayout;
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
				return sceneDescriptorSetLayout.get();
			case View:
				return viewDescriptorSetLayout.get();
			case Material:
				return materialDescriptorSetLayout.get();
			case Object:
				return objectDescriptorSetLayout.get();
		}
		return {};
	}
};

template <>
struct VulkanImpl<Shader>
{
	using type = VulkanShader;
};
