set(sources
    include/GeoLib/Angle.h
    include/GeoLib/Box.h
    include/GeoLib/ForwardDeclare.h
    include/GeoLib/Matrix.h
    include/GeoLib/Scalar.h
    include/GeoLib/Vector.h
    include/ShaderCompiler/ShaderCompiler.h
    include/Teide/Buffer.h
    include/Teide/BytesView.h
    include/Teide/Definitions.h
    include/Teide/Format.h
    include/Teide/ForwardDeclare.h
    include/Teide/GraphicsDevice.h
    include/Teide/Mesh.h
    include/Teide/ParameterBlock.h
    include/Teide/Pipeline.h
    include/Teide/PipelineData.h
    include/Teide/Renderer.h
    include/Teide/Shader.h
    include/Teide/ShaderData.h
    include/Teide/ShaderEnvironment.h
    include/Teide/Surface.h
    include/Teide/Task.h
    include/Teide/Texture.h
    include/Teide/TextureData.h
    src/GeoLib.natvis
    src/ShaderCompiler/ShaderCompiler.cpp
    src/Teide/CommandBuffer.cpp
    src/Teide/CommandBuffer.h
    src/Teide/CpuExecutor.cpp
    src/Teide/CpuExecutor.h
    src/Teide/Format.cpp
    src/Teide/GpuExecutor.cpp
    src/Teide/GpuExecutor.h
    src/Teide/MemoryAllocator.cpp
    src/Teide/MemoryAllocator.h
    src/Teide/Scheduler.cpp
    src/Teide/Scheduler.h
    src/Teide/ShaderData.cpp
    src/Teide/StaticMap.h
    src/Teide/TextureData.cpp
    src/Teide/Vulkan.cpp
    src/Teide/Vulkan.h
    src/Teide/VulkanBuffer.cpp
    src/Teide/VulkanBuffer.h
    src/Teide/VulkanGraphicsDevice.cpp
    src/Teide/VulkanGraphicsDevice.h
    src/Teide/VulkanMesh.h
    src/Teide/VulkanParameterBlock.cpp
    src/Teide/VulkanParameterBlock.h
    src/Teide/VulkanPipeline.h
    src/Teide/VulkanRenderer.cpp
    src/Teide/VulkanRenderer.h
    src/Teide/VulkanShader.h
    src/Teide/VulkanShaderEnvironment.h
    src/Teide/VulkanSurface.cpp
    src/Teide/VulkanSurface.h
    src/Teide/VulkanTexture.cpp
    src/Teide/VulkanTexture.h
    src/Vulkan.natvis)

set(test_sources
    tests/GeoLib/Angle.test.cpp
    tests/GeoLib/Matrix.test.cpp
    tests/GeoLib/TestUtils.h
    tests/GeoLib/Vector.test.cpp
    tests/Main.cpp
    tests/ShaderCompiler/ShaderCompilerTest.cpp
    tests/Teide/CpuExecutorTest.cpp
    tests/Teide/GpuExecutorTest.cpp
    tests/Teide/GraphicsDeviceTest.cpp
    tests/Teide/RendererTest.cpp
    tests/Teide/SchedulerTest.cpp
    tests/Teide/ShaderDataTest.cpp
    tests/Teide/SurfaceTest.cpp
    tests/Teide/TestData.h
    tests/Teide/TestUtils.cpp
    tests/Teide/TestUtils.h
    tests/Teide/TextureDataTest.cpp
    tests/Teide/TextureTest.cpp)
