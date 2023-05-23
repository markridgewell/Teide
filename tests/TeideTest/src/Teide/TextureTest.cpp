
#include "Teide/Texture.h"

#include "Teide/CommandBuffer.h"
#include "Teide/GraphicsDevice.h"
#include "Teide/TestUtils.h"
#include "Teide/TextureData.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace Teide;

TEST(TextureTest, GenerateMipmaps)
{
    auto device = CreateTestGraphicsDevice();
    const TextureData textureData = {
        .size = {2, 2},
        .format = Format::Byte4Norm,
        .mipLevelCount = 2,
        .sampleCount = 1,
        .pixels = HexToBytes("80 00 00 80 00 80 00 80 80 00 80 80 00 00 80 80"),
    };

    const auto texture = device->CreateTexture(textureData, "Texture");
    ASSERT_THAT(texture.get(), NotNull());

    auto renderer = device->CreateRenderer(nullptr);
    auto task = renderer->CopyTextureData(texture);
    const TextureData& data = task.get();

    const auto expectedPixels = HexToBytes("80 00 00 80 00 80 00 80 80 00 80 80 00 00 80 80" // Mip 0
                                           "40 20 40 80"                                     // Mip 1
    );
    EXPECT_THAT(data.pixels, Eq(expectedPixels));
}
