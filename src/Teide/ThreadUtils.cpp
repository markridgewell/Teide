
#include "Teide/ThreadUtils.h"

#if defined(_WIN32) && !defined(NDEBUG)
#    include <windows.h>

#    include <string>

void Teide::SetCurrentTheadName(std::string_view name [[maybe_unused]])
{
    const int convertResult = MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), nullptr, 0);
    if (convertResult > 0)
    {
        std::wstring wname;
        wname.resize(static_cast<std::size_t>(convertResult));
        if (MultiByteToWideChar(
                CP_UTF8, 0, name.data(), static_cast<int>(name.size()), wname.data(), static_cast<int>(wname.size()))
            > 0)
        {
            SetThreadDescription(GetCurrentThread(), wname.c_str());
        }
    }
}
#elif defined(__linux__)
#    include <pthread.h>

void Teide::SetCurrentTheadName(std::string_view name [[maybe_unused]])
{
    pthread_getname_np(pthread_self(), name.data(), name.size());
}
#endif
