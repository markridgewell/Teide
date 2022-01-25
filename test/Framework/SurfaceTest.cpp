
#include "Framework/GraphicsDevice.h"

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

TEST(SurfaceTest, CreateSurface)
{
	const auto window = UniqueSDLWindow(SDL_CreateWindow("Test", 0, 0, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN));
	ASSERT_THAT(window, NotNull()) << SDL_GetError();

	auto device = GraphicsDevice(window.get());
	auto surface = device.CreateSurface(window.get(), false);
	EXPECT_THAT(surface.GetExtent().width, Eq(800u));
	EXPECT_THAT(surface.GetExtent().height, Eq(600u));
}

TEST(SurfaceTest, CreateSurfaceMultisampled)
{
	auto window = UniqueSDLWindow(SDL_CreateWindow("Test", 0, 0, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN));
	ASSERT_THAT(window, NotNull()) << SDL_GetError();

	auto device = GraphicsDevice(window.get());
	auto surface = device.CreateSurface(window.get(), true);
	EXPECT_THAT(surface.GetExtent().width, Eq(800u));
	EXPECT_THAT(surface.GetExtent().height, Eq(600u));
}

} // namespace
