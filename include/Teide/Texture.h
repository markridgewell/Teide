
#pragma once

#include "GeoLib/Vector.h"
#include "Teide/BasicTypes.h"
#include "Teide/Handle.h"
#include "Teide/TextureData.h"

#include <functional>
#include <utility>

namespace Teide
{

struct TextureProperties
{
    Geo::Size2i size;
    Format format = Format::Unknown;
    uint32 mipLevelCount = 1;
    uint32 sampleCount = 1;

    void Visit(auto f) const { return f(size, format, mipLevelCount, sampleCount); }
    bool operator==(const TextureProperties&) const = default;
};

class Texture final : public Handle<TextureProperties>
{
public:
    using Handle::Handle;

    Geo::Size2i GetSize() const { return (*this)->size; }
    Format GetFormat() const { return (*this)->format; }
    uint32 GetMipLevelCount() const { return (*this)->mipLevelCount; }
    uint32 GetSampleCount() const { return (*this)->sampleCount; }

    bool operator==(const Texture&) const = default;
};

} // namespace Teide

template <>
struct std::hash<Teide::Texture>
{
    std::size_t operator()(const Teide::Texture& v) const
    {
        return std::hash<Teide::uint64>{}(static_cast<Teide::uint64>(v));
    }
};
