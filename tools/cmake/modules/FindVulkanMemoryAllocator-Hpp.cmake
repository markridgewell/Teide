# Find package in vcpkg ports under a different name
find_package(unofficial-vulkan-memory-allocator-hpp QUIET)
if(unofficial-vulkan-memory-allocator-hpp_FOUND)
    add_library(VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp ALIAS
                unofficial::VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp)
    find_package_handle_standard_args(VulkanMemoryAllocator-Hpp DEFAULT_MSG)
else()
    # Fall back to config mode to find the package locally
    find_package(VulkanMemoryAllocator-Hpp CONFIG)
endif()
