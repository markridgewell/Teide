
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
            {"inNormal", Type::Vector3},
            {"inTexCoord", Type::Vector2},
        }},
        .outputs = {{
            {"outColor", Type::Vector4},
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
	EXPECT_THAT(result.scenePblock, Eq(TestShader.scenePblock));
	EXPECT_THAT(result.viewPblock, Eq(TestShader.viewPblock));
	EXPECT_THAT(result.materialPblock, Eq(TestShader.materialPblock));
	EXPECT_THAT(result.objectPblock, Eq(TestShader.objectPblock));
}
