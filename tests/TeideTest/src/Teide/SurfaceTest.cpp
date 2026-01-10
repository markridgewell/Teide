
#include "TestData.h"
#include "TestUtils.h"

#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/Device.h"

#include <SDL.h>
#include <gmock/gmock.h>

using namespace testing;
using namespace Teide;

namespace
{
constexpr auto SurfaceSize = Geo::Size2i{800, 600};

struct SDLWindowDeleter
{
    void operator()(SDL_Window* window) { SDL_DestroyWindow(window); }
};

using UniqueSDLWindow = std::unique_ptr<SDL_Window, SDLWindowDeleter>;

class WindowSurfaceTest : public Test
{
public:
    void SetUp() override
    {
        if (g_windowless)
        {
            GTEST_SKIP();
        }

        m_window = UniqueSDLWindow(SDL_CreateWindow("Test", 0, 0, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN));
        ASSERT_THAT(m_window, NotNull()) << SDL_GetError();

        m_objects = CreateDeviceAndSurface(m_window.get());
    }

    Device& GetDevice() const { return *this->m_objects.device; }
    Surface& GetSurface() const { return *this->m_objects.surface; }

private:
    UniqueSDLWindow m_window;
    DeviceAndSurface m_objects;
};

class HeadlessSurfaceTest : public Test
{
public:
    void SetUp() override { m_objects = CreateHeadlessDeviceAndSurface(SurfaceSize); }

    Device& GetDevice() const { return *this->m_objects.device; }
    Surface& GetSurface() const { return *this->m_objects.surface; }

private:
    DeviceAndSurface m_objects;
};

template <class T>
class SurfaceTest : public T
{
public:
};

using TestTypes = ::testing::Types<WindowSurfaceTest, HeadlessSurfaceTest>;
TYPED_TEST_SUITE(SurfaceTest, TestTypes);

TYPED_TEST(SurfaceTest, CreateSurface)
{
    auto& surface = this->GetSurface();

    EXPECT_THAT(surface.GetExtent(), Eq(SurfaceSize));
}

TYPED_TEST(SurfaceTest, CreatePipelineForSurface)
{
    auto& surface = this->GetSurface();
    auto& device = this->GetDevice();

    ShaderCompiler const compiler;
    const auto shaderData = compiler.Compile(SimpleShader);
    const VertexLayout vertexLayout = {
        .topology = PrimitiveTopology::TriangleList,
        .bufferBindings = {{.stride = 0}},
        .attributes = {{.name = "inPosition", .format = Format::Float3, .bufferIndex = 0, .offset = 0}},
    };
    const FramebufferLayout framebufferLayout = {
        .colorFormat = surface.GetColorFormat(),
        .depthStencilFormat = surface.GetDepthFormat(),
        .sampleCount = surface.GetSampleCount(),
    };
    const auto pipeline = device.CreatePipeline({
        .shader = device.CreateShader(shaderData, "Shader"),
        .vertexLayout = vertexLayout,
        .renderStates = {},
        .renderPasses = {{.framebufferLayout = framebufferLayout}},
    });
    EXPECT_THAT(pipeline.get(), NotNull());
}

TYPED_TEST(SurfaceTest, RenderToSurface)
{
    auto& surface = this->GetSurface();
    auto& device = this->GetDevice();

    auto renderer = device.CreateRenderer(nullptr);

    renderer->BeginFrame({});
    const RenderList renderList = {
        .clearState = {.colorValue = Color{1.0f, 0.0f, 0.0f, 1.0f}},
    };
    renderer->RenderToSurface(surface, renderList);
    renderer->EndFrame();
    renderer->WaitForGpu();
}

TYPED_TEST(SurfaceTest, RenderToSurfaceWithoutClear)
{
    auto& surface = this->GetSurface();
    auto& device = this->GetDevice();

    auto renderer = device.CreateRenderer(nullptr);

    renderer->BeginFrame({});
    const auto renderList = RenderList{};
    renderer->RenderToSurface(surface, renderList);
    renderer->EndFrame();
}

TYPED_TEST(SurfaceTest, RenderToSurfaceTwice)
{
    auto& surface = this->GetSurface();
    auto& device = this->GetDevice();

    auto renderer = device.CreateRenderer(nullptr);

    const RenderList renderListWithClear = {
        .clearState = {.colorValue = Color{1.0f, 0.0f, 0.0f, 1.0f}},
    };
    const auto renderListWithoutClear = RenderList{};

    renderer->BeginFrame({});
    renderer->RenderToSurface(surface, renderListWithClear);
    renderer->EndFrame();

    renderer->BeginFrame({});
    renderer->RenderToSurface(surface, renderListWithoutClear);
    renderer->EndFrame();
}

} // namespace
