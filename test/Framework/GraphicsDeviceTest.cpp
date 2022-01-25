
#include "Framework/GraphicsDevice.h"

#include <SDL.h>
#include <gmock/gmock.h>

using namespace testing;

namespace
{

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
