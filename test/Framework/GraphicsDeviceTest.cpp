
#include "Framework/GraphicsDevice.h"

#include "Framework/Texture.h"
#include "ShaderCompiler.h"

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
	auto device = GraphicsDevice();
	const auto contents = std::array{1, 2, 3, 4};
	const auto bufferData = BufferData{
	    .usage = vk::BufferUsageFlagBits::eVertexBuffer,
	    .memoryFlags = vk::MemoryPropertyFlagBits ::eDeviceLocal,
	    .data = contents,
	};
	const auto buffer = device.CreateBuffer(bufferData, "Buffer");
	EXPECT_THAT(buffer.get(), NotNull());
	EXPECT_THAT(buffer->size, Eq(contents.size() * sizeof(contents[0])));
}

TEST(GraphicsDeviceTest, CreateDynamicBuffer)
{
	auto device = GraphicsDevice();
	const auto buffer = device.CreateDynamicBuffer(64u, vk::BufferUsageFlagBits::eVertexBuffer, "Buffer");
	EXPECT_THAT(buffer.get(), NotNull());
	EXPECT_THAT(buffer->size, Eq(64u));
}

TEST(GraphicsDeviceTest, CreateShader)
{
	auto device = GraphicsDevice();
	const auto shaderData = CompileShader(SimpleVertexShader, SimplePixelShader, ShaderLanguage::Glsl);
	const auto shader = device.CreateShader(shaderData, "Shader");
	EXPECT_THAT(shader.get(), NotNull());
}

TEST(GraphicsDeviceTest, CreateTexture)
{
	constexpr auto B00 = std::byte{0x00};
	constexpr auto Bff = std::byte{0xff};
	auto device = GraphicsDevice();
	const auto textureData = TextureData{
	    .size = {2, 2},
	    .format = vk::Format::eR8G8B8A8Srgb,
	    .mipLevelCount = 1,
	    .samples = vk::SampleCountFlagBits::e1,
	    .samplerInfo = {},
	    .pixels = {Bff, B00, B00, Bff, B00, Bff, B00, Bff, Bff, B00, Bff, Bff, B00, B00, Bff, Bff},
	};
	const auto texture = device.CreateTexture(textureData, "Texture");
	EXPECT_THAT(texture.get(), NotNull());
	EXPECT_THAT(texture->size.width, Eq(2u));
	EXPECT_THAT(texture->size.height, Eq(2u));
	EXPECT_THAT(texture->format, Eq(vk::Format::eR8G8B8A8Srgb));
	EXPECT_THAT(texture->mipLevelCount, Eq(1u));
	EXPECT_THAT(texture->samples, Eq(vk::SampleCountFlagBits::e1));
}

TEST(GraphicsDeviceTest, CreateRenderableTexture)
{
	auto device = GraphicsDevice();
	const auto textureData = TextureData{
	    .size = {600, 400},
	    .format = vk::Format::eR8G8B8A8Srgb,
	    .mipLevelCount = 1,
	    .samples = vk::SampleCountFlagBits::e1,
	    .samplerInfo = {},
	};
	const auto texture = device.CreateRenderableTexture(textureData, "Texture");
	EXPECT_THAT(texture.get(), NotNull());
	EXPECT_THAT(texture->size.width, Eq(600u));
	EXPECT_THAT(texture->size.height, Eq(400u));
	EXPECT_THAT(texture->format, Eq(vk::Format::eR8G8B8A8Srgb));
	EXPECT_THAT(texture->mipLevelCount, Eq(1u));
	EXPECT_THAT(texture->samples, Eq(vk::SampleCountFlagBits::e1));
}

TEST(GraphicsDeviceTest, CreatePipeline)
{
	auto device = GraphicsDevice();
	const auto shaderData = CompileShader(SimpleVertexShader, SimplePixelShader, ShaderLanguage::Glsl);
	const auto shader = device.CreateShader(shaderData, "Shader");
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
	const auto texture = device.CreateRenderableTexture(textureData, "Texture");
	const auto pipeline = device.CreatePipeline(*shader, vertexLayout, renderStates, *texture);
	EXPECT_THAT(pipeline.get(), NotNull());
	EXPECT_THAT(pipeline->layout, IsTrue());
	EXPECT_THAT(pipeline->pipeline.get(), IsTrue());
}

TEST(GraphicsDeviceTest, CreateParameterBlock)
{
	auto device = GraphicsDevice();
	const auto shaderData = CompileShader(VertexShaderWithParams, SimplePixelShader, ShaderLanguage::Glsl);
	const auto shader = device.CreateShader(shaderData, "Shader");
	const auto pblockData = ParameterBlockData{
	    .layout = shader->sceneDescriptorSetLayout.get(),
	    .uniformBufferSize = 64,
	    .textures = {},
	};
	const auto pblock = device.CreateParameterBlock(pblockData, "ParameterBlock");
	EXPECT_THAT(pblock.get(), NotNull());
	EXPECT_THAT(pblock->uniformBuffer->size, Eq(64u));
}

TEST(GraphicsDeviceTest, CreateDynamicParameterBlock)
{
	auto device = GraphicsDevice();
	const auto shaderData = CompileShader(VertexShaderWithParams, SimplePixelShader, ShaderLanguage::Glsl);
	const auto shader = device.CreateShader(shaderData, "Shader");
	const auto pblockData = ParameterBlockData{
	    .layout = shader->sceneDescriptorSetLayout.get(),
	    .uniformBufferSize = 64,
	    .textures = {},
	};
	const auto pblock = device.CreateDynamicParameterBlock(pblockData, "ParameterBlock");
	EXPECT_THAT(pblock.get(), NotNull());
	EXPECT_THAT(pblock->uniformBuffer->size, Eq(64u));
}

} // namespace
