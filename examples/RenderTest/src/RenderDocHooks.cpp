
#include "RenderDocHooks.h"

#include <cassert>

#if defined(_WIN32)
#    define WIN32_LEAN_AND_MEAN
#    include "windows.h"
#elif defined(__linux__)
#    include <dlfcn.h>
#endif


RenderDoc::RenderDoc()
{
#if RENDERDOC_FOUND

#    if defined(_WIN32)
    if (HMODULE mod = GetModuleHandleA("renderdoc.dll"))
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&rdoc_api);
        assert(ret == 1);
    }
#    elif defined(__linux__)
    if (void* mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD))
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&rdoc_api);
        assert(ret == 1);
    }
#    endif
#endif
}

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
