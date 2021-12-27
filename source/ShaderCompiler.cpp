
#include "ShaderCompiler.h"

#include "Definitions.h"

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

namespace
{
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

std::vector<vk::DescriptorSetLayoutBinding>& GetSetBindings(ShaderData& data, int set)
{
	switch (set)
	{
		case 0:
			return data.sceneBindings;
		case 1:
			return data.viewBindings;
		case 2:
			return data.materialBindings;
	}
	Unreachable();
}

vk::ShaderStageFlags GetShaderStageFlags(EShLanguageMask lang)
{
	vk::ShaderStageFlags ret{};
	if (lang & EShLangVertexMask)
	{
		ret |= vk::ShaderStageFlagBits::eVertex;
	}
	if (lang & EShLangTessControlMask)
	{
		ret |= vk::ShaderStageFlagBits::eTessellationControl;
	}
	if (lang & EShLangTessEvaluationMask)
	{
		ret |= vk::ShaderStageFlagBits::eTessellationEvaluation;
	}
	if (lang & EShLangGeometryMask)
	{
		ret |= vk::ShaderStageFlagBits::eGeometry;
	}
	if (lang & EShLangFragmentMask)
	{
		ret |= vk::ShaderStageFlagBits::eFragment;
	}
	if (lang & EShLangComputeMask)
	{
		ret |= vk::ShaderStageFlagBits::eCompute;
	}
	if (lang & EShLangRayGenMask)
	{
		ret |= vk::ShaderStageFlagBits::eRaygenKHR;
	}
	if (lang & EShLangIntersectMask)
	{
		ret |= vk::ShaderStageFlagBits::eIntersectionKHR;
	}
	if (lang & EShLangAnyHitMask)
	{
		ret |= vk::ShaderStageFlagBits::eAnyHitKHR;
	}
	if (lang & EShLangClosestHitMask)
	{
		ret |= vk::ShaderStageFlagBits::eClosestHitKHR;
	}
	if (lang & EShLangMissMask)
	{
		ret |= vk::ShaderStageFlagBits::eMissKHR;
	}
	if (lang & EShLangCallableMask)
	{
		ret |= vk::ShaderStageFlagBits::eCallableKHR;
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
		const auto set = uniformBlock.getType()->getQualifier().layoutSet;
		if (uniformBlock.getType()->getQualifier().isPushConstant())
		{
			ret.pushConstantRanges.push_back(vk::PushConstantRange{
			    .stageFlags = GetShaderStageFlags(uniformBlock.stages),
			    .offset = 0,
			    .size = static_cast<uint32_t>(uniformBlock.size),
			});
		}
		else
		{
			const auto binding = static_cast<uint32_t>(uniformBlock.getBinding());

			GetSetBindings(ret, set).push_back({
			    .binding = binding,
			    .descriptorType = vk::DescriptorType::eUniformBuffer,
			    .descriptorCount = 1,
			    .stageFlags = GetShaderStageFlags(uniformBlock.stages),
			});
		}
	}

	for (int i = 0; i < program.getNumUniformVariables(); i++)
	{
		const auto& uniform = program.getUniform(i);
		if (!uniform.getType()->isTexture())
			continue;

		const auto binding = static_cast<uint32_t>(uniform.getBinding());
		const auto set = uniform.getType()->getQualifier().layoutSet;

		GetSetBindings(ret, set).push_back({
		    .binding = binding,
		    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
		    .descriptorCount = 1,
		    .stageFlags = GetShaderStageFlags(uniform.stages),
		});
	}

	return ret;
}

EShLanguage GetEShLanguage(ShaderStage stage)
{
	switch (stage)
	{
		case ShaderStage::Vertex:
			return EShLangVertex;
		case ShaderStage::Pixel:
			return EShLangFragment;
	}
	Unreachable();
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

} // namespace

ShaderData CompileShader(std::string_view vertexSource, std::string_view pixelSource, ShaderLanguage language)
{
	return Compile(vertexSource, pixelSource, GetEShSource(language));
}
