td_add_library(
    Teide
    SOURCES Include/Teide/Buffer.h
            Include/Teide/BytesView.h
            Include/Teide/ForwardDeclare.h
            Include/Teide/GraphicsDevice.h
            Include/Teide/Internal/CommandBuffer.h
            Include/Teide/Internal/CpuExecutor.h
            Include/Teide/Internal/GpuExecutor.h
            Include/Teide/Internal/MemoryAllocator.h
            Include/Teide/Internal/Scheduler.h
            Include/Teide/Internal/Vulkan.h
            Include/Teide/ParameterBlock.h
            Include/Teide/Pipeline.h
            Include/Teide/Renderer.h
            Include/Teide/Shader.h
            Include/Teide/Surface.h
            Include/Teide/Texture.h
            Source/Buffer.cpp
            Source/GraphicsDevice.cpp
            Source/Internal/CommandBuffer.cpp
            Source/Internal/CpuExecutor.cpp
            Source/Internal/GpuExecutor.cpp
            Source/Internal/MemoryAllocator.cpp
            Source/Internal/Scheduler.cpp
            Source/Internal/Vulkan.cpp
            Source/ParameterBlock.cpp
            Source/Renderer.cpp
            Source/Surface.cpp
            Source/Texture.cpp
            Vulkan.natvis)

target_link_libraries(
    Teide
    PUBLIC Vulkan::Vulkan
           SDL2::SDL2
           fmt::fmt
           spdlog::spdlog
           Taskflow::Taskflow
           function2::function2)

target_compile_definitions(Teide PUBLIC VULKAN_HPP_NO_STRUCT_CONSTRUCTORS VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

# Test
td_add_test(
    TeideTest
    SOURCES Test/CpuExecutorTest.cpp
            Test/GpuExecutorTest.cpp
            Test/GraphicsDeviceTest.cpp
            Test/Main.cpp
            Test/RendererTest.cpp
            Test/SchedulerTest.cpp
            Test/SurfaceTest.cpp
            Test/TestUtils.cpp
            Test/TestUtils.h)
