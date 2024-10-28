
#pragma once

#include "CommandBuffer.h"
#include "DescriptorPool.h"
#include "Vulkan.h"
#include "VulkanDevice.h"
#include "VulkanParameterBlock.h"
#include "VulkanSurface.h"

#include "Teide/BasicTypes.h"
#include "Teide/BytesView.h"
#include "Teide/ForwardDeclare.h"
#include "Teide/Pipeline.h"
#include "Teide/Surface.h"
#include "Teide/Util/FrameArray.h"
#include "Teide/Util/ThreadUtils.h"

#include <array>
#include <deque>
#include <mutex>
#include <vector>

namespace Teide
{

class VulkanRenderer final : public Renderer
{
public:
    explicit VulkanRenderer(VulkanDevice& device, const QueueFamilies& queueFamilies, ShaderEnvironmentPtr shaderEnvironment);

    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer(VulkanRenderer&&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(VulkanRenderer&&) = delete;

    ~VulkanRenderer() override;

    void BeginFrame(ShaderParameters sceneParameters) override;
    void EndFrame() override;

    void WaitForCpu() override;
    void WaitForGpu() override;

    RenderToTextureResult RenderToTexture(const RenderTargetInfo& renderTarget, RenderList renderList) override;
    void RenderToSurface(Surface& surface, RenderList renderList) override;

    Task<TextureData> CopyTextureData(Texture texture) override;

private:
    template <std::invocable<CommandBuffer&> F>
    auto ScheduleGpu(F&& f) -> TaskForCallable<F, CommandBuffer&>
    {
        return m_device.GetScheduler().ScheduleGpu(std::forward<F>(f));
    }

    const TransientParameterBlock& GetSceneParameterBlock() const { return m_frameResources.Current().sceneParameters; }

    TransientParameterBlock* CreateViewParameterBlock(const ParameterBlockData& data, const char* name);

    void RecordRenderListCommands(
        CommandBuffer& commandBuffer, const RenderList& renderList, vk::RenderPass renderPass,
        const RenderPassDesc& renderPassDesc, const Framebuffer& framebuffer);
    void RecordRenderObjectCommands(
        CommandBuffer& commandBufferWrapper, const RenderObject& obj, const RenderPassDesc& renderPassDesc) const;

    std::optional<SurfaceImage> AddSurfaceToPresent(VulkanSurface& surface);

    vk::DescriptorSet GetDescriptorSet(const ParameterBlock& parameterBlock) const;

    struct ThreadResources
    {
        std::optional<DescriptorPool> viewDescriptorPool;
        std::vector<TransientParameterBlock> viewParameters;
    };

    struct FrameResources
    {
        explicit FrameResources(VulkanDevice& device, DescriptorPool& sceneDescriptorPool, const ShaderEnvironmentPtr& shaderEnvironment);

        vk::UniqueSemaphore renderFinished;
        vk::UniqueFence inFlightFence;

        TransientParameterBlock sceneParameters;
        ThreadMap<ThreadResources> threadResources;
    };

    VulkanDevice& m_device;
    vk::Queue m_graphicsQueue;
    vk::Queue m_presentQueue;

    ShaderEnvironmentPtr m_shaderEnvironment;

    Synchronized<std::vector<SurfaceImage>> m_surfacesToPresent;

    DescriptorPool m_sceneDescriptorPool;
    FrameArray<FrameResources, MaxFramesInFlight> m_frameResources;
};

} // namespace Teide
