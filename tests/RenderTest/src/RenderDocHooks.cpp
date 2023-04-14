
#include "RenderDocHooks.h"

#include <cassert>

#if defined(_WIN32)
#    define WIN32_LEAN_AND_MEAN
#    include "windows.h"
#elif defined(__linux__)
#    include <dlfcn.h>
#endif


#if RENDERDOC_FOUND
RenderDoc::RenderDoc()
{
    // reinterpret_cast is required here to convert the function pointers.
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
#    if defined(_WIN32)
    if (HMODULE mod = GetModuleHandleA("renderdoc.dll"))
    {
        auto RENDERDOC_GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(mod, "RENDERDOC_GetAPI"));
        [[maybe_unused]] const int ret
            = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, reinterpret_cast<void**>(&rdoc_api));
        assert(ret == 1);
    }
#    elif defined(__linux__)
    if (void* mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD))
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(dlsym(mod, "RENDERDOC_GetAPI"));
        [[maybe_unused]] const int ret
            = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, reinterpret_cast<void**>(&rdoc_api));
        assert(ret == 1);
    }
#    endif
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
}
#else
RenderDoc::RenderDoc() = default;
#endif

void RenderDoc::StartFrameCapture()
{
#if RENDERDOC_FOUND
    if (rdoc_api)
    {
        rdoc_api->StartFrameCapture(nullptr, nullptr);
    }
#endif
}

void RenderDoc::EndFrameCapture()
{
#if RENDERDOC_FOUND
    if (rdoc_api)
    {
        rdoc_api->EndFrameCapture(nullptr, nullptr);
    }
#endif
}
