
#pragma once

#include "Teide/MeshData.h"
#include "Teide/TextureData.h"

#include <bitset>

namespace Teide
{

enum class BlendFactor : uint8
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

enum class BlendOp : uint8
{
    Add,
    Subtract,
    RevSubtract,
    Min,
    Max,
};

enum class StencilOp : uint8
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

enum class FillMode : uint8
{
    Solid,
    Wireframe,
    Point,
};

enum class CullMode : uint8
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
    uint32 sampleCount = 1;
    bool captureColor = false;
    bool captureDepthStencil = false;
    bool resolveColor = false;
    bool resolveDepthStencil = false;

    bool operator==(const FramebufferLayout& other) const = default;
    void Visit(auto f) const
    {
        return f(colorFormat, depthStencilFormat, sampleCount, captureColor, captureDepthStencil, resolveColor, resolveDepthStencil);
    }
};

struct RenderOverrides
{
    std::optional<float> depthBiasConstant;
    std::optional<float> depthBiasSlope;

    bool operator==(const RenderOverrides&) const = default;
    void Visit(auto f) const { return f(depthBiasConstant, depthBiasSlope); }
};

struct RenderPassDesc
{
    FramebufferLayout framebufferLayout;
    RenderOverrides renderOverrides;

    bool operator==(const RenderPassDesc&) const = default;
    void Visit(auto f) const { return f(framebufferLayout, renderOverrides); }
};

struct PipelineData
{
    ShaderPtr shader;
    VertexLayout vertexLayout;
    RenderStates renderStates;
    std::vector<RenderPassDesc> renderPasses;
};

} // namespace Teide
