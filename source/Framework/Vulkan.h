
#pragma once

#include "Definitions.h"

#include <fmt/core.h>
#include <vulkan/vulkan.hpp>

#include <chrono>
#include <unordered_set>

template <class T>
inline uint32_t size32(const T& cont)
{
	using std::size;
	return static_cast<uint32_t>(size(cont));
}

bool HasDepthComponent(vk::Format format);
bool HasStencilComponent(vk::Format format);
bool HasDepthOrStencilComponent(vk::Format format);

void TransitionImageLayout(
    vk::CommandBuffer cmdBuffer, vk::Image image, vk::Format format, uint32_t mipLevelCount, vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout, vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask);

vk::UniqueSemaphore CreateSemaphore(vk::Device device);
vk::UniqueFence CreateFence(vk::Device device, vk::FenceCreateFlags flags = {});
vk::UniqueCommandPool CreateCommandPool(uint32_t queueFamilyIndex, vk::Device device);

auto GetHandle()
{
	return []<class T, class Dispatch>(const vk::UniqueHandle<T, Dispatch>& uniqueHandle) { return uniqueHandle.get(); };
}

template <class Type, class Dispatch>
void SetDebugName(vk::UniqueHandle<Type, Dispatch>& handle [[maybe_unused]], const char* debugName [[maybe_unused]])
{
	if constexpr (IsDebugBuild)
	{
		handle.getOwner().setDebugUtilsObjectNameEXT({
		    .objectType = handle->objectType,
		    .objectHandle = std::bit_cast<uint64_t>(handle.get()),
		    .pObjectName = debugName,
		});
	}
}

template <class Type, class Dispatch, class... FormatArgs>
void SetDebugName(
    vk::UniqueHandle<Type, Dispatch>& handle [[maybe_unused]],
    const fmt::format_string<FormatArgs...>& format [[maybe_unused]], FormatArgs&&... fmtArgs [[maybe_unused]])
{
	if constexpr (IsDebugBuild)
	{
		const auto string = fmt::vformat(format, fmt::make_format_args(fmtArgs...));
		SetDebugName(handle, string.c_str());
	}
}

template <class... Args>
std::string DebugFormat(fmt::format_string<Args...> fmt [[maybe_unused]], Args&&... args [[maybe_unused]])
{
	if constexpr (IsDebugBuild)
	{
		return fmt::vformat(fmt, fmt::make_format_args(std::forward<Args>(args)...));
	}
	else
	{
		return "";
	}
}

class VulkanError : public vk::Error, public std::exception
{
public:
	explicit VulkanError(std::string message) : m_what{std::move(message)} {}

	virtual const char* what() const noexcept { return m_what.c_str(); }

private:
	std::string m_what;
};

vk::ImageAspectFlags GetImageAspect(vk::Format format);

void CopyBufferToImage(
    vk::CommandBuffer cmdBuffer, vk::Buffer source, vk::Image destination, vk::Format imageFormat, vk::Extent3D imageExtent);
void CopyImageToBuffer(
    vk::CommandBuffer cmdBuffer, vk::Image source, vk::Buffer destination, vk::Format imageFormat, vk::Extent3D imageExtent);

std::uint32_t GetFormatElementSize(vk::Format format);

template <class Rep, class Period>
constexpr std::uint64_t Timeout(std::chrono::duration<Rep, Period> duration)
{
	return std::chrono::nanoseconds(duration).count();
}
