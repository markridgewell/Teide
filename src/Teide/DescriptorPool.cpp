
#include "DescriptorPool.h"

#include "VulkanParameterBlock.h"

#include "vkex/vkex.hpp"

namespace Teide
{

DescriptorPool::DescriptorPool(vk::Device device, const VulkanParameterBlockLayout& layout, uint32 maxSets) :
    m_device{device},
    m_layout{layout.setLayout.get()},
    m_descriptorTypeCounts{layout.descriptorTypeCounts},
    m_maxSets{maxSets},
    m_pool{CreatePool(device, m_descriptorTypeCounts, maxSets)}
{}

vk::DescriptorSet DescriptorPool::Allocate(const char* name)
{
    const vk::DescriptorSetAllocateInfo allocInfo = {
        .descriptorPool = m_pool.get(),
        .descriptorSetCount = 1,
        .pSetLayouts = &m_layout,
    };

    auto descriptorSet = m_device.allocateDescriptorSets(allocInfo).front();
    SetDebugName(m_device, descriptorSet, name);

    return descriptorSet;
}

void DescriptorPool::Reset()
{
    m_device.resetDescriptorPool(m_pool.get());
}

vk::UniqueDescriptorPool
DescriptorPool::CreatePool(vk::Device device, const std::vector<DescriptorTypeCount>& typeCounts, uint32 maxSets)
{
    return device.createDescriptorPoolUnique(vkex::DescriptorPoolCreateInfo{
        .maxSets = maxSets,
        .poolSizes = std::views::transform(
            typeCounts,
            [maxSets](const auto& typeCount) {
                return vk::DescriptorPoolSize{
                    .type = typeCount.type,
                    .descriptorCount = typeCount.count * maxSets,
                };
            }),
    });
}

} // namespace Teide
