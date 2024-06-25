
#pragma once

#include "Teide/BasicTypes.h"
#include "Teide/Handle.h"
#include "Teide/Util/ThreadUtils.h"

#include <spdlog/spdlog.h>

#include <deque>
#include <string>
#include <string_view>

namespace Teide
{

template <class HandleT, class ResourceT>
class ResourceMap : public RefCounter
{
public:
    explicit ResourceMap(std::string_view resourceType) : m_impl(Impl(resourceType)) {}

    HandleT Insert(ResourceT resource)
    {
        const auto& [index, slot] = m_impl.Lock(&Impl::Insert, std::move(resource));
        return HandleT(index, *this, slot.resource.properties);
    }

    ResourceT& Get(const HandleT& handle) { return m_impl.Lock(&Impl::Get, handle); }

    void AddRef(uint64 index) noexcept override { m_impl.Lock(&Impl::AddRef, index); }

    void DecRef(uint64 index) noexcept override { m_impl.Lock(&Impl::DecRef, index); }

private:
    struct Slot
    {
        uint32 refCount = 1;
        ResourceT resource;
    };

    class Impl
    {
    public:
        explicit Impl(std::string_view resourceType) : m_resourceType{resourceType} {}

        std::pair<uint64, const Slot&> Insert(ResourceT resource)
        {
            const auto index = static_cast<uint64>(m_list.size());
            spdlog::debug("Creating {} {}", m_resourceType, index);
            const auto& slot = m_list.emplace_back(Slot{.resource = std::move(resource)});
            return {index, slot};
        }

        ResourceT& Get(const HandleT& handle)
        {
            const auto index = static_cast<uint64>(handle);
            return m_list.at(index).resource;
        }

        void AddRef(uint64 index) noexcept
        {
            auto& resource = m_list[index];
            ++resource.refCount;
            spdlog::debug("Adding ref to {} {} (now {})", m_resourceType, index, resource.refCount);
        }

        void DecRef(uint64 index) noexcept
        {
            auto& resource = m_list[index];
            --resource.refCount;
            spdlog::debug("Decrementing ref from {} {} (now {})", m_resourceType, index, resource.refCount);
            if (resource.refCount == 0)
            {
                // Add resource to unused pool
                spdlog::debug("Destroying {} {}", m_resourceType, index);
                resource = {};
            }
        }

    private:
        std::string m_resourceType;
        std::deque<Slot> m_list;
    };

    Synchronized<Impl> m_impl;
};

} // namespace Teide
