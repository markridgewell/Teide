
#pragma once

#include "Teide/AbstractBase.h"
#include "Teide/BasicTypes.h"

namespace Teide
{

class RefCounter : AbstractBase
{
public:
    virtual void AddRef(uint64 index) noexcept = 0;
    virtual void DecRef(uint64 index) noexcept = 0;
};

template <class T>
class Handle
{
public:
    explicit Handle(uint64 index, RefCounter& owner, const T& data) : m_index{index}, m_owner{&owner}, m_data{&data} {}

    Handle(const Handle& other) : m_index{other.m_index}, m_owner{other.m_owner}, m_data{other.m_data}
    {
        m_owner->AddRef(m_index);
    }

    Handle(Handle&& other) noexcept : m_index{other.m_index}, m_owner{other.m_owner}, m_data{other.m_data}
    {
        other.m_owner = nullptr;
    }

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

} // namespace Teide
