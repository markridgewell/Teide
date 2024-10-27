
#pragma once

#include "GeoLib/Vector.h"
#include "Teide/BasicTypes.h"
#include "Teide/Handle.h"
#include "Teide/TextureData.h"

#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace Teide
{

struct TextureProperties
{
    Geo::Size2i size;
    Format format = Format::Unknown;
    uint32 mipLevelCount = 1;
    uint32 sampleCount = 1;
    std::string name;
};

class Texture final : public Handle<TextureProperties>
{
public:
    using Handle::Handle;

    std::string_view GetName() const { return (*this)->name; }
    Geo::Size2i GetSize() const { return (*this)->size; }
    Format GetFormat() const { return (*this)->format; }
    uint32 GetMipLevelCount() const { return (*this)->mipLevelCount; }
    uint32 GetSampleCount() const { return (*this)->sampleCount; }

    bool operator==(const Texture&) const = default;
};

} // namespace Teide
