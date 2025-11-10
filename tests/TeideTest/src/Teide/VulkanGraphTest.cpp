
#include "Teide/VulkanGraph.h"

#include "TestData.h"
#include "TestUtils.h"

#include "Teide/Device.h"
#include "Teide/VulkanRenderer.h"

#include <gmock/gmock.h>

#include <iterator>
#include <span>
#include <string>

using namespace testing;
using namespace Teide;

namespace
{
std::string MakeDotURL(std::string_view dot)
{
    std::string ret = "https://quickchart.io/graphviz?graph=";
    auto out = std::back_inserter(ret);

    for (char c : dot)
    {
        if (std::isalnum(c) || c == '$' || c == '-' || c == '_' || c == '.' || c == '+' || c == '\'' || c == '!' || c == '*' || c == '(' || c == ')')
        {
            *out = c;
            ++out;
        }
        else if (c != '\n' && c != '\r')
        {
            std::format_to(out, "%{:02X}", c);
        }
    }

    return ret;
}

std::string FormatDot(const std::string& dot)
{
    return dot + "\nGraph: " + MakeDotURL(dot);
}

class VulkanGraphTest : public testing::Test
{
public:
    VulkanGraphTest() :
        m_device{CreateTestDevice()}, m_emptyParameters{m_device->CreateParameterBlock({}, "EmptyParams")}
    {}

protected:
    Texture CreateDummyTexture(const char* name)
    {
        return m_device->AllocateTexture({
            .size = {1, 1},
            .format = Format::Byte4Norm,
            .name = name,
        });
    }

    ShaderData CompileShader(const ShaderSourceData& data) { return m_shaderCompiler.Compile(data); }

    VulkanGraph CreateThreeRenderPasses()
    {
        const Texture tex1 = CreateDummyTexture("tex1");
        const Texture tex2 = CreateDummyTexture("tex2");
        const Texture tex3 = CreateDummyTexture("tex3");

        const ParameterBlockDesc pblockDesc = {.parameters = {{"param", ShaderVariableType::BaseType::Texture2D}}};
        const auto pblockLayout = m_device->CreateParameterBlockLayout(pblockDesc, 2);
        const ParameterBlockData pblock = {.layout = pblockLayout, .parameters = {.textures = {tex1}}};
        const auto materialParameters = m_device->CreateParameterBlock(pblock, "matParams");

        const RenderList renderList1 = {.name = "render1"};
        const RenderList renderList2 = {
            .name = "render2",
            .objects = {{
                .materialParameters = m_emptyParameters,
                .objectParameters = {.textures = {tex1}},
            }},
        };
        const RenderList renderList3 = {
            .name = "render3",
            .objects = {{
                .materialParameters = materialParameters,
            }},
        };

        VulkanGraph graph;
        const auto texNode1 = graph.AddTextureNode(tex1);
        const auto texNode2 = graph.AddTextureNode(tex2);
        const auto texNode3 = graph.AddTextureNode(tex3);
        graph.AddRenderNode(renderList1, texNode1, std::nullopt);
        graph.AddRenderNode(renderList2, texNode2, std::nullopt);
        graph.AddRenderNode(renderList3, texNode3, std::nullopt);
        return graph;
    }

