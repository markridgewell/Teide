
#include "ShaderCompiler/ShaderCompiler.h"

#include "Teide/Assert.h"
#include "Teide/Definitions.h"

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <array>
#include <memory>
#include <numeric>
#include <optional>
#include <span>

using namespace Teide;

template <>
struct fmt::formatter<ShaderVariableType> : ostream_formatter
{};
template <>
struct fmt::formatter<ShaderVariableType::BaseType> : ostream_formatter
{};

namespace
{
constexpr auto PblockNames = std::array{"Scene", "View", "Material", "Object"};
constexpr auto PblockNamesLower = std::array{"scene", "view", "material", "object"};

constexpr int VulkanGlslDialectVersion = 450;

// Taken from glslang StandAlone/ResourceLimits.cpp
constexpr TBuiltInResource DefaultTBuiltInResource
    = {.maxLights = 32,
       .maxClipPlanes = 6,
       .maxTextureUnits = 32,
       .maxTextureCoords = 32,
       .maxVertexAttribs = 64,
       .maxVertexUniformComponents = 4096,
       .maxVaryingFloats = 64,
       .maxVertexTextureImageUnits = 32,
       .maxCombinedTextureImageUnits = 80,
       .maxTextureImageUnits = 32,
       .maxFragmentUniformComponents = 4096,
       .maxDrawBuffers = 32,
       .maxVertexUniformVectors = 128,
       .maxVaryingVectors = 8,
       .maxFragmentUniformVectors = 16,
       .maxVertexOutputVectors = 16,
       .maxFragmentInputVectors = 15,
       .minProgramTexelOffset = -8,
       .maxProgramTexelOffset = 7,
       .maxClipDistances = 8,
       .maxComputeWorkGroupCountX = 65535,
       .maxComputeWorkGroupCountY = 65535,
       .maxComputeWorkGroupCountZ = 65535,
       .maxComputeWorkGroupSizeX = 1024,
       .maxComputeWorkGroupSizeY = 1024,
       .maxComputeWorkGroupSizeZ = 64,
       .maxComputeUniformComponents = 1024,
       .maxComputeTextureImageUnits = 16,
       .maxComputeImageUniforms = 8,
       .maxComputeAtomicCounters = 8,
       .maxComputeAtomicCounterBuffers = 1,
       .maxVaryingComponents = 60,
       .maxVertexOutputComponents = 64,
       .maxGeometryInputComponents = 64,
       .maxGeometryOutputComponents = 128,
       .maxFragmentInputComponents = 128,
       .maxImageUnits = 8,
       .maxCombinedImageUnitsAndFragmentOutputs = 8,
       .maxCombinedShaderOutputResources = 8,
       .maxImageSamples = 0,
       .maxVertexImageUniforms = 0,
       .maxTessControlImageUniforms = 0,
       .maxTessEvaluationImageUniforms = 0,
       .maxGeometryImageUniforms = 0,
       .maxFragmentImageUniforms = 8,
       .maxCombinedImageUniforms = 8,
       .maxGeometryTextureImageUnits = 16,
       .maxGeometryOutputVertices = 256,
       .maxGeometryTotalOutputComponents = 1024,
       .maxGeometryUniformComponents = 1024,
       .maxGeometryVaryingComponents = 64,
       .maxTessControlInputComponents = 128,
       .maxTessControlOutputComponents = 128,
       .maxTessControlTextureImageUnits = 16,
       .maxTessControlUniformComponents = 1024,
       .maxTessControlTotalOutputComponents = 4096,
       .maxTessEvaluationInputComponents = 128,
       .maxTessEvaluationOutputComponents = 128,
       .maxTessEvaluationTextureImageUnits = 16,
       .maxTessEvaluationUniformComponents = 1024,
       .maxTessPatchComponents = 120,
       .maxPatchVertices = 32,
       .maxTessGenLevel = 64,
       .maxViewports = 16,
       .maxVertexAtomicCounters = 0,
       .maxTessControlAtomicCounters = 0,
       .maxTessEvaluationAtomicCounters = 0,
       .maxGeometryAtomicCounters = 0,
       .maxFragmentAtomicCounters = 8,
       .maxCombinedAtomicCounters = 8,
       .maxAtomicCounterBindings = 1,
       .maxVertexAtomicCounterBuffers = 0,
       .maxTessControlAtomicCounterBuffers = 0,
       .maxTessEvaluationAtomicCounterBuffers = 0,
       .maxGeometryAtomicCounterBuffers = 0,
       .maxFragmentAtomicCounterBuffers = 1,
       .maxCombinedAtomicCounterBuffers = 1,
       .maxAtomicCounterBufferSize = 16384,
       .maxTransformFeedbackBuffers = 4,
       .maxTransformFeedbackInterleavedComponents = 64,
       .maxCullDistances = 8,
       .maxCombinedClipAndCullDistances = 8,
       .maxSamples = 4,
       .maxMeshOutputVerticesNV = 256,
       .maxMeshOutputPrimitivesNV = 512,
       .maxMeshWorkGroupSizeX_NV = 32,
       .maxMeshWorkGroupSizeY_NV = 1,
       .maxMeshWorkGroupSizeZ_NV = 1,
       .maxTaskWorkGroupSizeX_NV = 32,
       .maxTaskWorkGroupSizeY_NV = 1,
       .maxTaskWorkGroupSizeZ_NV = 1,
       .maxMeshViewCountNV = 4,
       .maxDualSourceDrawBuffersEXT = 1,

       .limits = {
           .nonInductiveForLoops = true,
           .whileLoops = true,
           .doWhileLoops = true,
           .generalUniformIndexing = true,
           .generalAttributeMatrixVectorIndexing = true,
           .generalVaryingIndexing = true,
           .generalSamplerIndexing = true,
           .generalVariableIndexing = true,
           .generalConstantMatrixVectorIndexing = true,
       }};

constexpr std::string_view ShaderCommon = R"--(
#version 450
#extension GL_EXT_scalar_block_layout : enable

layout(std430) uniform;
layout(std430) buffer;

vec4 mul(mat4 m, vec4 v) {
    return v * m;
}

mat4 mul(mat4 m1, mat4 m2) {
    return m1 * m1;
}
)--";

