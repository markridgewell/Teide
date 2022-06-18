td_add_application(Application SOURCES Source/Application.cpp)
set_property(TARGET Application PROPERTY VS_DPI_AWARE "PerMonitor")

target_link_libraries(
    Application
    PRIVATE GeoLib
            Teide
            ShaderCompiler
            SDL2::SDL2
            SDL2::SDL2main
            fmt::fmt
            spdlog::spdlog
            assimp::assimp
            Taskflow::Taskflow)
target_include_directories(Application PRIVATE ${STB_INCLUDE_DIRS})

# Visual Studio debugging environment, from preset
set_property(TARGET Application PROPERTY VS_DEBUGGER_COMMAND ${TEIDE_VS_DEBUGGER_COMMAND})
set_property(TARGET Application PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS ${TEIDE_VS_DEBUGGER_COMMAND_ARGUMENTS})
set_property(TARGET Application PROPERTY VS_DEBUGGER_ENVIRONMENT ${TEIDE_VS_DEBUGGER_ENVIRONMENT})
set_property(TARGET Application PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${TEIDE_VS_DEBUGGER_WORKING_DIRECTORY})
