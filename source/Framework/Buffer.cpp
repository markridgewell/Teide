
#include "Framework/Buffer.h"

#include <cassert>

void DynamicBuffer::SetData(int currentFrame, BytesView data)
{
	const auto& allocation = buffers[currentFrame % buffers.size()].allocation;
	assert(allocation.mappedData);
	memcpy(allocation.mappedData, data.data(), data.size());
}
