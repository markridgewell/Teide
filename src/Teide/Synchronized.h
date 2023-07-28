
#pragma once

#include <functional>
#include <mutex>

template <class T>
class Synchronized
{
public:
    Synchronized() = default;

    Synchronized(const T& object) : m_object{object} {}
    Synchronized(T&& object) : m_object{std::move(object)} {}

    template <class F>
    auto Lock(const F& callable)
    {
        const auto lock = std::scoped_lock(m_mutex);
        return std::invoke(callable, m_object);
    }

private:
    T m_object;
    std::mutex m_mutex;
};