    VulkanDevicePtr m_device; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    ShaderCompiler m_shaderCompiler;
    ParameterBlock m_emptyParameters;
};

void PrintDependencies(testing::MatchResultListener& os, std::span<const VulkanGraph::ResourceNodeRef> dependencies)
{
    os << '(';
    for (const auto& elem : dependencies)
    {
        os << ' ' << to_string(elem.type) << ' ' << elem.index;
    }
    os << " )";
}

MATCHER_P(HasSource, source, "")
{
    static_cast<void>(*result_listener);
    return arg.source == source;
}

MATCHER(HasNoDependencies, "")
{
    *result_listener << "where dependencies are: ";
    PrintDependencies(*result_listener, arg.dependencies);
    return arg.dependencies.empty();
}

MATCHER_P(HasDependency, dep, "")
{
    *result_listener << "where dependencies are: ";
    PrintDependencies(*result_listener, arg.dependencies);
    return arg.dependencies.size() == 1u && arg.dependencies.front() == dep;
}

MATCHER_P(HasTextureDependency, dep, "")
{
    *result_listener << "where dependencies are: ";
    PrintDependencies(*result_listener, arg.dependencies);
    return arg.dependencies.size() == 1u && arg.dependencies.front() == VulkanGraph::TextureRef(dep);
}

TEST_F(VulkanGraphTest, DefaultConstructedGraphIsEmpty)
{
    const VulkanGraph graph;
    ASSERT_THAT(graph.renderNodes, IsEmpty());
    ASSERT_THAT(graph.textureNodes, IsEmpty());
}

TEST_F(VulkanGraphTest, BuildingEmptyGraphHasNoEffect)
{
    VulkanGraph graph;
    BuildGraph(graph, *m_device);
    ASSERT_THAT(graph.renderNodes, IsEmpty());
    ASSERT_THAT(graph.textureNodes, IsEmpty());
}

/*
TEST_F(VulkanGraphTest, BuildingGraphWithOneRenderNodeHasNoEffect)
{
    VulkanGraph graph;
    auto renderNode1 = graph.AddRenderNode({});
    graph.AddTextureNode(CreateDummyTexture("tex1"), renderNode1);

    BuildGraph(graph, *m_device);

    ASSERT_THAT(graph.renderNodes, ElementsAre(HasNoDependencies()));
    ASSERT_THAT(graph.textureNodes, ElementsAre(HasSource(renderNode1)));
}

TEST_F(VulkanGraphTest, BuildingGraphWithOneCopyNodeHasNoEffect)
{
    VulkanGraph graph;
    const auto tex = graph.AddTextureDataNode("texData1", OnePixelWhiteTexture);
    const auto copy = graph.AddCopyNode(tex);
    graph.AddTextureNode(CreateDummyTexture("tex1"), copy);

    BuildGraph(graph, *m_device);

    ASSERT_THAT(graph.copyNodes, ElementsAre(HasSource(tex)));
    ASSERT_THAT(graph.textureNodes, ElementsAre(HasSource(copy)));
}

TEST_F(VulkanGraphTest, BuildingGraphWithTwoIndependentRenderNodesHasNoEffect)
{
    VulkanGraph graph;
    auto render1 = graph.AddRenderNode({});
    auto render2 = graph.AddRenderNode({});
    graph.AddTextureNode(CreateDummyTexture("tex1"), render1);
    graph.AddTextureNode(CreateDummyTexture("tex2"), render2);

    BuildGraph(graph, *m_device);

    ASSERT_THAT(graph.renderNodes, ElementsAre(HasNoDependencies(), HasNoDependencies()));
    ASSERT_THAT(graph.textureNodes, ElementsAre(HasSource(render1), HasSource(render2)));
}

TEST_F(VulkanGraphTest, BuildingGraphWithTwoDependentRenderNodesAddsConnection)
{
    const Texture tex1 = CreateDummyTexture("tex1");
    const Texture tex2 = CreateDummyTexture("tex2");

    const RenderList render1 = {.name = "render1"};
    RenderList render2 = {.name = "render2"};
    render2.viewParameters.textures.push_back(tex1);

    VulkanGraph graph;
    const auto renderNode1 = graph.AddRenderNode(render1);
    const auto renderNode2 = graph.AddRenderNode(render2);
    const auto texNode1 = graph.AddTextureNode(tex1, VulkanGraph::RenderRef(0));
    const auto texNode2 = graph.AddTextureNode(tex2, VulkanGraph::RenderRef(1));

    (void)texNode2;

    BuildGraph(graph, *m_device);

    ASSERT_THAT(graph.renderNodes, ElementsAre(HasNoDependencies(), HasDependency(texNode1)));
    ASSERT_THAT(graph.textureNodes, ElementsAre(HasSource(renderNode1), HasSource(renderNode2)));
}
*/

TEST_F(VulkanGraphTest, BuildingGraphWithThreeDependentRenderNodesAddsConnections)
{
    auto graph = CreateThreeRenderPasses();
    BuildGraph(graph, *m_device);

    ASSERT_THAT(graph.renderNodes, ElementsAre(HasNoDependencies(), HasTextureDependency(0u), HasTextureDependency(0u)));
    ASSERT_THAT(
        graph.textureNodes,
        ElementsAre(
            HasSource(VulkanGraph::RenderRef(0u)), HasSource(VulkanGraph::RenderRef(1u)),
            HasSource(VulkanGraph::RenderRef(2u))));
}

constexpr auto ThreeRenderPassesDot = R"--(strict digraph {
    rankdir=LR
    node [shape=box]
    tex1
    tex2
    tex3
    node [shape=box, margin=0.5]
    render1 -> tex1
    render2 -> tex2
    tex1 -> render2
    render3 -> tex3
    tex1 -> render3
})--";

