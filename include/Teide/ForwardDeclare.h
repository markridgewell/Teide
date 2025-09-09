
#pragma once

#include "Teide/BasicTypes.h"

#include <memory>

namespace Teide
{

struct BufferData;
struct ShaderData;
struct KernelData;
struct ShaderEnvironmentData;
struct TextureData;
struct MeshData;
struct VertexLayout;
struct RenderStates;
struct ParameterBlockData;

class Buffer;
class Shader;
class Kernel;
class ShaderEnvironment;
class Texture;
class Mesh;
class Pipeline;
class ParameterBlockLayout;
class ParameterBlock;

using BufferPtr = std::shared_ptr<const Buffer>;
using ShaderPtr = std::shared_ptr<const Shader>;
using ShaderEnvironmentPtr = std::shared_ptr<const ShaderEnvironment>;
using MeshPtr = std::shared_ptr<const Mesh>;
using PipelinePtr = std::shared_ptr<const Pipeline>;
using ParameterBlockLayoutPtr = std::shared_ptr<const ParameterBlockLayout>;

enum class ResourceLifetime : uint8
{
    Permanent,
    Transient
};

} // namespace Teide
