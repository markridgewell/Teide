
#pragma once

#include "Teide/ForwardDeclare.h"
#include "Teide/ParameterBlock.h"
#include "Teide/Renderer.h"
#include "Teide/Surface.h"

#include <thread>

struct SDL_Window;

namespace Teide
{

void EnableSoftwareRendering();

struct GraphicsSettings
{
    uint32 numThreads = std::thread::hardware_concurrency();
};

class GraphicsDevice
{
public:
    virtual ~GraphicsDevice() = default;

    GraphicsDevice() = default;
    GraphicsDevice(const GraphicsDevice&) = delete;
    GraphicsDevice(GraphicsDevice&&) = delete;
    GraphicsDevice& operator=(const GraphicsDevice&) = delete;
    GraphicsDevice& operator=(GraphicsDevice&&) = delete;

    virtual SurfacePtr CreateSurface(SDL_Window* window, bool multisampled) = 0;
    virtual RendererPtr CreateRenderer(ShaderEnvironmentPtr shaderEnvironment) = 0;

    virtual BufferPtr CreateBuffer(const BufferData& data, const char* name) = 0;
    virtual ShaderEnvironmentPtr CreateShaderEnvironment(const ShaderEnvironmentData& data, const char* name) = 0;
    virtual ShaderPtr CreateShader(const ShaderData& data, const char* name) = 0;
    virtual TexturePtr CreateTexture(const TextureData& data, const char* name) = 0;
    virtual MeshPtr CreateMesh(const MeshData& data, const char* name) = 0;
    virtual PipelinePtr CreatePipeline(const PipelineData& data) = 0;
    virtual ParameterBlockPtr CreateParameterBlock(const ParameterBlockData& data, const char* name) = 0;
};

using GraphicsDevicePtr = std::unique_ptr<GraphicsDevice>;

struct DeviceAndSurface
{
    GraphicsDevicePtr device;
    SurfacePtr surface;
};

DeviceAndSurface CreateDeviceAndSurface(SDL_Window* window, bool multisampled = false, const GraphicsSettings& settings = {});
GraphicsDevicePtr CreateHeadlessDevice(const GraphicsSettings& settings = {});

} // namespace Teide
