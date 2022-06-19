
#include "Teide/Renderer.h"

#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/GraphicsDevice.h"
#include "Teide/Texture.h"

#include <gmock/gmock.h>

using namespace testing;

namespace
{
constexpr auto B00 = std::byte{0x00};
constexpr auto Bff = std::byte{0xff};

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

class RendererTest : public testing::Test
{
public:
	RendererTest() : m_renderer{m_device.CreateRenderer()} {}

protected:
	RenderableTexturePtr CreateRenderableTexture(vk::Extent2D size)
	{
		const auto textureData = TextureData{
		    .size = size,
		    .format = vk::Format::eR8G8B8A8Srgb,
		    .mipLevelCount = 1,
		    .samples = vk::SampleCountFlagBits::e1,
		};
		return m_device.CreateRenderableTexture(textureData, "Texture");
	}

	GraphicsDevice m_device;
	Renderer m_renderer;
};

TEST_F(RendererTest, RenderNothing)
{
	const auto texture = CreateRenderableTexture({2, 2});

	const auto clearColor = std::array{1.0f, 0.0f, 0.0f, 1.0f};
	const auto renderList = RenderList{
	    .clearValues = {{clearColor}},
	};

	m_renderer.RenderToTexture(texture, renderList);

	const TextureData outputData = m_renderer.CopyTextureData(texture).get().value();

	EXPECT_THAT(outputData.size, Eq(vk::Extent2D{2, 2}));
	EXPECT_THAT(outputData.format, Eq(vk::Format::eR8G8B8A8Srgb));
	EXPECT_THAT(outputData.mipLevelCount, Eq(1));
	EXPECT_THAT(outputData.samples, Eq(vk::SampleCountFlagBits::e1));

	const auto expectedPixels
	    = std::vector{Bff, B00, B00, Bff, Bff, B00, B00, Bff, Bff, B00, B00, Bff, Bff, B00, B00, Bff};
	EXPECT_THAT(outputData.pixels, ContainerEq(expectedPixels));
}

TEST_F(RendererTest, RenderFullscreenTri)
{
	const auto texture = CreateRenderableTexture({2, 2});

	constexpr auto vertices = std::array{-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f};
	const auto vbuffer = m_device.CreateBuffer(
	    {.usage = vk::BufferUsageFlagBits::eVertexBuffer, .memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal, .data = vertices},
	    "VertexBuffer");
	const auto vertexLayout = VertexLayout{
	    .inputAssembly = {.topology = vk::PrimitiveTopology::eTriangleList},
	    .vertexInputBindings = {{.binding = 0, .stride = sizeof(float) * 2}},
	    .vertexInputAttributes = {{.location = 0, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = 0}}};

	const auto shaderData = CompileShader(SimpleVertexShader, SimplePixelShader, ShaderLanguage::Glsl);
	const auto shader = m_device.CreateShader(shaderData, "SimpleShader");

	const auto pipeline = m_device.CreatePipeline(
	    *shader, vertexLayout,
	    RenderStates{
	        .viewport = {0, 0, 2, 2},
	        .scissor = {{0, 0}, {2, 2}},
	    },
	    *texture);

	const auto clearColor = std::array{1.0f, 0.0f, 0.0f, 1.0f};
	const auto renderList = RenderList{
	    .clearValues = {{clearColor}},
	    .objects = {RenderObject{.vertexBuffer = vbuffer, .indexCount = 3, .pipeline = pipeline}}};

	m_renderer.RenderToTexture(texture, renderList);

	const TextureData outputData = m_renderer.CopyTextureData(texture).get().value();

	EXPECT_THAT(outputData.size, Eq(vk::Extent2D{2, 2}));
	EXPECT_THAT(outputData.format, Eq(vk::Format::eR8G8B8A8Srgb));
	EXPECT_THAT(outputData.mipLevelCount, Eq(1));
	EXPECT_THAT(outputData.samples, Eq(vk::SampleCountFlagBits::e1));

	const auto expectedPixels
	    = std::vector{Bff, Bff, Bff, Bff, Bff, Bff, Bff, Bff, Bff, Bff, Bff, Bff, Bff, Bff, Bff, Bff};
	EXPECT_THAT(outputData.pixels, ContainerEq(expectedPixels));
}

} // namespace
