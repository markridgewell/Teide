
#pragma once

#include "VulkanConfig.h"

#include "Teide/BasicTypes.h"

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
    vk::UniqueDescriptorPool CreatePool(vk::Device device, const std::vector<DescriptorTypeCount>& typeCounts, uint32 maxSets);

    vk::Device m_device;
    vk::DescriptorSetLayout m_layout;
    std::vector<DescriptorTypeCount> m_descriptorTypeCounts;
    uint32 m_maxSets;
    vk::UniqueDescriptorPool m_pool;
};
} // namespace Teide
