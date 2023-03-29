
#include "TestData.h"
#include "TestUtils.h"

#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/GraphicsDevice.h"

#include <SDL.h>
#include <gmock/gmock.h>

using namespace testing;
using namespace Teide;

namespace
{
struct SDLWindowDeleter
{
    void operator()(SDL_Window* window) { SDL_DestroyWindow(window); }
};

using UniqueSDLWindow = std::unique_ptr<SDL_Window, SDLWindowDeleter>;

TEST(SurfaceTest, CreateSurface)
{
    const auto window = UniqueSDLWindow(SDL_CreateWindow("Test", 0, 0, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN));
    ASSERT_THAT(window, NotNull()) << SDL_GetError();

    auto device = CreateGraphicsDevice(window.get());
    auto surface = device->CreateSurface(window.get(), false);
    EXPECT_THAT(surface->GetExtent(), Eq(Geo::Size2i{800, 600}));
}

TEST(SurfaceTest, CreateSurfaceMultisampled)
{
    auto window = UniqueSDLWindow(SDL_CreateWindow("Test", 0, 0, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN));
    ASSERT_THAT(window, NotNull()) << SDL_GetError();

    auto device = CreateGraphicsDevice(window.get());
    auto surface = device->CreateSurface(window.get(), true);
    EXPECT_THAT(surface->GetExtent(), Eq(Geo::Size2i{800, 600}));
}

TEST(SurfaceTest, CreatePipelineForSurface)
{
    const auto window = UniqueSDLWindow(SDL_CreateWindow("Test", 0, 0, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN));
    ASSERT_THAT(window, NotNull()) << SDL_GetError();

    auto device = CreateGraphicsDevice(window.get());
    auto surface = device->CreateSurface(window.get(), true);
    const auto shaderData = CompileShader(SimpleShader);
    const VertexLayout vertexLayout = {
        .topology = PrimitiveTopology::TriangleList,
        .bufferBindings = {{.stride = 0}},
        .attributes = {{.name = "inPosition", .format = Format::Float3, .bufferIndex = 0, .offset = 0}},
    };
    const FramebufferLayout framebufferLayout = {
        .colorFormat = surface->GetColorFormat(),
        .depthStencilFormat = surface->GetDepthFormat(),
        .sampleCount = surface->GetSampleCount(),
    };
    const auto pipeline = device->CreatePipeline({
        .shader = device->CreateShader(shaderData, "Shader"),
        .vertexLayout = vertexLayout,
        .renderStates = {},
        .renderPasses = {{.framebufferLayout = framebufferLayout}},
    });
    EXPECT_THAT(pipeline.get(), NotNull());
}

TEST(SurfaceTest, RenderToSurface)
{
    auto window = UniqueSDLWindow(SDL_CreateWindow("Test", 0, 0, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN));
    ASSERT_THAT(window, NotNull()) << SDL_GetError();

    auto device = CreateGraphicsDevice(window.get());
    auto surface = device->CreateSurface(window.get(), true);
    auto renderer = device->CreateRenderer(nullptr);

    renderer->BeginFrame({});
    const RenderList renderList = {
        .clearState = {.colorValue = Color{1.0f, 0.0f, 0.0f, 1.0f}},
    };
    renderer->RenderToSurface(*surface, renderList);
    renderer->EndFrame();
}

TEST(SurfaceTest, RenderToSurfaceWithoutClear)
{
    auto window = UniqueSDLWindow(SDL_CreateWindow("Test", 0, 0, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN));
    ASSERT_THAT(window, NotNull()) << SDL_GetError();

    auto device = CreateGraphicsDevice(window.get());
    auto surface = device->CreateSurface(window.get(), true);
    auto renderer = device->CreateRenderer(nullptr);

    renderer->BeginFrame({});
    const auto renderList = RenderList{};
    renderer->RenderToSurface(*surface, renderList);
    renderer->EndFrame();
}

TEST(SurfaceTest, RenderToSurfaceTwice)
{
    auto window = UniqueSDLWindow(SDL_CreateWindow("Test", 0, 0, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN));
    ASSERT_THAT(window, NotNull()) << SDL_GetError();

    auto device = CreateGraphicsDevice(window.get());
    auto surface = device->CreateSurface(window.get(), true);
    auto renderer = device->CreateRenderer(nullptr);

    renderer->BeginFrame({});
    const RenderList renderListWithClear = {
        .clearState = {.colorValue = Color{1.0f, 0.0f, 0.0f, 1.0f}},
    };
    const auto renderListWithoutClear = RenderList{};
    renderer->RenderToSurface(*surface, renderListWithClear);
    renderer->RenderToSurface(*surface, renderListWithoutClear);
    renderer->EndFrame();
}

} // namespace
