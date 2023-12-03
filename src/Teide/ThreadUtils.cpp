
#include "Teide/ThreadUtils.h"

#if defined(NDEBUG)
void Teide::SetCurrentTheadName(const std::string& name [[maybe_unused]])
{}

#elif defined(_WIN32)
#    include <windows.h>

#    include <string>

void Teide::SetCurrentTheadName(const std::string& name [[maybe_unused]])
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

void Teide::SetCurrentTheadName(const std::string& name [[maybe_unused]])
{
    pthread_setname_np(pthread_self(), name.c_str());
}
#endif
