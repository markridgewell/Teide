
#pragma once

#include "GeoLib/Vector.h"
#include "Teide/Definitions.h"
#include "Teide/PipelineData.h"
#include "Teide/ShaderData.h"
#include "Teide/TextureData.h"

#include <fmt/format.h>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>
#include <vulkan/vulkan_extension_inspection.hpp>

#include <chrono>
#include <span>
#include <string_view>

struct SDL_Window;

template <std::size_t N>
struct fmt::formatter<vk::ArrayWrapper1D<char, N>> : formatter<std::string_view>
{
    auto format(const vk::ArrayWrapper1D<char, N>& value, format_context& ctx) const -> format_context::iterator
    {
        return fmt::format_to(ctx.out(), "{}", std::string_view(value.data(), value.size()));
    }
};

namespace Teide
{

struct FramebufferLayout;
class VulkanLoader;
enum class PrimitiveTopology : uint8;
enum class VertexClass : uint8;

struct QueueFamilies
{
    uint32 graphicsFamily = 0;
    uint32 transferFamily = 0;
    std::optional<uint32> presentFamily;
};

struct PhysicalDevice
{
    explicit PhysicalDevice(vk::PhysicalDevice pd, const QueueFamilies& qf);

    vk::PhysicalDevice physicalDevice;
    vk::PhysicalDeviceProperties properties;
    vk::PhysicalDeviceDepthStencilResolveProperties depthStencilResolveProperties;
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

    bool operator==(const RenderPassInfo&) const = default;
    void Visit(auto f) const
    {
        return f(colorLoadOp, colorStoreOp, depthLoadOp, depthStoreOp, stencilLoadOp, stencilStoreOp);
    }
};

struct Framebuffer
{
    vk::Framebuffer framebuffer;
    FramebufferLayout layout;
    Geo::Size2i size;
};

enum class FramebufferUsage : uint8
{
    Attachment,
    ShaderInput,
    PresentSrc
};

enum class Required : bool
{
    False = false,
    True = true
};

void EnableVulkanLayer(
    std::vector<const char*>& enabledLayers, const std::vector<vk::LayerProperties>& availableLayers,
    const char* layerName, Required required);
void EnableVulkanExtension(
    std::vector<const char*>& enabledExtensions, const std::vector<vk::ExtensionProperties>& availableExtensions,
    const char* extensionName, Required required);

class InstanceExtensionName
{
public:
    template <usize N>
    consteval InstanceExtensionName(const char (&name)[N]) : // cppcheck-suppress noExplicitConstructor
        m_name{&name[0]}
    {
#if (201907 <= __cpp_constexpr) && (!defined(__GNUC__) || (110400 < GCC_VERSION))
        if (not vk::isInstanceExtension(std::string(&name[0])))
        {
            throw std::runtime_error("Unknown instance extension name");
        }
#endif
    }

    constexpr std::string_view Get() const { return m_name; }
    constexpr operator const char*() const { return m_name; }

private:
    const char* m_name;
};

class VulkanExtensionsBase
{
public:
    using container = std::vector<const char*>;
    using value_type = container::value_type;
    using iterator = container::iterator;
    using const_iterator = container::const_iterator;
    using pointer = container::pointer;
    using reference = container::reference;

    explicit VulkanExtensionsBase(const std::vector<vk::ExtensionProperties>& available) : m_available{available} {}

    auto data() const { return m_enabled.data(); }
    auto size() const { return m_enabled.size(); }
    auto begin() const { return m_enabled.begin(); }
    auto end() const { return m_enabled.end(); }
    auto empty() const { return m_enabled.empty(); }

protected:
    auto IsAvailableImpl(std::string_view extensionName) -> bool;

    void AddRequiredImpl(const char* extensionName);
    void AddOptionalImpl(const char* extensionName);

    std::vector<const char*> m_enabled; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    const std::vector<vk::ExtensionProperties> m_available;
};

class VulkanInstanceExtensions : public VulkanExtensionsBase
{
public:
    VulkanInstanceExtensions() : VulkanExtensionsBase(vk::enumerateInstanceExtensionProperties()) {}

    explicit VulkanInstanceExtensions(const std::vector<vk::ExtensionProperties>& available) :
        VulkanExtensionsBase(available)
    {}

    auto IsAvailable(InstanceExtensionName extensionName) -> bool { return IsAvailableImpl(extensionName.Get()); }

