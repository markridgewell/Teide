
#include "Teide/VulkanGraph.h"

#include "TestData.h"
#include "TestUtils.h"

#include "Teide/Device.h"

#include <gmock/gmock.h>

#include <span>

using namespace testing;
using namespace Teide;

using RenderNode = VulkanGraph::RenderNode;
using TextureNode = VulkanGraph::TextureNode;

namespace
{
class VulkanGraphTest : public testing::Test
{
public:
    VulkanGraphTest() : m_device{CreateTestDevice()} {}

protected:
    Texture CreateDummyTexture(const char* name) { return m_device->CreateTexture(OnePixelWhiteTexture, name); }
    ShaderData CompileShader(const ShaderSourceData& data) { return m_shaderCompiler.Compile(data); }

    VulkanGraph CreateThreeRenderPasses()
    {
        Texture tex1 = CreateDummyTexture("tex1");
        Texture tex2 = CreateDummyTexture("tex2");
        Texture tex3 = CreateDummyTexture("tex3");

        RenderList renderList1 = {.name = "render1"};
        RenderList renderList2 = {.name = "render2"};
        RenderObject& obj1 = renderList2.objects.emplace_back();
        obj1.objectParameters.textures.push_back(tex1);
        RenderList renderList3 = {.name = "render3"};
        RenderObject& obj2 = renderList3.objects.emplace_back();
        const ParameterBlockDesc pblockDesc = {.parameters = {{"param", ShaderVariableType::BaseType::Texture2D}}};
        const auto pblockLayout = m_device->CreateParameterBlockLayout(pblockDesc, 2);
        ParameterBlockData pblock = {.layout = pblockLayout, .parameters = {.textures = {tex1}}};
        obj2.materialParameters = m_device->CreateParameterBlock(pblock, "matParams");

        VulkanGraph graph;
        auto render1 = graph.AddRenderNode(renderList1);
        auto render2 = graph.AddRenderNode(renderList2);
        auto render3 = graph.AddRenderNode(renderList3);
        graph.AddTextureNode(tex1, render1);
        graph.AddTextureNode(tex2, render2);
        graph.AddTextureNode(tex3, render3);
        return graph;
    }

    VulkanDevicePtr m_device; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    ShaderCompiler m_shaderCompiler;
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
    VulkanGraph graph;
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
    auto tex = graph.AddTextureDataNode("texData1", OnePixelWhiteTexture);
    auto copy = graph.AddCopyNode(tex);
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
    Texture tex1 = CreateDummyTexture("tex1");
    Texture tex2 = CreateDummyTexture("tex2");

    RenderList render1 = {.name = "render1"};
    RenderList render2 = {.name = "render2"};
    render2.viewParameters.textures.push_back(tex1);

    VulkanGraph graph;
    auto renderNode1 = graph.AddRenderNode(render1);
    auto renderNode2 = graph.AddRenderNode(render2);
    auto texNode1 = graph.AddTextureNode(tex1, VulkanGraph::RenderRef(0));
    auto texNode2 = graph.AddTextureNode(tex2, VulkanGraph::RenderRef(1));

    (void)texNode2;

    BuildGraph(graph, *m_device);

    ASSERT_THAT(graph.renderNodes, ElementsAre(HasNoDependencies(), HasDependency(texNode1)));
    ASSERT_THAT(graph.textureNodes, ElementsAre(HasSource(renderNode1), HasSource(renderNode2)));
}

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

TEST_F(VulkanGraphTest, VizualingGraphWithThreeDependentRenderNodes)
{
    auto graph = CreateThreeRenderPasses();
    BuildGraph(graph, *m_device);

    const auto dot = VisualizeGraph(graph);

    ASSERT_THAT(dot, Eq(ThreeRenderPassesDot)) << dot;
}

constexpr auto CopyCpuToGpuDot = R"--(strict digraph {
    rankdir=LR
    node [shape=box]
    tex
    texData
    node [shape=box, margin=0.5]
    copy1 -> tex
    texData -> copy1
})--";

TEST_F(VulkanGraphTest, VizualingGraphWithCopyToGpu)
{
    VulkanGraph graph;
    auto tex = graph.AddTextureDataNode("texData", OnePixelWhiteTexture);
    auto copy = graph.AddCopyNode(tex);
    graph.AddTextureNode(CreateDummyTexture("tex"), copy);

    const auto dot = VisualizeGraph(graph);

    ASSERT_THAT(dot, Eq(CopyCpuToGpuDot)) << dot;
}
} // namespace
