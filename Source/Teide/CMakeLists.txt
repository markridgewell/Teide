td_add_library(
    Teide
    SOURCES Include/Teide/Buffer.h
            Include/Teide/BytesView.h
            Include/Teide/ForwardDeclare.h
            Include/Teide/GraphicsDevice.h
            Include/Teide/ParameterBlock.h
            Include/Teide/Pipeline.h
            Include/Teide/Renderer.h
            Include/Teide/Shader.h
            Include/Teide/ShaderEnvironment.h
            Include/Teide/Surface.h
            Include/Teide/Task.h
            Include/Teide/Texture.h
            Source/CommandBuffer.cpp
            Source/CommandBuffer.h
            Source/CpuExecutor.cpp
            Source/CpuExecutor.h
            Source/GpuExecutor.cpp
            Source/GpuExecutor.h
            Source/MemoryAllocator.cpp
            Source/MemoryAllocator.h
            Source/Scheduler.cpp
            Source/Scheduler.h
            Source/Vulkan.cpp
            Source/Vulkan.h
            Source/VulkanBuffer.cpp
            Source/VulkanBuffer.h
            Source/VulkanGraphicsDevice.cpp
            Source/VulkanGraphicsDevice.h
            Source/VulkanParameterBlock.cpp
            Source/VulkanParameterBlock.h
            Source/VulkanPipeline.h
            Source/VulkanRenderer.cpp
            Source/VulkanRenderer.h
            Source/VulkanShader.h
            Source/VulkanShaderEnvironment.h
            Source/VulkanSurface.cpp
            Source/VulkanSurface.h
            Source/VulkanTexture.cpp
            Source/VulkanTexture.h
            Vulkan.natvis)

target_link_libraries(
    Teide
    PUBLIC Types
           GeoLib
           Vulkan::Vulkan
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
            Test/TestData.h
            Test/TestUtils.cpp
            Test/TestUtils.h
            Test/TextureTest.cpp)

target_link_libraries(TeideTest PRIVATE Teide ShaderCompiler GTest::gtest GTest::gmock)
