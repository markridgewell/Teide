
#include "Teide/TextureData.h"

#include "GeoLib/Vector.h"

#include <cstddef>

std::size_t GetByteSize(const TextureData& data)
{
	const auto pixelSize = GetFormatElementSize(data.format);

	std::size_t result = 0;
	Geo::Size2i mipExtent = data.size;

	for (auto i = 0u; i < data.mipLevelCount; i++)
	{
		result += mipExtent.x * mipExtent.y * pixelSize;
		mipExtent.x = std::max(1u, mipExtent.x / 2);
		mipExtent.y = std::max(1u, mipExtent.y / 2);
	}

	return result;
}
