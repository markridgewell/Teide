
#pragma once

#include "Definitions.h"

#include <fmt/core.h>
#include <vulkan/vulkan.hpp>

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
vk::UniqueFence CreateFence(vk::Device device);
vk::UniqueCommandPool CreateCommandPool(uint32_t queueFamilyIndex, vk::Device device);

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

class OneShotCommandBuffer
{
public:
	explicit OneShotCommandBuffer(vk::Device device, vk::CommandPool commandPool, vk::Queue queue);
	~OneShotCommandBuffer();

	OneShotCommandBuffer(const OneShotCommandBuffer&) = delete;
	OneShotCommandBuffer(OneShotCommandBuffer&&) = delete;
	OneShotCommandBuffer& operator=(const OneShotCommandBuffer&) = delete;
	OneShotCommandBuffer& operator=(OneShotCommandBuffer&&) = delete;

	operator vk::CommandBuffer() const;

	void TakeOwnership(vk::UniqueBuffer buffer);

private:
	vk::UniqueCommandBuffer m_cmdBuffer;
	vk::Queue m_queue;

	std::vector<vk::UniqueBuffer> m_ownedBuffers;
};

class VulkanError : public vk::Error, public std::exception
{
public:
	explicit VulkanError(std::string message) : m_what{std::move(message)} {}

	virtual const char* what() const noexcept { return m_what.c_str(); }

private:
	std::string m_what;
};
