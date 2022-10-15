td_add_library(
    Types
    SOURCES Include/Types/Format.h
            Include/Types/PipelineData.h
            Include/Types/ShaderData.h
            Include/Types/StaticMap.h
            Include/Types/TextureData.h
            Source/Format.cpp
            Source/ShaderData.cpp
            Source/TextureData.cpp
    PUBLIC_DEPS GeoLib)

td_add_test(
    TypesTest
    SOURCES Test/ShaderDataTest.cpp Test/TextureDataTest.cpp Test/Main.cpp
    DEPS Types GTest::gtest GTest::gmock)
