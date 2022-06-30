
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
	vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
	vk::SamplerCreateInfo samplerInfo;
	std::vector<std::byte> pixels;
};

std::size_t GetByteSize(const TextureData& data);
std::uint32_t GetFormatElementSize(vk::Format format);
