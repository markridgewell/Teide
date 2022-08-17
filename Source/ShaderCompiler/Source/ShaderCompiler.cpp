
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

ShaderData Compile(std::string_view vertexSource, std::string_view pixelSource, glslang::EShSource source)
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

	ShaderData ret;
	glslang::GlslangToSpv(*program.getIntermediate(EShLangVertex), ret.vertexShaderSpirv, &logger, &spvOptions);
	glslang::GlslangToSpv(*program.getIntermediate(EShLangFragment), ret.pixelShaderSpirv, &logger, &spvOptions);

	program.buildReflection();

	for (int i = 0; i < program.getNumUniformBlocks(); i++)
	{
		const auto& uniformBlock = program.getUniformBlock(i);
		if (uniformBlock.getType()->getQualifier().isPushConstant())
		{
			GetPblockLayout(ret, 3).isPushConstant = true;
			GetPblockLayout(ret, 3).uniformsSize = static_cast<std::uint32_t>(uniformBlock.size);
			GetPblockLayout(ret, 3).uniformsStages = GetShaderStageFlags(uniformBlock.stages);
		}
		else
		{
			const auto set = uniformBlock.getType()->getQualifier().layoutSet;
			GetPblockLayout(ret, set).uniformsSize = static_cast<std::uint32_t>(uniformBlock.size);
			GetPblockLayout(ret, set).uniformsStages = GetShaderStageFlags(uniformBlock.stages);
		}
	}

	for (int i = 0; i < program.getNumUniformVariables(); i++)
	{
		const auto& uniform = program.getUniform(i);
		if (!uniform.getType()->isTexture())
			continue;

		const auto set = uniform.getType()->getQualifier().layoutSet;
		GetPblockLayout(ret, set).textureCount++;
	}

	return ret;
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

std::size_t GetUniformSize(UniformType type)
{
	return 4 * type.rowCount * type.columnCount;
}

std::string ToString(UniformType type)
{
	assert(type.baseType == UniformBaseType::Float && "Only float variables supported for now!");

	if (type.columnCount > 1u)
	{
		assert(type.rowCount == type.columnCount);
		return fmt::format("mat{}", type.rowCount);
	}
	if (type.rowCount > 1u)
	{
		return fmt::format("vec{}", type.rowCount);
	}
	return "float";
}

void BuildUniformBuffer(std::string& source, std::span<const UniformDefinition> uniforms, int set)
{
	if (uniforms.empty())
	{
		return;
	}

	const std::size_t uniformSize
	    = std::accumulate(uniforms.begin(), uniforms.end(), std::size_t{0}, [](std::size_t i, const auto& uniform) {
		      return i + GetUniformSize(uniform.type);
	      });

	auto out = std::back_inserter(source);

	if (set == 3 && uniformSize <= 128u)
	{
		// Build push constants
		fmt::format_to(out, "layout(push_constant) uniform {}Uniforms {{\n", PblockNames[set]);
	}
	else
	{
		// Build uniform block
		fmt::format_to(out, "layout(set = {}, binding = 0) uniform {}Uniforms {{\n", set, PblockNames[set]);
	}

	for (const auto& uniform : uniforms)
	{
		std::string typeStr = ToString(uniform.type);
		fmt::format_to(out, "    {} {};\n", typeStr, uniform.name);
	}
	fmt::format_to(out, "}} {};\n\n", PblockNamesLower[set]);
}

void BuildTextureBindings(std::string& source, std::span<const TextureBindingDefinition> textures, int set)
{
	if (textures.empty())
	{
		return;
	}

	auto out = std::back_inserter(source);

	for (std::size_t i = 0; i < textures.size(); i++)
	{
		fmt::format_to(out, "layout(set = {}, binding = {}) uniform sampler2D {};\n", set, i, textures[i].name);
	}

	source += '\n';
}

void BuildBindings(std::string& source, const ParameterBlockDefinition& pblock, int set)
{
	BuildUniformBuffer(source, pblock.uniforms, set);
	BuildTextureBindings(source, pblock.textures, set);
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

ShaderData CompileShader(std::string_view vertexSource, std::string_view pixelSource, ShaderLanguage language)
{
	return Compile(vertexSource, pixelSource, GetEShSource(language));
}

ShaderData CompileShader(const ShaderSourceData& sourceData)
{
	std::string parameters = "";
	BuildBindings(parameters, sourceData.scenePblock, 0);
	BuildBindings(parameters, sourceData.viewPblock, 1);
	BuildBindings(parameters, sourceData.materialPblock, 2);
	BuildBindings(parameters, sourceData.objectPblock, 3);

	std::string vertexShader = parameters;
	BuildVaryings(vertexShader, sourceData.vertexShader);
	vertexShader += sourceData.vertexShader.source;

	std::string pixelShader = parameters;
	BuildVaryings(pixelShader, sourceData.pixelShader);
	pixelShader += sourceData.pixelShader.source;

	return Compile(vertexShader, pixelShader, GetEShSource(sourceData.language));
}
