
#pragma once

#include <mutex>

template <class T>
class Synchronized
{
public:
    template <class F>
    auto Lock(const F& callable)
    {
        const auto lock = std::scoped_lock(m_mutex);
        return callable(m_object);
    }

private:
    T m_object;
    std::mutex m_mutex;
};
