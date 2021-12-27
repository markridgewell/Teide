
#pragma once

#include <Framework/BytesView.h>
#include <Framework/MemoryAllocator.h>
#include <Framework/Vulkan.h>

struct ShaderData
{
	std::vector<uint32_t> vertexShaderSpirv;
	std::vector<uint32_t> pixelShaderSpirv;
};

struct Shader
{
	vk::UniqueShaderModule vertexShader;
	vk::UniqueShaderModule pixelShader;
};
