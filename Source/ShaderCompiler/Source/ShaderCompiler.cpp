
#include "ShaderCompiler/ShaderCompiler.h"

#include <fmt/format.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <array>
#include <memory>
#include <numeric>
#include <span>

namespace
{
constexpr auto PblockNames = std::array{"Scene", "View", "Material", "Object"};
constexpr auto PblockNamesLower = std::array{"scene", "view", "material", "object"};

#if _DEBUG
static constexpr bool IsDebugBuild = true;
#else
static constexpr bool IsDebugBuild = false;
#endif

#ifdef __GNUC__ // GCC 4.8+, Clang, Intel and other compilers compatible with GCC (-std=c++0x or above)
[[noreturn]] inline __attribute__((always_inline)) void Unreachable()
{
	__builtin_unreachable();
}
#elif defined(_MSC_VER) // MSVC
[[noreturn]] __forceinline void Unreachable()
{
	__assume(false);
}
#endif

constexpr auto RoundUp(std::integral auto a, std::integral auto b)
{
	return ((a - 1) / b + 1) * b;
}

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
           .nonInductiveForLoops = 1,
           .whileLoops = 1,
           .doWhileLoops = 1,
           .generalUniformIndexing = 1,
           .generalAttributeMatrixVectorIndexing = 1,
           .generalVaryingIndexing = 1,
           .generalSamplerIndexing = 1,
           .generalVariableIndexing = 1,
           .generalConstantMatrixVectorIndexing = 1,
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

struct StaticInit
{
	StaticInit() { glslang::InitializeProcess(); }
	~StaticInit() { glslang::FinalizeProcess(); }
} s_staticInit;

std::unique_ptr<glslang::TShader> CompileStage(std::string_view shaderSource, EShLanguage stage, glslang::EShSource source)
{
	auto shader = std::make_unique<glslang::TShader>(stage);
	const auto inputStrings = std::array{data(ShaderCommon), data(shaderSource)};
	const auto inputStringLengths = std::array{static_cast<int>(size(ShaderCommon)), static_cast<int>(size(shaderSource))};
	shader->setStringsWithLengths(data(inputStrings), data(inputStringLengths), static_cast<int>(size(inputStringLengths)));
	shader->setEnvInput(source, stage, glslang::EShClientVulkan, 450);
	shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
	shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

	if (!shader->parse(&DefaultTBuiltInResource, 110, false, EShMsgDefault))
	{
		throw CompileError(shader->getInfoLog());
	}
	return shader;
};

ParameterBlockLayout& GetPblockLayout(ShaderData& data, int set)
{
	switch (set)
	{
		case 0:
			return data.sceneBindings;
		case 1:
			return data.viewBindings;
		case 2:
			return data.materialBindings;
		case 3:
			return data.objectBindings;
	}
	Unreachable();
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

void ReflectUniforms(ParameterBlockLayout& pblock, const glslang::TObjectReflection& uniformBlock)
{
	pblock.uniformsSize = static_cast<std::uint32_t>(uniformBlock.size);
	pblock.uniformsStages = GetShaderStageFlags(uniformBlock.stages);

	if constexpr (IsDebugBuild)
	{
		assert(uniformBlock.getType());
		assert(uniformBlock.getType()->isStruct());

		for ([[maybe_unused]] const auto& u : *uniformBlock.getType()->getStruct())
		{
			assert(
			    std::ranges::find(pblock.uniformDescs, std::string_view{u.type->getFieldName()}, &UniformDesc::name)
			    != pblock.uniformDescs.end());
		}
	}
};

void Compile(ShaderData& data, std::string_view vertexSource, std::string_view pixelSource, glslang::EShSource source)
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

	glslang::GlslangToSpv(*program.getIntermediate(EShLangVertex), data.vertexShaderSpirv, &logger, &spvOptions);
	glslang::GlslangToSpv(*program.getIntermediate(EShLangFragment), data.pixelShaderSpirv, &logger, &spvOptions);

	program.buildReflection(EShReflectionAllBlockVariables | EShReflectionSeparateBuffers | EShReflectionAllIOVariables);

	for (int i = 0; i < program.getNumUniformBlocks(); i++)
	{
		const auto& uniformBlock = program.getUniformBlock(i);
		if (uniformBlock.getType()->getQualifier().isPushConstant())
		{
			GetPblockLayout(data, 3).isPushConstant = true;
			ReflectUniforms(GetPblockLayout(data, 3), uniformBlock);
		}
		else
		{
			const auto set = uniformBlock.getType()->getQualifier().layoutSet;
			ReflectUniforms(GetPblockLayout(data, set), uniformBlock);
		}
	}

	for (int i = 0; i < program.getNumUniformVariables(); i++)
	{
		const auto& uniform = program.getUniform(i);
		if (!uniform.getType()->isTexture())
			continue;

		const auto set = uniform.getType()->getQualifier().layoutSet;
		GetPblockLayout(data, set).textureCount++;
	}
}

glslang::EShSource GetEShSource(ShaderLanguage language)
{
	switch (language)
	{
		case ShaderLanguage::Glsl:
			return glslang::EShSourceGlsl;
		case ShaderLanguage::Hlsl:
			return glslang::EShSourceHlsl;
	}
	Unreachable();
}

struct SizeAndAlignment
{
	std::uint32_t size = 0;
	std::uint32_t alignment = 0;
};

bool IsResourceType(ShaderVariableType::BaseType type)
{
	switch (type)
	{
		using enum ShaderVariableType::BaseType;
		case Float:
		case Vector2:
		case Vector3:
		case Vector4:
		case Matrix4:
			return false;
		case Texture2D:
		case Texture2DShadow:
			return true;
	}
	Unreachable();
}


SizeAndAlignment GetSizeAndAlignment(ShaderVariableType::BaseType type)
{
	switch (type)
	{
		using enum ShaderVariableType::BaseType;
		case Float:
			return {.size = sizeof(float), .alignment = sizeof(float)};

		case Vector2:
			return {.size = sizeof(float) * 2, .alignment = sizeof(float) * 2};
		case Vector3:
			return {.size = sizeof(float) * 4, .alignment = sizeof(float) * 4};
		case Vector4:
			return {.size = sizeof(float) * 4, .alignment = sizeof(float) * 4};
		case Matrix4:
			return {.size = sizeof(float) * 4 * 4, .alignment = sizeof(float) * 4};

		case Texture2D:
		case Texture2DShadow:
			return {};
	}
	Unreachable();
}

SizeAndAlignment GetSizeAndAlignment(ShaderVariableType type)
{
	if (type.arraySize != 0 && type.baseType == ShaderVariableType::BaseType::Vector3)
	{
		const std::uint32_t arraySize = sizeof(float) * 3 * type.arraySize;
		return {.size = arraySize, .alignment = arraySize};
	}
	const auto [size, alignment] = GetSizeAndAlignment(type.baseType);
	return {size * std::max(1u, type.arraySize), alignment};
}

std::string ToString(ShaderVariableType::BaseType type)
{
	switch (type)
	{
		using enum ShaderVariableType::BaseType;
		case Float:
			return "float";
		case Vector2:
			return "vec2";
		case Vector3:
			return "vec3";
		case Vector4:
			return "vec4";
		case Matrix4:
			return "mat4";
		case Texture2D:
			return "sampler2D";
		case Texture2DShadow:
			return "sampler2DShadow";
	}
	Unreachable();
}

std::string ToString(ShaderVariableType type)
{
	std::string ret = ToString(type.baseType);
	if (type.arraySize != 0)
	{
		fmt::format_to(std::back_inserter(ret), "[{}]", type.arraySize);
	}
	return ret;
}

void BuildUniformBuffer(std::string& source, ParameterBlockLayout& bindings, std::span<const ShaderVariable> variables, int set)
{
	if (variables.empty())
	{
		return;
	}

	for (const auto& variable : variables)
	{
		const auto [size, alignment] = GetSizeAndAlignment(variable.type);
		if (size == 0)
			continue;

		const auto offset = RoundUp(bindings.uniformsSize, alignment);
		bindings.uniformDescs.push_back(UniformDesc{.name = variable.name, .type = variable.type, .offset = offset});
		bindings.uniformsSize = offset + size;
	}

	if (bindings.uniformDescs.empty())
	{
		return;
	}

	auto out = std::back_inserter(source);

	if (set == 3 && bindings.uniformsSize <= 128u)
	{
		// Build push constants
		fmt::format_to(out, "layout(push_constant) uniform {}Uniforms {{\n", PblockNames[set]);
	}
	else
	{
		// Build uniform block
		fmt::format_to(out, "layout(set = {}, binding = 0) uniform {}Uniforms {{\n", set, PblockNames[set]);
	}

	for (const auto& uniform : bindings.uniformDescs)
	{
		std::string typeStr = ToString(uniform.type);
		fmt::format_to(out, "    {} {};\n", typeStr, uniform.name);
	}
	fmt::format_to(out, "}} {};\n\n", PblockNamesLower[set]);
}

void BuildResourceBindings(std::string& source, std::span<const ShaderVariable> variables, int set)
{
	if (variables.empty())
	{
		return;
	}

	auto out = std::back_inserter(source);

	for (std::size_t i = 0; i < variables.size(); i++)
	{
		const auto& variable = variables[i];
		if (IsResourceType(variable.type.baseType))
		{
			fmt::format_to(
			    out, "layout(set = {}, binding = {}) uniform {} {};\n", set, i + 1, ToString(variable.type), variable.name);
		}
	}

	source += '\n';
}

void BuildBindings(std::string& source, ParameterBlockLayout& bindings, const ParameterBlockDesc& pblock, int set)
{
	BuildUniformBuffer(source, bindings, pblock.parameters, set);
	BuildResourceBindings(source, pblock.parameters, set);
}

void BuildVaryings(std::string& source, const ShaderStageDefinition& stage)
{
	auto out = std::back_inserter(source);

	for (std::size_t i = 0; i < stage.inputs.size(); i++)
	{
		const auto& input = stage.inputs[i];

		if (input.name.starts_with("gl_"))
			continue;

		fmt::format_to(out, "layout(location = {}) in {} {};\n", i, ToString(input.type), input.name);
	}

	for (std::size_t i = 0; i < stage.outputs.size(); i++)
	{
		const auto& output = stage.outputs[i];

		if (output.name.starts_with("gl_"))
			continue;

		fmt::format_to(out, "layout(location = {}) out {} {};\n", i, ToString(output.type), output.name);
	}

	source += '\n';
}

} // namespace

ShaderData CompileShader(const ShaderSourceData& sourceData)
{
	ShaderData data;

	std::string parameters = "";
	BuildBindings(parameters, data.sceneBindings, sourceData.scenePblock, 0);
	BuildBindings(parameters, data.viewBindings, sourceData.viewPblock, 1);
	BuildBindings(parameters, data.materialBindings, sourceData.materialPblock, 2);
	BuildBindings(parameters, data.objectBindings, sourceData.objectPblock, 3);

	std::string vertexShader = parameters;
	BuildVaryings(vertexShader, sourceData.vertexShader);
	vertexShader += sourceData.vertexShader.source;

	std::string pixelShader = parameters;
	BuildVaryings(pixelShader, sourceData.pixelShader);
	pixelShader += sourceData.pixelShader.source;

	Compile(data, vertexShader, pixelShader, GetEShSource(sourceData.language));
	return data;
}
