
#include "ShaderCompiler/ShaderCompiler.h"

#include <gmock/gmock.h>

using namespace testing;
using Type = Teide::ShaderVariableType::BaseType;

const Teide::ShaderEnvironmentData Env = {
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
};

const ShaderSourceData TestShader = {
    .language = ShaderLanguage::Glsl,
    .environment = Env,
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
            {"normal", Type::Vector3},
            {"texCoord", Type::Vector2},
            {"gl_Position", Type::Vector3},
        }},
        .source = R"--(
            void main() {
                texCoord = inTexCoord;
                normal = mul(object.model, vec4(inNormal, 0.0)).xyz;
                vec4 position = mul(object.model, vec4(inPosition, 1.0));
                gl_Position = mul(view.viewProj, position);
            }
        )--",
    },
    .pixelShader = {
        .inputs = {{
            {"normal", Type::Vector3},
            {"texCoord", Type::Vector2},
        }},
        .outputs = {{
            {"outColor", Type::Vector4},
        }},
        .source = R"--(
            void main() {
                const vec3 dirLight = clamp(dot(normal, -scene.lightDir), 0.0, 1.0) * scene.lightColor;
                const vec3 ambLight = mix(scene.ambientColorBottom, scene.ambientColorTop, normal.z * 0.5 + 0.5);
                const vec3 lighting = dirLight + ambLight;
                const vec3 color = lighting * texture(texSampler, texCoord).rgb;
                outColor = vec4(color, 1.0);
            }
        )--",
    },
};

const KernelSourceData TestKernel = {
    .language = ShaderLanguage::Glsl,
    .environment = Env,
    .kernelShader = {
        .inputs = {{
        }},
        .outputs = {{
            {"result", Type::Float},
        }},
        .source = R"--(
            void main() {
                result = 42.0f;
            }
        )--",
    },
};

TEST(ShaderCompilerTest, CompileSimpleShader)
{
    ShaderCompiler const compiler;
    const auto result = compiler.Compile(TestShader);
    EXPECT_THAT(result.vertexShader.spirv, Not(IsEmpty()));
    EXPECT_THAT(result.pixelShader.spirv, Not(IsEmpty()));
    EXPECT_THAT(result.environment.scenePblock.parameters, Eq(TestShader.environment.scenePblock.parameters));
    EXPECT_THAT(result.environment.scenePblock.uniformsStages, Eq(Teide::ShaderStageFlags::Pixel));
    EXPECT_THAT(result.environment.viewPblock.parameters, Eq(TestShader.environment.viewPblock.parameters));
    EXPECT_THAT(result.environment.viewPblock.uniformsStages, Eq(Teide::ShaderStageFlags::Vertex));
    EXPECT_THAT(result.materialPblock.parameters, Eq(TestShader.materialPblock.parameters));
    EXPECT_THAT(result.materialPblock.uniformsStages, Eq(Teide::ShaderStageFlags::None));
    EXPECT_THAT(result.objectPblock.parameters, Eq(TestShader.objectPblock.parameters));
    EXPECT_THAT(result.objectPblock.uniformsStages, Eq(Teide::ShaderStageFlags::Vertex));
}

TEST(ShaderCompilerTest, CompileSimpleKernel)
{
    const ShaderCompiler compiler;
    const auto result = compiler.Compile(TestKernel);
    EXPECT_THAT(result.computeShader.spirv, Not(IsEmpty()));
    EXPECT_THAT(result.environment.scenePblock.parameters, Eq(TestKernel.environment.scenePblock.parameters));
    EXPECT_THAT(result.environment.scenePblock.uniformsStages, Eq(Teide::ShaderStageFlags::None));
    EXPECT_THAT(result.environment.viewPblock.parameters, Eq(TestKernel.environment.viewPblock.parameters));
    EXPECT_THAT(result.environment.viewPblock.uniformsStages, Eq(Teide::ShaderStageFlags::None));
    EXPECT_THAT(result.paramsPblock.uniformsStages, Eq(Teide::ShaderStageFlags::None));
}
