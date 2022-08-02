
#pragma once

#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

struct TextureData
{
	vk::Extent2D size;
	vk::Format format;
	std::uint32_t mipLevelCount = 1;
	vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;
	vk::SamplerCreateInfo samplerInfo;
	std::vector<std::byte> pixels;
};

enum class FormatElementType
{
	Float,
	Int,
	Uint
};

std::size_t GetByteSize(const TextureData& data);
std::uint32_t GetFormatElementSize(vk::Format format);
FormatElementType GetFormatElementType(vk::Format format);
bool HasDepthComponent(vk::Format format);
bool HasStencilComponent(vk::Format format);
bool HasDepthOrStencilComponent(vk::Format format);
