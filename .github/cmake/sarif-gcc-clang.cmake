set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -fdiagnostics-format=sarif -Wno-sarif-format-unstable"
    CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -fdiagnostics-format=sarif -Wno-sarif-format-unstable"
    CACHE STRING "" FORCE)
