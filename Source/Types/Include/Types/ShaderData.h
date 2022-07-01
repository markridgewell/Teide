
#pragma once

#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <vector>

struct ShaderData
{
	std::vector<std::uint32_t> vertexShaderSpirv;
	std::vector<std::uint32_t> pixelShaderSpirv;
	std::vector<vk::DescriptorSetLayoutBinding> sceneBindings;
	std::vector<vk::DescriptorSetLayoutBinding> viewBindings;
	std::vector<vk::DescriptorSetLayoutBinding> materialBindings;
	std::vector<vk::PushConstantRange> pushConstantRanges;
};
