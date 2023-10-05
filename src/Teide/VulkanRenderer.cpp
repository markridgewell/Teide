
#include "VulkanRenderer.h"

#include "Vulkan.h"
#include "VulkanBuffer.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanMesh.h"
#include "VulkanParameterBlock.h"
#include "VulkanPipeline.h"
#include "VulkanShader.h"
#include "VulkanShaderEnvironment.h"
#include "VulkanTexture.h"
#include "VulkanUtils.h"

#include "Teide/Renderer.h"
#include "Teide/TextureData.h"

#include <fmt/chrono.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <ranges>

namespace Teide
{

namespace
{
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
                clearValues.push_back(vk::ClearColorValue{*clearState.colorValue});
            }
            else
            {
                clearValues.emplace_back();
            }
        }
        if (framebuffer.layout.depthStencilFormat.has_value())
        {
            clearValues.emplace_back(vk::ClearDepthStencilValue{
                .depth = clearState.depthValue.value_or(1.0f),
                .stencil = clearState.stencilValue.value_or(0),
            });
        };
        return clearValues;
    }

    constexpr auto NotNull = [](const auto& handle) { return static_cast<bool>(handle); };

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

VulkanRenderer::VulkanRenderer(VulkanGraphicsDevice& device, const QueueFamilies& queueFamilies, ShaderEnvironmentPtr shaderEnvironment) :
    m_device{device},
    m_graphicsQueue{device.GetVulkanDevice().getQueue(queueFamilies.graphicsFamily, 0)},
    m_shaderEnvironment{std::move(shaderEnvironment)}
{
    const auto vkdevice = device.GetVulkanDevice();

    if (queueFamilies.presentFamily)
    {
        m_presentQueue = vkdevice.getQueue(*queueFamilies.presentFamily, 0);
    }

    std::ranges::generate(m_renderFinished, [=] { return vkdevice.createSemaphoreUnique({}, s_allocator); });
    std::ranges::generate(m_inFlightFences, [=] {
        return vkdevice.createFenceUnique({.flags = vk::FenceCreateFlagBits::eSignaled}, s_allocator);
    });
}

VulkanRenderer::~VulkanRenderer()
{
    WaitForGpu();

    std::array<vk::Fence, std::tuple_size_v<decltype(m_inFlightFences)>> fences;
    std::ranges::transform(m_inFlightFences, fences.begin(), [](const auto& f) { return f.get(); });

    constexpr auto timeout = std::chrono::seconds{1};
    if (m_device.GetVulkanDevice().waitForFences(fences, true, Timeout(timeout)) == vk::Result::eTimeout)
    {
        spdlog::error("Timeout (>{}) while waiting for all in-flight command buffers to complete!", timeout);
    }
}

uint32 VulkanRenderer::GetFrameNumber() const
{
    return m_frameNumber;
}

