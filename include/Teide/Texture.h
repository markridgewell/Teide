
#pragma once

#include "GeoLib/Vector.h"
#include "Teide/AbstractBase.h"
#include "Teide/BasicTypes.h"
#include "Teide/TextureData.h"

namespace Teide
{

class Texture : AbstractBase
{
public:
    virtual Geo::Size2i GetSize() const = 0;
    virtual Format GetFormat() const = 0;
    virtual uint32 GetMipLevelCount() const = 0;
    virtual uint32 GetSampleCount() const = 0;
};

} // namespace Teide
