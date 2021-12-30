
#pragma once

#include <memory>

struct Buffer;
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
struct DescriptorSet;

using BufferPtr = std::shared_ptr<const Buffer>;
using WritableBufferPtr = std::shared_ptr<Buffer>;
using ShaderPtr = std::shared_ptr<const Shader>;
using TexturePtr = std::shared_ptr<const Texture>;
using RenderableTexturePtr = std::shared_ptr<RenderableTexture>;
using PipelinePtr = std::shared_ptr<const Pipeline>;