std::unique_ptr<glslang::TShader> CompileStage(std::string_view shaderSource, EShLanguage stage, glslang::EShSource source)
{
    auto shader = std::make_unique<glslang::TShader>(stage);
    const auto inputStrings = std::array{data(ShaderCommon), data(shaderSource)};
    const auto inputStringLengths = std::array{static_cast<int>(size(ShaderCommon)), static_cast<int>(size(shaderSource))};
    shader->setStringsWithLengths(data(inputStrings), data(inputStringLengths), static_cast<int>(size(inputStringLengths)));
    shader->setEnvInput(source, stage, glslang::EShClientVulkan, VulkanGlslDialectVersion);
    shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
    shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    if (!shader->parse(&DefaultTBuiltInResource, VulkanGlslDialectVersion, false, EShMsgDefault))
    {
        throw CompileError(shader->getInfoLog());
    }
    return shader;
};

ParameterBlockDesc& GetPblockLayout(ShaderData& data, int64 set)
{
    switch (set)
    {
        case 0: return data.environment.scenePblock;
        case 1: return data.environment.viewPblock;
        case 2: return data.materialPblock;
        case 3: return data.objectPblock;

        default: Unreachable();
    }
}

ShaderStageFlags GetShaderStageFlags(EShLanguageMask lang)
{
    ShaderStageFlags ret{};
    if (lang & EShLangVertexMask)
    {
        ret |= ShaderStageFlags::Vertex;
    }
    if (lang & EShLangTessControlMask)
    {
        // ret |= vk::ShaderStageFlagBits::eTessellationControl;
    }
    if (lang & EShLangTessEvaluationMask)
    {
        // ret |= vk::ShaderStageFlagBits::eTessellationEvaluation;
    }
    if (lang & EShLangGeometryMask)
    {
        // ret |= vk::ShaderStageFlagBits::eGeometry;
    }
    if (lang & EShLangFragmentMask)
    {
        ret |= ShaderStageFlags::Pixel;
    }
    if (lang & EShLangComputeMask)
    {
        // ret |= vk::ShaderStageFlagBits::eCompute;
    }
    if (lang & EShLangRayGenMask)
    {
        // ret |= vk::ShaderStageFlagBits::eRaygenKHR;
    }
    if (lang & EShLangIntersectMask)
    {
        // ret |= vk::ShaderStageFlagBits::eIntersectionKHR;
    }
    if (lang & EShLangAnyHitMask)
    {
        // ret |= vk::ShaderStageFlagBits::eAnyHitKHR;
    }
    if (lang & EShLangClosestHitMask)
    {
        // ret |= vk::ShaderStageFlagBits::eClosestHitKHR;
    }
    if (lang & EShLangMissMask)
    {
        // ret |= vk::ShaderStageFlagBits::eMissKHR;
    }
    if (lang & EShLangCallableMask)
    {
        // ret |= vk::ShaderStageFlagBits::eCallableKHR;
    }
    return ret;
}

