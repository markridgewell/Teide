
#include "ShaderCompiler/ShaderCompiler.h"

#include <gmock/gmock.h>

using namespace testing;
using Type = ShaderVariableType::BaseType;

const ShaderSourceData TestShader = {
    .language = ShaderLanguage::Glsl,
    .scenePblock = {
        .parameters = {
            {"lightDir", Type::Vector3},
            {"lightColor", Type::Vector3},
            {"ambientColorTop", Type::Vector3},
            {"ambientColorBottom", Type::Vector3},
            {"shadowMatrix", Type::Matrix4}
        },
    },
    .viewPblock = {
        .parameters = {
            {"viewProj", Type::Matrix4},
        }
    },
    .materialPblock = {
        .parameters = {
            {"texSampler", Type::Texture2D},
        },
    },
    .objectPblock = {
        .parameters = {
            {"model", Type::Matrix4}
        },
    },
    .vertexShader = {
        .inputs = {{
            {"inPosition", Type::Vector3},
            {"inNormal", Type::Vector3},
            {"inTexCoord", Type::Vector2},
        }},
        .outputs = {{
            {"outNormal", Type::Vector3},
            {"outTexCoord", Type::Vector2},
            {"gl_Position", Type::Vector3},
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
            {"inNormal", {Type::Float, 3}},
            {"inTexCoord", {Type::Float, 2}},
        }},
        .outputs = {{
            {"outColor", {Type::Float, 4}},
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

constexpr auto EmptyVertexShader = R"--(
void main() {
    gl_Position = vec4(0.0);
}
)--";

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

TEST(ShaderCompilerTest, UniformOffsetFloat)
{
	const ShaderSourceData Shader = {
        .language = ShaderLanguage::Glsl,
        .scenePblock = {
            .parameters = {
                {"pad0", Type::Float},
                {"test", Type::Float},
                {"pad1", Type::Float},
            },
        },
        .vertexShader = {.source = EmptyVertexShader},
        .pixelShader = {
            .outputs = {{
                {"outColor", Type::Vector4},
            }},
            .source = R"--(
                void main() {
                    outColor = vec4(scene.test, 0.0, 0.0, 1.0);
                }
            )--",
        },
    };

	const auto result = CompileShader(Shader);
	EXPECT_THAT(result.vertexShaderSpirv, Not(IsEmpty()));
	EXPECT_THAT(result.pixelShaderSpirv, Not(IsEmpty()));
	EXPECT_THAT(result.sceneBindings.uniformsSize, Eq(12));
	ASSERT_THAT(result.sceneBindings.uniformDescs.size(), Eq(3));
	EXPECT_THAT(result.sceneBindings.uniformDescs[0].offset, Eq(0));
	EXPECT_THAT(result.sceneBindings.uniformDescs[1].offset, Eq(4));
	EXPECT_THAT(result.sceneBindings.uniformDescs[2].offset, Eq(8));
}

TEST(ShaderCompilerTest, UniformOffsetFloat2)
{
	const ShaderSourceData Shader = {
        .language = ShaderLanguage::Glsl,
        .scenePblock = {
            .parameters = {
                {"pad0", Type::Float},
                {"test", Type::Vector2},
                {"pad1", Type::Float},
            },
        },
        .vertexShader = {.source = EmptyVertexShader},
        .pixelShader = {
            .outputs = {{
                {"outColor", Type::Vector4},
            }},
            .source = R"--(
                void main() {
                    outColor = vec4(scene.test.x, 0.0, 0.0, 1.0);
                }
            )--",
        },
    };

	const auto result = CompileShader(Shader);
	EXPECT_THAT(result.vertexShaderSpirv, Not(IsEmpty()));
	EXPECT_THAT(result.pixelShaderSpirv, Not(IsEmpty()));
	EXPECT_THAT(result.sceneBindings.uniformsSize, Eq(20));
	ASSERT_THAT(result.sceneBindings.uniformDescs.size(), Eq(3));
	EXPECT_THAT(result.sceneBindings.uniformDescs[0].offset, Eq(0));
	EXPECT_THAT(result.sceneBindings.uniformDescs[1].offset, Eq(8));
	EXPECT_THAT(result.sceneBindings.uniformDescs[2].offset, Eq(16));
}
