
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
	vk::UniquePipelineLayout pipelineLayout;
};

struct VulkanShader : VulkanShaderBase, public Shader
{
	explicit VulkanShader(VulkanShaderBase base) : VulkanShaderBase{std::move(base)} {}

	vk::DescriptorSetLayout GetSceneDescriptorSetLayout() const override { return sceneDescriptorSetLayout.get(); }
	vk::DescriptorSetLayout GetViewDescriptorSetLayout() const override { return viewDescriptorSetLayout.get(); }
	vk::DescriptorSetLayout GetMaterialDescriptorSetLayout() const override
	{
		return materialDescriptorSetLayout.get();
	}
};

template <>
struct VulkanImpl<Shader>
{
	using type = VulkanShader;
};
