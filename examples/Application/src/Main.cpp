
#include "Application.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
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

namespace
{

struct SDLWindowDeleter
{
    void operator()(SDL_Window* window) { SDL_DestroyWindow(window); }
};

using UniqueSDLWindow = std::unique_ptr<SDL_Window, SDLWindowDeleter>;

int Run(std::span<const char* const> args)
{
    const char* const imageFilename = (args.size() >= 2) ? args[1] : nullptr;
    const char* const modelFilename = (args.size() >= 3) ? args[2] : nullptr;

    const auto windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
    const auto window = UniqueSDLWindow(
        SDL_CreateWindow("Teide", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 768, windowFlags), {});
    if (!window)
    {
        spdlog::critical("SDL error: {}", SDL_GetError());
        const std::string message = fmt::format("The following error occurred when initializing SDL: {}", SDL_GetError());
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
        const std::string message
            = fmt::format("The following error occurred when initializing the application:\n{}", e.what());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), window.get());
        return 1;
    }

    return 0;
}
} // namespace

int main(int argc, char* argv[])
{
#ifdef _WIN32
    if (IsDebuggerPresent())
    {
        // Send log output to Visual Studio's Output window when running in the debugger.
        spdlog::set_default_logger(std::make_shared<spdlog::logger>("msvc", std::make_shared<spdlog::sinks::msvc_sink_mt>()));
    }
    SetThreadDescription(GetCurrentThread(), L"Main");
#endif

    spdlog::set_pattern("[%Y-%m-%D %H:%M:%S.%e] [%l] %v");

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        spdlog::critical("Couldn't initialise SDL: {}", SDL_GetError());
        return 1;
    }

    spdlog::info("SDL initialised successfully");

    const int retcode = Run({argv, static_cast<std::size_t>(argc)});

    SDL_Quit();

    return retcode;
}
