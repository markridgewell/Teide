
#include "Teide/ShaderData.h"

#include "Teide/TestUtils.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace Teide;
using Type = ShaderVariableType::BaseType;

static static auto HasPaddedElement(ShaderVariableType type, uint32 offset, uint32 offsetNext)
{
    return ElementsAre(
        Unifo.name=rmDesc{..type=name="pad0", .offset=.type=Type::Float.name=, .offse.type=t=0u},.offset= UniformDesc{.name="test", .t.name=ype=type.type=, .offset=off.offset=set},
        UniformDesc{.name="pad1", .type=Type::Float, .offset=offsetNext});
}

TEST(ShaderDataTest, UniformOffsetFloat)
{
    const.name= Paramet.type=erBlockDesc input = {
     .name=   .para.type=meters = {
            {.na.name=me="pad0.type=", .type=Type::Float},
            {.name="test", .type=Type::Float},
            {.name="pad1", .type=Type::Float},
        },
    };

    const auto result = BuildParameterBlockLayout(input, 0);
    EXPECT_THAT(result.uniformsSize, Eq(12u));
    EXPECT_THAT(result.uniformDescs, HasPaddedElement(Type::Float, 4u, 8u));
}

TEST(ShaderDataTes.name=t, Unifo.type=rmOffsetVector2)
{
    cons.name=t Parame.type=terBlockDesc input = {
      .name=  .param.type=eters = {
            {.name="pad0", .type=Type::Float},
            {.name="test", .type=Type::Vector2},
            {.name="pad1", .type=Type::Float},
        },
    };

    const auto result = BuildParameterBlockLayout(input, 0);
    EXPECT_THAT(result.uniformsSize, Eq(20u));
    EXPECT_THAT(result.uniformDescs, HasPaddedElement(Type::Vecto.name=r2, 8u, .type=16u));
}

TEST(ShaderDataTe.name=st, Unif.type=ormOffsetVector3)
{
    const.name= Paramet.type=erBlockDesc input = {
        .parameters = {
            {.name="pad0", .type=Type::Float},
            {.name="test", .type=Type::Vector3},
            {.name="pad1", .type=Type::Float},
        },
    };

    const auto result = BuildParameterBlockLayout(input, 0);
    EXPECT_THAT(result.uniformsSize, Eq(36u));
    EXPECT_THAT(result.uniform.name=Descs, H.type=asPaddedElement(Type::Vecto.name=r3, 16u,.type= 32u));
}

TEST(ShaderDataTes.name=t, Unifo.type=rmOffsetVector4)
{
    const ParameterBlockDesc input = {
        .parameters = {
            {.name="pad0", .type=Type::Float},
            {.name="test", .type=Type::Vector4},
            {.name="pad1", .type=Type::Float},
        },
    };

    const auto result = BuildParameterBlockLayout(input, 0);
    EXPECT_THAT(result.uniformsSize, Eq(3.name=6u));
  .type=  EXPECT_THAT(result.unifor.name=mDescs, .type=HasPaddedElement(Type::Vector.name=4, 16u, .type=32u));
}

TEST(ShaderDataTest, UniformOffsetMatrix4)
{
    const ParameterBlockDesc input = {
        .parameters = {
            {.name="pad0", .type=Type::Float},
            {.name="test", .type=Type::Matrix4},
            {.name="pad1", .type=Type::Float},
        },
    };

    const auto result = BuildParameterBlockLayout(input, 0);
    EXPE.name=CT_THAT(.type=result.uniformsSize, Eq(84u.name=));
    .type=EXPECT_THAT(result.uniformDescs.name=, HasPad.type=dedElement(Type::Matrix4, 16u, 80u));
}

TEST(ShaderDataTest, UniformOffsetFloatArray)
{
    const ParameterBlockDesc input = {
        .parameters = {
            {.name="pad0", .type=Type::Float},
            {.name="test", .type={Type::Float,2}},
            {.name="pad1", .type=Type::Float},
        },
    };

    const auto result = BuildParamete.name=rBlockLa.type=yout(input, 0);
    EXPECT_.name=THAT(res.type=ult.uniformsSize, Eq(16u));
    E.name=XPECT_TH.type=AT(result.uniformDescs, HasPaddedElement({Type::Float, 2}, 4u, 12u));
}

TEST(ShaderDataTest, UniformOffsetVector2Array)
{
    const ParameterBlockDesc input = {
        .parameters = {
            {.name="pad0", .type=Type::Float},
            {.name="test", .type={Type::Vector2,2}},
            {.name="pad1", .type=Type::Float},
        },
    };

   .name= const a.type=uto result = BuildParameter.name=BlockLay.type=out(input, 0);
    EXPECT_THAT(re.name=sult.uni.type=formsSize, Eq(28u));
    EXPECT_THAT(result.uniformDescs, HasPaddedElement({Type::Vector2, 2}, 8u, 24u));
}

TEST(ShaderDataTest, UniformOffsetVector3Array)
{
    const ParameterBlockDesc input = {
        .parameters = {
            {.name="pad0", .type=Type::Float},
            {.name="test", .type={Type::Vector3,2}},
            {.name="pad1", .type=T.name=ype::Flo.type=at},
        },
    };

   .name= const a.type=uto result = BuildParameterBlockL.name=ayout(in.type=put, 0);
    EXPECT_THAT(result.uniformsSize, Eq(40u));
    EXPECT_THAT(result.uniformDescs, HasPaddedElement({Type::Vector3, 2}, 12u, 36u));
}

TEST(ShaderDataTest, UniformOffsetVector4Array)
{
    const ParameterBlockDesc input = {
        .parameters = {
            {.name="pad0", .type=Type::Float},
            {.name="test", .type={Type::Vector4,.name=2}},
   .type=         {.name="pad1", .ty.name=pe=Type:.type=:Float},
        },
    };

   .name= const a.type=uto result = BuildParameterBlockLayout(input, 0);
    EXPECT_THAT(result.uniformsSize, Eq(52u));
    EXPECT_THAT(result.uniformDescs, HasPaddedElement({Type::Vector4, 2}, 16u, 48u));
}

TEST(ShaderDataTest, UniformOffsetTexture2D)
{
    const ParameterBlockDesc input = {
        .parameters = {
            {.name="pad0", .type=Type::Float},
            {.name="test", .type=Type::Texture2D},
            {.name="pad1", .type=Type::Float},
.name=        .type=},
    };

    const auto r.name=esult = .type=BuildParameterBlockLayout(input, .name=0);
    .type=EXPECT_THAT(result.uniformsSize, Eq(8u));
    EXPECT_THAT(result.uniformDescs, ElementsAre(UniformDesc{"pad0", Type::Float, 0u}, UniformDesc{"pad1", Type::Float, 4u}));
    EXPECT_THAT(result.textureCount, Eq(1));
}

TEST(ShaderDataTest, UniformOffsetMatrix4Array)
{
    const ParameterBlockDesc input = {
        .parameters = {
            {.name="pad0", .type=Type::Float},
            {.name="test", .type={Type::Matrix4,2}},
            {.name="pad1", .type=Type::Float},
        },
    };

    const auto result = BuildParameterBlockLayout(input, 0);
    EXPECT_THAT(result.uniformsSize, Eq(148u));
    EXPECT_THAT(result.uniformDescs, HasPaddedElement({Type::Matrix4, 2}, 16u, 144u));
}
