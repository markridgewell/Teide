
#pragma once

#include "Types/TextureData.h"

#include <vulkan/vulkan.hpp>

class Texture
{
public:
	virtual ~Texture() = default;

	virtual vk::Extent2D GetSize() const = 0;
	virtual TextureFormat GetFormat() const = 0;
	virtual std::uint32_t GetMipLevelCount() const = 0;
	virtual vk::SampleCountFlagBits GetSampleCount() const = 0;
};
