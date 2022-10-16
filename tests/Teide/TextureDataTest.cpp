
#include "Teide/TextureData.h"

#include <gmock/gmock.h>

using namespace testing;

TEST(TextureTest, GetByteSize)
{
	const auto textureData = TextureData{
	    .size = {2, 2},
	    .format = Format::Byte4,
	    .mipLevelCount = 1,
	    .sampleCount = 1,
	    .samplerState = {},
	};
	EXPECT_THAT(GetByteSize(textureData), Eq(16));
}

TEST(TextureTest, GetByteSizeWithMipmaps)
{
	const auto textureData = TextureData{
	    .size = {2, 2},
	    .format = Format::Byte4,
	    .mipLevelCount = 2,
	    .sampleCount = 1,
	    .samplerState = {},
	};
	EXPECT_THAT(GetByteSize(textureData), Eq(20));
}

TEST(TextureTest, GetFormatElementSize)
{
	EXPECT_THAT(GetFormatElementSize(Format::Byte1), Eq(1));
	EXPECT_THAT(GetFormatElementSize(Format::Byte2), Eq(2));
	EXPECT_THAT(GetFormatElementSize(Format::Byte4), Eq(4));
	EXPECT_THAT(GetFormatElementSize(Format::Unknown), Eq(0));
}
