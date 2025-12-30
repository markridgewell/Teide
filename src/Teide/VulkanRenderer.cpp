
#include "VulkanRenderer.h"

#include "Vulkan.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanMesh.h"
#include "VulkanParameterBlock.h"
#include "VulkanPipeline.h"
#include "VulkanShader.h"
#include "VulkanShaderEnvironment.h"
#include "VulkanTexture.h"

#include "Teide/Renderer.h"
#include "Teide/TextureData.h"
#include "vkex/vkex.hpp"

#include <fmt/chrono.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_handles.hpp>

#include <algorithm>
#include <array>
#include <ranges>

namespace Teide
{

namespace
{
    constexpr uint32 ViewDescriptorPoolSize = 8;

    const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

    vk::Viewport MakeViewport(Geo::Size2i size, const ViewportRegion& region = {})
    {
        return {
            .x = region.left * static_cast<float>(size.x),
            .y = region.top * static_cast<float>(size.y),
            .width = region.right * static_cast<float>(size.x),
            .height = region.bottom * static_cast<float>(size.y),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
    }

    std::vector<vk::ClearValue> MakeClearValues(const Framebuffer& framebuffer, const ClearState& clearState)
    {
        auto clearValues = std::vector<vk::ClearValue>();
        if (framebuffer.layout.colorFormat.has_value())
        {
            if (clearState.colorValue.has_value())
            {
                clearValues.emplace_back(vk::ClearColorValue{*clearState.colorValue});
            }
            else
            {
                clearValues.emplace_back();
            }
        }
        if (framebuffer.layout.depthStencilFormat.has_value())
        {
            clearValues.emplace_back(
                vk::ClearDepthStencilValue{
                    .depth = clearState.depthValue.value_or(1.0f),
                    .stencil = clearState.stencilValue.value_or(0),
                });
        };
        return clearValues;
    }

    DescriptorPool MakeSceneDescriptorPool(VulkanDevice& device, const ShaderEnvironmentPtr& shaderEnvironment)
    {
        const auto vkdevice = device.GetVulkanDevice();

        if (shaderEnvironment)
        {
            const auto scenePblockLayout = device.GetImpl(shaderEnvironment->GetScenePblockLayout());
            return DescriptorPool(vkdevice, *scenePblockLayout, MaxFramesInFlight);
        }

        return DescriptorPool(vkdevice, {}, MaxFramesInFlight);
    }

} // namespace

/*
This is how CPU-GPU synchronisation works, using an example where the application is GPU-bound.
The frame number is modded with the MaxFramesInFlight (2 in this example).

 1. The CPU processes frame 0 and submits it to the GPU
 2. The CPU immediately moves on to frame 1 while the GPU starts processing frame 0
    (NOTE: the GPU might actually start before the CPU is finished with the frame, since command buffers
    can be submitted to the Vulkan queue at any point in the frame)
 3. The CPU finishes processing frame 1 and the work is queued for execution on the GPU
 4. The CPU waits for the GPU to finish frame 0, and then begins frame 0
 5. The CPU starts work on the new frame 0 while the GPU starts work on frame 1
 6. The CPU finishes processing frame 0 and the work is queued for execution on the GPU
 7. The CPU waits for the GPU to finish frame 1, and then begins frame 1
 8. The CPU starts work on the new frame 1 while the GPU starts work on frame 0
 9. Repeat steps 3-8 ad infinitum

    +-------+-------+              +-------+              +-------+              +-------+
CPU |   0   |   1   |              |   0   |              |   1   |              |   0   |
    +-------+-------+--------------+-------+--------------+-------+--------------+-------+--------------+
GPU         |          0           |           1          |          0           |           1          |
            +----------------------+----------------------+----------------------+----------------------+

This allows the CPU and GPU to work concurrently, while ensuring they never work on the same frame at the same time.
*/

VulkanRenderer::VulkanRenderer(VulkanDevice& device, const QueueFamilies& queueFamilies, ShaderEnvironmentPtr shaderEnvironment) :
    m_device{device},
    m_graphicsQueue{device.GetVulkanDevice().getQueue(queueFamilies.graphicsFamily, 0)},
    m_shaderEnvironment{std::move(shaderEnvironment)},
    m_sceneDescriptorPool(MakeSceneDescriptorPool(device, m_shaderEnvironment)),
    m_frameResources(device, m_sceneDescriptorPool, m_shaderEnvironment)
{
    using std::ranges::generate;
    const auto vkdevice = device.GetVulkanDevice();

    if (queueFamilies.presentFamily)
    {
        m_presentQueue = vkdevice.getQueue(*queueFamilies.presentFamily, 0);
    }
}


VulkanRenderer::~VulkanRenderer()
{
    WaitForGpu();

    // Wait for all frames' fences
    constexpr auto timeout = std::chrono::seconds{1};
    for (usize i = 0; i < MaxFramesInFlight; i++)
    {
        const vk::Fence fence = m_frameResources.Current().inFlightFence.get();
        m_frameResources.NextFrame();
        if (m_device.GetVulkanDevice().waitForFences(fence, true, Timeout(timeout)) == vk::Result::eTimeout)
        {
            spdlog::error("Timeout (>{}) while waiting for all in-flight command buffers to complete!", timeout);
        }
    }
}

void VulkanRenderer::BeginFrame(ShaderParameters sceneParameters)
{
    m_frameResources.NextFrame();

    constexpr uint64_t timeout = std::numeric_limits<uint64_t>::max();

    const vk::Fence inFlightFence = m_frameResources.Current().inFlightFence.get();
    [[maybe_unused]] const auto waitResult = m_device.GetVulkanDevice().waitForFences(inFlightFence, true, timeout);
    TEIDE_ASSERT(waitResult == vk::Result::eSuccess); // TODO check if waitForFences can fail with no timeout

    m_device.GetScheduler().NextFrame();

    auto& frameResources = m_frameResources.Current();
    const ParameterBlockData pblockData = {
        .layout = m_shaderEnvironment ? m_shaderEnvironment->GetScenePblockLayout() : nullptr,
        .lifetime = ResourceLifetime::Transient,
        .parameters = std::move(sceneParameters),
    };
    m_device.UpdateTransientParameterBlock(frameResources.sceneParameters, pblockData);
    frameResources.threadResources.LockAll([](ThreadResources& threadResources) {
        if (threadResources.viewDescriptorPool)
        {
            threadResources.viewDescriptorPool->Reset();
        }
        threadResources.viewParameters.clear();
    });
}

void VulkanRenderer::EndFrame()
{
    using std::views::transform;

    const auto device = m_device.GetVulkanDevice();

    WaitForCpu();

    std::vector<SurfaceImage> images = m_surfacesToPresent.Lock([&](auto& s) { return std::exchange(s, {}); });
    if (images.empty())
    {
        return;
    }

    TEIDE_ASSERT(m_presentQueue, "Can't present without a present queue");

    auto fenceToSignal = m_frameResources.Current().inFlightFence.get();

    device.resetFences(fenceToSignal);

    const auto waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    const vkex::SubmitInfo submitInfo = {
        .waitSemaphores = transform(images, &SurfaceImage::imageAvailable),
        .waitDstStageMask = transform(images, [=](auto&&) { return waitStage; }),
        .signalSemaphores = m_frameResources.Current().renderFinished.get(),
    };
    m_graphicsQueue.submit(submitInfo.map(), fenceToSignal);

    // Present
    const vkex::PresentInfoKHR presentInfo = {
        .waitSemaphores = m_frameResources.Current().renderFinished.get(),
        .swapchains = transform(images, &SurfaceImage::swapchain),
        .imageIndices = transform(images, &SurfaceImage::imageIndex),
    };

    const auto presentResult = m_presentQueue.presentKHR(presentInfo);
    if (presentResult == vk::Result::eSuboptimalKHR)
    {
        spdlog::warn("Suboptimal swapchain image");
    }
}

void VulkanRenderer::WaitForCpu()
{
    m_device.GetScheduler().WaitForCpu();
}

void VulkanRenderer::WaitForGpu()
{
    m_device.GetScheduler().WaitForGpu();
}

RenderToTextureResult VulkanRenderer::RenderToTexture(const RenderTargetInfo& renderTarget, RenderList renderList)
{
    TEIDE_ASSERT(
        renderTarget.framebufferLayout.captureColor || renderTarget.framebufferLayout.captureDepthStencil,
        "Nothing to capture in RTT pass");
    TEIDE_ASSERT(
        renderTarget.framebufferLayout.captureColor || !renderTarget.framebufferLayout.resolveColor,
        "Cannot resolve color unless also capturing color");
    TEIDE_ASSERT(
        renderTarget.framebufferLayout.captureDepthStencil || !renderTarget.framebufferLayout.resolveDepthStencil,
        "Cannot resolve depth/stencil unless also capturing depth/stencil");

    const auto CreateRenderableTexture
        = [&](std::optional<Format> format, uint32 sampleCount, const char* name) -> std::optional<Texture> {
        if (!format)
        {
            return std::nullopt;
        }

        const TextureData data = {
            .size = renderTarget.size,
            .format = *format,
            .mipLevelCount = 1,
            .sampleCount = sampleCount,
            .samplerState = renderTarget.samplerState,
        };

        const auto textureName = fmt::format("{}:{}", renderList.name, name);
        return m_device.CreateRenderableTexture(data, textureName.c_str());
    };


    const auto& fb = renderTarget.framebufferLayout;
    const auto sampleCount = renderTarget.framebufferLayout.sampleCount;
    const RenderTarget rt = {
        .color = CreateRenderableTexture(fb.colorFormat, sampleCount, "color"),
        .depthStencil = CreateRenderableTexture(fb.depthStencilFormat, sampleCount, "depthStencil"),
        .colorResolved = sampleCount > 1 && renderTarget.framebufferLayout.resolveColor
            ? CreateRenderableTexture(fb.colorFormat, 1, "colorResolved")
            : std::nullopt,
        .depthStencilResolved = sampleCount > 1 && renderTarget.framebufferLayout.resolveDepthStencil
            ? CreateRenderableTexture(fb.depthStencilFormat, 1, "depthStencilResolved")
            : std::nullopt,
    };

    ScheduleGpu([this, renderList = std::move(renderList), rt, renderTarget](CommandBuffer& commandBuffer) mutable {
        const auto viewParameters = CreateViewParameters(renderList);

        const auto sceneParameters = GetSceneParameterBlock().descriptorSet;

        CreateRenderCommandBuffer(m_device, commandBuffer, renderList, renderTarget, rt, sceneParameters, viewParameters);

        commandBuffer.TakeOwnership(std::move(renderList));
    });

    const auto& colorRet = rt.colorResolved ? rt.colorResolved : rt.color;
    const auto& depthRet = rt.depthStencilResolved ? rt.depthStencilResolved : rt.depthStencil;
    return {
        .colorTexture = renderTarget.framebufferLayout.captureColor ? colorRet : std::nullopt,
        .depthStencilTexture = renderTarget.framebufferLayout.captureDepthStencil ? depthRet : std::nullopt,
    };
}

void VulkanRenderer::RenderToSurface(Surface& surface, RenderList renderList)
{
    auto& surfaceImpl = dynamic_cast<VulkanSurface&>(surface);
    if (const auto surfaceImageOpt = AddSurfaceToPresent(surfaceImpl))
    {
        const auto& surfaceImage = *surfaceImageOpt;

        const auto framebuffer = surfaceImage.framebuffer;

        ScheduleGpu([this, renderList = std::move(renderList), framebuffer](CommandBuffer& commandBuffer) mutable {
            const auto renderPassDesc = RenderPassDesc{
                .framebufferLayout = framebuffer.layout,
                .renderOverrides = renderList.renderOverrides,
            };

            const auto renderPass
                = m_device.CreateRenderPass(framebuffer.layout, renderList.clearState, FramebufferUsage::PresentSrc);

            const auto viewPblockLayout = m_shaderEnvironment ? m_shaderEnvironment->GetViewPblockLayout() : nullptr;

            const auto viewParameters = CreateViewParameters(renderList);

            const auto sceneParameters = GetSceneParameterBlock().descriptorSet;

            RecordRenderListCommands(
                m_device, commandBuffer, renderList, renderPass, renderPassDesc, framebuffer, sceneParameters, viewParameters);

            commandBuffer.TakeOwnership(std::move(renderList));
        });
    }
}

Task<TextureData> VulkanRenderer::CopyTextureData(Texture texture)
{
    TEIDE_ASSERT(texture.GetSampleCount() == 1, "Cannot copy data of a multisampled texture");

    const VulkanTexture& textureImpl = m_device.GetImpl(texture);

    const TextureData textureData = {
        .size = textureImpl.properties.size,
        .format = textureImpl.properties.format,
        .mipLevelCount = textureImpl.properties.mipLevelCount,
        .sampleCount = textureImpl.properties.sampleCount,
    };

    const auto bufferSize = GetByteSize(textureData);

    auto task = ScheduleGpu([this, texture = std::move(texture), bufferSize](CommandBuffer& commandBuffer) {
        auto buffer = m_device.CreateBufferUninitialized(
            bufferSize, vk::BufferUsageFlagBits::eTransferDst,
            vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessRandom);

        const VulkanTexture& textureImpl = m_device.GetImpl(texture);

        commandBuffer.AddReference(texture);

        const bool isDepthStencil = HasDepthOrStencilComponent(textureImpl.properties.format);
        TextureState textureState = {
            .layout = isDepthStencil ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal,
            .lastPipelineStageUsage = vk::PipelineStageFlagBits::eFragmentShader,
        };
        textureImpl.TransitionToTransferSrc(textureState, commandBuffer);
        const vk::Extent3D extent = {
            .width = textureImpl.properties.size.x,
            .height = textureImpl.properties.size.y,
            .depth = 1,
        };
        CopyImageToBuffer(
            commandBuffer, textureImpl.image.get(), buffer.buffer.get(), textureImpl.properties.format, extent,
            textureImpl.properties.mipLevelCount);
        textureImpl.TransitionToShaderInput(textureState, commandBuffer);

        return std::make_shared<VulkanBuffer>(std::move(buffer));
    });

    return m_device.GetScheduler().ScheduleAfter(task, [this, textureData](const BufferPtr& buffer) {
        const auto& data = m_device.GetImpl(*buffer).mappedData;

        TextureData ret = textureData;
        ret.pixels.resize(data.size());
        std::ranges::copy(data, ret.pixels.data());
        return ret;
    });
}

void VulkanRenderer::CreateRenderCommandBuffer(
    VulkanDevice& device, vk::CommandBuffer commandBuffer, const RenderList& renderList, const RenderTargetInfo& renderTarget,
    const RenderTarget& rt, vk::DescriptorSet sceneParameters, vk::DescriptorSet viewParameters)
{
    std::vector<vk::ImageView> attachments;

    const auto addAttachment = [&](const std::optional<Texture>& texture) {
        spdlog::debug("texture: {}", texture ? texture->GetName() : "null");
        if (texture.has_value())
        {
            const auto& textureImpl = device.GetImpl(*texture);
            TextureState textureState{};
            textureImpl.TransitionToRenderTarget(textureState, commandBuffer);
            attachments.push_back(textureImpl.imageView.get());
        }
    };

    addAttachment(rt.color);
    addAttachment(rt.depthStencil);
    addAttachment(rt.colorResolved);
    addAttachment(rt.depthStencilResolved);

    const RenderPassDesc renderPassDesc = {
        .framebufferLayout = renderTarget.framebufferLayout,
        .renderOverrides = renderList.renderOverrides,
    };

    const auto renderPass
        = device.CreateRenderPass(renderTarget.framebufferLayout, renderList.clearState, FramebufferUsage::ShaderInput);
    const auto framebuffer
        = device.CreateFramebuffer(renderPass, renderTarget.framebufferLayout, renderTarget.size, attachments);

    RecordRenderListCommands(
        device, commandBuffer, renderList, renderPass, renderPassDesc, framebuffer, sceneParameters, viewParameters);
}

auto VulkanRenderer::CreateViewParameters(const RenderList& renderList) -> vk::DescriptorSet
{
    const auto viewPblockLayout = m_shaderEnvironment ? m_shaderEnvironment->GetViewPblockLayout() : nullptr;

    const ParameterBlockData viewParamsData = {
        .layout = viewPblockLayout,
        .lifetime = ResourceLifetime::Transient,
        .parameters = renderList.viewParameters,
    };
    const auto viewParamsName = fmt::format("{}:View", renderList.name);

    return m_shaderEnvironment
        ? m_frameResources.Current()
              .threadResources
              .LockCurrent(&ThreadResources::CreateViewParameterBlock, m_device, viewParamsData, viewParamsName.c_str())
              ->descriptorSet
        : vk::DescriptorSet{};
}

void VulkanRenderer::RecordRenderListCommands(
    VulkanDevice& device, vk::CommandBuffer commandBuffer, const RenderList& renderList, vk::RenderPass renderPass,
    const RenderPassDesc& renderPassDesc, const Framebuffer& framebuffer, vk::DescriptorSet sceneParameters,
    vk::DescriptorSet viewParameters)
{
    const vkex::RenderPassBeginInfo renderPassBegin = {
        .renderPass = renderPass,
        .framebuffer = framebuffer.framebuffer,
        .renderArea = {.offset = {.x = 0, .y = 0}, .extent = {.width = framebuffer.size.x, .height = framebuffer.size.y}},
        .clearValues = MakeClearValues(framebuffer, renderList.clearState),
    };

    const auto viewport = MakeViewport(framebuffer.size, renderList.viewportRegion);
    commandBuffer.setViewport(0, viewport);
    const auto scissor = renderList.scissor
        ? ToVulkan(*renderList.scissor)
        : vk::Rect2D{.extent = {.width = framebuffer.size.x, .height = framebuffer.size.y}};
    commandBuffer.setScissor(0, scissor);

    commandBuffer.beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);

