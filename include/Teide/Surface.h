
#pragma once

#include "GeoLib/Vector.h"
#include "Teide/AbstractBase.h"
#include "Teide/BasicTypes.h"
#include "Teide/TextureData.h"

#include <memory>

namespace Teide
{

class Surface : AbstractBase
{
public:
    virtual Geo::Size2i GetExtent() const = 0;
    virtual Format GetColorFormat() const = 0;
    virtual Format GetDepthFormat() const = 0;
    virtual uint32 GetSampleCount() const = 0;

    virtual void OnResize() = 0;
};

using SurfacePtr = std::unique_ptr<Surface>;

} // namespace Teide
