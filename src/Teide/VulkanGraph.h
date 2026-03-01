
#pragma once

#include "Teide/Assert.h"
#include "Teide/BasicTypes.h"
#include "Teide/Renderer.h"
#include "Teide/Surface.h"
#include "Teide/Util/ThreadUtils.h"
#include "Teide/Util/TypeHelpers.h"
#include "Teide/VulkanBuffer.h"
#include "Teide/VulkanTexture.h"

#include <stdexec/execution.hpp>
#include <vulkan/vulkan.hpp>

#include <string>
#include <variant>
#include <vector>

namespace Teide
{
class VulkanDevice;
class Queue;

namespace ex = stdexec;

enum class ResourceType : uint8
{
    Texture,
    TextureData,
};
std::string to_string(ResourceType type);

enum class CommandType : uint8
{
    Write,
    Read,
    Render,
    Dispatch,
    Present,
};
std::string to_string(CommandType type);

template <class T>
using CompletionSignaturesFor = stdexec::completion_signatures<stdexec::set_value_t(T), stdexec::set_stopped_t()>;

template <class T>
struct CompletionState : MoveOnly
{
    using Sigs = CompletionSignaturesFor<T>;

    struct AwaitingResult
    {};

    struct OnResult
    {
        std::function<void(T)> setValue;
        std::function<void()> setStopped;
    };

    using ResultValue = T;

    struct Canceled
    {};

    using State = std::variant<AwaitingResult, OnResult, ResultValue, Canceled>;

    auto SetReceiver(ex::receiver_of<Sigs> auto& receiver) -> bool
    {
        return state.Lock([&receiver](State& state) {
            TEIDE_ASSERT(!std::holds_alternative<OnResult>(state));

            if (std::holds_alternative<Canceled>(state))
            {
                receiver.set_stopped();
                return true;
            }

            if (auto* result = std::get_if<ResultValue>(&state))
            {
                receiver.set_value(std::move(*result));
                return true;
            }

            if (std::holds_alternative<AwaitingResult>(state))
            {
                state = OnResult{
                    .setValue = [&receiver](T value) { receiver.set_value(std::move(value)); },
                    .setStopped = [&receiver] { receiver.set_stopped(); },
                };
            }

            return false;
        });
    }

    void SetValue(T value)
    {
        state.Lock([&value](State& state) {
            TEIDE_ASSERT(!std::holds_alternative<ResultValue>(state));

            if (std::holds_alternative<Canceled>(state))
            {
                // nothing to do
                return;
            }

            if (auto* onResult = std::get_if<OnResult>(&state))
            {
                onResult->setValue(std::move(value));
            }

            if (std::holds_alternative<AwaitingResult>(state))
            {
                state = ResultValue{std::move(value)};
            }
        });
    }

private:
    Synchronized<State> state;
};

template <class T, class Receiver>
class ReadOperation : Immovable
{
public:
    ReadOperation(std::shared_ptr<CompletionState<T>> completion, Receiver&& receiver) :
        m_completion{std::move(completion)}, m_receiver{std::move(receiver)}
    {}

    void start() noexcept
    {
        if (m_completion->SetReceiver(m_receiver))
        {
            m_completion.reset();
        }
    }

private:
    std::shared_ptr<CompletionState<T>> m_completion;
    Receiver m_receiver;
};

template <class T>
class ReadSender
{
public:
    explicit ReadSender(std::shared_ptr<CompletionState<T>> completion) : m_completion{std::move(completion)} {}

    using sender_concept = ex::sender_t;

    using completion_signatures = CompletionSignaturesFor<T>;

    template <ex::receiver R>
    friend auto tag_invoke(ex::connect_t /*tag*/, const ReadSender& sender, R&& receiver) -> ReadOperation<T, R>
    {
        return ReadOperation(std::move(sender.m_completion), std::forward<R>(receiver));
    }

private:
    std::shared_ptr<CompletionState<T>> m_completion;
};

struct VulkanGraph
{
    struct ResourceNodeRef
    {
        ResourceType type;
        usize index;

        bool operator==(const ResourceNodeRef&) const = default;
    };

    struct CommandNodeRef
    {
        CommandType type;
        usize index;

        bool operator==(const CommandNodeRef&) const = default;
    };

    static auto WriteRef(usize i) -> CommandNodeRef { return {.type = CommandType::Write, .index = i}; }
    static auto ReadRef(usize i) -> CommandNodeRef { return {.type = CommandType::Read, .index = i}; }
    static auto RenderRef(usize i) -> CommandNodeRef { return {.type = CommandType::Render, .index = i}; }
    static auto DispatchRef(usize i) -> CommandNodeRef { return {.type = CommandType::Dispatch, .index = i}; }
    static auto PresentRef(usize i) -> CommandNodeRef { return {.type = CommandType::Present, .index = i}; }
    static auto TextureRef(usize i) -> ResourceNodeRef { return {.type = ResourceType::Texture, .index = i}; }
    static auto TextureDataRef(usize i) -> ResourceNodeRef { return {.type = ResourceType::TextureData, .index = i}; }

