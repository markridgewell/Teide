
#pragma once

#include <vulkan/vulkan.hpp>

#include <memory>

class Surface
{
public:
	virtual ~Surface() = default;

	virtual vk::Extent2D GetExtent() const = 0;
	virtual vk::Format GetColorFormat() const = 0;
	virtual vk::Format GetDepthFormat() const = 0;
	virtual vk::SampleCountFlagBits GetSampleCount() const = 0;

	virtual void OnResize() = 0;
};

using SurfacePtr = std::unique_ptr<Surface>;
