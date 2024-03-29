
#include "Teide/Renderer.h"

#include "TestData.h"
#include "TestUtils.h"

#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/Buffer.h"
#include "Teide/Device.h"
#include "Teide/Mesh.h"
#include "Teide/Texture.h"
#include "Teide/TextureData.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace Teide;

namespace
{
class RendererTest : public testing::Test
{
public:
    RendererTest() : m_device{CreateTestDevice()}, m_renderer{m_device->CreateRenderer(nullptr)} {}

    void CreateRenderer(const ShaderEnvironmentData& data)
    {
        m_renderer = m_device->CreateRenderer(m_device->CreateShaderEnvironment(data, "ShaderEnv"));
    }

protected:
    ShaderData CompileShader(const ShaderSourceData& data) { return m_shaderCompiler.Compile(data); }

    static RenderTargetInfo CreateRenderTargetInfo(Geo::Size2i size, Format format = Format::Byte4Srgb)
    {
        return {
            .size = size,
            .framebufferLayout = {
                .colorFormat = format,
                .depthStencilFormat = std::nullopt,
                .sampleCount=1,
            },
            .samplerState = {},
            .captureColor = true,
            .captureDepthStencil = false,
        };
    }

    RenderObject CreateFullscreenTri(const Teide::RenderTargetInfo& renderTarget)
    {
        const auto vertices = MakeBytes<float>({-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f});
        const auto mesh = m_device->CreateMesh({.vertexData = vertices, .vertexCount = 3}, "Mesh");
        const VertexLayout vertexLayout
            = {.topology = PrimitiveTopology::TriangleList,
               .bufferBindings = {{.stride = sizeof(float) * 2}},
               .attributes = {{.name = "inPosition", .format = Format::Float2, .bufferIndex = 0, .offset = 0}}};

        const auto shaderData = CompileShader(SimpleShader);
        const auto shader = m_device->CreateShader(shaderData, "SimpleShader");

        const auto pipeline = m_device->CreatePipeline({
            .shader = shader,
            .vertexLayout = vertexLayout,
            .renderPasses = {{.framebufferLayout = renderTarget.framebufferLayout}},
        });

        return RenderObject{.mesh = mesh, .pipeline = pipeline};
    };