void ReflectUniforms(ParameterBlockDesc& pblock, const glslang::TObjectReflection& uniformBlock)
{
    pblock.uniformsStages = GetShaderStageFlags(uniformBlock.stages);
};

void CompileShader(ShaderData& data, std::string_view vertexSource, std::string_view pixelSource, glslang::EShSource source)
{
    auto vertexShader = CompileStage(vertexSource, EShLangVertex, source);
    auto pixelShader = CompileStage(pixelSource, EShLangFragment, source);

    auto program = glslang::TProgram();
    program.addShader(vertexShader.get());
    program.addShader(pixelShader.get());

    if (!program.link(EShMsgDefault))
    {
        throw CompileError(program.getInfoLog());
    }

    spv::SpvBuildLogger logger;
    glslang::SpvOptions spvOptions;
    spvOptions.generateDebugInfo = IsDebugBuild;
    spvOptions.stripDebugInfo = !IsDebugBuild;
    spvOptions.disableOptimizer = false;
    spvOptions.optimizeSize = true;
    spvOptions.disassemble = false;
    spvOptions.validate = true;

    glslang::GlslangToSpv(*program.getIntermediate(EShLangVertex), data.vertexShader.spirv, &logger, &spvOptions);
    glslang::GlslangToSpv(*program.getIntermediate(EShLangFragment), data.pixelShader.spirv, &logger, &spvOptions);

    program.buildReflection(EShReflectionAllBlockVariables | EShReflectionSeparateBuffers | EShReflectionAllIOVariables);

    for (int i = 0; i < program.getNumUniformBlocks(); i++)
    {
        const auto& uniformBlock = program.getUniformBlock(i);
        const auto& name = uniformBlock.name;
        const int64 set = std::distance(PblockNames.begin(), std::ranges::find(PblockNames, name));
        ReflectUniforms(GetPblockLayout(data, set), uniformBlock);
    }
}

glslang::EShSource GetEShSource(ShaderLanguage language)
{
    switch (language)
    {
        case ShaderLanguage::Glsl: return glslang::EShSourceGlsl;
        case ShaderLanguage::Hlsl: return glslang::EShSourceHlsl;
    }
    Unreachable();
}

