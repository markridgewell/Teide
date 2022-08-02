
#pragma once

#include <memory>

struct BufferData;
struct ShaderData;
struct TextureData;
struct VertexLayout;
struct RenderStates;
struct ParameterBlockData;

class Buffer;
class DynamicBuffer;
class Shader;
class Texture;
class Pipeline;
struct ParameterBlock;

using BufferPtr = std::shared_ptr<const Buffer>;
using DynamicBufferPtr = std::shared_ptr<DynamicBuffer>;
using ShaderPtr = std::shared_ptr<const Shader>;
using TexturePtr = std::shared_ptr<const Texture>;
using DynamicTexturePtr = std::shared_ptr<Texture>;
using PipelinePtr = std::shared_ptr<const Pipeline>;
using ParameterBlockPtr = std::shared_ptr<const ParameterBlock>;
using DynamicParameterBlockPtr = std::shared_ptr<ParameterBlock>;
