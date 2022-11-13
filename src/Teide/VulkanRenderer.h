
#pragma once

#include "CommandBuffer.h"
#include "Teide/BasicTypes.h"
#include "Teide/BytesView.h"
#include "Teide/ForwardDeclare.h"
#include "Teide/Pipeline.h"
#include "Teide/Surface.h"
#include "Vulkan.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanSurface.h"

#include <array>
#include <deque>
#include <mutex>

namespace Teide
{

class VulkanRenderer : public Renderer
{
public:
    explicit VulkanRenderer(
        VulkanGraphicsDevice& device, uint32_t graphicsFamilyIndex, std::optional<uint32_t> presentFamilyIndex,
        ShaderEnvironmentPtr shaderEnvironment);

    ~VulkanRenderer();

    uint32 GetFrameNumber() const override;
    void BeginFrame(ShaderParameters sceneParameters) override;
    void EndFrame() override;

    RenderToTextureResult RenderToTexture(const RenderTargetInfo& renderTarget, RenderList renderList) override;
    void RenderToSurface(Surface& surface, RenderList renderList) override;

    Task<TextureData> CopyTextureData(TexturePtr texture) override;

private:
    template <std::invocable<CommandBuffer&> F>
    auto ScheduleGpu(F&& f) -> TaskForCallable<F, CommandBuffer&>
    {
        return m_device.GetScheduler().ScheduleGpu(std::forward<F>(f));
    }

    const ParameterBlockPtr& GetSceneParameterBlock() const { return m_frameResources[m_frameNumber].sceneParameters; }
    const ParameterBlockPtr& AddViewParameterBlock(uint32 threadIndex, ParameterBlockPtr p)
    {
        return m_frameResources[m_frameNumber].threadResources[threadIndex].viewParameters.emplace_back(std::move(p));
    }

    void BuildCommandBuffer(
        CommandBuffer& commandBuffer, const RenderList& renderList, vk::RenderPass renderPass, const Framebuffer& framebuffer);

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

    VulkanGraphicsDevice& m_device;
    vk::Queue m_graphicsQueue;
    vk::Queue m_presentQueue;
    std::array<vk::UniqueSemaphore, MaxFramesInFlight> m_renderFinished;
    std::array<vk::UniqueFence, MaxFramesInFlight> m_inFlightFences;
    uint32_t m_frameNumber = 0;

    ShaderEnvironmentPtr m_shaderEnvironment;

    std::mutex m_surfaceCommandBuffersMutex;
    std::vector<vk::CommandBuffer> m_surfaceCommandBuffers;

    std::mutex m_surfacesToPresentMutex;
    std::vector<SurfaceImage> m_surfacesToPresent;

    std::array<FrameResources, MaxFramesInFlight> m_frameResources;
};

} // namespace Teide
