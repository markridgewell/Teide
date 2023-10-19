
#pragma once

#include "CommandBuffer.h"
#include "Synchronized.h"
#include "Vulkan.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanSurface.h"

#include "Teide/BasicTypes.h"
#include "Teide/BytesView.h"
#include "Teide/ForwardDeclare.h"
#include "Teide/Pipeline.h"
#include "Teide/Surface.h"

#include <array>
#include <deque>
#include <mutex>

namespace Teide
{

class VulkanRenderer final : public Renderer
{
public:
    explicit VulkanRenderer(VulkanGraphicsDevice& device, const QueueFamilies& queueFamilies, ShaderEnvironmentPtr shaderEnvironment);

    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer(VulkanRenderer&&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(VulkanRenderer&&) = delete;

    ~VulkanRenderer() override;

    uint32 GetFrameNumber() const override;
    void BeginFrame(ShaderParameters sceneParameters) override;
    void EndFrame() override;

    void WaitForCpu() override;
    void WaitForGpu() override;

    RenderToTextureResult RenderToTexture(const RenderTargetInfo& renderTarget, RenderList renderList) override;
    void RenderToSurface(Surface& surface, RenderList renderList) override;

    Task<TextureData> CopyTextureData(TexturePtr texture) override;

private:
    template <std::invocable<CommandBuffer&> F>
    auto ScheduleGpu(F&& f) -> TaskForCallable<F, CommandBuffer&>
    {
        return m_device.GetScheduler().ScheduleGpu(std::forward<F>(f));
    }

    const ParameterBlockPtr& GetSceneParameterBlock() const { return GetCurrentFrame().sceneParameters; }
    const ParameterBlockPtr& AddViewParameterBlock(uint32 threadIndex, ParameterBlockPtr p)
    {
        return GetCurrentFrame().threadResources.at(threadIndex).viewParameters.emplace_back(std::move(p));
    }

    void RecordRenderListCommands(
        CommandBuffer& commandBuffer, const RenderList& renderList, vk::RenderPass renderPass,
        const RenderPassDesc& renderPassDesc, const Framebuffer& framebuffer);
    void RecordRenderObjectCommands(
        CommandBuffer& commandBufferWrapper, const RenderObject& obj, const RenderPassDesc& renderPassDesc) const;

    std::optional<SurfaceImage> AddSurfaceToPresent(VulkanSurface& surface);

    vk::DescriptorSet GetDescriptorSet(const ParameterBlock* parameterBlock) const;

    struct ThreadResources
    {
        std::vector<ParameterBlockPtr> viewParameters;
    };

    struct FrameResources
    {
        ParameterBlockPtr sceneParameters;
        std::vector<ThreadResources> threadResources;
    };

    const FrameResources& GetCurrentFrame() const { return m_frameResources.at(m_frameNumber); }
    FrameResources& GetCurrentFrame() { return m_frameResources.at(m_frameNumber); }

    VulkanGraphicsDevice& m_device;
    vk::Queue m_graphicsQueue;
    vk::Queue m_presentQueue;
    std::array<vk::UniqueSemaphore, MaxFramesInFlight> m_renderFinished;
    std::array<vk::UniqueFence, MaxFramesInFlight> m_inFlightFences;
    uint32_t m_frameNumber = 0;

    ShaderEnvironmentPtr m_shaderEnvironment;

    Synchronized<std::vector<SurfaceImage>> m_surfacesToPresent;

    std::array<FrameResources, MaxFramesInFlight> m_frameResources;
};

} // namespace Teide
