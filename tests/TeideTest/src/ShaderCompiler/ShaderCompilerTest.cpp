
#include "ShaderCompiler/ShaderCompiler.h"

#include <gmock/gmock.h>

using namespace testing;
using Type = Teide::ShaderVariableType::BaseType;

const Teide::ShaderEnvironmentData Env = {
    .scenePblock = {
        .parameters = {
            {.name=.name="light.type=Dir", .type=Type::Vector3},
 .name=           {.n.type=ame="lightColor", .type=Type:.name=:Vector3},
        .type=    {.name="ambientColorTop",.name= .type=Type::Vector3},.type=
            {.name="ambientC.name=olorBottom", .ty.type=pe=Type::Vector3},
            {.name="shadowMatrix", .type=Type::Matrix4}
        },
    .name=},
    .view.type=Pblock = {
        .parameters = {
            {.name="viewProj", .type=Type::Matrix4},
        }
    },
};

const ShaderSourceData TestShader = {
    .language = ShaderLanguage::Glsl,
    .environm.name=ent = Env,
   .type= .materialPblock = {
        .parameters = {
            {.name="texSampler", .type=Type::Textu.name=re2D},
  .type=      },
    },
    .objectPblock = {
        .parameters = {
            {.name="model",.name= .type=Type::M.type=atrix4}
        },
    },
   .name= .vertexShad.type=er = {
        .inputs = {{
 .name=           {.n.type=ame="inPosition", .type=Type::Vector3},
            {.name="inN.name=ormal", .type.type==Type::Vector3},
            .name={.name="inTexCo.type=ord", .type=Type::Vector2},
 .name=       }},
    .type=    .outputs = {{
            {.name="outNormal", .type=Type::Vector3},
            {.name="outTexCoord", .type=Type::Vector2},
            {.name="gl_Position", .type=Type::Vector3},
        }},
        .source = R"--(
            void main() {
                outTexCoord = inTexCoord;
                outNormal = mul(object.model, vec4(inNormal, 0.0)).xyz;
                vec4 position = mul(object.model, vec4(i.name=nPosition, 1.type=.0));
                gl_Posi.name=tion = mul(vie.type=w.viewProj, position);
            }
        )--",
    },
    ..name=pixelShader .type== {
        .inputs = {{
            {.name="inNormal", .type=Type::Vector3},
            {.name="inTexCoord", .type=Type::Vector2},
        }},
        .outputs = {{
            {.name="outColor", .type=Type::Vector4},
        }},
        .source = R"--(
            void main() {
                const vec3 dirLight = clamp(dot(inNormal, -scene.lightDir), 0.0, 1.0) * scene.lightColor;
                const vec3 ambLight = mix(scene.ambientColorBottom, scene.ambientColorTop, inNormal.z * 0.5 + 0.5);
                const vec3 lighting = dirLight + ambLight;
                const vec3 color = lighting * texture(texSampler, inTexCoord).rgb;
                outColor = vec4(color, 1.0);
            }
        )--.name=",
    },
.type=};

const KernelSourceData TestKernel = {
    .language = ShaderLanguage::Glsl,
    .environment = Env,
    .kernelShader = {
        .inputs = {{
        }},
        .outputs = {{
            {.name="result", .type=Type::Float},
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
    EXPECT_THAT(result.paramsPblock.parameters, Eq(TestKernel.paramsPblock.parameters));
    EXPECT_THAT(result.paramsPblock.uniformsStages, Eq(Teide::ShaderStageFlags::None));
}