    struct CommandNode
    {
        uint32 index;
    };

    struct WriteNode : CommandNode
    {
        static constexpr auto NodeType = CommandType::Write;

        std::string name;
        ResourceNodeRef source;
        ResourceNodeRef target;
        VulkanBufferData stagingBuffer;

        void Process(VulkanGraph& graph, VulkanDevice& device, vk::CommandBuffer cmdBuffer);
    };

    struct ReadNode : CommandNode
    {
        static constexpr auto NodeType = CommandType::Read;

        std::string name;
        ResourceNodeRef source;
        VulkanBufferData stagingBuffer;
        std::weak_ptr<CompletionState<TextureData>> completion;

        void Process(VulkanGraph& graph, VulkanDevice& device, vk::CommandBuffer cmdBuffer);
    };

    struct RenderNode : CommandNode
    {
        static constexpr auto NodeType = CommandType::Render;

        RenderList renderList;
        RenderTargetInfo renderTargetInfo;
        std::optional<ResourceNodeRef> colorTarget;
        std::optional<ResourceNodeRef> depthStencilTarget;
        std::vector<ResourceNodeRef> dependencies;

        void Process(VulkanGraph& graph, VulkanDevice& device, vk::CommandBuffer cmdBuffer);
    };

    struct DispatchNode : CommandNode
    {
        std::string name;
        Kernel kernel;
        std::vector<ResourceNodeRef> dependencies;
        std::vector<ResourceNodeRef> outputs;

        void Process(VulkanGraph& graph, VulkanDevice& device, vk::CommandBuffer cmdBuffer);
    };

    struct PresentNode : CommandNode
    {
        static constexpr auto NodeType = CommandType::Present;

        std::string name;
        ResourceNodeRef source;

        void Process(VulkanGraph& graph, VulkanDevice& device, vk::CommandBuffer cmdBuffer);
    };

    struct TextureNode
    {
        static constexpr auto NodeType = ResourceType::Texture;

        Texture texture;
        CommandNodeRef source;
        vk::ImageUsageFlags usage;

        struct StateChange
        {
            uint32 nodeIndex;
            TextureState expectedState;
            TextureState ensuredState;
        };

        std::vector<StateChange> stateChanges;

        void AddStateChange(uint32 nodeIndex, TextureState expectedState, TextureState ensuredState);
        auto GetStateTransition(uint32 nodeIndex) const -> std::optional<TextureStateTransition>;

        std::string_view GetName() const { return texture.GetName(); }
    };

    struct TextureDataNode
    {
        static constexpr auto NodeType = ResourceType::TextureData;

        std::string name;
        TextureData data;
        std::optional<CommandNodeRef> source;

        std::string_view GetName() const { return name; }
    };

    std::vector<WriteNode> writeNodes;
    std::vector<ReadNode> readNodes;
    std::vector<RenderNode> renderNodes;
    std::vector<DispatchNode> dispatchNodes;
    std::vector<PresentNode> presentNodes;
    uint32 commandNodeCount = 0;

    std::vector<TextureNode> textureNodes;
    std::vector<TextureDataNode> textureDataNodes;

    static constexpr auto NodeLists = std::tuple{
        &VulkanGraph::writeNodes,   &VulkanGraph::readNodes,        &VulkanGraph::renderNodes,
        &VulkanGraph::textureNodes, &VulkanGraph::textureDataNodes,
    };

    auto NewCommandNode() { return CommandNode{.index = commandNodeCount++}; }

    auto AddWriteNode(ResourceNodeRef source, ResourceNodeRef target)
    {
        auto name = fmt::format("write{}", writeNodes.size() + 1);
        AddUsage(target, vk::ImageUsageFlagBits::eTransferDst);
        writeNodes.emplace_back(NewCommandNode(), std::move(name), source, target);
        const auto r = WriteRef(writeNodes.size() - 1);
        SetSource(target, r);
        return r;
    }

    auto AddReadNode(ResourceNodeRef source)
    {
        auto name = fmt::format("read{}", readNodes.size() + 1);
        AddUsage(source, vk::ImageUsageFlagBits::eTransferSrc);
        auto& node = readNodes.emplace_back(NewCommandNode(), std::move(name), source);

        auto completion = std::make_shared<CompletionState<TextureData>>();
        node.completion = completion;
        return ReadSender<TextureData>(std::move(completion));
    }

    auto AddDispatchNode(Kernel kernel, std::vector<ResourceNodeRef> inputs, std::vector<ResourceNodeRef> outputs)
    {
        auto name = fmt::format("dispatch{}", dispatchNodes.size() + 1);
        const auto& node = dispatchNodes.emplace_back(
            NewCommandNode(), std::move(name), std::move(kernel), std::move(inputs), std::move(outputs));
        const auto r = DispatchRef(dispatchNodes.size() - 1);

        for (const auto input : node.dependencies)
        {
            AddUsage(input, vk::ImageUsageFlagBits::eSampled);
        }
        for (const auto output : node.outputs)
        {
            AddUsage(output, vk::ImageUsageFlagBits::eStorage);
            SetSource(output, r);
        }
        return r;
    }