    DevicePtr m_device;     // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    RendererPtr m_renderer; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    ShaderCompiler m_shaderCompiler;
};

TEST_F(RendererTest, RenderNothingAndCheckPixels)
{
    const auto renderTarget = CreateRenderTargetInfo({2, 2});

    const RenderList renderList = {
        .clearState = {.colorValue = Color{1.0f, 0.0f, 0.0f, 1.0f}},
    };

    const Texture texture = m_renderer->RenderToTexture(renderTarget, renderList).colorTexture.value();

    const TextureData outputData = m_renderer->CopyTextureData(texture).get();

    EXPECT_THAT(outputData.size, Eq(Geo::Size2i{2, 2}));
    EXPECT_THAT(outputData.format, Eq(Format::Byte4Srgb));
    EXPECT_THAT(outputData.mipLevelCount, Eq(1u));
    EXPECT_THAT(outputData.sampleCount, Eq(1u));

    const auto expectedPixels = HexToBytes("ff 00 00 ff ff 00 00 ff ff 00 00 ff ff 00 00 ff");
    EXPECT_THAT(outputData.pixels, ContainerEq(expectedPixels));
}

TEST_F(RendererTest, RenderFullscreenTriAndCheckPixels)
{
    const auto renderTarget = CreateRenderTargetInfo({2, 2});
    const auto fullscreenTri = CreateFullscreenTri(renderTarget);

    const RenderList renderList = {
        .clearState = {.colorValue = Color{1.0f, 0.0f, 0.0f, 1.0f}},
        .objects = {fullscreenTri},
    };

    const Texture texture = m_renderer->RenderToTexture(renderTarget, renderList).colorTexture.value();

    const TextureData outputData = m_renderer->CopyTextureData(texture).get();

    EXPECT_THAT(outputData.size, Eq(Geo::Size2i{2, 2}));
    EXPECT_THAT(outputData.format, Eq(Format::Byte4Srgb));
    EXPECT_THAT(outputData.mipLevelCount, Eq(1u));
    EXPECT_THAT(outputData.sampleCount, Eq(1u));

    const auto expectedPixels = HexToBytes("ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff");
    EXPECT_THAT(outputData.pixels, ContainerEq(expectedPixels));
}

TEST_F(RendererTest, RenderWithViewParameters)
{
    const auto renderTarget = CreateRenderTargetInfo({2, 2}, Format::Byte4Norm);

    const auto vertices = MakeBytes<float>({-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f});
    const auto mesh = m_device->CreateMesh({.vertexData = vertices, .vertexCount = 3}, "Mesh");
    const VertexLayout vertexLayout
        = {.topology = PrimitiveTopology::TriangleList,
           .bufferBindings = {{.stride = sizeof(float) * 2}},
           .attributes = {{.name = "inPosition", .format = Format::Float2, .bufferIndex = 0, .offset = 0}}};

    const auto shaderData = CompileShader(ViewTextureShader);
    CreateRenderer(ViewTextureEnvironment);
    const auto shader = m_device->CreateShader(shaderData, "ViewTextureShader");

    const auto pipeline = m_device->CreatePipeline({
        .shader = shader,
        .vertexLayout = vertexLayout,
        .renderPasses = {{.framebufferLayout = renderTarget.framebufferLayout}},
    });

    const TextureData textureData = {
        .size = {2, 2},
        .format = Format::Byte4Norm,
        .pixels = MakeBytes<uint8>({20, 20, 20, 255, 20, 20, 20, 255, 20, 20, 20, 255, 20, 20, 20, 255}),
    };
    Texture texture = m_device->CreateTexture(textureData, "initialTexture");

    const RenderList renderList = {
        .clearState = {.colorValue = Color{1.0f, 0.0f, 0.0f, 1.0f}},
        .viewParameters = {.textures = {texture}},
        .objects = {RenderObject{.mesh = mesh, .pipeline = pipeline}},
    };

    texture = m_renderer->RenderToTexture(renderTarget, renderList).colorTexture.value();

    const TextureData outputData = m_renderer->CopyTextureData(texture).get();

    EXPECT_THAT(outputData.size, Eq(Geo::Size2i{2, 2}));
    EXPECT_THAT(outputData.format, Eq(Format::Byte4Norm));
    EXPECT_THAT(outputData.mipLevelCount, Eq(1u));
    EXPECT_THAT(outputData.sampleCount, Eq(1u));

    const auto expectedPixels = HexToBytes("15 15 15 ff 15 15 15 ff 15 15 15 ff 15 15 15 ff");
    EXPECT_THAT(outputData.pixels, ContainerEq(expectedPixels));
}

TEST_F(RendererTest, RenderMultipleFramesWithViewParameters)
{
    const auto renderTarget = CreateRenderTargetInfo({2, 2}, Format::Byte4Norm);

    const auto vertices = MakeBytes<float>({-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f});
    const auto mesh = m_device->CreateMesh({.vertexData = vertices, .vertexCount = 3}, "Mesh");
    const VertexLayout vertexLayout
        = {.topology = PrimitiveTopology::TriangleList,
           .bufferBindings = {{.stride = sizeof(float) * 2}},
           .attributes = {{.name = "inPosition", .format = Format::Float2, .bufferIndex = 0, .offset = 0}}};

    const auto shaderData = CompileShader(ViewTextureShader);
    CreateRenderer(ViewTextureEnvironment);
    const auto shader = m_device->CreateShader(shaderData, "ViewTextureShader");

    const auto pipeline = m_device->CreatePipeline({
        .shader = shader,
        .vertexLayout = vertexLayout,
        .renderPasses = {{.framebufferLayout = renderTarget.framebufferLayout}},
    });

    const TextureData textureData = {
        .size = {2, 2},
        .format = Format::Byte4Norm,
        .pixels = MakeBytes<uint8>({20, 20, 20, 255, 20, 20, 20, 255, 20, 20, 20, 255, 20, 20, 20, 255}),
    };
    Texture texture = m_device->CreateTexture(textureData, "initialTexture");

    constexpr int numFrames = 100;
    for (int i = 0; i < numFrames; i++)
    {
        m_renderer->BeginFrame({});

        const RenderList renderList = {
            .clearState = {.colorValue = Color{1.0f, 0.0f, 0.0f, 1.0f}},
            .viewParameters = {.textures = {texture}},
            .objects = {RenderObject{.mesh = mesh, .pipeline = pipeline}},
        };

        texture = m_renderer->RenderToTexture(renderTarget, renderList).colorTexture.value();

        m_renderer->EndFrame();
    }

    const TextureData outputData = m_renderer->CopyTextureData(texture).get();

    EXPECT_THAT(outputData.size, Eq(Geo::Size2i{2, 2}));
    EXPECT_THAT(outputData.format, Eq(Format::Byte4Norm));
    EXPECT_THAT(outputData.mipLevelCount, Eq(1u));
    EXPECT_THAT(outputData.sampleCount, Eq(1u));

    const auto expectedPixels = HexToBytes("78 78 78 ff 78 78 78 ff 78 78 78 ff 78 78 78 ff");
    EXPECT_THAT(outputData.pixels, ContainerEq(expectedPixels));
}

TEST_F(RendererTest, RenderMultiplePassesWithViewParameters)
{
    const auto renderTarget = CreateRenderTargetInfo({2, 2}, Format::Byte4Norm);

    const auto vertices = MakeBytes<float>({-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f});
    const auto mesh = m_device->CreateMesh({.vertexData = vertices, .vertexCount = 3}, "Mesh");
    const VertexLayout vertexLayout
        = {.topology = PrimitiveTopology::TriangleList,
           .bufferBindings = {{.stride = sizeof(float) * 2}},
           .attributes = {{.name = "inPosition", .format = Format::Float2, .bufferIndex = 0, .offset = 0}}};

    const auto shaderData = CompileShader(ViewTextureShader);
    CreateRenderer(ViewTextureEnvironment);
    const auto shader = m_device->CreateShader(shaderData, "ViewTextureShader");

    const auto pipeline = m_device->CreatePipeline({
        .shader = shader,
        .vertexLayout = vertexLayout,
        .renderPasses = {{.framebufferLayout = renderTarget.framebufferLayout}},
    });

    const TextureData textureData = {
        .size = {2, 2},
        .format = Format::Byte4Norm,
        .pixels = MakeBytes<uint8>({20, 20, 20, 255, 20, 20, 20, 255, 20, 20, 20, 255, 20, 20, 20, 255}),
    };
    Texture texture = m_device->CreateTexture(textureData, "initialTexture");

    constexpr int numPasses = 100;
    for (int i = 0; i < numPasses; i++)
    {
        const RenderList renderList = {
            .clearState = {.colorValue = Color{1.0f, 0.0f, 0.0f, 1.0f}},
            .viewParameters = {.textures = {texture}},
            .objects = {RenderObject{.mesh = mesh, .pipeline = pipeline}},
        };

        texture = m_renderer->RenderToTexture(renderTarget, renderList).colorTexture.value();
    }

    const TextureData outputData = m_renderer->CopyTextureData(texture).get();

    EXPECT_THAT(outputData.size, Eq(Geo::Size2i{2, 2}));
    EXPECT_THAT(outputData.format, Eq(Format::Byte4Norm));
    EXPECT_THAT(outputData.mipLevelCount, Eq(1u));
    EXPECT_THAT(outputData.sampleCount, Eq(1u));

    const auto expectedPixels = HexToBytes("78 78 78 ff 78 78 78 ff 78 78 78 ff 78 78 78 ff");
    EXPECT_THAT(outputData.pixels, ContainerEq(expectedPixels));
}

TEST_F(RendererTest, RenderMultipleFramesWithMultiplePassesWithViewParameters)
{
    const auto renderTarget = CreateRenderTargetInfo({2, 2}, Format::Byte4Norm);

    const auto vertices = MakeBytes<float>({-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f});
    const auto mesh = m_device->CreateMesh({.vertexData = vertices, .vertexCount = 3}, "Mesh");
    const VertexLayout vertexLayout
        = {.topology = PrimitiveTopology::TriangleList,
           .bufferBindings = {{.stride = sizeof(float) * 2}},
           .attributes = {{.name = "inPosition", .format = Format::Float2, .bufferIndex = 0, .offset = 0}}};

    const auto shaderData = CompileShader(ViewTextureShader);
    CreateRenderer(ViewTextureEnvironment);
    const auto shader = m_device->CreateShader(shaderData, "ViewTextureShader");

    const auto pipeline = m_device->CreatePipeline({
        .shader = shader,
        .vertexLayout = vertexLayout,
        .renderPasses = {{.framebufferLayout = renderTarget.framebufferLayout}},
    });

    const TextureData textureData = {
        .size = {2, 2},
        .format = Format::Byte4Norm,
        .pixels = MakeBytes<uint8>({20, 20, 20, 255, 20, 20, 20, 255, 20, 20, 20, 255, 20, 20, 20, 255}),
    };
    Texture texture = m_device->CreateTexture(textureData, "initialTexture");

    constexpr int numFrames = 10;
    constexpr int numPasses = 10;
    for (int i = 0; i < numFrames; i++)
    {
        m_renderer->BeginFrame({});

        for (int j = 0; j < numPasses; j++)
        {
            const RenderList renderList = {
                .clearState = {.colorValue = Color{1.0f, 0.0f, 0.0f, 1.0f}},
                .viewParameters = {.textures = {texture}},
                .objects = {RenderObject{.mesh = mesh, .pipeline = pipeline}},
            };

            texture = m_renderer->RenderToTexture(renderTarget, renderList).colorTexture.value();
        }

        m_renderer->EndFrame();
    }

    const TextureData outputData = m_renderer->CopyTextureData(texture).get();

    EXPECT_THAT(outputData.size, Eq(Geo::Size2i{2, 2}));
    EXPECT_THAT(outputData.format, Eq(Format::Byte4Norm));
    EXPECT_THAT(outputData.mipLevelCount, Eq(1u));
    EXPECT_THAT(outputData.sampleCount, Eq(1u));

    const auto expectedPixels = HexToBytes("78 78 78 ff 78 78 78 ff 78 78 78 ff 78 78 78 ff");
    EXPECT_THAT(outputData.pixels, ContainerEq(expectedPixels));
}

} // namespace
