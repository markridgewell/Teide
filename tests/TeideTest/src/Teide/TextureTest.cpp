
#include "Teide/Texture.h"

#include "TestData.h"
#include "TestUtils.h"

#include "Teide/CommandBuffer.h"
#include "Teide/Device.h"
#include "Teide/TextureData.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace Teide;

TEST(TextureTest, CompareHandles)
{
    auto device = CreateTestDevice();
    const Texture texture1 = device->CreateTexture(OnePixelWhiteTexture, "Tex1");
    const Texture texture2 = texture1; // NOLINT(performance-unnecessary-copy-initialization)
    EXPECT_THAT(texture1, Eq(texture2));
    const Handle texture3 = device->CreateTexture(OnePixelWhiteTexture, "Tex2");
    EXPECT_THAT(texture1, Ne(texture3));
}

TEST(TextureTest, GenerateMipmaps)
{
    auto device = CreateTestDevice();
    const TextureData textureData = {
        .size = {2, 2},
        .format = Format::Byte4Norm,
        .mipLevelCount = 2,
        .sampleCount = 1,
        .pixels = HexToBytes("80 00 00 80 00 80 00 80 80 00 80 80 00 00 80 80"),
    };

    const auto texture = device->CreateTexture(textureData, "Texture");

    auto renderer = device->CreateRenderer(nullptr);
    auto task = renderer->CopyTextureData(texture);
    const TextureData& data = task.get();

    const auto expectedPixels = HexToBytes("80 00 00 80 00 80 00 80 80 00 80 80 00 00 80 80" // Mip 0
                                           "40 20 40 80"                                     // Mip 1
    );
    EXPECT_THAT(data.pixels, Eq(expectedPixels));
}
