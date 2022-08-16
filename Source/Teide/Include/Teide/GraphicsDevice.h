
#pragma once

#include "Teide/ForwardDeclare.h"
#include "Teide/ParameterBlock.h"
#include "Teide/Renderer.h"
#include "Teide/Surface.h"

struct SDL_Window;

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
	virtual RendererPtr CreateRenderer(ShaderPtr shaderEnvironment) = 0;

	virtual BufferPtr CreateBuffer(const BufferData& data, const char* name) = 0;

	virtual ShaderPtr CreateShader(const ShaderData& data, const char* name) = 0;

	virtual TexturePtr CreateTexture(const TextureData& data, const char* name) = 0;
	virtual DynamicTexturePtr CreateRenderableTexture(const TextureData& data, const char* name) = 0;

	virtual PipelinePtr CreatePipeline(const PipelineData& data) = 0;

	virtual ParameterBlockPtr CreateParameterBlock(const ParameterBlockData& data, const char* name) = 0;
};

using GraphicsDevicePtr = std::unique_ptr<GraphicsDevice>;

GraphicsDevicePtr CreateGraphicsDevice(SDL_Window* window = nullptr);
