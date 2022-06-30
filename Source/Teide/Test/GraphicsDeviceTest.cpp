
#include "Teide/GraphicsDevice.h"

#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/Buffer.h"
#include "Teide/Texture.h"
#include "TestUtils.h"
#include "Types/TextureData.h"

#include <SDL.h>
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

TEST(GraphicsDeviceTest, CreateBuffer)
{
	auto device = CreateGraphicsDevice();
	const auto contents = std::array{1, 2, 3, 4};
	const auto bufferData = BufferData{
	    .usage = vk::BufferUsageFlagBits::eVertexBuffer,
	    .memoryFlags = vk::MemoryPropertyFlagBits ::eDeviceLocal,
	    .data = contents,
	};
	const auto buffer = device->CreateBuffer(bufferData, "Buffer");
	EXPECT_THAT(buffer.get(), NotNull());
	EXPECT_THAT(buffer->size, Eq(contents.size() * sizeof(contents[0])));
}

TEST(GraphicsDeviceTest, CreateDynamicBuffer)
{
	auto device = CreateGraphicsDevice();
	const auto buffer = device->CreateDynamicBuffer(64u, vk::BufferUsageFlagBits::eVertexBuffer, "Buffer");
	EXPECT_THAT(buffer.get(), NotNull());
	EXPECT_THAT(buffer->size, Eq(64u));
}

TEST(GraphicsDeviceTest, CreateShader)
{
	auto device = CreateGraphicsDevice();
	const auto shaderData = CompileShader(SimpleVertexShader, SimplePixelShader, ShaderLanguage::Glsl);
	const auto shader = device->CreateShader(shaderData, "Shader");
	EXPECT_THAT(shader.get(), NotNull());
}

TEST(GraphicsDeviceTest, CreateTexture)
{
	auto device = CreateGraphicsDevice();
	const auto textureData = TextureData{
	    .size = {2, 2},
	    .format = vk::Format::eR8G8B8A8Srgb,
	    .mipLevelCount = 1,
	    .samples = vk::SampleCountFlagBits::e1,
	    .samplerInfo = {},
	    .pixels = HexToBytes("ff 00 00 ff 00 ff 00 ff ff 00 ff ff 00 00 ff ff"),
	};
	const auto texture = device->CreateTexture(textureData, "Texture");
	EXPECT_THAT(texture.get(), NotNull());
	EXPECT_THAT(texture->GetSize().width, Eq(2u));
	EXPECT_THAT(texture->GetSize().height, Eq(2u));
	EXPECT_THAT(texture->GetFormat(), Eq(vk::Format::eR8G8B8A8Srgb));
	EXPECT_THAT(texture->GetMipLevelCount(), Eq(1u));
	EXPECT_THAT(texture->GetSampleCount(), Eq(vk::SampleCountFlagBits::e1));
}

TEST(GraphicsDeviceTest, CreateRenderableTexture)
{
	auto device = CreateGraphicsDevice();
	const auto textureData = TextureData{
	    .size = {600, 400},
	    .format = vk::Format::eR8G8B8A8Srgb,
	    .mipLevelCount = 1,
	    .samples = vk::SampleCountFlagBits::e1,
	    .samplerInfo = {},
	};
	const auto texture = device->CreateRenderableTexture(textureData, "Texture");
	EXPECT_THAT(texture.get(), NotNull());
	EXPECT_THAT(texture->GetSize().width, Eq(600u));
	EXPECT_THAT(texture->GetSize().height, Eq(400u));
	EXPECT_THAT(texture->GetFormat(), Eq(vk::Format::eR8G8B8A8Srgb));
	EXPECT_THAT(texture->GetMipLevelCount(), Eq(1u));
	EXPECT_THAT(texture->GetSampleCount(), Eq(vk::SampleCountFlagBits::e1));
}

TEST(GraphicsDeviceTest, CreatePipeline)
{
	auto device = CreateGraphicsDevice();
	const auto shaderData = CompileShader(SimpleVertexShader, SimplePixelShader, ShaderLanguage::Glsl);
	const auto shader = device->CreateShader(shaderData, "Shader");
	const auto vertexLayout = VertexLayout{
	    .inputAssembly = {.topology = vk::PrimitiveTopology::eTriangleList},
	    .vertexInputBindings = {{.binding = 0, .stride = 0}},
	    .vertexInputAttributes = {{.location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = 0}},
	};
	const auto renderStates = RenderStates{
	    .viewport = {0, 0, 600, 400},
	    .rasterizationState = {.lineWidth = 1.0f},
	};
	const auto textureData = TextureData{
	    .size = {600, 400},
	    .format = vk::Format::eR8G8B8A8Srgb,
	    .mipLevelCount = 1,
	    .samples = vk::SampleCountFlagBits::e1,
	};
	const auto texture = device->CreateRenderableTexture(textureData, "Texture");
	const auto pipeline = device->CreatePipeline(*shader, vertexLayout, renderStates, *texture);
	EXPECT_THAT(pipeline.get(), NotNull());
	EXPECT_THAT(pipeline->layout, IsTrue());
	EXPECT_THAT(pipeline->pipeline.get(), IsTrue());
}

TEST(GraphicsDeviceTest, CreateParameterBlock)
{
	auto device = CreateGraphicsDevice();
	const auto shaderData = CompileShader(VertexShaderWithParams, SimplePixelShader, ShaderLanguage::Glsl);
	const auto shader = device->CreateShader(shaderData, "Shader");
	const auto pblockData = ParameterBlockData{
	    .layout = shader->sceneDescriptorSetLayout.get(),
	    .uniformBufferSize = 64,
	    .textures = {},
	};
	const auto pblock = device->CreateParameterBlock(pblockData, "ParameterBlock");
	EXPECT_THAT(pblock.get(), NotNull());
	EXPECT_THAT(pblock->uniformBuffer->size, Eq(64u));
}

TEST(GraphicsDeviceTest, CreateDynamicParameterBlock)
{
	auto device = CreateGraphicsDevice();
	const auto shaderData = CompileShader(VertexShaderWithParams, SimplePixelShader, ShaderLanguage::Glsl);
	const auto shader = device->CreateShader(shaderData, "Shader");
	const auto pblockData = ParameterBlockData{
	    .layout = shader->sceneDescriptorSetLayout.get(),
	    .uniformBufferSize = 64,
	    .textures = {},
	};
	const auto pblock = device->CreateDynamicParameterBlock(pblockData, "ParameterBlock");
	EXPECT_THAT(pblock.get(), NotNull());
	EXPECT_THAT(pblock->uniformBuffer->size, Eq(64u));
}

} // namespace
