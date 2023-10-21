
#pragma once

#include "VulkanConfig.h"

#include "Teide/BasicTypes.h"

#include <vector>

namespace Teide
{
struct DescriptorTypeCount;
struct VulkanParameterBlockLayout;

class DescriptorPool
{
public:
    explicit DescriptorPool(vk::Device device, const VulkanParameterBlockLayout& layout, uint32 maxSets);

    vk::DescriptorSet Allocate(const char* name);

    void Reset();

private:
    void AddPool();

    vk::Device m_device;
    vk::DescriptorSetLayout m_layout;
    std::vector<DescriptorTypeCount> m_descriptorTypeCounts;
    std::vector<vk::UniqueDescriptorPool> m_pools;
    uint32 m_maxSets;
    uint32 m_numAllocatedSets = 0;
};
} // namespace Teide
