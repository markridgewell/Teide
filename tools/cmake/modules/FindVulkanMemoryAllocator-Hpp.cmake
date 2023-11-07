# Try to find the package in CONFIG mode
find_package(VulkanMemoryAllocator-Hpp CONFIG QUIET)

if(NOT VulkanMemoryAllocator-Hpp_FOUND)
    # Find package in vcpkg ports under a different name
    find_package(unofficial-vulkan-memory-allocator-hpp)
    if(NOT TARGET VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp)
        add_library(VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp ALIAS
                    unofficial::VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp)
    endif()
    find_package_handle_standard_args(VulkanMemoryAllocator-Hpp DEFAULT_MSG)
endif()
