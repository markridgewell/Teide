
#pragma once

#include <Framework/BytesView.h>
#include <Framework/MemoryAllocator.h>
#include <Framework/Vulkan.h>

#include <cstdint>
#include <vector>

struct ShaderData
{
	std::vector<uint32_t> vertexShaderSpirv;
	std::vector<uint32_t> pixelShaderSpirv;
	std::vector<vk::DescriptorSetLayoutBinding> sceneBindings;
	std::vector<vk::DescriptorSetLayoutBinding> viewBindings;
	std::vector<vk::DescriptorSetLayoutBinding> materialBindings;
	std::vector<vk::PushConstantRange> pushConstantRanges;
};

struct Shader
{
	vk::UniqueShaderModule vertexShader;
	vk::UniqueShaderModule pixelShader;
	vk::UniqueDescriptorSetLayout sceneDescriptorSetLayout;
	vk::UniqueDescriptorSetLayout viewDescriptorSetLayout;
	vk::UniqueDescriptorSetLayout materialDescriptorSetLayout;
	vk::UniquePipelineLayout pipelineLayout;
};
