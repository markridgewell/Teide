
#include "Types/Format.h"

#include "Types/StaticMap.h"

enum TypeSizes : std::uint32_t
{
	FloatSize = 4,
	HalfSize = 2,
	IntSize = 4,
	UintSize = 4,
	ShortSize = 2,
	UshortSize = 2,
	ByteSize = 1,
	UbyteSize = 1,
};

std::uint32_t GetFormatElementSize(Format format)
{
	static constexpr StaticMap<Format, std::uint32_t, FormatCount> map = {
	    {Format::Unknown, 0},

	    {Format::Byte1, ByteSize * 1},
	    {Format::Byte1Norm, ByteSize * 1},
	    {Format::Short1, ShortSize * 1},
	    {Format::Short1Norm, ShortSize * 1},
	    {Format::Ushort1, UshortSize * 1},
	    {Format::Ushort1Norm, UshortSize * 1},
	    {Format::Half1, HalfSize * 1},
	    {Format::Int1, IntSize * 1},
	    {Format::Uint1, UintSize * 1},
	    {Format::Float1, FloatSize * 1},

	    {Format::Byte2, ByteSize * 2},
	    {Format::Byte2Norm, ByteSize * 2},
	    {Format::Short2, ShortSize * 2},
	    {Format::Short2Norm, ShortSize * 2},
	    {Format::Ushort2, UshortSize * 2},
	    {Format::Ushort2Norm, UshortSize * 2},
	    {Format::Half2, HalfSize * 2},
	    {Format::Int2, IntSize * 2},
	    {Format::Uint2, UintSize * 2},
	    {Format::Float2, FloatSize * 2},

	    {Format::Byte3, ByteSize * 3},
	    {Format::Byte3Norm, ByteSize * 3},
	    {Format::Short3, ShortSize * 3},
	    {Format::Short3Norm, ShortSize * 3},
	    {Format::Ushort3, UshortSize * 3},
	    {Format::Ushort3Norm, UshortSize * 3},
	    {Format::Half3, HalfSize * 3},
	    {Format::Int3, IntSize * 3},
	    {Format::Uint3, UintSize * 3},
	    {Format::Float3, FloatSize * 3},

	    {Format::Byte4, ByteSize * 4},
	    {Format::Byte4Norm, ByteSize * 4},
	    {Format::Byte4Srgb, ByteSize * 4},
	    {Format::Byte4SrgbBGRA, ByteSize * 4},
	    {Format::Short4, ShortSize * 4},
	    {Format::Short4Norm, ShortSize * 4},
	    {Format::Ushort4, UshortSize * 4},
	    {Format::Ushort4Norm, UshortSize * 4},
	    {Format::Half4, HalfSize * 4},
	    {Format::Int4, IntSize * 4},
	    {Format::Uint4, UintSize * 4},
	    {Format::Float4, FloatSize * 4},

	    {Format::Depth16, 2},
	    {Format::Depth32, 4},
	    {Format::Depth16Stencil8, 3},
	    {Format::Depth24Stencil8, 4},
	    {Format::Depth32Stencil8, 5},
	    {Format::Stencil8, 1},
	};

	return map.at(format);
}

bool HasDepthComponent(Format format)
{
	return format == Format::Depth16 || format == Format::Depth32 || format == Format::Depth16Stencil8
	    || format == Format::Depth24Stencil8 || format == Format::Depth32Stencil8;
}

bool HasStencilComponent(Format format)
{
	return format == Format::Stencil8 || format == Format::Depth16Stencil8 || format == Format::Depth24Stencil8
	    || format == Format::Depth32Stencil8;
}

bool HasDepthOrStencilComponent(Format format)
{
	return format == Format::Depth16 || format == Format::Depth32 || format == Format::Depth16Stencil8
	    || format == Format::Depth24Stencil8 || format == Format::Depth32Stencil8 || format == Format::Stencil8;
}
