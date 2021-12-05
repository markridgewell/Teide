
#pragma once

#include "Framework/Vulkan.h"

#include <array>
#include <cstdint>
#include <span>

constexpr uint32_t MaxFramesInFlight = 2;

template <class T>
inline uint32_t size32(const T& cont)
{
	using std::size;
	return static_cast<uint32_t>(size(cont));
}

template <class T>
concept Span = requires(T t)
{
	{std::span{t}};
};

template <class T>
concept TrivialSpan
    = Span<T> && std::is_trivially_copyable_v<typename T::value_type> && std::is_standard_layout_v<typename T::value_type>;

template <class T>
concept TrivialObject
    = !Span<T> && std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T> && !std::is_pointer_v<T>;

class BytesView
{
public:
	BytesView() = default;
	BytesView(const BytesView&) = default;
	BytesView(BytesView&&) = default;
	BytesView& operator=(const BytesView&) = default;
	BytesView& operator=(BytesView&&) = default;

	BytesView(std::span<const std::byte> bytes) : m_span{bytes} {}

	template <TrivialSpan T>
	BytesView(const T& data) : m_span{std::as_bytes(std::span(data))}
	{}

	template <TrivialObject T>
	BytesView(const T& data) : m_span{std::as_bytes(std::span(&data, 1))}
	{}

	auto data() const { return m_span.data(); }
	auto size() const { return m_span.size(); }
	auto begin() const { return m_span.begin(); }
	auto end() const { return m_span.end(); }
	auto rbegin() const { return m_span.rbegin(); }
	auto rend() const { return m_span.rend(); }

private:
	std::span<const std::byte> m_span;
};

struct DescriptorSet
{
	std::vector<vk::DescriptorSet> sets;
};

struct RenderObject
{
	vk::Buffer vertexBuffer;
	vk::Buffer indexBuffer;
	uint32_t indexCount = 0;
	vk::Pipeline pipeline;
	vk::PipelineLayout pipelineLayout;
	const DescriptorSet* materialDescriptorSet = nullptr;
	BytesView pushConstants;
};

struct RenderList
{
	vk::RenderPass renderPass;
	vk::Framebuffer framebuffer;
	vk::Extent2D framebufferSize;
	std::span<const vk::ClearValue> clearValues;

	const DescriptorSet* sceneDescriptorSet = nullptr;
	const DescriptorSet* viewDescriptorSet = nullptr;

	std::vector<RenderObject> objects;
};

class Renderer
{
public:
	explicit Renderer(vk::Device device, uint32_t queueFamilyIndex, uint32_t numThreads);

	void BeginFrame(uint32_t frameNumber);
	void EndFrame(vk::Semaphore waitSemaphore, vk::Semaphore signalSemaphore, vk::Fence fence);

	void RenderToTexture(vk::Image texture, vk::Format format, const RenderList& renderList, uint32_t threadIndex);
	void RenderToSurface(const RenderList& renderList, uint32_t threadIndex);

private:
	struct ThreadResources
	{
		vk::UniqueCommandPool commandPool;
		vk::UniqueCommandBuffer commandBuffer;
		bool usedThisFrame = false;

		void Reset(vk::Device device)
		{
			device.resetCommandPool(commandPool.get());
			usedThisFrame = false;
		}
	};

	vk::CommandBuffer GetCommandBuffer(uint32_t threadIndex);
	void Render(const RenderList& renderList, vk::CommandBuffer commandBuffer);

	static std::vector<ThreadResources> CreateThreadResources(vk::Device device, uint32_t queueFamilyIndex, uint32_t numThreads);

	vk::DescriptorSet GetDescriptorSet(const DescriptorSet* descriptorSet) const;

	vk::Device m_device;
	vk::Queue m_queue;
	std::array<std::vector<ThreadResources>, MaxFramesInFlight> m_frameResources;
	uint32_t m_frameNumber = 0;
};
