
#pragma once

#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

enum class TextureFormat : std::uint16_t
{
	Unknown = 0,

	Byte1,
	Int8x1,
	Short1,
	Int1,
	Half1,
	Float1,

	Byte2,
	Int8x2,
	Short2,
	Int2,
	Half2,
	Float2,

	Byte4,
	Int8x4,
	Short4,
	Int4,
	Half4,
	Float4,

	Byte4Srgb,
	Byte4SrgbBGRA,

	Depth16,
	Depth32,
	Depth16Stencil8,
	Depth24Stencil8,
	Depth32Stencil8,
	Stencil8,
};

struct TextureData
{
	vk::Extent2D size;
	TextureFormat format;
	std::uint32_t mipLevelCount = 1;
	vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;
	vk::SamplerCreateInfo samplerInfo;
	std::vector<std::byte> pixels;
};

std::size_t GetByteSize(const TextureData& data);
std::uint32_t GetFormatElementSize(TextureFormat format);
bool HasDepthComponent(TextureFormat format);
bool HasStencilComponent(TextureFormat format);
bool HasDepthOrStencilComponent(TextureFormat format);
