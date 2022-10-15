
#include "Teide/Renderer.h"

#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/Buffer.h"
#include "Teide/GraphicsDevice.h"
#include "Teide/Texture.h"
#include "TestData.h"
#include "TestUtils.h"
#include "Types/TextureData.h"

#include <gmock/gmock.h>

using namespace testing;

namespace
{
class RendererTest : public testing::Test
{
public:
	RendererTest() : m_device{CreateGraphicsDevice()}, m_renderer{m_device->CreateRenderer(nullptr)} {}

protected:
	DynamicTexturePtr CreateRenderableTexture(Geo::Size2i size)
	{
		const auto textureData = TextureData{
		    .size = size,
		    .format = Format::Byte4Srgb,
		    .mipLevelCount = 1,
		    .sampleCount = 1,
		};
		return m_device->CreateRenderableTexture(textureData, "Texture");
	}

	GraphicsDevicePtr m_device;
	RendererPtr m_renderer;
};

TEST_F(RendererTest, RenderNothing)
{
	const auto texture = CreateRenderableTexture({2, 2});

	const auto renderList = RenderList{
	    .clearColorValue = Color{1.0f, 0.0f, 0.0f, 1.0f},
	};

	m_renderer->RenderToTexture(texture, renderList);

	const TextureData outputData = m_renderer->CopyTextureData(texture).get().value();

	EXPECT_THAT(outputData.size, Eq(Geo::Size2i{2, 2}));
	EXPECT_THAT(outputData.format, Eq(Format::Byte4Srgb));
	EXPECT_THAT(outputData.mipLevelCount, Eq(1));
	EXPECT_THAT(outputData.sampleCount, Eq(1));

	const auto expectedPixels = HexToBytes("ff 00 00 ff ff 00 00 ff ff 00 00 ff ff 00 00 ff");
	EXPECT_THAT(outputData.pixels, ContainerEq(expectedPixels));
}

TEST_F(RendererTest, RenderFullscreenTri)
{
	const auto texture = CreateRenderableTexture({2, 2});

	constexpr auto vertices = std::array{-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f};
	const auto vbuffer = m_device->CreateBuffer(
	    {.usage = BufferUsage::Vertex, .lifetime = ResourceLifetime::Permanent, .data = vertices}, "VertexBuffer");
	const auto vertexLayout = VertexLayout{
	    .topology = PrimitiveTopology::TriangleList,
	    .bufferBindings = {{.stride = sizeof(float) * 2}},
	    .attributes = {{.name = "inPosition", .format = Format::Float2, .bufferIndex = 0, .offset = 0}}};

	const auto shaderData = CompileShader(SimpleShader);
	const auto shader = m_device->CreateShader(shaderData, "SimpleShader");

	const auto pipeline = m_device->CreatePipeline(
	    {.shader = shader,
	     .vertexLayout = vertexLayout,
	     .framebufferLayout = {
	         .colorFormat = texture->GetFormat(),
	     }});

	const auto renderList = RenderList{
	    .clearColorValue = Color{1.0f, 0.0f, 0.0f, 1.0f},
	    .objects = {RenderObject{.vertexBuffer = vbuffer, .indexCount = 3, .pipeline = pipeline}}};

	m_renderer->RenderToTexture(texture, renderList);

	const TextureData outputData = m_renderer->CopyTextureData(texture).get().value();

	EXPECT_THAT(outputData.size, Eq(Geo::Size2i{2, 2}));
	EXPECT_THAT(outputData.format, Eq(Format::Byte4Srgb));
	EXPECT_THAT(outputData.mipLevelCount, Eq(1));
	EXPECT_THAT(outputData.sampleCount, Eq(1));

	const auto expectedPixels = HexToBytes("ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff");
	EXPECT_THAT(outputData.pixels, ContainerEq(expectedPixels));
}

} // namespace
