
#include "Teide/TextureData.h"

#include "GeoLib/Vector.h"

#include <cstddef>

std::size_t GetByteSize(const TextureData& data)
{
	const auto pixelSize = GetFormatElementSize(data.format);

	std::size_t result = 0;
	std::size_t mipw = data.size.x;
	std::size_t miph = data.size.y;

	for (auto i = 0u; i < data.mipLevelCount; i++)
	{
		result += mipw * miph * pixelSize;
		mipw = std::max(std::size_t{1}, mipw / 2);
		miph = std::max(std::size_t{1}, miph / 2);
	}

	return result;
}
