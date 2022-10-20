
#include "Teide/Texture.h"

#include "Teide/CommandBuffer.h"
#include "Teide/GraphicsDevice.h"
#include "Teide/TestUtils.h"
#include "Teide/TextureData.h"

#include <gmock/gmock.h>

using namespace testing;

TEST(TextureTest, GenerateMipmaps)
{
	auto device = CreateGraphicsDevice();
	const TextureData textureData = {
	    .size = {2, 2},
	    .format = Format::Byte4Norm,
	    .mipLevelCount = 2,
	    .sampleCount = 1,
	    .pixels = HexToBytes("ff 00 00 ff 00 ff 00 ff ff 00 ff ff 00 00 ff ff"),
	};

	const auto texture = device->CreateTexture(textureData, "Texture");
	ASSERT_THAT(texture.get(), NotNull());

	auto renderer = device->CreateRenderer(nullptr);
	auto result = renderer->CopyTextureData(texture);
	ASSERT_TRUE(result.get().has_value());
	const TextureData data = result.get().value();

	const auto expectedPixels = HexToBytes("ff 00 00 ff 00 ff 00 ff ff 00 ff ff 00 00 ff ff" // Mip 0
	                                       "80 40 80 ff"                                     // Mip 1
	);
	EXPECT_THAT(data.pixels, Eq(expectedPixels));
}
