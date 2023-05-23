include(FileLists.cmake)

td_add_application(
    Application
    WIN32 DPI_AWARE
    SOURCES "${sources}"
    DEPS Teide
         SDL2::SDL2
         SDL2::SDL2main
         SDL2_image::SDL2_image
         fmt::fmt
         spdlog::spdlog
         assimp::assimp)

# Visual Studio debugging environment, from preset
set_property(TARGET Application PROPERTY VS_DEBUGGER_COMMAND ${TEIDE_VS_DEBUGGER_COMMAND})
set_property(TARGET Application PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS ${TEIDE_VS_DEBUGGER_COMMAND_ARGUMENTS})
set_property(TARGET Application PROPERTY VS_DEBUGGER_ENVIRONMENT ${TEIDE_VS_DEBUGGER_ENVIRONMENT})
set_property(TARGET Application PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${TEIDE_VS_DEBUGGER_WORKING_DIRECTORY})
