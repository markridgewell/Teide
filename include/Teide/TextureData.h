
#pragma once

#include "GeoLib/Vector.h"
#include "Teide/BasicTypes.h"
#include "Teide/Format.h"

#include <optional>
#include <vector>

namespace Teide
{

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
    uint32 mipLevelCount = 1;
    uint32 sampleCount = 1;
    SamplerState samplerState;
    std::vector<byte> pixels;
};

usize GetByteSize(const TextureData& data);

} // namespace Teide