void VulkanRenderer::BeginFrame(ShaderParameters sceneParameters)
{
    m_frameNumber = (m_frameNumber + 1) % MaxFramesInFlight;

    constexpr uint64_t timeout = std::numeric_limits<uint64_t>::max();

    [[maybe_unused]] const auto waitResult
        = m_device.GetVulkanDevice().waitForFences(m_inFlightFences[m_frameNumber].get(), true, timeout);
    assert(waitResult == vk::Result::eSuccess); // TODO check if waitForFences can fail with no timeout

    m_device.GetScheduler().NextFrame();

    auto& frameResources = m_frameResources[m_frameNumber];
    const ParameterBlockData pblockData = {
        .layout = m_shaderEnvironment ? m_shaderEnvironment->GetScenePblockLayout() : nullptr,
        .lifetime = ResourceLifetime::Transient,
        .parameters = std::move(sceneParameters),
    };
    frameResources.sceneParameters = m_device.CreateParameterBlock(pblockData, "Scene");
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

    assert(m_presentQueue && "Can't present without a present queue");

    auto fenceToSignal = m_inFlightFences[m_frameNumber].get();

    device.resetFences(fenceToSignal);

    // Submit the surface command buffer(s)
    std::vector<vk::CommandBuffer> commandBuffers
        = m_surfaceCommandBuffers.Lock([](auto& c) { return std::exchange(c, {}); });
    std::ranges::transform(images, std::back_inserter(commandBuffers), &SurfaceImage::prePresentCommandBuffer);

    const auto waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    const vkex::SubmitInfo submitInfo = {
        .waitSemaphores = transform(images, &SurfaceImage::imageAvailable),
        .waitDstStageMask = transform(images, [=](auto&&) { return waitStage; }),
        .commandBuffers = commandBuffers,
        .signalSemaphores = m_renderFinished[m_frameNumber].get(),
    };
    m_graphicsQueue.submit(submitInfo.map(), fenceToSignal);

    // Present
    const vkex::PresentInfoKHR presentInfo = {
        .waitSemaphores = m_renderFinished[m_frameNumber].get(),
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
    assert((renderTarget.captureColor || renderTarget.captureDepthStencil) && "Nothing to capture in RTT pass");

    const auto CreateRenderableTexture = [&](std::optional<Format> format, const char* name) -> TexturePtr {
        if (!format)
        {
            return nullptr;
        }

        const TextureData data = {
            .size = renderTarget.size,
            .format = *format,
            .mipLevelCount = 1,
            .sampleCount = renderTarget.framebufferLayout.sampleCount,
            .samplerState = renderTarget.samplerState,
        };

        const auto textureName = fmt::format("{}:{}", renderList.name, name);
        return m_device.CreateRenderableTexture(data, textureName.c_str());
    };

    const auto& fb = renderTarget.framebufferLayout;
    const auto color = CreateRenderableTexture(fb.colorFormat, "color");
    const auto depthStencil = CreateRenderableTexture(fb.depthStencilFormat, "depthStencil");

    ScheduleGpu([this, renderList = std::move(renderList), renderTarget, color, depthStencil](CommandBuffer& commandBuffer) {
        commandBuffer.AddReference(depthStencil);

        std::vector<vk::ImageView> attachments;

        TextureState colorTextureState;
        TextureState depthStencilTextureState;

        const auto addAttachment = [&](const TexturePtr& texture, TextureState& textureState) {
            const auto& textureImpl = m_device.GetImpl(*texture);
            commandBuffer.AddReference(texture);
            textureImpl.TransitionToRenderTarget(textureState, commandBuffer);
            attachments.push_back(textureImpl.imageView.get());
        };

        if (color)
        {
            addAttachment(color, colorTextureState);
        }
        if (depthStencil)
        {
            addAttachment(depthStencil, depthStencilTextureState);
        }

        const RenderPassDesc renderPassDesc = {
            .framebufferLayout = renderTarget.framebufferLayout,
            .renderOverrides = renderList.renderOverrides,
        };

        const auto renderPass = m_device.CreateRenderPass(renderTarget.framebufferLayout, renderList.clearState);
        const auto framebuffer
            = m_device.CreateFramebuffer(renderPass, renderTarget.framebufferLayout, renderTarget.size, attachments);

        RecordRenderListCommands(commandBuffer, renderList, renderPass, renderPassDesc, framebuffer);

        if (color && renderTarget.captureColor)
        {
            m_device.GetImpl(*color).TransitionToShaderInput(colorTextureState, commandBuffer);
        }
        if (depthStencil && renderTarget.captureDepthStencil)
        {
            m_device.GetImpl(*depthStencil).TransitionToShaderInput(depthStencilTextureState, commandBuffer);
        }
    });

    return {
        .colorTexture = renderTarget.captureColor ? color : nullptr,
        .depthStencilTexture = renderTarget.captureDepthStencil ? depthStencil : nullptr,
    };
}

void VulkanRenderer::RenderToSurface(Surface& surface, RenderList renderList)
{
    auto& surfaceImpl = dynamic_cast<VulkanSurface&>(surface);
    if (const auto surfaceImageOpt = AddSurfaceToPresent(surfaceImpl))
    {
        const auto& surfaceImage = *surfaceImageOpt;

        const auto framebuffer = surfaceImage.framebuffer;

        m_device.GetScheduler().Schedule([=, this, renderList = std::move(renderList)](uint32 taskIndex) {
            CommandBuffer& commandBuffer = m_device.GetScheduler().GetCommandBuffer(taskIndex);

            const RenderPassDesc renderPassDesc = {
                .framebufferLayout = framebuffer.layout,
                .renderOverrides = renderList.renderOverrides,
            };

            const auto renderPass = m_device.CreateRenderPass(framebuffer.layout, renderList.clearState);

            RecordRenderListCommands(commandBuffer, renderList, renderPass, renderPassDesc, framebuffer);

            commandBuffer.Get()->end();

            m_surfaceCommandBuffers.Lock([&commandBuffer](auto& s) { s.push_back(commandBuffer); });
        });
    }
}

Task<TextureData> VulkanRenderer::CopyTextureData(TexturePtr texture)
{
    const auto& textureImpl = m_device.GetImpl(*texture);

    const TextureData textureData = {
        .size = textureImpl.size,
        .format = textureImpl.format,
        .mipLevelCount = textureImpl.mipLevelCount,
        .sampleCount = textureImpl.sampleCount,
    };

    const auto bufferSize = GetByteSize(textureData);

    auto task = ScheduleGpu([this, texture = std::move(texture), bufferSize](CommandBuffer& commandBuffer) {
        auto buffer = m_device.CreateBufferUninitialized(
            bufferSize, vk::BufferUsageFlagBits::eTransferDst,
            vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessRandom);

        const auto& textureImpl = m_device.GetImpl(*texture);

        commandBuffer.AddReference(texture);

        TextureState textureState = {
            .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .lastPipelineStageUsage = vk::PipelineStageFlagBits::eFragmentShader,
        };
        textureImpl.TransitionToTransferSrc(textureState, commandBuffer);
        const auto extent = vk::Extent3D{textureImpl.size.x, textureImpl.size.y, 1};
        CopyImageToBuffer(
            commandBuffer, textureImpl.image.get(), buffer.buffer.get(), textureImpl.format, extent,
            textureImpl.mipLevelCount);
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

void VulkanRenderer::RecordRenderListCommands(
    CommandBuffer& commandBufferWrapper, const RenderList& renderList, vk::RenderPass renderPass,
    const RenderPassDesc& renderPassDesc, const Framebuffer& framebuffer)
{
    using std::data;

    const vk::CommandBuffer commandBuffer = commandBufferWrapper;

    const auto clearValues = MakeClearValues(framebuffer, renderList.clearState);

    const vk::RenderPassBeginInfo renderPassBegin = {
        .renderPass = renderPass,
        .framebuffer = framebuffer.framebuffer,
        .renderArea = {.offset = {0, 0}, .extent = {framebuffer.size.x, framebuffer.size.y}},
        .clearValueCount = size32(clearValues),
        .pClearValues = data(clearValues),
    };

    const auto viewport = MakeViewport(framebuffer.size, renderList.viewportRegion);
    commandBuffer.setViewport(0, viewport);
    const auto scissor = renderList.scissor ? ToVulkan(*renderList.scissor)
                                            : vk::Rect2D{.extent = {framebuffer.size.x, framebuffer.size.y}};
    commandBuffer.setScissor(0, scissor);

    const auto threadIndex = m_device.GetScheduler().GetThreadIndex();
    const ParameterBlockData viewParamsData = {
        .layout = m_shaderEnvironment ? m_shaderEnvironment->GetViewPblockLayout() : nullptr,
        .lifetime = ResourceLifetime::Transient,
        .parameters = renderList.viewParameters,
    };
    const auto viewParamsName = fmt::format("{}:View", renderList.name);
    const auto viewParameters
        = m_device.CreateParameterBlock(viewParamsData, viewParamsName.c_str(), commandBufferWrapper, threadIndex);
    commandBufferWrapper.AddReference(viewParameters);

    commandBuffer.beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);

    if (!renderList.objects.empty())
    {
        const auto descriptorSets = std::array{
            GetDescriptorSet(GetSceneParameterBlock().get()),
            GetDescriptorSet(viewParameters.get()),
        };
        const auto firstActiveSet
            = static_cast<uint32>(std::distance(descriptorSets.begin(), std::ranges::find_if(descriptorSets, NotNull)));
        const auto activeDescriptorSets = std::span{descriptorSets}.subspan(firstActiveSet);

        if (!activeDescriptorSets.empty())
        {
            const auto& firstPipeline = m_device.GetImpl(*renderList.objects.front().pipeline);
            commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics, firstPipeline.layout, firstActiveSet, activeDescriptorSets, {});
        }

        for (const RenderObject& obj : renderList.objects)
        {
            RecordRenderObjectCommands(commandBufferWrapper, obj, renderPassDesc);
        }
    }

    commandBuffer.endRenderPass();
}

void VulkanRenderer::RecordRenderObjectCommands(
    CommandBuffer& commandBufferWrapper, const RenderObject& obj, const RenderPassDesc& renderPassDesc) const
{
    const vk::CommandBuffer commandBuffer = commandBufferWrapper;
    const auto& pipeline = m_device.GetImpl(*obj.pipeline);

    commandBufferWrapper.AddReference(obj.mesh);
    commandBufferWrapper.AddReference(obj.materialParameters);
    commandBufferWrapper.AddReference(obj.pipeline);

    if (obj.materialParameters)
    {
        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, pipeline.layout, 2, GetDescriptorSet(obj.materialParameters.get()), {});
    }

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.GetPipeline(renderPassDesc));

    const auto& meshImpl = m_device.GetImpl(*obj.mesh);
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

        if (const auto result = surface.AcquireNextImage(m_inFlightFences[m_frameNumber].get()))
        {
            return surfacesToPresent.emplace_back(*result);
        }

        return std::nullopt;
    });
}

vk::DescriptorSet VulkanRenderer::GetDescriptorSet(const ParameterBlock* parameterBlock) const
{
    if (parameterBlock == nullptr)
    {
        return {};
    }
    const auto& parameterBlockImpl = m_device.GetImpl(*parameterBlock);
    return parameterBlockImpl.descriptorSet.get();
}

} // namespace Teide
