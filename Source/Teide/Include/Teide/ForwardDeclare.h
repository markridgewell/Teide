
#pragma once

#include <memory>

struct BufferData;
struct ShaderData;
struct TextureData;
struct VertexLayout;
struct RenderStates;

class Buffer;
class DynamicBuffer;
struct Shader;
class Texture;
struct Pipeline;
struct ParameterBlock;
struct ParameterBlockData;

using BufferPtr = std::shared_ptr<const Buffer>;
using DynamicBufferPtr = std::shared_ptr<DynamicBuffer>;
using WritableBufferPtr = std::shared_ptr<Buffer>;
using ShaderPtr = std::shared_ptr<const Shader>;
using TexturePtr = std::shared_ptr<const Texture>;
using DynamicTexturePtr = std::shared_ptr<Texture>;
using PipelinePtr = std::shared_ptr<const Pipeline>;
using ParameterBlockPtr = std::shared_ptr<const ParameterBlock>;
using DynamicParameterBlockPtr = std::shared_ptr<ParameterBlock>;
