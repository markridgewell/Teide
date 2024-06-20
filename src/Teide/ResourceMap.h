
#pragma once

#include "Teide/Assert.h"
#include "Teide/BasicTypes.h"
#include "Teide/Handle.h"
#include "Teide/Hash.h"
#include "Teide/ThreadUtils.h"

#include <spdlog/spdlog.h>

#include <deque>
#include <ranges>
#include <string>
#include <string_view>

namespace Teide
{

template <class T>
struct Resource
{
    using PropertiesType = T;
    T properties;
};

template <class HandleT, class ResourceT>
class ResourceMap : public RefCounter
{
public:
    using HandleType = HandleT;
    using ResourceType = ResourceT;
    using PropertiesType = ResourceType::PropertiesType;

    explicit ResourceMap(std::string_view resourceType, uint32 keepAliveFrames = 3) :
        m_impl(Impl(resourceType, keepAliveFrames))
    {}

    HandleType Insert(ResourceType resource) { return m_impl.Lock(&Impl::Insert, *this, std::move(resource)); }

    [[nodiscard]] std::optional<HandleType> TryReuse(const PropertiesType& properties)
    {
        return m_impl.Lock(&Impl::TryReuse, *this, properties);
    }

    void NextFrame() { m_impl.Lock(&Impl::NextFrame); }

    ResourceType& Get(const HandleType& handle) { return m_impl.Lock(&Impl::Get, handle); }

    void AddRef(uint64 index) noexcept override { m_impl.Lock(&Impl::AddRef, index); }

    void DecRef(uint64 index) noexcept override { m_impl.Lock(&Impl::DecRef, index); }

private:
    struct Slot
    {
        uint32 refCount = 1;
        ResourceType resource;

        explicit Slot(ResourceType resource) : resource{std::move(resource)} {}

        Slot(const Slot&) = default;
        Slot(Slot&&) noexcept = default;
        Slot& operator=(const Slot&) = default;
        Slot& operator=(Slot&&) noexcept = default;
        ~Slot() noexcept = default;

        void Reset() { resource = {}; }
    };

    struct UnusedSlot
    {
        uint64 index;
        uint32 framesLeft;

        explicit UnusedSlot(uint64 index, uint32 framesLeft) : index{index}, framesLeft{framesLeft} {}
    };

    class Impl
    {
    public:
        explicit Impl(std::string_view resourceType, uint32 keepAliveFrames) :
            m_resourceType{resourceType}, m_keepAliveFrames{keepAliveFrames}
        {}

        [[nodiscard]] HandleType Insert(RefCounter& self, ResourceType resource)
        {
            const auto index = static_cast<uint64>(m_list.size());
            spdlog::debug("Creating {} {}", m_resourceType, index);
            const auto& slot = m_list.emplace_back(std::move(resource));
            return HandleType(index, self, slot.resource.properties);
        }

        [[nodiscard]] std::optional<HandleType> TryReuse(RefCounter& self, const PropertiesType& properties)
        {
            const auto it = m_unusedPool.find(properties);
            if (it == m_unusedPool.end())
            {
                return std::nullopt;
            }
            const uint64 index = it->second.index;
            m_unusedPool.erase(it);
            spdlog::debug("Reusing {} {}", m_resourceType, index);
            auto& slot = m_list[index];
            TEIDE_ASSERT(slot.refCount == 0);
            slot.refCount = 1;
            return HandleType(index, self, slot.resource.properties);
        }

        void NextFrame()
        {
            for (auto& [index, framesLeft] : std::views::values(m_unusedPool))
            {
                --framesLeft;
                if (framesLeft == 0)
                {
                    TEIDE_ASSERT(m_list[index].refCount == 0);
                    spdlog::debug("Destroying {} {}", m_resourceType, index);
                    m_list[index].Reset();
                }
            }
            std::erase_if(m_unusedPool, [](const auto& entry) { return entry.second.framesLeft == 0; });
        }

        ResourceType& Get(const HandleType& handle)
        {
            const auto index = static_cast<uint64>(handle);
            TEIDE_ASSERT(m_list[index].refCount > 0);
            return m_list.at(index).resource;
        }

        void AddRef(uint64 index) noexcept
        {
            auto& slot = m_list[index];
            ++slot.refCount;
            spdlog::debug("Adding ref to {} {} (now {})", m_resourceType, index, slot.refCount);
        }

        void DecRef(uint64 index) noexcept
        {
            auto& slot = m_list[index];
            TEIDE_ASSERT(slot.refCount > 0);
            --slot.refCount;
            spdlog::debug("Decrementing ref from {} {} (now {})", m_resourceType, index, slot.refCount);
            if (slot.refCount == 0)
            {
                if (m_keepAliveFrames == 0)
                {
                    // Destroy resource immediately
                    spdlog::debug("Destroying {} {}", m_resourceType, index);
                    slot.Reset();
                }
                else
                {
                    // Add resource to unused pool
                    spdlog::debug("Adding {} {} to unused pool", m_resourceType, index);
                    m_unusedPool.emplace(slot.resource.properties, UnusedSlot(index, m_keepAliveFrames));
                }
            }
        }

    private:
        std::string m_resourceType;
        std::deque<Slot> m_list;
        std::unordered_multimap<PropertiesType, UnusedSlot, Hash<PropertiesType>> m_unusedPool;
        uint32 m_keepAliveFrames = 0;
    };

    Synchronized<Impl> m_impl;
};

} // namespace Teide
