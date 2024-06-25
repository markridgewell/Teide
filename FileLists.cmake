set(sources
    include/GeoLib/Angle.h
    include/GeoLib/Box.h
    include/GeoLib/ForwardDeclare.h
    include/GeoLib/Matrix.h
    include/GeoLib/Scalar.h
    include/GeoLib/Vector.h
    include/ShaderCompiler/ShaderCompiler.h
    include/Teide/AbstractBase.h
    include/Teide/Assert.h
    include/Teide/BasicTypes.h
    include/Teide/Buffer.h
    include/Teide/BytesView.h
    include/Teide/Definitions.h
    include/Teide/Device.h
    include/Teide/Format.h
    include/Teide/ForwardDeclare.h
    include/Teide/Handle.h
    include/Teide/Hash.h
    include/Teide/Mesh.h
    include/Teide/MeshData.h
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
    include/Teide/Visitable.h
    src/GeoLib.natvis
    src/ShaderCompiler/ShaderCompiler.cpp
    src/Teide.natvis
    src/Teide/Assert.cpp
    src/Teide/CommandBuffer.cpp
    src/Teide/CommandBuffer.h
    src/Teide/CpuExecutor.cpp
    src/Teide/CpuExecutor.h
    src/Teide/DescriptorPool.cpp
    src/Teide/DescriptorPool.h
    src/Teide/Format.cpp
    src/Teide/GpuExecutor.cpp
    src/Teide/GpuExecutor.h
    src/Teide/Scheduler.cpp
    src/Teide/Scheduler.h
    src/Teide/ShaderData.cpp
    src/Teide/TextureData.cpp
    src/Teide/Util/ResourceMap.h
    src/Teide/Util/StaticMap.h
    src/Teide/Util/ThreadUtils.cpp
    src/Teide/Util/ThreadUtils.h
    src/Teide/VmaImpl.cpp
    src/Teide/Vulkan.cpp
    src/Teide/Vulkan.h
    src/Teide/VulkanBuffer.cpp
    src/Teide/VulkanBuffer.h
    src/Teide/VulkanConfig.h
    src/Teide/VulkanDevice.cpp
    src/Teide/VulkanDevice.h
    src/Teide/VulkanLoader.cpp
    src/Teide/VulkanLoader.h
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
    src/Vulkan.natvis
    src/vkex/vkex_utils.hpp)