    auto AddPresentNode(ResourceNodeRef source)
    {
        auto name = fmt::format("present{}", presentNodes.size() + 1);
        presentNodes.emplace_back(NewCommandNode(), std::move(name), source);
        return PresentRef(presentNodes.size() - 1);
    }

    auto AddRenderNode(RenderList renderList, std::optional<ResourceNodeRef> colorTarget, std::optional<ResourceNodeRef> depthStencilTarget)
    {
        auto renderTargetInfo = RenderTargetInfo{};
        const auto r = RenderRef(renderNodes.size());

        if (colorTarget)
        {
            AddUsage(*colorTarget, vk::ImageUsageFlagBits::eColorAttachment);
            SetSource(*colorTarget, r);
            const auto& tex = Get<TextureNode>(colorTarget->index);
            renderTargetInfo.size = tex.texture.GetSize();
            renderTargetInfo.framebufferLayout.colorFormat = tex.texture.GetFormat();
        }

        if (depthStencilTarget)
        {
            AddUsage(*depthStencilTarget, vk::ImageUsageFlagBits::eDepthStencilAttachment);
            SetSource(*depthStencilTarget, r);
            const auto& tex = Get<TextureNode>(depthStencilTarget->index);
            renderTargetInfo.size = tex.texture.GetSize();
            renderTargetInfo.framebufferLayout.depthStencilFormat = tex.texture.GetFormat();
        }

        renderNodes.emplace_back(NewCommandNode(), std::move(renderList), renderTargetInfo, colorTarget, depthStencilTarget);
        return r;
    }

    auto AddTextureNode(Texture texture)
    {
        textureNodes.emplace_back(std::move(texture));
        return TextureRef(textureNodes.size() - 1);
    }

    auto AddTextureDataNode(std::string name, TextureData texture)
    {
        textureDataNodes.emplace_back(std::move(name), std::move(texture));
        return TextureDataRef(textureDataNodes.size() - 1);
    }

    void AddStateChange(ResourceNodeRef ref, uint32 nodeIndex, TextureState expectedState, TextureState ensuredState);

    std::string_view GetNodeName(ResourceNodeRef ref);

    void ForEachPresentNode(auto f)
    {
        for (auto i = VulkanGraph::PresentRef(0); i.index < presentNodes.size(); i.index++)
        {
            const auto& node = presentNodes[i.index];
            f(i, fmt::format("present{}", i.index + 1), std::span(&node.source, 1));
        }
    }

    void ForEachCommandNode(auto f)
    {
        for (auto& node : writeNodes)
        {
            f(node);
        }
        for (auto& node : readNodes)
        {
            f(node);
        }
        for (auto& node : renderNodes)
        {
            f(node);
        }
        for (auto& node : dispatchNodes)
        {
            f(node);
        }
        for (auto& node : presentNodes)
        {
            f(node);
        }
    }

    template <class T>
    T& Get(usize index)
    {
        using List = std::vector<T>;
        using MemPtrType = List VulkanGraph::*;
        constexpr auto MemPtr = std::get<MemPtrType>(NodeLists);
        auto& list = this->*MemPtr;
        TEIDE_ASSERT(index < list.size());
        return list[index];
    }

    template <class T>
    T& Get(ResourceNodeRef ref)
    {
        TEIDE_ASSERT(ref.type == T::NodeType);
        return Get<T>(ref.index);
    }

    template <class T>
    T& Get(CommandNodeRef ref)
    {
        TEIDE_ASSERT(ref.type == T::NodeType);
        return Get<T>(ref.index);
    }

    template <class T>
    T* GetIf(CommandNodeRef ref)
    {
        return (ref.type == T::NodeType) ? &Get<T>(ref.index) : nullptr;
    }

    template <class T>
    T* GetIf(ResourceNodeRef ref)
    {
        return (ref.type == T::NodeType) ? &Get<T>(ref.index) : nullptr;
    }

private:
    void AddUsage(ResourceNodeRef ref, vk::ImageUsageFlags usage)
    {
        if (auto* texture = GetIf<TextureNode>(ref))
        {
            texture->usage |= usage;
        }
    }

    void SetSource(ResourceNodeRef ref, CommandNodeRef source)
    {
        switch (ref.type)
        {
            case ResourceType::Texture: Get<TextureNode>(ref.index).source = source; break;
            case ResourceType::TextureData: Get<TextureDataNode>(ref.index).source = source; break;
        }
    }
};

void BuildGraph(VulkanGraph& graph, VulkanDevice& device);
std::string VisualizeGraph(VulkanGraph& graph);
void ExecuteGraph(VulkanGraph& graph, VulkanDevice& device, Queue& queue);

} // namespace Teide
