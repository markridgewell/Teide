
#pragma once

#include "Teide/AbstractBase.h"
#include "Teide/BasicTypes.h"

#include <memory>
#include <type_traits>

namespace Teide
{

class RefCounter : AbstractBase
{
public:
    virtual void AddRef(uint64 index) noexcept = 0;
    virtual void DecRef(uint64 index) noexcept = 0;
};

class BaseHandle
{
public:
    bool operator==(const BaseHandle&) const = default;
};

template <class T = void>
class Handle : public BaseHandle
{
public:
    using PropertiesType = T;

    explicit Handle(uint64 index, RefCounter& owner, const T& data) : m_index{index}, m_owner{&owner}, m_data{&data} {}

    Handle(const Handle& other) : m_index{other.m_index}, m_owner{other.m_owner}, m_data{other.m_data}
    {
        m_owner->AddRef(m_index);
    }

    Handle(Handle&& other) noexcept : m_index{other.m_index}, m_owner{other.m_owner}, m_data{other.m_data}
    {
        other.m_owner = nullptr;
    }

    // Self-assignment is handled by adding and then decrementing ref count
    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    Handle& operator=(const Handle& other)
    {
        other.m_owner->AddRef(other.m_index);
        Destroy();
        m_index = other.m_index;
        m_owner = other.m_owner;
        m_data = other.m_data;
        return *this;
    }

    Handle& operator=(Handle&& other) noexcept
    {
        if (this != &other)
        {
            Destroy();
            m_index = other.m_index;
            m_owner = other.m_owner;
            m_data = other.m_data;
            other.m_owner = nullptr;
        }
        return *this;
    }

    ~Handle() { Destroy(); }

    explicit operator uint64() const { return m_index; }

    bool operator==(const Handle&) const = default;

protected:
    const T* operator->() const { return m_data; }

private:
    void Destroy() noexcept
    {
        if (m_owner)
        {
            m_owner->DecRef(m_index);
        }
    }

    uint64 m_index;
    RefCounter* m_owner;
    const T* m_data;
};

template <>
class Handle<void> : public BaseHandle
{
public:
    using PropertiesType = void;

    explicit Handle(uint64 index, RefCounter& owner) : m_index{index}, m_owner{&owner} {}

    Handle(const Handle& other) : m_index{other.m_index}, m_owner{other.m_owner} { m_owner->AddRef(m_index); }

    Handle(Handle&& other) noexcept : m_index{other.m_index}, m_owner{other.m_owner} { other.m_owner = nullptr; }

    // Self-assignment is handled by adding and then decrementing ref count
    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    Handle& operator=(const Handle& other)
    {
        other.m_owner->AddRef(other.m_index);
        Destroy();
        m_index = other.m_index;
        m_owner = other.m_owner;
        return *this;
    }

    Handle& operator=(Handle&& other) noexcept
    {
        if (this != &other)
        {
            Destroy();
            m_index = other.m_index;
            m_owner = other.m_owner;
            other.m_owner = nullptr;
        }
        return *this;
    }

    ~Handle() { Destroy(); }

    explicit operator uint64() const { return m_index; }

    bool operator==(const Handle&) const = default;

private:
    void Destroy() noexcept
    {
        if (m_owner)
        {
            m_owner->DecRef(m_index);
        }
    }

    uint64 m_index;
    RefCounter* m_owner;
};

template <class T>
static constexpr bool HasProperties = !std::is_void_v<typename T::PropertiesType>;

} // namespace Teide

template <class T>
    requires std::derived_from<T, Teide::BaseHandle>
struct std::hash<T>
{
    std::size_t operator()(const T& v) const { return std::hash<Teide::uint64>{}(static_cast<Teide::uint64>(v)); }
};
