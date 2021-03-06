
#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/GraphicsDevice.h"

#include <SDL.h>
#include <gmock/gmock.h>

using namespace testing;

namespace
{
struct SDLWindowDeleter
{
	void operator()(SDL_Window* window) { SDL_DestroyWindow(window); }
};

using UniqueSDLWindow = std::unique_ptr<SDL_Window, SDLWindowDeleter>;

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

TEST(SurfaceTest, CreateSurface)
{
	const auto window = UniqueSDLWindow(SDL_CreateWindow("Test", 0, 0, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN));
	ASSERT_THAT(window, NotNull()) << SDL_GetError();

	auto device = CreateGraphicsDevice(window.get());
	auto surface = device->CreateSurface(window.get(), false);
	EXPECT_THAT(surface->GetExtent().width, Eq(800u));
	EXPECT_THAT(surface->GetExtent().height, Eq(600u));
}

TEST(SurfaceTest, CreateSurfaceMultisampled)
{
	auto window = UniqueSDLWindow(SDL_CreateWindow("Test", 0, 0, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN));
	ASSERT_THAT(window, NotNull()) << SDL_GetError();

	auto device = CreateGraphicsDevice(window.get());
	auto surface = device->CreateSurface(window.get(), true);
	EXPECT_THAT(surface->GetExtent().width, Eq(800u));
	EXPECT_THAT(surface->GetExtent().height, Eq(600u));
}

TEST(SurfaceTest, CreatePipelineForSurface)
{
	const auto window = UniqueSDLWindow(SDL_CreateWindow("Test", 0, 0, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN));
	ASSERT_THAT(window, NotNull()) << SDL_GetError();

	auto device = CreateGraphicsDevice(window.get());
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
	auto surface = device->CreateSurface(window.get(), true);
	const auto pipeline = device->CreatePipeline(*shader, vertexLayout, renderStates, *surface);
	EXPECT_THAT(pipeline.get(), NotNull());
	EXPECT_THAT(pipeline->layout, IsTrue());
	EXPECT_THAT(pipeline->pipeline, IsTrue());
}

TEST(SurfaceTest, RenderToSurface)
{
	auto window = UniqueSDLWindow(SDL_CreateWindow("Test", 0, 0, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN));
	ASSERT_THAT(window, NotNull()) << SDL_GetError();

	auto device = CreateGraphicsDevice(window.get());
	auto surface = device->CreateSurface(window.get(), true);
	auto renderer = device->CreateRenderer();

	renderer->BeginFrame();
	const auto clearColor = std::array{255, 0, 0, 255};
	const auto clearDepthStencil = vk::ClearDepthStencilValue{1.0f, 0};
	const auto clearValues = std::vector{
	    vk::ClearValue{clearColor},
	    vk::ClearValue{clearDepthStencil},
	};
	const auto renderList = RenderList{
	    .clearValues = clearValues,
	};
	renderer->RenderToSurface(*surface, renderList);
	renderer->EndFrame();
}

} // namespace
