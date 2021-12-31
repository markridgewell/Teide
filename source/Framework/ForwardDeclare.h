
#pragma once

#include <memory>

struct Buffer;
struct DynamicBuffer;
struct BufferData;
struct Shader;
struct ShaderData;
struct Texture;
struct TextureData;
struct RenderableTexture;
struct RenderableTextureData;
struct VertexLayout;
struct RenderStates;
struct Pipeline;
struct ParameterBlock;
struct ParameterBlockData;

using BufferPtr = std::shared_ptr<const Buffer>;
using DynamicBufferPtr = std::shared_ptr<DynamicBuffer>;
using WritableBufferPtr = std::shared_ptr<Buffer>;
using ShaderPtr = std::shared_ptr<const Shader>;
using TexturePtr = std::shared_ptr<const Texture>;
using RenderableTexturePtr = std::shared_ptr<RenderableTexture>;
using PipelinePtr = std::shared_ptr<const Pipeline>;
using ParameterBlockPtr = std::shared_ptr<const ParameterBlock>;
using DynamicParameterBlockPtr = std::shared_ptr<ParameterBlock>;