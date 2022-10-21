
#pragma once

#include "Teide/TextureData.h"

#include <bitset>

namespace Teide
{

enum class BlendFactor
{
	Zero,
	One,
	SrcColor,
	InvSrcColor,
	SrcAlpha,
	InvSrcAlpha,
	DestAlpha,
	InvDestAlpha,
	DestColor,
	InvDestColor,
	SrcAlphaSaturate,
};

enum class BlendOp
{
	Add,
	Subtract,
	RevSubtract,
	Min,
	Max,
};

enum class StencilOp
{
	Keep,
	Zero,
	Replace,
	Increment,
	Decrement,
	IncrementSaturate,
	DecrementSaturate,
	Invert,
};

enum class FillMode
{
	Solid,
	Wireframe,
	Point,
};

enum class CullMode
{
	None,
	Clockwise,
	Anticlockwise,
};

using Mask8 = std::bitset<8>;

struct DepthState
{
	bool depthTest = true;
	bool depthWrite = true;
	CompareOp depthFunc = CompareOp::LessEqual;
};

struct BlendFunc
{
	BlendFactor source = BlendFactor::One;
	BlendFactor dest = BlendFactor::Zero;
	BlendOp op = BlendOp::Add;
};

struct BlendState
{
	BlendFunc blendFunc;
	std::optional<BlendFunc> alphaBlendFunc;
};

struct ColorMask
{
	bool red : 1 = true;
	bool green : 1 = true;
	bool blue : 1 = true;
	bool alpha : 1 = true;
};

struct StencilState
{
	CompareOp stencilFunc = CompareOp::Always;
	int stencilRef = 0;
	StencilOp stencilFail = StencilOp::Keep;
	StencilOp stencilZFail = StencilOp::Keep;
	StencilOp stencilPass = StencilOp::Keep;
	Mask8 stencilMask = 0xFF;
	Mask8 stencilWriteMask = 0xFF;
};

struct RasterState
{
	FillMode fillMode = FillMode::Solid;
	CullMode cullMode = CullMode::Anticlockwise;
	float depthBiasConstant = 0.0f;
	float depthBiasSlope = 0.0f;
	bool multisampleEnable = false;
	bool antialiasedLineEnable = false;
	float lineWidth = 1.0f;
};

struct RenderStates
{
	std::optional<BlendState> blendState;
	ColorMask colorWriteMask;
	DepthState depthState;
	std::optional<StencilState> stencilState;
	RasterState rasterState;
};

struct FramebufferLayout
{
	std::optional<Format> colorFormat;
	std::optional<Format> depthStencilFormat;
	std::uint32_t sampleCount = 1;

	auto operator<=>(const FramebufferLayout&) const = default;
};

} // namespace Teide
