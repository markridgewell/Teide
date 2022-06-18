
#include "Teide/Renderer.h"

#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/GraphicsDevice.h"
#include "Teide/Texture.h"

#include <gmock/gmock.h>

using namespace testing;

namespace
{
constexpr std::string_view SimpleVertexShader = R"--(
layout(location = 0) in vec4 inPosition;

void main() {
    gl_Position = inPosition;
})--";

constexpr std::string_view SimplePixelShader = R"--(
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
})--";

constexpr std::string_view VertexShaderWithParams = R"--(
layout(location = 0) in vec4 inPosition;

layout(set = 0, binding = 0) uniform Uniforms {
    mat4 mvp;
};

void main() {
    gl_Position = mvp * inPosition;
})--";

TEST(RendererTest, RenderToTexture)
{
	auto device = GraphicsDevice();
	auto renderer = device.CreateRenderer();

	const auto textureData = TextureData{
	    .size = {2, 2},
	    .format = vk::Format::eR8G8B8A8Srgb,
	    .mipLevelCount = 1,
	    .samples = vk::SampleCountFlagBits::e1,
	};
	const auto texture = device.CreateRenderableTexture(textureData, "Texture");

	const auto clearColor = std::array{1.0f, 0.0f, 0.0f, 1.0f};
	const auto renderList = RenderList{
	    .clearValues = {{clearColor}},
	};

	renderer.RenderToTexture(texture, renderList);

	const TextureData outputData = renderer.CopyTextureData(texture).get().value();

	EXPECT_THAT(outputData.size, Eq(textureData.size));
	EXPECT_THAT(outputData.format, Eq(textureData.format));
	EXPECT_THAT(outputData.mipLevelCount, Eq(textureData.mipLevelCount));
	EXPECT_THAT(outputData.samples, Eq(textureData.samples));

	constexpr auto B00 = std::byte{0x00};
	constexpr auto Bff = std::byte{0xff};
	const auto expectedPixels
	    = std::vector{Bff, B00, B00, Bff, Bff, B00, B00, Bff, Bff, B00, B00, Bff, Bff, B00, B00, Bff};
	EXPECT_THAT(outputData.pixels, ContainerEq(expectedPixels));
}

} // namespace
