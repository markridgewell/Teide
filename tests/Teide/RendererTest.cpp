
#include "Teide/Renderer.h"

#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/Buffer.h"
#include "Teide/GraphicsDevice.h"
#include "Teide/Texture.h"
#include "Teide/TextureData.h"
#include "TestData.h"
#include "TestUtils.h"

#include <gmock/gmock.h>

using namespace testing;

namespace
{
class RendererTest : public testing::Test
{
public:
	RendererTest() : m_device{CreateGraphicsDevice()}, m_renderer{m_device->CreateRenderer(nullptr)} {}

protected:
	RenderTargetInfo CreateRenderTargetInfo(Geo::Size2i size)
	{
		return {
			.size = size,
			.framebufferLayout = {
				.colorFormat = Format::Byte4Srgb,
				.depthStencilFormat = std::nullopt,
				.sampleCount=1,
			},
			.samplerState = {},
			.captureColor = true,
			.captureDepthStencil = false,
		};
	}

	GraphicsDevicePtr m_device;
	RendererPtr m_renderer;
};

TEST_F(RendererTest, RenderNothing)
{
	const auto renderTarget = CreateRenderTargetInfo({2, 2});

	const auto renderList = RenderList{
	    .clearColorValue = Color{1.0f, 0.0f, 0.0f, 1.0f},
	};

	const auto texture = m_renderer->RenderToTexture(renderTarget, renderList).colorTexture;

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
	const auto renderTarget = CreateRenderTargetInfo({2, 2});

	constexpr auto vertices = std::array{-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f};
	const auto vbuffer = m_device->CreateBuffer(
	    {.usage = BufferUsage::Vertex, .lifetime = ResourceLifetime::Permanent, .data = vertices}, "VertexBuffer");
	const auto vertexLayout = VertexLayout{
	    .topology = PrimitiveTopology::TriangleList,
	    .bufferBindings = {{.stride = sizeof(float) * 2}},
	    .attributes = {{.name = "inPosition", .format = Format::Float2, .bufferIndex = 0, .offset = 0}}};

	const auto shaderData = CompileShader(SimpleShader);
	const auto shader = m_device->CreateShader(shaderData, "SimpleShader");

	const auto pipeline = m_device->CreatePipeline({
	    .shader = shader,
	    .vertexLayout = vertexLayout,
	    .framebufferLayout = renderTarget.framebufferLayout,
	});

	const auto renderList = RenderList{
	    .clearColorValue = Color{1.0f, 0.0f, 0.0f, 1.0f},
	    .objects = {RenderObject{.vertexBuffer = vbuffer, .indexCount = 3, .pipeline = pipeline}}};

	const auto texture = m_renderer->RenderToTexture(renderTarget, renderList).colorTexture;

	const TextureData outputData = m_renderer->CopyTextureData(texture).get().value();

	EXPECT_THAT(outputData.size, Eq(Geo::Size2i{2, 2}));
	EXPECT_THAT(outputData.format, Eq(Format::Byte4Srgb));
	EXPECT_THAT(outputData.mipLevelCount, Eq(1));
	EXPECT_THAT(outputData.sampleCount, Eq(1));

	const auto expectedPixels = HexToBytes("ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff");
	EXPECT_THAT(outputData.pixels, ContainerEq(expectedPixels));
}

} // namespace
