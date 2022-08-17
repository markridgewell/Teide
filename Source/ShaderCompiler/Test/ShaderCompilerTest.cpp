
#include "ShaderCompiler/ShaderCompiler.h"

#include <gmock/gmock.h>

using namespace testing;

constexpr std::string_view VertexShader = R"--(
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoord;

layout(set = 1, binding = 0) uniform ViewUniforms {
    mat4 viewProj;
} view;

layout(push_constant) uniform ObjectUniforms {
    mat4 model;
} object;

void main() {
    outTexCoord = inTexCoord;
    outNormal = mul(object.model, vec4(inNormal, 0.0)).xyz;
    vec4 position = mul(object.model, vec4(inPosition, 1.0));
    gl_Position = mul(view.viewProj, position);
})--";

constexpr std::string_view PixelShader = R"--(
layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D texSampler;

layout(set = 0, binding = 0) uniform GlobalUniforms {
    vec3 lightDir;
    vec3 lightColor;
    vec3 ambientColorTop;
    vec3 ambientColorBottom;
    mat4 shadowMatrix;
} ubo;

void main() {
    const vec3 dirLight = clamp(dot(inNormal, -ubo.lightDir), 0.0, 1.0) * ubo.lightColor;
    const vec3 ambLight = mix(ubo.ambientColorBottom, ubo.ambientColorTop, inNormal.z * 0.5 + 0.5);
    const vec3 lighting = dirLight + ambLight;

    const vec3 color = lighting * texture(texSampler, inTexCoord).rgb;
    outColor = vec4(color, 1.0);
})--";

const ShaderSourceData TestShader = {
    .language = ShaderLanguage::Glsl,
    .scenePblock = {
        .uniforms = {
            {"lightDir",{UniformBaseType::Float, 3}},
            {"lightColor", {UniformBaseType::Float, 3}},
            {"ambientColorTop", {UniformBaseType::Float, 3}},
            {"ambientColorBottom", {UniformBaseType::Float, 3}},
            {"shadowMatrix", {UniformBaseType::Float, 4, 4}}
        },
    },
    .viewPblock = {
        .uniforms = {
            {"viewProj", {UniformBaseType::Float, 4, 4}},
        }
    },
    .materialPblock = {
        .textures = {
            {.name = "texSampler"},
        },
    },
    .objectPblock = {
        .uniforms = {
            {"model", {UniformBaseType::Float, 4, 4}}
        },
    },
    .vertexShader = {
        .inputs = {{
            {"inPosition", {UniformBaseType::Float, 3}},
            {"inNormal", {UniformBaseType::Float, 3}},
            {"inTexCoord", {UniformBaseType::Float, 2}},
        }},
        .outputs = {{
            {"outNormal", {UniformBaseType::Float, 3}},
            {"outTexCoord", {UniformBaseType::Float, 2}},
            {"gl_Position", {UniformBaseType::Float, 3}},
        }},
        .source = R"--(
            void main() {
                outTexCoord = inTexCoord;
                outNormal = mul(object.model, vec4(inNormal, 0.0)).xyz;
                vec4 position = mul(object.model, vec4(inPosition, 1.0));
                gl_Position = mul(view.viewProj, position);
            }
        )--",
    },
    .pixelShader = {
        .inputs = {{
            {"inNormal", {UniformBaseType::Float, 3}},
            {"inTexCoord", {UniformBaseType::Float, 2}},
        }},
        .outputs = {{
            {"outColor", {UniformBaseType::Float, 4}},
        }},
        .source = R"--(
            void main() {
                const vec3 dirLight = clamp(dot(inNormal, -scene.lightDir), 0.0, 1.0) * scene.lightColor;
                const vec3 ambLight = mix(scene.ambientColorBottom, scene.ambientColorTop, inNormal.z * 0.5 + 0.5);
                const vec3 lighting = dirLight + ambLight;
                const vec3 color = lighting * texture(texSampler, inTexCoord).rgb;
                outColor = vec4(color, 1.0);
            }
        )--",
    },
};

TEST(ShaderCompilerTest, CompileSimpleOldAPI)
{
	const auto result = CompileShader(VertexShader, PixelShader, ShaderLanguage::Glsl);
	EXPECT_THAT(result.vertexShaderSpirv, Not(IsEmpty()));
	EXPECT_THAT(result.pixelShaderSpirv, Not(IsEmpty()));
	EXPECT_THAT(result.sceneBindings.uniformsSize, Eq(128));
	EXPECT_THAT(result.sceneBindings.uniformsStages, Eq(ShaderStageFlags::Pixel));
	EXPECT_THAT(result.sceneBindings.isPushConstant, IsFalse());
	EXPECT_THAT(result.sceneBindings.textureCount, Eq(0));
	EXPECT_THAT(result.viewBindings.uniformsSize, Eq(64));
	EXPECT_THAT(result.viewBindings.uniformsStages, Eq(ShaderStageFlags::Vertex));
	EXPECT_THAT(result.viewBindings.isPushConstant, IsFalse());
	EXPECT_THAT(result.viewBindings.textureCount, Eq(0));
	EXPECT_THAT(result.materialBindings.uniformsSize, Eq(0));
	EXPECT_THAT(result.materialBindings.uniformsStages, Eq(ShaderStageFlags{}));
	EXPECT_THAT(result.materialBindings.isPushConstant, IsFalse());
	EXPECT_THAT(result.materialBindings.textureCount, Eq(1));
	EXPECT_THAT(result.objectBindings.uniformsSize, Eq(64));
	EXPECT_THAT(result.objectBindings.uniformsStages, Eq(ShaderStageFlags::Vertex));
	EXPECT_THAT(result.objectBindings.isPushConstant, IsTrue());
	EXPECT_THAT(result.objectBindings.textureCount, Eq(0));
}

TEST(ShaderCompilerTest, CompileSimple)
{
	const auto result = CompileShader(TestShader);
	EXPECT_THAT(result.vertexShaderSpirv, Not(IsEmpty()));
	EXPECT_THAT(result.pixelShaderSpirv, Not(IsEmpty()));
	EXPECT_THAT(result.sceneBindings.uniformsSize, Eq(128));
	EXPECT_THAT(result.sceneBindings.uniformsStages, Eq(ShaderStageFlags::Pixel));
	EXPECT_THAT(result.sceneBindings.isPushConstant, IsFalse());
	EXPECT_THAT(result.sceneBindings.textureCount, Eq(0));
	EXPECT_THAT(result.viewBindings.uniformsSize, Eq(64));
	EXPECT_THAT(result.viewBindings.uniformsStages, Eq(ShaderStageFlags::Vertex));
	EXPECT_THAT(result.viewBindings.isPushConstant, IsFalse());
	EXPECT_THAT(result.viewBindings.textureCount, Eq(0));
	EXPECT_THAT(result.materialBindings.uniformsSize, Eq(0));
	EXPECT_THAT(result.materialBindings.uniformsStages, Eq(ShaderStageFlags{}));
	EXPECT_THAT(result.materialBindings.isPushConstant, IsFalse());
	EXPECT_THAT(result.materialBindings.textureCount, Eq(1));
	EXPECT_THAT(result.objectBindings.uniformsSize, Eq(64));
	EXPECT_THAT(result.objectBindings.uniformsStages, Eq(ShaderStageFlags::Vertex));
	EXPECT_THAT(result.objectBindings.isPushConstant, IsTrue());
	EXPECT_THAT(result.objectBindings.textureCount, Eq(0));
}
