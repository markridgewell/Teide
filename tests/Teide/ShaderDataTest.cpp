
#include "Teide/ShaderData.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace Teide;
using Type = ShaderVariableType::BaseType;

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
    EXPECT_THAT(result.uniformsSize, Eq(12));
    ASSERT_THAT(result.uniformDescs.size(), Eq(3));
    EXPECT_THAT(result.uniformDescs[0].offset, Eq(0));
    EXPECT_THAT(result.uniformDescs[1].offset, Eq(4));
    EXPECT_THAT(result.uniformDescs[2].offset, Eq(8));
}

TEST(ShaderDataTest, UniformOffsetFloat2)
{
    const ParameterBlockDesc input = {
        .parameters = {
            {"pad0", Type::Float},
            {"test", Type::Vector2},
            {"pad1", Type::Float},
        },
    };

    const auto result = BuildParameterBlockLayout(input, 0);
    EXPECT_THAT(result.uniformsSize, Eq(20));
    ASSERT_THAT(result.uniformDescs.size(), Eq(3));
    EXPECT_THAT(result.uniformDescs[0].offset, Eq(0));
    EXPECT_THAT(result.uniformDescs[1].offset, Eq(8));
    EXPECT_THAT(result.uniformDescs[2].offset, Eq(16));
}
