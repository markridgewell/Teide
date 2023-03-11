
#include "Application.h"

#include <SDL.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <windows.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct SDLWindowDeleter
{
    void operator()(SDL_Window* window) { SDL_DestroyWindow(window); }
};

using UniqueSDLWindow = std::unique_ptr<SDL_Window, SDLWindowDeleter>;

int Run(int argc, char* argv[])
{
    const char* const imageFilename = (argc >= 2) ? argv[1] : nullptr;
    const char* const modelFilename = (argc >= 3) ? argv[2] : nullptr;

    const auto windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
    const auto window = UniqueSDLWindow(
        SDL_CreateWindow("Teide", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 768, windowFlags), {});
    if (!window)
    {
        spdlog::critical("SDL error: {}", SDL_GetError());
        std::string message = fmt::format("The following error occurred when initializing SDL: {}", SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), window.get());
        return 1;
    }

    try
    {
        auto application = Application(window.get(), imageFilename, modelFilename);
        application.MainLoop();
    }
    catch (const std::exception& e)
    {
        spdlog::critical("Error: {}", e.what());
        std::string message = fmt::format("The following error occurred when initializing the application:\n{}", e.what());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), window.get());
        return 1;
    }

    return 0;
}

int SDL_main(int argc, char* argv[])
{
#ifdef _WIN32
    if (IsDebuggerPresent())
    {
        // Send log output to Visual Studio's Output window when running in the debugger.
        spdlog::set_default_logger(std::make_shared<spdlog::logger>("msvc", std::make_shared<spdlog::sinks::msvc_sink_mt>()));
    }
#endif

    spdlog::set_pattern("[%Y-%m-%D %H:%M:%S.%e] [%l] %v");

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        spdlog::critical("Couldn't initialise SDL: {}", SDL_GetError());
        return 1;
    }

    spdlog::info("SDL initialised successfully");

    int retcode = Run(argc, argv);

    SDL_Quit();

    return retcode;
}