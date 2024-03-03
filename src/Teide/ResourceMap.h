
#include "Teide/BasicTypes.h"
#include "Teide/ThreadUtils.h"

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
    explicit ResourceMap(std::string_view resourceType) : m_resourceType{resourceType} {}

    HandleT Insert(ResourceT resource)
    {
        return m_list.Lock([&resource, this](auto& list) {
            const auto index = static_cast<uint64>(list.size());
            spdlog::debug("Creating {} {}", m_resourceType, index);
            const auto& obj = list.emplace_back(std::move(resource));
            return HandleT(index, *this, obj.properties);
        });
    }

    ResourceT& Get(const HandleT& handle)
    {
        const auto index = static_cast<uint64>(handle);
        return m_list.Lock([index](auto& list) -> ResourceT& { return list.at(index); });
    }

    void AddRef(uint64 index) override
    {
        const char* type = m_resourceType.c_str();
        m_list.Lock([index, type](auto& list) {
            auto& resource = list.at(index);
            ++resource.refCount;
            spdlog::debug("Adding ref to {} {} (now {})", type, index, resource.refCount);
        });
    }

    void DecRef(uint64 index) override
    {
        const char* type = m_resourceType.c_str();
        m_list.Lock([index, type](auto& list) {
            auto& resource = list.at(index);
            --resource.refCount;
            spdlog::debug("Decrementing ref from {} {} (now {})", type, index, resource.refCount);
            if (resource.refCount == 0)
            {
                // Add resource to unused pool
                spdlog::debug("Destroying {} {}", type, index);
                resource = {};
            }
        });
    }

private:
    std::string m_resourceType;
    Synchronized<std::deque<VulkanTexture>> m_list;
};

} // namespace Teide