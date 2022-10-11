
#include "Teide/GraphicsDevice.h"

#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/Buffer.h"
#include "Teide/Shader.h"
#include "Teide/Texture.h"
#include "TestData.h"
#include "TestUtils.h"
#include "Types/TextureData.h"

#include <SDL.h>
#include <gmock/gmock.h>

using namespace testing;

namespace
{
TEST(GraphicsDeviceTest, CreateBuffer)
{
	auto device = CreateGraphicsDevice();
	const auto contents = std::array{1, 2, 3, 4};
	const auto bufferData = BufferData{
	    .usage = BufferUsage::Vertex,
	    .lifetime = ResourceLifetime::Permanent,
	    .data = contents,
	};
	const auto buffer = device->CreateBuffer(bufferData, "Buffer");
	EXPECT_THAT(buffer.get(), NotNull());
	EXPECT_THAT(buffer->GetSize(), Eq(contents.size() * sizeof(contents[0])));
}

TEST(GraphicsDeviceTest, CreateShader)
{
	auto device = CreateGraphicsDevice();
	const auto shaderData = CompileShader(SimpleShader);
	const auto shader = device->CreateShader(shaderData, "Shader");
	EXPECT_THAT(shader.get(), NotNull());
}

TEST(GraphicsDeviceTest, CreateTexture)
{
	auto device = CreateGraphicsDevice();
	const auto textureData = TextureData{
	    .size = {2, 2},
	    .format = TextureFormat::Byte4Srgb,
	    .mipLevelCount = 1,
	    .sampleCount = 1,
	    .pixels = HexToBytes("ff 00 00 ff 00 ff 00 ff ff 00 ff ff 00 00 ff ff"),
	};
	const auto texture = device->CreateTexture(textureData, "Texture");
	EXPECT_THAT(texture.get(), NotNull());
	EXPECT_THAT(texture->GetSize(), Eq(Geo::Size2i{2, 2}));
	EXPECT_THAT(texture->GetFormat(), Eq(TextureFormat::Byte4Srgb));
	EXPECT_THAT(texture->GetMipLevelCount(), Eq(1u));
	EXPECT_THAT(texture->GetSampleCount(), Eq(1u));
}

TEST(GraphicsDeviceTest, CreateRenderableTexture)
{
	auto device = CreateGraphicsDevice();
	const auto textureData = TextureData{
	    .size = {600, 400},
	    .format = TextureFormat::Byte4Srgb,
	    .mipLevelCount = 1,
	    .sampleCount = 1,
	};
	const auto texture = device->CreateRenderableTexture(textureData, "Texture");
	EXPECT_THAT(texture.get(), NotNull());
	EXPECT_THAT(texture->GetSize(), Eq(Geo::Size2i{600, 400}));
	EXPECT_THAT(texture->GetFormat(), Eq(TextureFormat::Byte4Srgb));
	EXPECT_THAT(texture->GetMipLevelCount(), Eq(1u));
	EXPECT_THAT(texture->GetSampleCount(), Eq(1u));
}

TEST(GraphicsDeviceTest, CreatePipeline)
{
	auto device = CreateGraphicsDevice();
	const auto shaderData = CompileShader(SimpleShader);

	const auto pipelineData = PipelineData{
	    .shader = device->CreateShader(shaderData, "Shader"),
	    .vertexLayout = {
	        .inputAssembly = {.topology = vk::PrimitiveTopology::eTriangleList},
	        .vertexInputBindings = {{.binding = 0, .stride = 0}},
	        .vertexInputAttributes = {{.location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = 0}},
	    },
	    .renderStates = {
	        .rasterizationState = {.lineWidth = 1.0f},
	    },
	    .framebufferLayout = {
			.colorFormat = TextureFormat::Byte4Srgb,
			.depthStencilFormat = TextureFormat::Depth16,
			.sampleCount = 2,
	    },
	};

	const auto pipeline = device->CreatePipeline(pipelineData);
	EXPECT_THAT(pipeline.get(), NotNull());
}

TEST(GraphicsDeviceTest, CreateParameterBlockWithUniforms)
{
	auto device = CreateGraphicsDevice();
	const auto shaderData = CompileShader(ShaderWithMaterialParams);
	const auto shader = device->CreateShader(shaderData, "Shader");
	const auto pblockData = ParameterBlockData{
	    .layout = shader->GetMaterialPblockLayout(),
	    .parameters = {
	        .uniformData = std::vector<std::byte>(16u, std::byte{}),
	        .textures = {},
	    },
	};
	const auto pblock = device->CreateParameterBlock(pblockData, "ParameterBlock");
	ASSERT_THAT(pblock.get(), NotNull());
	EXPECT_THAT(pblock->GetUniformBufferSize(), Eq(16u));
	EXPECT_THAT(pblock->GetPushConstantSize(), Eq(0u));
}

TEST(GraphicsDeviceTest, CreateParameterBlockWithPushConstants)
{
	auto device = CreateGraphicsDevice();
	const auto shaderData = CompileShader(ShaderWithObjectParams);
	const auto shader = device->CreateShader(shaderData, "Shader");
	const auto pblockData = ParameterBlockData{
	    .layout = shader->GetObjectPblockLayout(),
	    .parameters = {
	        .uniformData = std::vector<std::byte>(64u, std::byte{}),
	        .textures = {},
	    },
	};
	const auto pblock = device->CreateParameterBlock(pblockData, "ParameterBlock");
	ASSERT_THAT(pblock.get(), NotNull());
	EXPECT_THAT(pblock->GetUniformBufferSize(), Eq(0u));
	EXPECT_THAT(pblock->GetPushConstantSize(), Eq(64u));
}

} // namespace