    if (!renderList.objects.empty())
    {
        const auto& firstPipeline = device.GetImpl(*renderList.objects.front().pipeline);

        if (sceneParameters)
        {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, firstPipeline.layout, 0, sceneParameters, {});
        }
        if (viewParameters)
        {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, firstPipeline.layout, 1, viewParameters, {});
        }

        for (const RenderObject& obj : renderList.objects)
        {
            RecordRenderObjectCommands(device, commandBuffer, obj, renderPassDesc);
        }
    }

    commandBuffer.endRenderPass();
}

void VulkanRenderer::RecordRenderObjectCommands(
    VulkanDevice& device, vk::CommandBuffer commandBuffer, const RenderObject& obj, const RenderPassDesc& renderPassDesc)
{
    const auto& pipeline = device.GetImpl(*obj.pipeline);

    // commandBufferWrapper.AddReference(obj.mesh);
    // commandBufferWrapper.AddReference(obj.materialParameters);
    // commandBufferWrapper.AddReference(obj.pipeline);

    if (const auto dset = device.GetDescriptorSet(obj.materialParameters))
    {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout, 2, dset, {});
    }

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.GetPipeline(renderPassDesc));

    const auto& meshImpl = device.GetImpl(*obj.mesh);
    commandBuffer.bindVertexBuffers(0, meshImpl.vertexBuffer->buffer.get(), vk::DeviceSize{0});

    if (pipeline.shader->objectPblockLayout && pipeline.shader->objectPblockLayout->pushConstantRange.has_value())
    {
        commandBuffer.pushConstants(
            pipeline.layout, pipeline.shader->objectPblockLayout->uniformsStages, 0,
            size32(obj.objectParameters.uniformData), data(obj.objectParameters.uniformData));
    }

    if (meshImpl.indexBuffer)
    {
        commandBuffer.bindIndexBuffer(meshImpl.indexBuffer->buffer.get(), vk::DeviceSize{0}, meshImpl.indexType);
        commandBuffer.drawIndexed(meshImpl.indexCount, 1, 0, 0, 0);
    }
    else
    {
        commandBuffer.draw(meshImpl.vertexCount, 1, 0, 0);
    }
}

