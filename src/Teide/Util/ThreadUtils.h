
#pragma once

#include "Teide/BasicTypes.h"
#include "Teide/Definitions.h"

#include <algorithm>
#include <atomic>
#include <functional>
#include <mutex>
#include <span>
#include <string>
#include <thread>
#include <type_traits>

namespace Teide
{

template <class T>
class Synchronized
{
public:
    Synchronized() = default;

    explicit Synchronized(const T& object) : m_object{object} {}
    explicit Synchronized(T&& object) : m_object{std::move(object)} {}

    template <class... Args>
    explicit Synchronized(Args&&... args) : m_object{std::forward<Args>(args)...}
    {}

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

template <class T>
class ThreadMap
{
public:
    explicit ThreadMap(uint32 threadCount)
        requires std::is_default_constructible_v<T>
        : ThreadMap(threadCount, [] { return T(); })
    {}

    explicit ThreadMap(uint32 threadCount, std::function<T()> initFunc) : m_initFunc{std::move(initFunc)}
    {
        m_entries.reserve(threadCount);
        for (uint32 i = 0; i < threadCount; i++)
        {
            m_entries.emplace_back(std::this_thread::get_id(), m_initFunc());
        }
    }

    template <class F, class... Args>
    decltype(auto) LockCurrent(const F& callable, Args&&... args)
    {
        auto& object = GetCurrentObject();
        return std::invoke(callable, object, std::forward<Args>(args)...);
    }

    template <class F, class... Args>
    void LockAll(const F& callable, const Args&... args)
    {
        for (Entry& entry : m_entries)
        {
            std::invoke(callable, entry.second, args...);
        }
    }

private:
    using Entry = std::pair<std::thread::id, T>;

    T& GetCurrentObject()
    {
        const auto usedEntries = std::span(m_entries).subspan(0, std::min(m_size.load(), m_entries.size()));
        const auto it = std::ranges::find(usedEntries, std::this_thread::get_id(), &Entry::first);
        if (it != usedEntries.end())
        {
            return it->second;
        }

        const auto index = m_size.fetch_add(1);
        if (index >= m_entries.size())
        {
            --m_size;
            throw std::length_error("Exceeded capacity of ThreadMap");
        }

        m_entries[index].first = std::this_thread::get_id();
        return m_entries[index].second;
    }

    std::atomic<usize> m_size = 0;
    std::vector<Entry> m_entries;
    std::function<T()> m_initFunc;
};

void SetCurrentTheadName(const std::string& name);

} // namespace Teide