TEST_F(VulkanGraphTest, VisualizingGraphWithThreeDependentRenderNodes)
{
    auto graph = CreateThreeRenderPasses();
    BuildGraph(graph, *m_device);

    const auto dot = VisualizeGraph(graph);

    ASSERT_THAT(dot, Eq(ThreeRenderPassesDot)) << FormatDot(dot);
}

/*
constexpr auto CopyCpuToGpuDot = R"--(strict digraph {
    rankdir=LR
    node [shape=box]
    tex
    texData
    node [shape=box, margin=0.5]
    copy1 -> tex
    texData -> copy1
})--";

TEST_F(VulkanGraphTest, VisualizingGraphWithCopyToGpu)
{
    VulkanGraph graph;
    const auto tex = graph.AddTextureDataNode("texData", OnePixelWhiteTexture);
    const auto copy = graph.AddCopyNode(tex);
    graph.AddTextureNode(CreateDummyTexture("tex"), copy);

    const auto dot = VisualizeGraph(graph);

    ASSERT_THAT(dot, Eq(CopyCpuToGpuDot)) << FormatDot(dot);
}

constexpr auto CopyGpuToCpuDot = R"--(strict digraph {
    rankdir=LR
    node [shape=box]
    tex
    texData
    node [shape=box, margin=0.5]
    render1 -> tex
    copy1 -> texData
    tex -> copy1
})--";

TEST_F(VulkanGraphTest, VisualizingGraphWithCopyToCpu)
{
    VulkanGraph graph;
    const auto render = graph.AddRenderNode({.name = "render1"});

    const auto tex = graph.AddTextureNode(CreateDummyTexture("tex"), render);
    const auto copy = graph.AddCopyNode(tex);
    graph.AddTextureDataNode("texData", OnePixelWhiteTexture, copy);

    const auto dot = VisualizeGraph(graph);

    ASSERT_THAT(dot, Eq(CopyGpuToCpuDot)) << FormatDot(dot) << "\nGraph: " << MakeDotURL(dot);
}
*/

TEST_F(VulkanGraphTest, ExecutingGraphWithCopyNode)
{
    VulkanGraph graph;
    const auto texDataInput = graph.AddTextureDataNode("input", OnePixelWhiteTexture);
    const auto tex = graph.AddTextureNode(CreateDummyTexture("tex"));
    const auto texDataOutput = graph.AddTextureDataNode("output", {});
    graph.AddCopyNode(texDataInput, tex);
    graph.AddCopyNode(tex, texDataOutput);

    auto renderer = m_device->CreateRenderer({});

    spdlog::info(VisualizeGraph(graph));
    auto queue = Queue(m_device->GetVulkanDevice(), m_device->GetGraphicsQueue());
    ExecuteGraph(graph, *m_device, queue);

    const VulkanTexture& texture = m_device->GetImpl(graph.textureNodes.at(tex.index).texture);
    EXPECT_THAT(texture.image, IsValidVkHandle());
    EXPECT_THAT(texture.imageView, IsValidVkHandle());
    const auto tdi = graph.Get<VulkanGraph::TextureDataNode>(texDataInput.index).data;
    const auto tdo = graph.Get<VulkanGraph::TextureDataNode>(texDataOutput.index).data;
    EXPECT_THAT(tdo, Eq(tdi));
}

} // namespace
