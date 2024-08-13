
#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_SPACESHIP_OPERATOR

#include <vulkan/vulkan.hpp>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#if !defined(NDEBUG)
#    define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#    define VMA_DEBUG_MARGIN 16
#    define VMA_DEBUG_DETECT_CORRUPTION 1
#endif

namespace vk::detail
{
// HACK: Workaround for vulkan-memory-allocator-hpp using the wrong vulkan-hpp namespace
using namespace vk;
} // namespace vk::detail

#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>
