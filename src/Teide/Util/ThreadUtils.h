
#pragma once

#include <functional>
#include <mutex>
#include <string>

namespace Teide
{

template <class T>
class Synchronized
{
public:
    Synchronized() = default;

    explicit Synchronized(const T& object) : m_object{object} {}
    explicit Synchronized(T&& object) : m_object{std::move(object)} {}

    template <class F, class... Args>
    decltype(auto) Lock(const F& callable, Args&&... args)
    {
        const auto lock = std::scoped_lock(m_mutex);
        return std::invoke(callable, m_object, std::forward<Args>(args)...);
    }

private:
    T m_object;
    std::mutex m_mutex;
};

void SetCurrentTheadName(const std::string& name);

} // namespace Teide
