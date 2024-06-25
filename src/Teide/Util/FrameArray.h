
#pragma once

#include "Teide/BasicTypes.h"

namespace Teide
{

template <class T, usize N>
class FrameArray
{
public:
    template <class... Args>
    explicit FrameArray(Args&&... args)
    {
        for (usize i = 0; i < N; i++)
        {
            new (m_storage + i * sizeof(T)) T(args...);
        }
    }

    FrameArray(const FrameArray&) = delete;
    FrameArray(FrameArray&&) = delete;
    FrameArray& operator=(const FrameArray&) = delete;
    FrameArray& operator=(FrameArray&&) = delete;

    ~FrameArray()
    {
        for (usize i = 0; i < N; i++)
        {
            reinterpret_cast<T*>(m_storage + i * sizeof(T))->~T();
        }
    }

    void NextFrame() { m_frameNumber = (m_frameNumber + 1) % N; }

    T& Current() { return *reinterpret_cast<T*>(m_storage + m_frameNumber * sizeof(T)); }
    const T& Current() const { return *reinterpret_cast<const T*>(m_storage + m_frameNumber * sizeof(T)); }

private:
    alignas(T[N]) std::byte m_storage[sizeof(T) * N];
    uint32_t m_frameNumber = 0;
};

} // namespace Teide
