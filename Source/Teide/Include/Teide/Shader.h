
#pragma once

#include <vulkan/vulkan.hpp>

class Shader
{
public:
	virtual ~Shader() = default;

	virtual vk::DescriptorSetLayout GetSceneDescriptorSetLayout() const = 0;
	virtual vk::DescriptorSetLayout GetViewDescriptorSetLayout() const = 0;
	virtual vk::DescriptorSetLayout GetMaterialDescriptorSetLayout() const = 0;
};
