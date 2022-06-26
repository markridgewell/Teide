
#pragma once

#include "Teide/BytesView.h"
#include "Teide/ForwardDeclare.h"
#include "Teide/Pipeline.h"
#include "Teide/Surface.h"
#include "Teide/Task.h"

#include <array>
#include <cstdint>
#include <deque>
#include <mutex>

class GraphicsDevice;

struct RenderObject
{
	BufferPtr vertexBuffer = nullptr;
	BufferPtr indexBuffer = nullptr;
	uint32_t indexCount = 0;
	PipelinePtr pipeline = nullptr;
	ParameterBlockPtr materialParameters;
	BytesView pushConstants;
};

struct RenderList
{
	std::vector<vk::ClearValue> clearValues;

	ParameterBlockPtr sceneParameters;
	ParameterBlockPtr viewParameters;

	std::vector<RenderObject> objects;
};

class Renderer
{
public:
	virtual ~Renderer() = default;

	Renderer() = default;
	Renderer(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	virtual std::uint32_t GetFrameNumber() const = 0;
	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;

	virtual void RenderToTexture(RenderableTexturePtr texture, RenderList renderList) = 0;
	virtual void RenderToSurface(Surface& surface, RenderList renderList) = 0;

	virtual Task<TextureData> CopyTextureData(RenderableTexturePtr texture) = 0;
};

using RendererPtr = std::unique_ptr<Renderer>;
