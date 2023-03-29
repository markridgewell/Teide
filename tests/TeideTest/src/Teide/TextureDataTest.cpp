
#include "Teide/TextureData.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace Teide;

TEST(TextureTest, GetByteSize)
{
    const TextureData textureData = {
        .size = {2, 2},
        .format = Format::Byte4,
        .mipLevelCount = 1,
        .sampleCount = 1,
        .samplerState = {},
    };
    EXPECT_THAT(GetByteSize(textureData), Eq(16u));
}

TEST(TextureTest, GetByteSizeWithMipmaps)
{
    const TextureData textureData = {
        .size = {2, 2},
        .format = Format::Byte4,
        .mipLevelCount = 2,
        .sampleCount = 1,
        .samplerState = {},
    };
    EXPECT_THAT(GetByteSize(textureData), Eq(20u));
}

TEST(TextureTest, GetFormatElementSize)
{
    EXPECT_THAT(GetFormatElementSize(Format::Byte1), Eq(1u));
    EXPECT_THAT(GetFormatElementSize(Format::Byte2), Eq(2u));
    EXPECT_THAT(GetFormatElementSize(Format::Byte4), Eq(4u));
    EXPECT_THAT(GetFormatElementSize(Format::Unknown), Eq(0u));
}
