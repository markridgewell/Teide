
#include "DescriptorPool.h"

#include "VulkanParameterBlock.h"

#include "vkex/vkex.hpp"

namespace Teide
{

DescriptorPool::DescriptorPool(vk::Device device, const VulkanParameterBlockLayout& layout, uint32 maxSets) :
    m_device{device}, m_layout{layout.setLayout.get()}, m_descriptorTypeCounts{layout.descriptorTypeCounts}, m_maxSets{maxSets}
{
    AddPool();
}

vk::DescriptorSet DescriptorPool::Allocate(const char* name)
{
    if (m_numAllocatedSets == m_maxSets)
    {
        m_maxSets *= 2;
        AddPool();
        m_numAllocatedSets = 0;
    }

    const vk::DescriptorSetAllocateInfo allocInfo = {
        .descriptorPool = m_pools.back().get(),
        .descriptorSetCount = 1,
        .pSetLayouts = &m_layout,
    };

    auto descriptorSet = m_device.allocateDescriptorSets(allocInfo).front();
    SetDebugName(m_device, descriptorSet, name);

    m_numAllocatedSets++;

    return descriptorSet;
}

void DescriptorPool::Reset()
{
    m_pools.erase(m_pools.begin(), m_pools.end() - 1);
    m_device.resetDescriptorPool(m_pools.back().get());
    m_numAllocatedSets = 0;
}

void DescriptorPool::AddPool()
{
    m_pools.push_back(m_device.createDescriptorPoolUnique(vkex::DescriptorPoolCreateInfo{
        .maxSets = m_maxSets,
        .poolSizes = std::views::transform(
            m_descriptorTypeCounts,
            [maxSets = m_maxSets](const auto& typeCount) {
                return vk::DescriptorPoolSize{
                    .type = typeCount.type,
                    .descriptorCount = typeCount.count * maxSets,
                };
            }),
    }));
}

} // namespace Teide
