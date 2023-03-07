
#pragma once

#if __has_include("renderdoc_app.h")
#    include "renderdoc_app.h"
#    define RENDERDOC_FOUND 1
#else
#    define RENDERDOC_FOUND 0
#endif

class RenderDoc
{
public:
    RenderDoc();
    void StartFrameCapture();
    void EndFrameCapture();

#if RENDERDOC_FOUND
private:
    RENDERDOC_API_1_1_2* rdoc_api = nullptr;
#endif
};
