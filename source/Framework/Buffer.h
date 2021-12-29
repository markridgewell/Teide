
#pragma once

#include "Framework/BytesView.h"
#include "Framework/MemoryAllocator.h"
#include "Framework/Vulkan.h"

struct Buffer
{
	vk::UniqueBuffer buffer;
	MemoryAllocation allocation;

	void SetData(BytesView data);
};
