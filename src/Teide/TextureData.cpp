
#include "Teide/TextureData.h"

#include "GeoLib/Vector.h"
#include "Teide/BasicTypes.h"

namespace Teide
{

usize GetByteSize(const TextureData& data)
{
	const auto pixelSize = GetFormatElementSize(data.format);

	usize result = 0;
	usize mipw = data.size.x;
	usize miph = data.size.y;

	for (auto i = 0u; i < data.mipLevelCount; i++)
	{
		result += mipw * miph * pixelSize;
		mipw = std::max(usize{1}, mipw / 2);
		miph = std::max(usize{1}, miph / 2);
	}

	return result;
}

} // namespace Teide
