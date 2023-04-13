
#pragma once

#include "VulkanConfig.h"

#include "GeoLib/Vector.h"
#include "Teide/Definitions.h"
#include "Teide/PipelineData.h"
#include "Teide/TextureData.h"

#include <fmt/core.h>

#include <chrono>
#include <span>
#include <unordered_set>

struct SDL_Window;

namespace Teide
{

struct FramebufferLayout;
class VulkanLoader;
enum class PrimitiveTopology;
enum class VertexClass;

struct QueueFamilies
{
    uint32 graphicsFamily = 0;
    uint32 transferFamily = 0;
    std::optional<uint32> presentFamily;
};

struct PhysicalDevice
{
    vk::PhysicalDevice physicalDevice;
    vk::PhysicalDeviceProperties properties;
    std::vector<const char*> requiredExtensions;

    QueueFamilies queueFamilies;
    std::vector<uint32> queueFamilyIndices;
};

struct RenderPassInfo
{
    vk::AttachmentLoadOp colorLoadOp = vk::AttachmentLoadOp::eDontCare;
    vk::AttachmentStoreOp colorStoreOp = vk::AttachmentStoreOp::eStore;
    vk::AttachmentLoadOp depthLoadOp = vk::AttachmentLoadOp::eDontCare;
    vk::AttachmentStoreOp depthStoreOp = vk::AttachmentStoreOp::eDontCare;
    vk::AttachmentLoadOp stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

    auto operator<=>(const RenderPassInfo&) const = default;
};

struct Framebuffer
{
    vk::Framebuffer framebuffer;
    FramebufferLayout layout;
    Geo::Size2i size;
};

void EnableOptionalVulkanLayer(
    std::vector<const char*>& enabledLayers, const std::vector<vk::LayerProperties>& availableLayers, const char* layerName);
void EnableRequiredVulkanExtension(
    std::vector<const char*>& enabledExtensions, const std::vector<vk::ExtensionProperties>& availableExtensions,
    const char* extensionName);

vk::DebugUtilsMessengerCreateInfoEXT GetDebugCreateInfo();

vk::UniqueInstance CreateInstance(VulkanLoader& loader, SDL_Window* window = nullptr);
vk::UniqueDevice CreateDevice(VulkanLoader& loader, const PhysicalDevice& physicalDevice);

template <class T>
inline uint32_t size32(const T& cont)
{
    using std::size;
    return static_cast<uint32_t>(size(cont));
}

void TransitionImageLayout(
    vk::CommandBuffer cmdBuffer, vk::Image image, Format format, uint32_t mipLevelCount, vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout, vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask);

vk::UniqueCommandPool CreateCommandPool(uint32_t queueFamilyIndex, vk::Device device, const char* debugName = "");

inline auto GetHandle()
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

    const char* what() const noexcept override { return m_what.c_str(); }

private:
    std::string m_what;
};

vk::ImageAspectFlags GetImageAspect(Format format);

void CopyBufferToImage(
    vk::CommandBuffer cmdBuffer, vk::Buffer source, vk::Image destination, Format imageFormat, vk::Extent3D imageExtent);
void CopyImageToBuffer(
    vk::CommandBuffer cmdBuffer, vk::Image source, vk::Buffer destination, Format imageFormat, vk::Extent3D imageExtent,
    uint32 numMipLevels);

vk::UniqueRenderPass CreateRenderPass(vk::Device device, const FramebufferLayout& layout);
vk::UniqueRenderPass CreateRenderPass(vk::Device device, const FramebufferLayout& layout, const RenderPassInfo& renderPassInfo);
vk::UniqueFramebuffer
CreateFramebuffer(vk::Device device, vk::RenderPass renderPass, Geo::Size2i size, std::span<const vk::ImageView> imageViews);

vk::Format ToVulkan(Format);
vk::Filter ToVulkan(Filter);
vk::SamplerMipmapMode ToVulkan(MipmapMode);
vk::SamplerAddressMode ToVulkan(SamplerAddressMode);
vk::CompareOp ToVulkan(CompareOp);
vk::Offset2D ToVulkan(Geo::Point2i);
vk::Extent2D ToVulkan(Geo::Size2i);
vk::Rect2D ToVulkan(Geo::Box2i);
vk::BlendFactor ToVulkan(BlendFactor);
vk::BlendOp ToVulkan(BlendOp);
vk::StencilOp ToVulkan(StencilOp);
vk::PolygonMode ToVulkan(FillMode);
vk::CullModeFlags ToVulkan(CullMode);
vk::ColorComponentFlags ToVulkan(ColorMask);
vk::PrimitiveTopology ToVulkan(PrimitiveTopology);
vk::VertexInputRate ToVulkan(VertexClass);

Format FromVulkan(vk::Format format);

template <class Rep, class Period>
constexpr uint64 Timeout(std::chrono::duration<Rep, Period> duration)
{
    return std::chrono::nanoseconds(duration).count();
}

template <class T>
struct VulkanImpl;

} // namespace Teide
