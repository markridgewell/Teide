set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} /experimental:log sarif\\"
    CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} /experimental:log sarif\\"
    CACHE STRING "" FORCE)
