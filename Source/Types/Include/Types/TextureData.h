
#pragma once

#include "GeoLib/Vector.h"

#include <cstddef>
#include <cstdint>
#include <optional>
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
constexpr std::size_t TextureFormatCount = 27;

enum class Filter
{
	Nearest,
	Linear,
};

enum class MipmapMode
{
	Nearest,
	Linear,
};

enum class SamplerAddressMode
{
	Repeat,
	Mirror,
	Clamp,
	Border,
};

enum class CompareOp
{
	Never,
	Less,
	Equal,
	LessEqual,
	Greater,
	GreaterEqual,
	NotEqual,
	Always,
};

struct SamplerState
{
	Filter magFilter = Filter::Nearest;
	Filter minFilter = Filter::Nearest;
	MipmapMode mipmapMode = MipmapMode::Nearest;
	SamplerAddressMode addressModeU = SamplerAddressMode::Repeat;
	SamplerAddressMode addressModeV = SamplerAddressMode::Repeat;
	SamplerAddressMode addressModeW = SamplerAddressMode::Repeat;
	std::optional<float> maxAnisotropy = {};
	std::optional<CompareOp> compareOp = {};
};

struct TextureData
{
	Geo::Size2i size;
	TextureFormat format;
	std::uint32_t mipLevelCount = 1;
	std::uint32_t sampleCount = 1;
	SamplerState samplerState;
	std::vector<std::byte> pixels;
};

std::size_t GetByteSize(const TextureData& data);
std::uint32_t GetFormatElementSize(TextureFormat format);
bool HasDepthComponent(TextureFormat format);
bool HasStencilComponent(TextureFormat format);
bool HasDepthOrStencilComponent(TextureFormat format);
