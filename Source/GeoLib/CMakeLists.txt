td_add_library(
    GeoLib
    SOURCES Include/GeoLib/ForwardDeclare.h
            Include/GeoLib/Matrix.h
            Include/GeoLib/Angle.h
            Include/GeoLib/Vector.h
            Include/GeoLib/Scalar.h
            Include/GeoLib/Box.h
            GeoLib.natvis)

td_add_test(
    GeoLibTest
    SOURCES Test/Main.cpp
            Test/TestUtils.h
            Test/AngleTest.cpp
            Test/MatrixTest.cpp
            Test/VectorTest.cpp
    DEPS GeoLib GTest::gtest GTest::gmock fmt::fmt)
