
#pragma once

#include "CommandBuffer.h"
#include "Scheduler.h"
#include "Vulkan.h"
#include "VulkanBuffer.h"
#include "VulkanLoader.h"
#include "VulkanParameterBlock.h"
#include "VulkanTexture.h"

#include "Teide/BasicTypes.h"
#include "Teide/Device.h"
#include "Teide/Hash.h"
#include "Teide/Renderer.h"
#include "Teide/Surface.h"

#include <spdlog/spdlog.h>
#include <vulkan/vulkan_hash.hpp>

#include <compare>
#include <concepts>
#include <optional>
#include <thread>
#include <type_traits>
#include <unordered_map>

namespace Teide
{

struct ParameterBlockDesc;
struct VulkanParameterBlockLayout;
class DescriptorPool;

using VulkanParameterBlockLayoutPtr = std::shared_ptr<const VulkanParameterBlockLayout>;

class VulkanDevice : public Device, public RefCounter
{
public:
    explicit VulkanDevice(
        VulkanLoader loader, vk::UniqueInstance instance, Teide::PhysicalDevice physicalDevice,
        const GraphicsSettings& settings = {});

    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice(VulkanDevice&&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;
    VulkanDevice& operator=(VulkanDevice&&) = delete;

    ~VulkanDevice() override;

    SurfacePtr CreateSurface(SDL_Window* window, bool multisampled) override;
    RendererPtr CreateRenderer(ShaderEnvironmentPtr shaderEnvironment) override;

    BufferPtr CreateBuffer(const BufferData& data, const char* name) override;

    ShaderEnvironmentPtr CreateShaderEnvironment(const ShaderEnvironmentData& data, const char* name) override;
    ShaderPtr CreateShader(const ShaderData& data, const char* name) override;
    Texture CreateTexture(const TextureData& data, const char* name) override;
    MeshPtr CreateMesh(const MeshData& data, const char* name) override;
    PipelinePtr CreatePipeline(const PipelineData& data) override;
    ParameterBlockPtr CreateParameterBlock(const ParameterBlockData& data, const char* name) override;

    vk::PhysicalDeviceProperties GetProperties() const { return m_physicalDevice.physicalDevice.getProperties(); }

    void AddRef(uint64 index) override
    {
        auto& texture = m_textures.at(index);
        ++texture.refCount;
        spdlog::debug("Adding ref to texture {} (now {})", index, texture.refCount);
    }

    void DecRef(uint64 index) override
    {
        auto& texture = m_textures.at(index);
        --texture.refCount;
        spdlog::debug("Decrementing ref from texture {} (now {})", index, texture.refCount);
        if (texture.refCount == 0)
        {
            // Add texture to unused pool
            spdlog::debug("Destroying texture {}", index);
            texture = {};
        }
    }

    // Internal
    vk::Device GetVulkanDevice() { return m_device.get(); }
    vma::Allocator& GetAllocator() { return m_allocator.get(); }
    Scheduler& GetScheduler() { return m_scheduler; }

    template <class T>
    auto& GetImpl(T& obj)
    {
        return dynamic_cast<const typename VulkanImpl<std::remove_const_t<T>>::type&>(obj);
    }

    const VulkanTexture& GetImpl(Texture& obj)
    {
        const auto index = static_cast<uint64>(obj);
        return m_textures.at(index);
    }

    const VulkanTexture& GetImpl(const Texture& obj)
    {
        const auto index = static_cast<uint64>(obj);
        return m_textures.at(index);
    }

    template <class T>
    auto GetImpl(const std::shared_ptr<T>& ptr)
    {
        return std::dynamic_pointer_cast<const typename VulkanImpl<std::remove_const_t<T>>::type>(ptr);
    }

    struct TextureAndState
    {
        VulkanTexture texture;
        TextureState state;
    };

    TextureAndState
    CreateTextureImpl(const TextureData& data, vk::ImageUsageFlags usage, CommandBuffer& cmdBuffer, const char* debugName);

    VulkanBuffer CreateBufferUninitialized(
        vk::DeviceSize size, vk::BufferUsageFlags usage, vma::AllocationCreateFlags allocationFlags = {},
        vma::MemoryUsage memoryUsage = vma::MemoryUsage::eAuto);
    VulkanBuffer CreateBufferWithData(BytesView data, BufferUsage usage, ResourceLifetime lifetime, CommandBuffer& cmdBuffer);
    void SetBufferData(VulkanBuffer& buffer, BytesView data);

    SurfacePtr CreateSurface(vk::UniqueSurfaceKHR surface, SDL_Window* window, bool multisampled);
    BufferPtr CreateBuffer(const BufferData& data, const char* name, CommandBuffer& cmdBuffer);
    VulkanBuffer CreateTransientBuffer(const BufferData& data, const char* name);
    Texture CreateTexture(const TextureData& data, const char* name, CommandBuffer& cmdBuffer);
    Texture CreateRenderableTexture(const TextureData& data, const char* name);
    Texture CreateRenderableTexture(const TextureData& data, const char* name, CommandBuffer& cmdBuffer);
    MeshPtr CreateMesh(const MeshData& data, const char* name, CommandBuffer& cmdBuffer);
    ParameterBlockPtr
    CreateParameterBlock(const ParameterBlockData& data, const char* name, CommandBuffer& cmdBuffer, uint32 threadIndex);
    ParameterBlockPtr CreateParameterBlock(
        const ParameterBlockData& data, const char* name, CommandBuffer& cmdBuffer, vk::DescriptorPool descriptorPool);
    TransientParameterBlock
    CreateTransientParameterBlock(const ParameterBlockData& data, const char* name, DescriptorPool& descriptorPool);
    void UpdateTransientParameterBlock(TransientParameterBlock& pblock, const ParameterBlockData& data);

    vk::RenderPass CreateRenderPassLayout(const FramebufferLayout& framebufferLayout);
    vk::RenderPass CreateRenderPass(const FramebufferLayout& framebufferLayout, const ClearState& clearState);
    Framebuffer CreateFramebuffer(
        vk::RenderPass renderPass, const FramebufferLayout& layout, Geo::Size2i size, std::vector<vk::ImageView> attachments);

private:
    struct RenderPassDesc
    {
        FramebufferLayout framebufferLayout;
        RenderPassInfo renderPassInfo;

        bool operator==(const RenderPassDesc&) const = default;
        void Visit(auto f) const { return f(framebufferLayout, renderPassInfo); }
    };

    struct FramebufferDesc
    {
        vk::RenderPass renderPass;
        Geo::Size2i size;
        std::vector<vk::ImageView> attachments;

        bool operator==(const FramebufferDesc&) const = default;
        void Visit(auto f) const { return f(renderPass, size, attachments); }
    };

    vk::UniqueDescriptorSet CreateUniqueDescriptorSet(
        vk::DescriptorPool pool, vk::DescriptorSetLayout layout, const Buffer* uniformBuffer,
        std::span<const Texture> textures, const char* name);
    void WriteDescriptorSet(vk::DescriptorSet descriptorSet, const Buffer* uniformBuffer, std::span<const Texture> textures);

    VulkanParameterBlockLayoutPtr CreateParameterBlockLayout(const ParameterBlockDesc& desc, int set);

    VulkanLoader m_loader;
    vk::UniqueInstance m_instance;
    vk::UniqueDebugUtilsMessengerEXT m_debugMessenger;
    PhysicalDevice m_physicalDevice;
    vk::UniqueDevice m_device;
    GraphicsSettings m_settings;

    std::mutex m_renderPassCacheMutex;
    std::unordered_map<RenderPassDesc, vk::UniqueRenderPass, Hash<RenderPassDesc>> m_renderPassCache;

    std::mutex m_framebufferCacheMutex;
    std::unordered_map<FramebufferDesc, vk::UniqueFramebuffer, Hash<FramebufferDesc>> m_framebufferCache;

    vk::Queue m_graphicsQueue;
    vk::UniqueDescriptorPool m_mainDescriptorPool;
    std::vector<vk::UniqueDescriptorPool> m_workerDescriptorPools;
    vk::UniqueCommandPool m_setupCommandPool;
    vk::UniqueCommandPool m_surfaceCommandPool;

    vma::UniqueAllocator m_allocator;

    std::vector<VulkanTexture> m_textures;

    Scheduler m_scheduler;
};

using VulkanDevicePtr = std::unique_ptr<VulkanDevice>;

} // namespace Teide
