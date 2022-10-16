
#pragma once

#include "GeoLib/Vector.h"
#include "Teide/TextureData.h"

#include <cstdint>
#include <memory>

class Surface
{
public:
	virtual ~Surface() = default;

	virtual Geo::Size2i GetExtent() const = 0;
	virtual Format GetColorFormat() const = 0;
	virtual Format GetDepthFormat() const = 0;
	virtual std::uint32_t GetSampleCount() const = 0;

	virtual void OnResize() = 0;
};

using SurfacePtr = std::unique_ptr<Surface>;
