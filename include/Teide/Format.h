
#pragma once

#include "Teide/BasicTypes.h"

namespace Teide
{

enum class Format : uint8
{
    Unknown,

    Byte1,
    Byte1Norm,
    Short1,
    Short1Norm,
    Ushort1,
    Ushort1Norm,
    Half1,
    Int1,
    Uint1,
    Float1,

    Byte2,
    Byte2Norm,
    Short2,
    Short2Norm,
    Ushort2,
    Ushort2Norm,
    Half2,
    Int2,
    Uint2,
    Float2,

    Byte3,
    Byte3Norm,
    Short3,
    Short3Norm,
    Ushort3,
    Ushort3Norm,
    Half3,
    Int3,
    Uint3,
    Float3,

    Byte4,
    Byte4Norm,
    Byte4Srgb,
    Byte4SrgbBGRA,
    Short4,
    Short4Norm,
    Ushort4,
    Ushort4Norm,
    Half4,
    Int4,
    Uint4,
    Float4,

    Depth16,
    Depth32,
    Depth16Stencil8,
    Depth24Stencil8,
    Depth32Stencil8,
    Stencil8,
};
constexpr usize FormatCount = 49;

uint32 GetFormatElementSize(Format format);
bool HasDepthComponent(Format format);
bool HasStencilComponent(Format format);
bool HasDepthOrStencilComponent(Format format);

} // namespace Teide
