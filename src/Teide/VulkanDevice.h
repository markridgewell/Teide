
#pragma once

#include "CommandBuffer.h"
#include "Scheduler.h"
#include "Vulkan.h"
#include "VulkanBuffer.h"
#include "VulkanKernel.h"
#include "VulkanLoader.h"
#include "VulkanParameterBlock.h"
#include "VulkanTexture.h"

#include "Teide/BasicTypes.h"
#include "Teide/Device.h"
#include "Teide/Hash.h"
#include "Teide/Renderer.h"
#include "Teide/Surface.h"
#include "Teide/Util/ResourceMap.h"
#include "Teide/Util/ThreadUtils.h"

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

class VulkanDevice : public Device
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
    Kernel CreateKernel(const KernelData& data, const char* name) override;
    Texture CreateTexture(const TextureData& data, const char* name) override;
    MeshPtr CreateMesh(const MeshData& data, const char* name) override;
    PipelinePtr CreatePipeline(const PipelineData& data) override;
    ParameterBlock CreateParameterBlock(const ParameterBlockData& data, const char* name) override;

    // Internal
    vk::Device GetVulkanDevice() { return m_device.get(); }
    vk::Queue GetGraphicsQueue() { return m_graphicsQueue; }
    vma::Allocator& GetAllocator() { return m_allocator.get(); }
    Scheduler& GetScheduler() { return m_scheduler; }
    QueueFamilies GetQueueFamilies() const { return m_physicalDevice.queueFamilies; }

    template <class T>
    auto& GetImpl(T& obj)
        requires std::is_polymorphic_v<T>
    {
        return dynamic_cast<typename VulkanImpl<std::remove_const_t<T>>::type&>(obj);
    }

    template <class T>
    auto& GetImpl(const T& obj)
        requires std::is_polymorphic_v<T>
    {
        return dynamic_cast<const typename VulkanImpl<std::remove_const_t<T>>::type&>(obj);
    }

    template <class T>
    auto& GetImpl(T& obj)
    {
        using Handle = std::remove_const_t<T>;
        using Impl = VulkanImpl<Handle>::type;
        using Map = ResourceMap<Handle, Impl>;
        using MemPtrType = Map VulkanDevice::*;
        constexpr auto MemPtr = std::get<MemPtrType>(ResourceMaps);
        return (this->*MemPtr).Get(obj);
    }

    template <class T>
    auto GetImpl(const std::shared_ptr<T>& ptr)
    {
        return std::dynamic_pointer_cast<const typename VulkanImpl<std::remove_const_t<T>>::type>(ptr);
    }

    TextureState CreateTextureImpl(VulkanTexture& texture, vk::ImageUsageFlags usage);

    VulkanBufferData CreateBufferUninitialized(
        vk::DeviceSize size, vk::BufferUsageFlags usage, vma::AllocationCreateFlags allocationFlags = {},
        vma::MemoryUsage memoryUsage = vma::MemoryUsage::eAuto);
    VulkanBuffer CreateBufferWithData(BytesView data, BufferUsage usage, ResourceLifetime lifetime, CommandBuffer& cmdBuffer);
    void SetBufferData(VulkanBuffer& buffer, BytesView data);

    SurfacePtr CreateSurface(vk::UniqueSurfaceKHR surface, Geo::Size2i size, bool multisampled);
    BufferPtr CreateBuffer(const BufferData& data, const char* name, CommandBuffer& cmdBuffer);
    VulkanBuffer CreateTransientBuffer(const BufferData& data, const char* name);
    Texture AllocateTexture(const TextureProperties& props, const SamplerState& samplerState = {});
    Texture CreateTexture(const TextureData& data, const char* name, CommandBuffer& cmdBuffer);
    Texture CreateRenderableTexture(const TextureData& data, const char* name);
    Texture CreateRenderableTexture(const TextureData& data, const char* name, CommandBuffer& cmdBuffer);
    MeshPtr CreateMesh(const MeshData& data, const char* name, CommandBuffer& cmdBuffer);
    ParameterBlock
    CreateParameterBlock(const ParameterBlockData& data, const char* name, CommandBuffer& cmdBuffer, uint32 threadIndex);
    ParameterBlock CreateParameterBlock(
        const ParameterBlockData& data, const char* name, CommandBuffer& cmdBuffer, vk::DescriptorPool descriptorPool);
    void InitParameterBlock(VulkanParameterBlock& pblock);
    TransientParameterBlock
    CreateTransientParameterBlock(const ParameterBlockData& data, const char* name, DescriptorPool& descriptorPool);
    void UpdateTransientParameterBlock(TransientParameterBlock& pblock, const ParameterBlockData& data);

    vk::RenderPass CreateRenderPassLayout(const FramebufferLayout& framebufferLayout);
    vk::RenderPass
    CreateRenderPass(const FramebufferLayout& framebufferLayout, const ClearState& clearState, FramebufferUsage usage);
    Framebuffer CreateFramebuffer(
        vk::RenderPass renderPass, const FramebufferLayout& layout, Geo::Size2i size, std::vector<vk::ImageView> attachments);

    VulkanParameterBlockLayoutPtr CreateParameterBlockLayout(const ParameterBlockDesc& desc, int set);

    vk::DescriptorSet GetDescriptorSet(const ParameterBlock& parameterBlock);

private:
    struct RenderPassDesc
    {
        FramebufferLayout framebufferLayout;
        RenderPassInfo renderPassInfo;
        FramebufferUsage usage = FramebufferUsage::ShaderInput;

        bool operator==(const RenderPassDesc&) const = default;
        void Visit(auto f) const { return f(framebufferLayout, renderPassInfo, usage); }
    };

    struct FramebufferDesc
    {
        vk::RenderPass renderPass;
        Geo::Size2i size;
        std::vector<vk::ImageView> attachments;

        bool operator==(const FramebufferDesc&) const = default;
        void Visit(auto f) const { return f(renderPass, size, attachments); }
    };

    vk::UniqueSampler CreateSampler(const SamplerState& ss);

    vk::UniqueDescriptorSet CreateUniqueDescriptorSet(
        vk::DescriptorPool pool, vk::DescriptorSetLayout layout, const Buffer* uniformBuffer,
        std::span<const Texture> textures, const char* name);
    bool WriteDescriptorSet(vk::DescriptorSet descriptorSet, const Buffer* uniformBuffer, std::span<const Texture> textures);

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

    ResourceMap<Texture, VulkanTexture> m_textures;
    ResourceMap<Kernel, VulkanKernel> m_kernels;
    ResourceMap<ParameterBlock, VulkanParameterBlock> m_parameterBlocks;

    static constexpr auto ResourceMaps = std::tuple{
        &VulkanDevice::m_textures,
        &VulkanDevice::m_kernels,
        &VulkanDevice::m_parameterBlocks,
    };

    Scheduler m_scheduler;
};

using VulkanDevicePtr = std::unique_ptr<VulkanDevice>;

} // namespace Teide
