
#include "Types/TextureData.h"

#include <gmock/gmock.h>

using namespace testing;

TEST(TextureTest, GetByteSize)
{
	const auto textureData = TextureData{
	    .size = {2, 2},
	    .format = vk::Format::eR8G8B8A8Unorm,
	    .mipLevelCount = 1,
	    .sampleCount = vk::SampleCountFlagBits::e1,
	    .samplerInfo = {}};
	EXPECT_THAT(GetByteSize(textureData), Eq(16));
}

TEST(TextureTest, GetByteSizeWithMipmaps)
{
	const auto textureData = TextureData{
	    .size = {2, 2},
	    .format = vk::Format::eR8G8B8A8Unorm,
	    .mipLevelCount = 2,
	    .sampleCount = vk::SampleCountFlagBits::e1,
	    .samplerInfo = {}};
	EXPECT_THAT(GetByteSize(textureData), Eq(20));
}

TEST(TextureTest, GetFormatElementSize)
{
	EXPECT_THAT(GetFormatElementSize(vk::Format::eR8Unorm), Eq(1));
	EXPECT_THAT(GetFormatElementSize(vk::Format::eR8G8Unorm), Eq(2));
	EXPECT_THAT(GetFormatElementSize(vk::Format::eR8G8B8Unorm), Eq(3));
	EXPECT_THAT(GetFormatElementSize(vk::Format::eR8G8B8A8Unorm), Eq(4));
	EXPECT_THAT(GetFormatElementSize(vk::Format{-1}), Eq(0));
}
