
#pragma once

#include "Teide/Buffer.h"

class MemoryAllocator;

Buffer CreateBufferUninitialized(
    vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryFlags, vk::Device device,
    MemoryAllocator& allocator);
