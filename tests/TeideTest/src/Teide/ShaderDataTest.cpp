
#include "Teide/ShaderData.h"

#include "Teide/TestUtils.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace Teide;
using Type = ShaderVariableType::BaseType;

auto HasPaddedElement(Type type, uint32 offset, uint32 offsetNext)
{
    return ElementsAre(
        UniformDesc{"pad0", Type::Float, 0u}, UniformDesc{"test", type, offset}, UniformDesc{"pad1", Type::Float, offsetNext});
}

TEST(ShaderDataTest, UniformOffsetFloat)
{
    const ParameterBlockDesc input = {
        .parameters = {
            {"pad0", Type::Float},
            {"test", Type::Float},
            {"pad1", Type::Float},
        },
    };

    const auto result = BuildParameterBlockLayout(input, 0);
    EXPECT_THAT(result.uniformsSize, Eq(12u));
    EXPECT_THAT(result.uniformDescs, HasPaddedElement(Type::Float, 4u, 8u));
}

TEST(ShaderDataTest, UniformOffsetVector2)
{
    const ParameterBlockDesc input = {
        .parameters = {
            {"pad0", Type::Float},
            {"test", Type::Vector2},
            {"pad1", Type::Float},
        },
    };

    const auto result = BuildParameterBlockLayout(input, 0);
    EXPECT_THAT(result.uniformsSize, Eq(20u));
    EXPECT_THAT(result.uniformDescs, HasPaddedElement(Type::Vector2, 8u, 16u));
}

TEST(ShaderDataTest, UniformOffsetVector3)
{
    const ParameterBlockDesc input = {
        .parameters = {
            {"pad0", Type::Float},
            {"test", Type::Vector3},
            {"pad1", Type::Float},
        },
    };

    const auto result = BuildParameterBlockLayout(input, 0);
    EXPECT_THAT(result.uniformsSize, Eq(36u));
    EXPECT_THAT(result.uniformDescs, HasPaddedElement(Type::Vector3, 16u, 32u));
}

TEST(ShaderDataTest, UniformOffsetVector4)
{
    const ParameterBlockDesc input = {
        .parameters = {
            {"pad0", Type::Float},
            {"test", Type::Vector4},
            {"pad1", Type::Float},
        },
    };

    const auto result = BuildParameterBlockLayout(input, 0);
    EXPECT_THAT(result.uniformsSize, Eq(36u));
    EXPECT_THAT(result.uniformDescs, HasPaddedElement(Type::Vector4, 16u, 32u));
}

TEST(ShaderDataTest, UniformOffsetMatrix4)
{
    const ParameterBlockDesc input = {
        .parameters = {
            {"pad0", Type::Float},
            {"test", Type::Matrix4},
            {"pad1", Type::Float},
        },
    };

    const auto result = BuildParameterBlockLayout(input, 0);
    EXPECT_THAT(result.uniformsSize, Eq(84u));
    EXPECT_THAT(result.uniformDescs, HasPaddedElement(Type::Matrix4, 16u, 80u));
}