template <int Set>
void BuildUniformBuffer(std::string& source, const ParameterBlockDesc& pblock)
{
    if (std::ranges::count_if(pblock.parameters, [](const auto& v) { return !IsResourceType(v.type.baseType); }) == 0)
    {
        // No uniforms in pblock
        return;
    }

    auto out = std::back_inserter(source);

    if (BuildParameterBlockLayout(pblock, Set).isPushConstant)
    {
        // Build push constants
        fmt::format_to(out, "layout(push_constant) uniform {} {{\n", PblockNames[Set]);
    }
    else
    {
        // Build uniform block
        fmt::format_to(out, "layout(set = {}, binding = 0) uniform {} {{\n", Set, PblockNames[Set]);
    }

    for (const auto& variable : pblock.parameters)
    {
        if (IsResourceType(variable.type.baseType))
        {
            continue;
        }

        fmt::format_to(out, "    {} {};\n", variable.type, variable.name);
    }
    fmt::format_to(out, "}} {};\n\n", PblockNamesLower[Set]);
}

template <int Set>
void BuildResourceBindings(std::string& source, const ParameterBlockDesc& pblock)
{
    if (std::ranges::count_if(pblock.parameters, [](const auto& v) { return IsResourceType(v.type.baseType); }) == 0)
    {
        // No resources in pblock
        return;
    }

    auto out = std::back_inserter(source);

    usize slot = 1;
    for (const auto& parameter : pblock.parameters)
    {
        if (IsResourceType(parameter.type.baseType))
        {
            fmt::format_to(out, "layout(set = {}, binding = {}) uniform {} {};\n", Set, slot, parameter.type, parameter.name);
            slot++;
        }
    }

    source += '\n';
}

template <int Set>
void BuildBindings(std::string& source, const ParameterBlockDesc& pblock)
{
    BuildUniformBuffer<Set>(source, pblock);
    BuildResourceBindings<Set>(source, pblock);
}

void BuildVaryings(std::string& source, ShaderStageData& data, const ShaderStageDefinition& sourceStage)
{
    auto out = std::back_inserter(source);

    for (usize i = 0; i < sourceStage.inputs.size(); i++)
    {
        const auto& input = sourceStage.inputs[i];

        if (input.name.starts_with("gl_"))
        {
            continue;
        }

        data.inputs.push_back(input);
        fmt::format_to(out, "layout(location = {}) in {} {};\n", i, input.type, input.name);
    }

    for (usize i = 0; i < sourceStage.outputs.size(); i++)
    {
        const auto& output = sourceStage.outputs[i];

        if (output.name.starts_with("gl_"))
        {
            continue;
        }

        data.outputs.push_back(output);
        fmt::format_to(out, "layout(location = {}) out {} {};\n", i, output.type, output.name);
    }

    source += '\n';
}

std::atomic<int> s_numInstances;

} // namespace

ShaderCompiler::ShaderCompiler()
{
    if (s_numInstances.fetch_add(1) == 0)
    {
        glslang::InitializeProcess();
    }
}

ShaderCompiler::~ShaderCompiler()
{
    if (s_numInstances.fetch_sub(1) == 1)
    {
        glslang::FinalizeProcess();
    }
}

// This function cannot be static because it depends on side-effects invoked by the constructor.
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
ShaderData ShaderCompiler::Compile(const ShaderSourceData& sourceData) const
{
    ShaderData data;
    data.environment = sourceData.environment;
    data.materialPblock = sourceData.materialPblock;
    data.objectPblock = sourceData.objectPblock;

    std::string parameters;
    BuildBindings<0>(parameters, sourceData.environment.scenePblock);
    BuildBindings<1>(parameters, sourceData.environment.viewPblock);
    BuildBindings<2>(parameters, sourceData.materialPblock);
    BuildBindings<3>(parameters, sourceData.objectPblock);

    std::string vertexShader = parameters;
    BuildVaryings(vertexShader, data.vertexShader, sourceData.vertexShader);
    vertexShader += sourceData.vertexShader.source;

    std::string pixelShader = parameters;
    BuildVaryings(pixelShader, data.pixelShader, sourceData.pixelShader);
    pixelShader += sourceData.pixelShader.source;

    CompileShader(data, vertexShader, pixelShader, GetEShSource(sourceData.language));
    return data;
}
