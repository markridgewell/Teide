
#pragma once

#include "Teide/BasicTypes.h"

namespace Teide
{

template <class T, usize N>
class FrameArray
{
public:
    template <class... Args>
    explicit FrameArray(Args&&... args) // NOLINT(cppcoreguidelines-pro-type-member-init, cppcoreguidelines-missing-std-forward)
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
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<T*>(m_storage + i * sizeof(T))->~T();
        }
    }

    void NextFrame() { m_frameNumber = (m_frameNumber + 1) % N; }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    T& Current() { return *reinterpret_cast<T*>(m_storage + m_frameNumber * sizeof(T)); }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const T& Current() const { return *reinterpret_cast<const T*>(m_storage + m_frameNumber * sizeof(T)); }

private:
    alignas(T[N]) std::byte m_storage[sizeof(T) * N];
    uint32_t m_frameNumber = 0;
};

} // namespace Teide
