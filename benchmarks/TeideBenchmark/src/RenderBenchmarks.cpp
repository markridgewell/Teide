
#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/Buffer.h"
#include "Teide/GraphicsDevice.h"
#include "Teide/Renderer.h"

#include <benchmark/benchmark.h>
#include <spdlog/spdlog.h>

inline const std::string SimpleVertexShader = R"--(
void main() {
    gl_Position = inPosition;
})--";

inline const std::string SimplePixelShader = R"--(
void main() {
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
})--";

using Type = Teide::ShaderVariableType::BaseType;

inline const ShaderSourceData SimpleShader = {
    .language = ShaderLanguage::Glsl,
    .vertexShader = {
        .inputs = {{
            {"inPosition", Type::Vector4},
        }},
        .outputs = {{
            {"gl_Position", Type::Vector3},
        }},
        .source = SimpleVertexShader,
    },
    .pixelShader = {
        .outputs = {{
            {"outColor", Type::Vector4},
        }},
        .source = SimplePixelShader,
    },
};

constexpr Teide::RenderTargetInfo CreateRenderTargetInfo(Geo::Size2i size)
{
    return {
        .size = size,
        .framebufferLayout = {
            .colorFormat = Teide::Format::Byte4Srgb,
            .depthStencilFormat = std::nullopt,
            .sampleCount=1,
        },
        .samplerState = {},
        .captureColor = true,
        .captureDepthStencil = false,
    };
}

constexpr Teide::RenderTargetInfo CreateRenderTargetInfo(Teide::uint32 size)
{
    return CreateRenderTargetInfo(Geo::Size2i{size});
}

constexpr Teide::RenderTargetInfo RenderTarget2x2 = CreateRenderTargetInfo(2);

Teide::TexturePtr
Render(const Teide::RendererPtr& renderer, const Teide::RenderTargetInfo& renderTarget, std::vector<Teide::RenderObject> objects)
{
    const Teide::RenderList renderList = {
        .clearState = {.colorValue = Teide::Color{1.0f, 0.0f, 0.0f, 1.0f}},
        .objects = std::move(objects),
    };

    return renderer->RenderToTexture(renderTarget, renderList).colorTexture;
}


void BM_RenderNothing(benchmark::State& state)
{
    spdlog::set_level(spdlog::level::err);

    const Teide::RenderTargetInfo renderTarget = CreateRenderTargetInfo(static_cast<Teide::uint32>(state.range(0)));

    Teide::GraphicsDevicePtr device = Teide::CreateHeadlessDevice();
    Teide::RendererPtr renderer = device->CreateRenderer(nullptr);

    for (auto _ : state)
    {
        const auto texture = Render(renderer, renderTarget, {});
    }
}
BENCHMARK(BM_RenderNothing)->Arg(8); //->Arg(256)->Arg(4096);

void BM_RenderToTexture(benchmark::State& state)
{
    spdlog::set_level(spdlog::level::err);

    const Teide::RenderTargetInfo renderTarget = CreateRenderTargetInfo(static_cast<Teide::uint32>(state.range(0)));

    Teide::GraphicsDevicePtr device = Teide::CreateHeadlessDevice();
    Teide::RendererPtr renderer = device->CreateRenderer(nullptr);

    const auto vertices = Teide::MakeBytes<float>({-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f});
    const auto mesh = device->CreateMesh({.vertexData = vertices, .vertexCount = 3}, "Mesh");
    const Teide::VertexLayout vertexLayout
        = {.topology = Teide::PrimitiveTopology::TriangleList,
           .bufferBindings = {{.stride = sizeof(float) * 2}},
           .attributes = {{.name = "inPosition", .format = Teide::Format::Float2, .bufferIndex = 0, .offset = 0}}};

    const auto shaderData = CompileShader(SimpleShader);
    const auto shader = device->CreateShader(shaderData, "SimpleShader");

    const auto pipeline = device->CreatePipeline({
        .shader = shader,
        .vertexLayout = vertexLayout,
        .renderPasses = {{.framebufferLayout = renderTarget.framebufferLayout}},
    });
    const std::vector<Teide::RenderObject> renderObjects = {{.mesh = mesh, .pipeline = pipeline}};

    for (auto _ : state)
    {
        const auto texture = Render(renderer, renderTarget, renderObjects);
    }
}
BENCHMARK(BM_RenderToTexture)->Arg(8); //->Arg(256)->Arg(4096);