std::optional<SurfaceImage> VulkanRenderer::AddSurfaceToPresent(VulkanSurface& surface)
{
    return m_surfacesToPresent.Lock([&](auto& surfacesToPresent) -> std::optional<SurfaceImage> {
        if (const auto it = std::ranges::find(surfacesToPresent, surface.GetVulkanSurface(), &SurfaceImage::surface);
            it != surfacesToPresent.end())
        {
            return *it;
        }

        if (const auto result = surface.AcquireNextImage(m_frameResources.Current().inFlightFence.get()))
        {
            return surfacesToPresent.emplace_back(*result);
        }

        return std::nullopt;
    });
}

TransientParameterBlock* VulkanRenderer::ThreadResources::CreateViewParameterBlock(
    VulkanDevice& device, const ParameterBlockData& data, const char* name)
{
    if (!viewDescriptorPool.has_value())
    {
        return nullptr;
    }

    auto p = device.CreateTransientParameterBlock(data, name, *viewDescriptorPool);
    return &viewParameters.emplace_back(std::move(p));
}

VulkanRenderer::FrameResources::FrameResources(
    VulkanDevice& device, DescriptorPool& sceneDescriptorPool, const ShaderEnvironmentPtr& shaderEnvironment) :
    threadResources(device.GetScheduler().GetThreadCount())
{
    const auto vkdevice = device.GetVulkanDevice();

    renderFinished = vkdevice.createSemaphoreUnique({}, s_allocator);
    inFlightFence = vkdevice.createFenceUnique({.flags = vk::FenceCreateFlagBits::eSignaled}, s_allocator);

    if (shaderEnvironment)
    {
        const auto scenePblockLayout = device.GetImpl(shaderEnvironment->GetScenePblockLayout());
        const auto viewPblockLayout = device.GetImpl(shaderEnvironment->GetViewPblockLayout());

        if (scenePblockLayout->HasDescriptors())
        {
            const ParameterBlockData pblockData = {
                .layout = scenePblockLayout,
                .lifetime = ResourceLifetime::Transient,
            };
            sceneParameters = device.CreateTransientParameterBlock(pblockData, "Scene", sceneDescriptorPool);
        }

        threadResources.LockAll([&vkdevice, &viewPblockLayout](ThreadResources& threadResources) {
            if (viewPblockLayout->HasDescriptors())
            {
                threadResources.viewDescriptorPool.emplace(vkdevice, *viewPblockLayout, ViewDescriptorPoolSize);
            }
        });
    }
}

} // namespace Teide
