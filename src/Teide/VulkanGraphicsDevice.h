
#pragma once

#include "CommandBuffer.h"
#include "MemoryAllocator.h"
#include "Scheduler.h"
#include "Vulkan.h"
#include "VulkanLoader.h"

#include "Teide/BasicTypes.h"
#include "Teide/GraphicsDevice.h"
#include "Teide/Renderer.h"
#include "Teide/Surface.h"

#include <compare>
#include <map>
#include <optional>
#include <thread>
#include <type_traits>
#include <unordered_map>

namespace Teide
{

struct ParameterBlockDesc;
struct VulkanParameterBlockLayout;

using VulkanParameterBlockLayoutPtr = std::shared_ptr<const VulkanParameterBlockLayout>;

class VulkanGraphicsDevice : public GraphicsDevice
{
public:
    explicit VulkanGraphicsDevice(
        VulkanLoader loader, vk::UniqueInstance instance, vk::UniqueSurfaceKHR surface,
        const GraphicsSettings& settings = {});

    VulkanGraphicsDevice(const VulkanGraphicsDevice&) = delete;
    VulkanGraphicsDevice(VulkanGraphicsDevice&&) = delete;
    VulkanGraphicsDevice& operator=(const VulkanGraphicsDevice&) = delete;
    VulkanGraphicsDevice& operator=(VulkanGraphicsDevice&&) = delete;

    ~VulkanGraphicsDevice() override;

    SurfacePtr CreateSurface(SDL_Window* window, bool multisampled) override;
    RendererPtr CreateRenderer(ShaderEnvironmentPtr shaderEnvironment) override;

    BufferPtr CreateBuffer(const BufferData& data, const char* name) override;

    ShaderEnvironmentPtr CreateShaderEnvironment(const ShaderEnvironmentData& data, const char* name) override;
    ShaderPtr CreateShader(const ShaderData& data, const char* name) override;
    TexturePtr CreateTexture(const TextureData& data, const char* name) override;
    MeshPtr CreateMesh(const MeshData& data, const char* name) override;
    PipelinePtr CreatePipeline(const PipelineData& data) override;
    ParameterBlockPtr CreateParameterBlock(const ParameterBlockData& data, const char* name) override;

    const vk::PhysicalDeviceProperties GetProperties() const { return m_physicalDevice.physicalDevice.getProperties(); }

    // Internal
    vk::Device GetVulkanDevice() { return m_device.get(); }
    MemoryAllocator& GetMemoryAllocator() { return m_allocator; }
    Scheduler& GetScheduler() { return m_scheduler; }

    template <class T>
    auto& GetImpl(T& obj)
    {
        return dynamic_cast<const typename VulkanImpl<std::remove_const_t<T>>::type&>(obj);
    }

    template <class T>
    auto GetImpl(const std::shared_ptr<T>& ptr)
    {
        return std::dynamic_pointer_cast<const typename VulkanImpl<std::remove_const_t<T>>::type>(ptr);
    }

    BufferPtr CreateBuffer(const BufferData& data, const char* name, CommandBuffer& cmdBuffer);
    TexturePtr CreateTexture(const TextureData& data, const char* name, CommandBuffer& cmdBuffer);
    TexturePtr CreateRenderableTexture(const TextureData& data, const char* name);
    TexturePtr CreateRenderableTexture(const TextureData& data, const char* name, CommandBuffer& cmdBuffer);
    MeshPtr CreateMesh(const MeshData& data, const char* name, CommandBuffer& cmdBuffer);
    ParameterBlockPtr
    CreateParameterBlock(const ParameterBlockData& data, const char* name, CommandBuffer& cmdBuffer, uint32 threadIndex);
    ParameterBlockPtr CreateParameterBlock(
        const ParameterBlockData& data, const char* name, CommandBuffer& cmdBuffer, vk::DescriptorPool descriptorPool);

    vk::RenderPass CreateRenderPassLayout(const FramebufferLayout& framebufferLayout);
    vk::RenderPass CreateRenderPass(const FramebufferLayout& framebufferLayout, const ClearState& clearState);
    Framebuffer CreateFramebuffer(
        vk::RenderPass renderPass, const FramebufferLayout& layout, Geo::Size2i size, std::vector<vk::ImageView> attachments);

private:
    struct RenderPassDesc
    {
        FramebufferLayout framebufferLayout;
        RenderPassInfo renderPassInfo;

        auto operator<=>(const RenderPassDesc&) const = default;
    };

    struct FramebufferDesc
    {
        vk::RenderPass renderPass;
        Geo::Size2i size;
        std::vector<vk::ImageView> attachments;

        auto operator<=>(const FramebufferDesc&) const = default;
    };

    vk::UniqueDescriptorSet CreateDescriptorSet(
        vk::DescriptorPool pool, vk::DescriptorSetLayout layout, const Buffer* uniformBuffer,
        std::span<const TexturePtr> textures, const char* name);

    VulkanParameterBlockLayoutPtr CreateParameterBlockLayout(const ParameterBlockDesc& desc, int set);

    VulkanLoader m_loader;
    vk::UniqueInstance m_instance;
    vk::UniqueDebugUtilsMessengerEXT m_debugMessenger;
    PhysicalDevice m_physicalDevice;
    vk::UniqueDevice m_device;
    GraphicsSettings m_settings;

    std::mutex m_renderPassCacheMutex;
    std::map<RenderPassDesc, vk::UniqueRenderPass> m_renderPassCache;

    std::mutex m_framebufferCacheMutex;
    std::map<FramebufferDesc, vk::UniqueFramebuffer> m_framebufferCache;

    vk::Queue m_graphicsQueue;
    vk::UniqueDescriptorPool m_mainDescriptorPool;
    std::vector<vk::UniqueDescriptorPool> m_workerDescriptorPools;
    vk::UniqueCommandPool m_setupCommandPool;
    vk::UniqueCommandPool m_surfaceCommandPool;

    MemoryAllocator m_allocator;

    Scheduler m_scheduler;
};

} // namespace Teide
