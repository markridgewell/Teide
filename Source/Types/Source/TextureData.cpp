
#include "Types/TextureData.h"

std::size_t GetByteSize(const TextureData& data)
{
	const auto pixelSize = GetFormatElementSize(data.format);

	std::size_t result = 0;
	vk::Extent2D mipExtent = data.size;

	for (auto i = 0u; i < data.mipLevelCount; i++)
	{
		result += mipExtent.width * mipExtent.height * pixelSize;
		mipExtent.width = std::max(1u, mipExtent.width / 2);
		mipExtent.height = std::max(1u, mipExtent.height / 2);
	}

	return result;
}

std::uint32_t GetFormatElementSize(TextureFormat format)
{
	switch (format)
	{
		using enum TextureFormat;
		case Unknown:
			return 0;

		case Byte1:
			return 1;
		case Int8x1:
			return 1;
		case Short1:
			return 2;
		case Int1:
			return 4;
		case Half1:
			return 2;
		case Float1:
			return 4;

		case Byte2:
			return 2;
		case Int8x2:
			return 2;
		case Short2:
			return 4;
		case Int2:
			return 8;
		case Half2:
			return 4;
		case Float2:
			return 8;

		case Byte4:
			return 4;
		case Int8x4:
			return 4;
		case Short4:
			return 8;
		case Int4:
			return 16;
		case Half4:
			return 8;
		case Float4:
			return 16;

		case Byte4Srgb:
			return 4;
		case Byte4SrgbBGRA:
			return 4;

		case Depth16:
			return 2;
		case Depth32:
			return 4;
		case Depth16Stencil8:
			return 4;
		case Depth24Stencil8:
			return 4;
		case Depth32Stencil8:
			return 8;
		case Stencil8:
			return 1;
	}
	return 0;
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
