
#include "Types/TextureData.h"

#include "Types/StaticMap.h"

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

std::uint32_t GetFormatElementSize(TextureFormat format)
{
	static constexpr StaticMap<TextureFormat, std::uint32_t, TextureFormatCount> map = {
	    {TextureFormat::Unknown, 0},
	    {TextureFormat::Byte1, 1},
	    {TextureFormat::Int8x1, 1},
	    {TextureFormat::Short1, 2},
	    {TextureFormat::Int1, 4},
	    {TextureFormat::Half1, 2},
	    {TextureFormat::Float1, 4},
	    {TextureFormat::Byte2, 2},
	    {TextureFormat::Int8x2, 2},
	    {TextureFormat::Short2, 4},
	    {TextureFormat::Int2, 8},
	    {TextureFormat::Half2, 4},
	    {TextureFormat::Float2, 8},
	    {TextureFormat::Byte4, 4},
	    {TextureFormat::Int8x4, 4},
	    {TextureFormat::Short4, 8},
	    {TextureFormat::Int4, 1},
	    {TextureFormat::Half4, 8},
	    {TextureFormat::Float4, 1},
	    {TextureFormat::Byte4Srgb, 4},
	    {TextureFormat::Byte4SrgbBGRA, 4},
	    {TextureFormat::Depth16, 2},
	    {TextureFormat::Depth32, 4},
	    {TextureFormat::Depth16Stencil8, 4},
	    {TextureFormat::Depth24Stencil8, 4},
	    {TextureFormat::Depth32Stencil8, 8},
	    {TextureFormat::Stencil8, 1},
	};

	return map.at(format);
}

bool HasDepthComponent(TextureFormat format)
{
	return format == TextureFormat::Depth16 || format == TextureFormat::Depth32 || format == TextureFormat::Depth16Stencil8
	    || format == TextureFormat::Depth24Stencil8 || format == TextureFormat::Depth32Stencil8;
}

bool HasStencilComponent(TextureFormat format)
{
	return format == TextureFormat::Stencil8 || format == TextureFormat::Depth16Stencil8
	    || format == TextureFormat::Depth24Stencil8 || format == TextureFormat::Depth32Stencil8;
}

bool HasDepthOrStencilComponent(TextureFormat format)
{
	return format == TextureFormat::Depth16 || format == TextureFormat::Depth32
	    || format == TextureFormat::Depth16Stencil8 || format == TextureFormat::Depth24Stencil8
	    || format == TextureFormat::Depth32Stencil8 || format == TextureFormat::Stencil8;
}
