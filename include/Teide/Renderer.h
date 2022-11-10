
#pragma once

#include "GeoLib/Box.h"
#include "GeoLib/Vector.h"
#include "Teide/BytesView.h"
#include "Teide/ForwardDeclare.h"
#include "Teide/ParameterBlock.h"
#include "Teide/Pipeline.h"
#include "Teide/Surface.h"
#include "Teide/Task.h"

#include <array>
#include <compare>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>

namespace Teide
{

class GraphicsDevice;

struct ViewportRegion
{
	float left = 0.0f;
	float top = 0.0f;
	float right = 1.0f;
	float bottom = 1.0f;
};

struct RenderObject
{
	MeshPtr mesh = nullptr;
	PipelinePtr pipeline = nullptr;
	ParameterBlockPtr materialParameters = nullptr;
	ShaderParameters objectParameters;
};

using Color = std::array<float, 4>;

struct RenderList
{
	std::string name;

	std::optional<Color> clearColorValue;
	std::optional<float> clearDepthValue;
	std::optional<std::uint32_t> clearStencilValue;

	ShaderParameters viewParameters;

	ViewportRegion viewportRegion;
	std::optional<Geo::Box2i> scissor;

	std::vector<RenderObject> objects;
};

struct RenderToTextureResult
{
	TexturePtr colorTexture;
	TexturePtr depthStencilTexture;
};

struct RenderTargetInfo
{
	Geo::Size2i size;
	FramebufferLayout framebufferLayout;
	SamplerState samplerState;
	bool captureColor = false;
	bool captureDepthStencil = false;
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
	virtual void BeginFrame(ShaderParameters sceneParameters) = 0;
	virtual void EndFrame() = 0;

	virtual RenderToTextureResult RenderToTexture(const RenderTargetInfo& renderTarget, RenderList renderList) = 0;
	virtual void RenderToSurface(Surface& surface, RenderList renderList) = 0;

	virtual Task<TextureData> CopyTextureData(TexturePtr texture) = 0;
};

using RendererPtr = std::unique_ptr<Renderer>;

} // namespace Teide