    void AddSurfaceExtensions(SDL_Window* window);
    void AddRequired(InstanceExtensionName extensionName) { AddRequiredImpl(extensionName); }
    void AddOptional(InstanceExtensionName extensionName) { AddOptionalImpl(extensionName); }
};

vk::DebugUtilsMessengerCreateInfoEXT GetDebugCreateInfo();

vk::UniqueInstance CreateInstance(VulkanLoader& loader, SDL_Window* window = nullptr);
vk::UniqueDevice CreateDevice(VulkanLoader& loader, const PhysicalDevice& physicalDevice);
vma::UniqueAllocator
CreateAllocator(VulkanLoader& loader, vk::Instance instance, vk::Device device, vk::PhysicalDevice physicalDevice);

template <class T>
inline uint32_t size32(const T& cont)
{
    return static_cast<uint32_t>(std::ranges::size(cont));
}

void TransitionImageLayout(
    vk::CommandBuffer cmdBuffer, vk::Image image, Format format, uint32_t mipLevelCount, vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout, vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask);

vk::UniqueCommandPool CreateCommandPool(uint32_t queueFamilyIndex, vk::Device device, const char* debugName = "");

void RegisterDebugName(uint64 handle, const char* debugName);
const char* GetRegisteredDebugName(uint64 handle);

template <class Handle>
void SetDebugName(vk::Device device [[maybe_unused]], Handle handle [[maybe_unused]], const char* debugName [[maybe_unused]])
{
    if constexpr (IsDebugBuild)
    {
        if (VULKAN_HPP_DEFAULT_DISPATCHER.vkSetDebugUtilsObjectNameEXT != nullptr)
        {
            device.setDebugUtilsObjectNameEXT({
                .objectType = handle.objectType,
                .objectHandle = std::bit_cast<uint64>(handle),
                .pObjectName = debugName,
            });
        }
        RegisterDebugName(std::bit_cast<uint64>(handle), debugName);
    }
}

template <class Type, class Dispatch>
void SetDebugName(vk::UniqueHandle<Type, Dispatch>& handle [[maybe_unused]], const char* debugName [[maybe_unused]])
{
    if constexpr (IsDebugBuild)
    {
        SetDebugName(handle.getOwner(), handle.get(), debugName);
    }
}

template <class Type, class Dispatch, class... FormatArgs>
void SetDebugName(
    vk::UniqueHandle<Type, Dispatch>& handle [[maybe_unused]],
    const fmt::format_string<FormatArgs...>& format [[maybe_unused]], FormatArgs&&... fmtArgs [[maybe_unused]])
{
    if constexpr (IsDebugBuild)
    {
        const auto string = fmt::vformat(format, fmt::make_format_args(std::forward<FormatArgs>(fmtArgs)...));
        SetDebugName(handle, string.c_str());
    }
}

template <std::ranges::range R>
void GenerateDebugNames(R& handles, const char* nameBase)
{
    std::ranges::for_each(handles, [nameBase, index = 0](auto& elem) mutable {
        SetDebugName(elem, "{}{}", nameBase, index);
        index++;
    });
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

    [[nodiscard]] const char* what() const noexcept override { return m_what.c_str(); }

private:
    std::string m_what;
};

vk::ImageAspectFlags GetImageAspect(Format format);

void CopyBufferToImage(
    vk::CommandBuffer cmdBuffer, vk::Buffer source, vk::Image destination, Format imageFormat, vk::Extent3D imageExtent);
void CopyImageToBuffer(
    vk::CommandBuffer cmdBuffer, vk::Image source, vk::Buffer destination, Format imageFormat, vk::Extent3D imageExtent,
    uint32 numMipLevels);

vk::UniqueRenderPass CreateRenderPass(vk::Device device, const FramebufferLayout& layout, FramebufferUsage usage);
vk::UniqueRenderPass CreateRenderPass(
    vk::Device device, const FramebufferLayout& layout, FramebufferUsage usage, const RenderPassInfo& renderPassInfo);
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
vk::DescriptorType ToVulkan(ShaderVariableType::BaseType);

Format FromVulkan(vk::Format format);

template <class Rep, class Period>
constexpr uint64 Timeout(std::chrono::duration<Rep, Period> duration)
{
    return static_cast<uint64>(std::chrono::nanoseconds(duration).count());
}

template <class T>
struct VulkanImpl;

template <class T>
auto MakeHandle(T&& object)
{
    return std::make_shared<typename std::remove_const_t<T>>(std::forward<T>(object));
}

inline std::string_view GetLayerName(const vk::LayerProperties& obj)
{
    return obj.layerName;
}
inline std::string_view GetExtensionName(const vk::ExtensionProperties& obj)
{
    return obj.extensionName;
}

} // namespace Teide

template <class T>
    requires vk::isVulkanHandleType<T>::value
struct fmt::formatter<T> : formatter<std::string>
{
    auto format(const T& handle, format_context& ctx) const -> format_context::iterator
    {
        const auto handleValue = std::bit_cast<Teide::uint64>(handle);
        std::string result = fmt::format("0x{:08x}", handleValue);
        if (const char* debugName = Teide::GetRegisteredDebugName(handleValue))
        {
            fmt::format_to(std::back_inserter(result), "[{}]", debugName);
        }
        return fmt::formatter<std::string>::format(result, ctx);
    }
};

template <class Type, class Dispatch>
struct fmt::formatter<vk::UniqueHandle<Type, Dispatch>> : formatter<std::string>
{
    auto format(const vk::UniqueHandle<Type, Dispatch>& handle, format_context& ctx) const -> format_context::iterator
    {
        const auto handleValue = std::bit_cast<Teide::uint64>(handle.get());
        std::string result = fmt::format("0x{:08x}", handleValue);
        if (const char* debugName = Teide::GetRegisteredDebugName(handleValue))
        {
            fmt::format_to(std::back_inserter(result), "[{}]", debugName);
        }
        return fmt::formatter<std::string>::format(result, ctx);
    }
};
