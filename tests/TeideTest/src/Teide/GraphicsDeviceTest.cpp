
#include "Teide/GraphicsDevice.h"

#include "TestData.h"
#include "TestUtils.h"

#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/Buffer.h"
#include "Teide/Mesh.h"
#include "Teide/Shader.h"
#include "Teide/Texture.h"
#include "Teide/TextureData.h"

#include <SDL.h>
#include <gmock/gmock.h>

using namespace testing;
using namespace Teide;

namespace
{
class GraphicsDeviceTest : public testing::Test
{
public:
    GraphicsDeviceTest() : m_device{CreateTestGraphicsDevice()} {}

protected:
    GraphicsDevicePtr m_device;
};

TEST_F(GraphicsDeviceTest, CreateBuffer)
{
    const auto contents = std::vector<std::byte>{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
    const BufferData bufferData = {
        .usage = BufferUsage::Vertex,
        .lifetime = ResourceLifetime::Permanent,
        .data = contents,
    };
    const auto buffer = m_device->CreateBuffer(bufferData, "Buffer");
    EXPECT_THAT(buffer.get(), NotNull());
    EXPECT_THAT(buffer->GetSize(), Eq(contents.size() * sizeof(contents[0])));
}

TEST_F(GraphicsDeviceTest, CreateShader)
{
    const auto shaderData = CompileShader(SimpleShader);
    const auto shader = m_device->CreateShader(shaderData, "Shader");
    EXPECT_THAT(shader.get(), NotNull());
}

TEST_F(GraphicsDeviceTest, CreateTexture)
{
    const TextureData textureData = {
        .size = {2, 2},
        .format = Format::Byte4Srgb,
        .mipLevelCount = 1,
        .sampleCount = 1,
        .pixels = HexToBytes("ff 00 00 ff 00 ff 00 ff ff 00 ff ff 00 00 ff ff"),
    };
    const auto texture = m_device->CreateTexture(textureData, "Texture");
    EXPECT_THAT(texture.get(), NotNull());
    EXPECT_THAT(texture->GetSize(), Eq(Geo::Size2i{2, 2}));
    EXPECT_THAT(texture->GetFormat(), Eq(Format::Byte4Srgb));
    EXPECT_THAT(texture->GetMipLevelCount(), Eq(1u));
    EXPECT_THAT(texture->GetSampleCount(), Eq(1u));
}

TEST_F(GraphicsDeviceTest, CreateMesh)
{
    const MeshData meshData = {
        .vertexData = MakeBytes<float>({1, 2, 3, 4, 5, 6}),
        .vertexCount = 3,
    };
    const auto mesh = m_device->CreateMesh(meshData, "Mesh");
    ASSERT_THAT(mesh.get(), NotNull());
    ASSERT_THAT(mesh->GetVertexBuffer(), NotNull());
    EXPECT_THAT(mesh->GetVertexBuffer()->GetSize(), Eq(meshData.vertexData.size()));
    EXPECT_THAT(mesh->GetVertexCount(), 3);
    EXPECT_THAT(mesh->GetIndexBuffer(), IsNull());
    EXPECT_THAT(mesh->GetIndexCount(), 0);
}

TEST_F(GraphicsDeviceTest, CreateMeshWithIndices)
{
    const MeshData meshData = {
        .vertexData = MakeBytes<float>({1, 2, 3, 4, 5, 6}),
        .indexData = MakeBytes<std::uint16_t>({0, 1, 2}),
        .vertexCount = 3,
    };
    const auto mesh = m_device->CreateMesh(meshData, "Mesh");
    ASSERT_THAT(mesh.get(), NotNull());
    ASSERT_THAT(mesh->GetVertexBuffer(), NotNull());
    EXPECT_THAT(mesh->GetVertexBuffer()->GetSize(), Eq(meshData.vertexData.size()));
    EXPECT_THAT(mesh->GetVertexCount(), 3);
    EXPECT_THAT(mesh->GetIndexBuffer(), NotNull());
    EXPECT_THAT(mesh->GetIndexBuffer()->GetSize(), Eq(meshData.indexData.size()));
    EXPECT_THAT(mesh->GetIndexCount(), 3);
}

TEST_F(GraphicsDeviceTest, CreatePipeline)
{
    const auto shaderData = CompileShader(SimpleShader);

    const PipelineData pipelineData = {
        .shader = m_device->CreateShader(shaderData, "Shader"),
        .vertexLayout = {
            .topology = PrimitiveTopology::TriangleList,
            .bufferBindings = {{.stride = 0}},
            .attributes = {{.name = "inPosition", .format = Format::Float3, .bufferIndex = 0, .offset = 0}},
        },
        .renderPasses = {{
            .framebufferLayout = {
                .colorFormat = Format::Byte4Srgb,
                .depthStencilFormat = Format::Depth16,
                .sampleCount = 2,
            },
        }},
    };

    const auto pipeline = m_device->CreatePipeline(pipelineData);
    EXPECT_THAT(pipeline.get(), NotNull());
}

TEST_F(GraphicsDeviceTest, CreateParameterBlockWithUniforms)
{
    const auto shaderData = CompileShader(ShaderWithMaterialParams);
    const auto shader = m_device->CreateShader(shaderData, "Shader");
    const ParameterBlockData pblockData = {
        .layout = shader->GetMaterialPblockLayout(),
        .parameters = {
            .uniformData = std::vector<std::byte>(16u, std::byte{}),
            .textures = {},
        },
    };
    const auto pblock = m_device->CreateParameterBlock(pblockData, "ParameterBlock");
    ASSERT_THAT(pblock.get(), NotNull());
    EXPECT_THAT(pblock->GetUniformBufferSize(), Eq(16u));
    EXPECT_THAT(pblock->GetPushConstantSize(), Eq(0u));
}

TEST_F(GraphicsDeviceTest, CreateParameterBlockWithPushConstants)
{
    const auto shaderData = CompileShader(ShaderWithObjectParams);
    const auto shader = m_device->CreateShader(shaderData, "Shader");
    const ParameterBlockData pblockData = {
        .layout = shader->GetObjectPblockLayout(),
        .parameters = {
            .uniformData = std::vector<std::byte>(64u, std::byte{}),
            .textures = {},
        },
    };
    const auto pblock = m_device->CreateParameterBlock(pblockData, "ParameterBlock");
    ASSERT_THAT(pblock.get(), NotNull());
    EXPECT_THAT(pblock->GetUniformBufferSize(), Eq(0u));
    EXPECT_THAT(pblock->GetPushConstantSize(), Eq(64u));
}

} // namespace
