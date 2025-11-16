
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
    RendererTest() :
        m_device{CreateTestDevice()},
        m_renderer{m_device->CreateRenderer(nullptr)},
        m_emptyParameters{m_device->CreateParameterBlock({}, "EmptyParams")}
    {}

    void CreateRenderer(const ShaderEnvironmentData& data)
    {
        m_renderer = m_device->CreateRenderer(m_device->CreateShaderEnvironment(data, "ShaderEnv"));
    }

protected:
    ShaderData CompileShader(const ShaderSourceData& data) { return m_shaderCompiler.Compile(data); }
    KernelData CompileKernel(const KernelSourceData& data) { return m_shaderCompiler.Compile(data); }

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

        return RenderObject{.mesh = mesh, .pipeline = pipeline, .materialParameters = m_emptyParameters};
    };

    RenderToTextureResult RenderFullscreenTri(const Teide::RenderTargetInfo& renderTarget)
    {
        const auto fullscreenTri = CreateFullscreenTri(renderTarget);

        const RenderList renderList = {
            .clearState = {
                .colorValue = Color{1.0f, 0.0f, 0.0f, 1.0f},
                .depthValue = 1.0f,
            },
            .objects = {fullscreenTri},
        };

        return m_renderer->RenderToTexture(renderTarget, renderList);
    }

    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    DevicePtr m_device;
    RendererPtr m_renderer;
    ParameterBlock m_emptyParameters;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    ShaderCompiler m_shaderCompiler;
};

MATCHER_P(MatchesColorTarget, renderTarget, "")
{
    (void)result_listener;
    return arg.size
        == renderTarget.size
        && arg.format
        == renderTarget.framebufferLayout.colorFormat
        && arg.mipLevelCount
        == 1
        && arg.sampleCount
        == renderTarget.framebufferLayout.sampleCount;
}

MATCHER_P(MatchesDepthTarget, renderTarget, "")
{
    (void)result_listener;
    return arg.size
        == renderTarget.size
        && arg.format
        == renderTarget.framebufferLayout.depthStencilFormat
        && arg.mipLevelCount
        == 1
        && arg.sampleCount
        == renderTarget.framebufferLayout.sampleCount;
}

MATCHER_P(MatchesResolvedColorTarget, renderTarget, "")
{
    (void)result_listener;
    return arg.size
        == renderTarget.size
        && arg.format
        == renderTarget.framebufferLayout.colorFormat
        && arg.mipLevelCount
        == 1
        && arg.sampleCount
        == 1;
}

MATCHER_P(MatchesResolvedDepthTarget, renderTarget, "")
{
    (void)result_listener;
    return arg.size
        == renderTarget.size
        && arg.format
        == renderTarget.framebufferLayout.depthStencilFormat
        && arg.mipLevelCount
        == 1
        && arg.sampleCount
        == 1;
}

MATCHER_P2(MatchesOutput, dispatchInfo, index, "")
{
    (void)result_listener;
    return arg.size == dispatchInfo.size && arg.format == dispatchInfo.output.at(index).format;
}

auto BytesEq(std::string_view bytesStr)
{
    return ContainerEq(HexToBytes(bytesStr));
}

TEST_F(RendererTest, RenderNothing)
{
    const RenderTargetInfo renderTarget = {
        .size = {2,2},
        .framebufferLayout = {
            .colorFormat = Format::Byte4Srgb,
            .captureColor = true,
        },
    };
    const RenderList renderList = {
        .clearState = {.colorValue = Color{1.0f, 0.0f, 0.0f, 1.0f}},
    };

    const Texture texture = m_renderer->RenderToTexture(renderTarget, renderList).colorTexture.value();
    const TextureData outputData = m_renderer->CopyTextureData(texture).get();

    EXPECT_THAT(outputData, MatchesColorTarget(renderTarget));
    EXPECT_THAT(outputData.pixels, BytesEq("ff 00 00 ff ff 00 00 ff ff 00 00 ff ff 00 00 ff"));
}

TEST_F(RendererTest, RenderFullscreenTri)
{
    const RenderTargetInfo renderTarget = {
        .size = {2,2},
        .framebufferLayout = {
            .colorFormat = Format::Byte4Srgb,
            .captureColor = true,
        },
    };

    const Texture texture = RenderFullscreenTri(renderTarget).colorTexture.value();
    const TextureData outputData = m_renderer->CopyTextureData(texture).get();

    EXPECT_THAT(outputData, MatchesColorTarget(renderTarget));
    EXPECT_THAT(outputData.pixels, BytesEq("ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"));
}

TEST_F(RendererTest, RenderMultisampledFullscreenTri)
{
    const RenderTargetInfo renderTarget = {
        .size = {2,2},
        .framebufferLayout = {
            .colorFormat = Format::Byte4Srgb,
            .sampleCount = 4,
            .captureColor = true,
            .resolveColor = true,
        },
    };

    const Texture texture = RenderFullscreenTri(renderTarget).colorTexture.value();
    const TextureData outputData = m_renderer->CopyTextureData(texture).get();

    EXPECT_THAT(outputData, MatchesResolvedColorTarget(renderTarget));
    EXPECT_THAT(outputData.pixels, BytesEq("ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"));
}

