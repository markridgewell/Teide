
#pragma once

#include "GeoLib/Vector.h"
#include "Types/Format.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

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
	Format format;
	std::uint32_t mipLevelCount = 1;
	std::uint32_t sampleCount = 1;
	SamplerState samplerState;
	std::vector<std::byte> pixels;
};

std::size_t GetByteSize(const TextureData& data);
