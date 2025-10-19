
#include "Teide/Format.h"

#include "Teide/Definitions.h"

namespace Teide
{

enum TypeSizes : uint8
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

uint32 GetFormatElementSize(Format format)
{
    switch (format)
    {
        case Format::Unknown: return 0;

        case Format::Byte1:
        case Format::Byte1Norm: return ByteSize * 1;
        case Format::Short1:
        case Format::Short1Norm: return ShortSize * 1;
        case Format::Ushort1:
        case Format::Ushort1Norm: return UshortSize * 1;
        case Format::Half1: return HalfSize * 1;
        case Format::Int1: return IntSize * 1;
        case Format::Uint1: return UintSize * 1;
        case Format::Float1: return FloatSize * 1;

        case Format::Byte2:
        case Format::Byte2Norm: return ByteSize * 2;
        case Format::Short2:
        case Format::Short2Norm: return ShortSize * 2;
        case Format::Ushort2:
        case Format::Ushort2Norm: return UshortSize * 2;
        case Format::Half2: return HalfSize * 2;
        case Format::Int2: return IntSize * 2;
        case Format::Uint2: return UintSize * 2;
        case Format::Float2: return FloatSize * 2;

        case Format::Byte3:
        case Format::Byte3Norm: return ByteSize * 3;
        case Format::Short3:
        case Format::Short3Norm: return ShortSize * 3;
        case Format::Ushort3:
        case Format::Ushort3Norm: return UshortSize * 3;
        case Format::Half3: return HalfSize * 3;
        case Format::Int3: return IntSize * 3;
        case Format::Uint3: return UintSize * 3;
        case Format::Float3: return FloatSize * 3;

        case Format::Byte4:
        case Format::Byte4Norm:
        case Format::Byte4Srgb:
        case Format::Byte4SrgbBGRA: return ByteSize * 4;
        case Format::Short4:
        case Format::Short4Norm: return ShortSize * 4;
        case Format::Ushort4:
        case Format::Ushort4Norm: return UshortSize * 4;
        case Format::Half4: return HalfSize * 4;
        case Format::Int4: return IntSize * 4;
        case Format::Uint4: return UintSize * 4;
        case Format::Float4: return FloatSize * 4;

        case Format::Depth16: return 2;
        case Format::Depth32: return 4;
        case Format::Depth16Stencil8: return 3;
        case Format::Depth24Stencil8: return 4;
        case Format::Depth32Stencil8: return 5;
        case Format::Stencil8: return 1;
    }

    Unreachable();
}

bool HasDepthComponent(Format format)
{
    return format
        == Format::Depth16
        || format
        == Format::Depth32
        || format
        == Format::Depth16Stencil8
        || format
        == Format::Depth24Stencil8
        || format
        == Format::Depth32Stencil8;
}

bool HasStencilComponent(Format format)
{
    return format
        == Format::Stencil8
        || format
        == Format::Depth16Stencil8
        || format
        == Format::Depth24Stencil8
        || format
        == Format::Depth32Stencil8;
}

bool HasDepthOrStencilComponent(Format format)
{
    return format
        == Format::Depth16
        || format
        == Format::Depth32
        || format
        == Format::Depth16Stencil8
        || format
        == Format::Depth24Stencil8
        || format
        == Format::Depth32Stencil8
        || format
        == Format::Stencil8;
}

} // namespace Teide