TEST_F(RendererTest, RenderMultisampledFullscreenTriWithDepthCaptureColor)
{
    const RenderTargetInfo renderTarget = {
        .size = {2,2},
        .framebufferLayout = {
            .colorFormat = Format::Byte4Srgb,
            .depthStencilFormat = Format::Depth16,
            .sampleCount = 4,
            .captureColor = true,
            .captureDepthStencil = true,
            .resolveColor = true,
            .resolveDepthStencil = false,
        },
    };

    const Texture texture = RenderFullscreenTri(renderTarget).colorTexture.value();
    const TextureData outputData = m_renderer->CopyTextureData(texture).get();

    EXPECT_THAT(outputData, MatchesResolvedColorTarget(renderTarget));
    EXPECT_THAT(outputData.pixels, BytesEq("ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"));
}

TEST_F(RendererTest, RenderMultisampledFullscreenTriWithDepthNoResolve)
{
    const RenderTargetInfo renderTarget = {
        .size = {2, 2},
        .framebufferLayout = {
            .colorFormat = Format::Byte4Srgb,
            .depthStencilFormat = Format::Depth16,
            .sampleCount = 4,
            .captureColor = true,
            .captureDepthStencil = true,
            .resolveColor = false,
            .resolveDepthStencil = false,
        },
    };
    const auto [color, depth] = RenderFullscreenTri(renderTarget);

    // Can't copy the pixels of a multisampled texture, but we can confirm it returns... something
    EXPECT_THAT(color, Ne(std::nullopt));
    EXPECT_THAT(depth, Ne(std::nullopt));
}

TEST_F(RendererTest, RenderWithViewParameters)
{
    const RenderTargetInfo renderTarget = {
        .size = {2,2},
        .framebufferLayout = {
            .colorFormat = Format::Byte4Norm,
            .captureColor = true,
        },
    };

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
        .objects = {RenderObject{.mesh = mesh, .pipeline = pipeline, .materialParameters = m_emptyParameters}},
    };

    texture = m_renderer->RenderToTexture(renderTarget, renderList).colorTexture.value();
    const TextureData outputData = m_renderer->CopyTextureData(texture).get();

    EXPECT_THAT(outputData, MatchesColorTarget(renderTarget));
    EXPECT_THAT(outputData.pixels, BytesEq("15 15 15 ff 15 15 15 ff 15 15 15 ff 15 15 15 ff"));
}

TEST_F(RendererTest, RenderMultipleFramesWithViewParameters)
{
    const RenderTargetInfo renderTarget = {
        .size = {2,2},
        .framebufferLayout = {
            .colorFormat = Format::Byte4Norm,
            .captureColor = true,
        },
    };

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
            .objects = {RenderObject{.mesh = mesh, .pipeline = pipeline, .materialParameters = m_emptyParameters}},
        };

        texture = m_renderer->RenderToTexture(renderTarget, renderList).colorTexture.value();

        m_renderer->EndFrame();
    }

    const TextureData outputData = m_renderer->CopyTextureData(texture).get();

    EXPECT_THAT(outputData, MatchesColorTarget(renderTarget));
    EXPECT_THAT(outputData.pixels, BytesEq("78 78 78 ff 78 78 78 ff 78 78 78 ff 78 78 78 ff"));
}

TEST_F(RendererTest, RenderMultiplePassesWithViewParameters)
{
    const RenderTargetInfo renderTarget = {
        .size = {2,2},
        .framebufferLayout = {
            .colorFormat = Format::Byte4Norm,
            .captureColor = true,
        },
    };

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
            .objects = {RenderObject{.mesh = mesh, .pipeline = pipeline, .materialParameters = m_emptyParameters}},
        };

        texture = m_renderer->RenderToTexture(renderTarget, renderList).colorTexture.value();
    }

    const TextureData outputData = m_renderer->CopyTextureData(texture).get();

    EXPECT_THAT(outputData, MatchesColorTarget(renderTarget));
    EXPECT_THAT(outputData.pixels, BytesEq("78 78 78 ff 78 78 78 ff 78 78 78 ff 78 78 78 ff"));
}

TEST_F(RendererTest, RenderMultipleFramesWithMultiplePassesWithViewParameters)
{
    const RenderTargetInfo renderTarget = {
        .size = {2,2},
        .framebufferLayout = {
            .colorFormat = Format::Byte4Norm,
            .captureColor = true,
        },
    };

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
                .objects = {RenderObject{.mesh = mesh, .pipeline = pipeline, .materialParameters = m_emptyParameters}},
            };

            texture = m_renderer->RenderToTexture(renderTarget, renderList).colorTexture.value();
        }

        m_renderer->EndFrame();
    }

    const TextureData outputData = m_renderer->CopyTextureData(texture).get();

    EXPECT_THAT(outputData, MatchesColorTarget(renderTarget));
    EXPECT_THAT(outputData.pixels, BytesEq("78 78 78 ff 78 78 78 ff 78 78 78 ff 78 78 78 ff"));
}

TEST_F(RendererTest, DispatchSimple)
{
    /*
    const auto kernelData = CompileKernel(ViewTextureShader);
    const auto kernel = m_device.CreateKernel(kernelData, "TestKernel");

    const DispatchInfo dispatch = {
        .kernel = kernel;
        .size = {2, 2},
        .outputFormats = {{
            { "result", Format::Float },
        }},
    };
    const auto result = m_renderer.Dispatch(dispatch);
    ASSERT_THAT(result.outputs.size(), Eq(1u));
    const Texture texture = result.outputs[0];
    const TextureData outputData = m_renderer->CopyTextureData(texture).get();

    EXPECT_THAT(outputData, MatchesOutput(dispatch, 0));
    EXPECT_THAT(outputData.pixels, BytesEq("00 00 28 42 00 00 28 42 00 00 28 42 00 00 28 42"));
    "));
    */
}

} // namespace
