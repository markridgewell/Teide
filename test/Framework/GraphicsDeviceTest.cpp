
#include "Framework/GraphicsDevice.h"

#include <SDL.h>
#include <gmock/gmock.h>

using namespace testing;

namespace
{
TEST(GraphicsDeviceTest, CreateSurface)
{
	SDL_Window* window = SDL_CreateWindow("Test", 0, 0, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);

	auto device = GraphicsDevice(window);
	auto surface = device.CreateSurface(window, false);
	EXPECT_THAT(surface.GetExtent().width, Eq(800u));
	EXPECT_THAT(surface.GetExtent().height, Eq(600u));
}

TEST(GraphicsDeviceTest, CreateSurfaceMultisampled)
{
	SDL_Window* window = SDL_CreateWindow("Test", 0, 0, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);

	auto device = GraphicsDevice(window);
	auto surface = device.CreateSurface(window, true);
	EXPECT_THAT(surface.GetExtent().width, Eq(800u));
	EXPECT_THAT(surface.GetExtent().height, Eq(600u));
}

TEST(GraphicsDeviceTest, CreateBuffer)
{
	auto device = GraphicsDevice();
	const auto contents = std::array{1, 2, 3, 4};
	const auto data = BufferData{
	    .usage = vk::BufferUsageFlagBits::eVertexBuffer,
	    .memoryFlags = vk::MemoryPropertyFlagBits ::eDeviceLocal,
	    .data = contents,
	};
	auto buffer = device.CreateBuffer(data, "");
	EXPECT_THAT(buffer.get(), NotNull());
	EXPECT_THAT(buffer->size, Eq(contents.size() * sizeof(contents[0])));
}

} // namespace
