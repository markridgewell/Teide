
#pragma once

#include "GeoLib/Vector.h"
#include "Teide/TextureData.h"

#include <cstdint>

namespace Teide
{

class Texture
{
public:
	virtual ~Texture() = default;

	virtual Geo::Size2i GetSize() const = 0;
	virtual Format GetFormat() const = 0;
	virtual std::uint32_t GetMipLevelCount() const = 0;
	virtual std::uint32_t GetSampleCount() const = 0;
};

} // namespace Teide
