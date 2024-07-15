
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

    VulkanDevicePtr m_device; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    ShaderCompiler m_shaderCompiler;
};

void PrintDependencies(testing::MatchResultListener& os, std::span<const VulkanGraph::ResourceNodeRef> dependencies)
{
    os << '(';
    for (const auto& elem : dependencies)
    {
        os << ' ' << elem;
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
    graph.renderNodes.push_back({});
    graph.textureNodes.emplace_back(CreateDummyTexture("tex1"), 0);

    BuildGraph(graph, *m_device);

    ASSERT_THAT(graph.renderNodes, ElementsAre(HasNoDependencies()));
    ASSERT_THAT(graph.textureNodes, ElementsAre(HasSource(0u)));
}

TEST_F(VulkanGraphTest, BuildingGraphWithTwoIndependentRenderNodesHasNoEffect)
{
    VulkanGraph graph;
    graph.renderNodes.push_back({});
    graph.renderNodes.push_back({});
    graph.textureNodes.emplace_back(CreateDummyTexture("tex1"), 0);
    graph.textureNodes.emplace_back(CreateDummyTexture("tex2"), 1);

    BuildGraph(graph, *m_device);

    ASSERT_THAT(graph.renderNodes, ElementsAre(HasNoDependencies(), HasNoDependencies()));
    ASSERT_THAT(graph.textureNodes, ElementsAre(HasSource(0u), HasSource(1u)));
}

TEST_F(VulkanGraphTest, BuildingGraphWithTwoDependentRenderNodesAddsConnection)
{
    Texture tex1 = CreateDummyTexture("tex1");
    Texture tex2 = CreateDummyTexture("tex2");

    RenderList render1 = {.name = "render1"};
    RenderList render2 = {.name = "render2"};
    render2.viewParameters.textures.push_back(tex1);

    VulkanGraph graph;
    graph.renderNodes.push_back({.renderList = render1});
    graph.renderNodes.push_back({.renderList = render2});
    graph.textureNodes.emplace_back(tex1, 0);
    graph.textureNodes.emplace_back(tex2, 1);

    BuildGraph(graph, *m_device);

    ASSERT_THAT(graph.renderNodes, ElementsAre(HasNoDependencies(), HasDependency(0u)));
    ASSERT_THAT(graph.textureNodes, ElementsAre(HasSource(0u), HasSource(1u)));
}

TEST_F(VulkanGraphTest, BuildingGraphWithThreeDependentRenderNodesAddsConnections)
{
    Texture tex1 = CreateDummyTexture("tex1");
    Texture tex2 = CreateDummyTexture("tex2");
    Texture tex3 = CreateDummyTexture("tex3");

    RenderList render1 = {.name = "render1"};
    RenderList render2 = {.name = "render2"};
    RenderObject& obj1 = render2.objects.emplace_back();
    obj1.objectParameters.textures.push_back(tex1);
    RenderList render3 = {.name = "render3"};
    RenderObject& obj2 = render3.objects.emplace_back();
    const ParameterBlockDesc pblockDesc = {.parameters = {{"param", ShaderVariableType::BaseType::Texture2D}}};
    const auto pblockLayout = m_device->CreateParameterBlockLayout(pblockDesc, 2);
    ParameterBlockData pblock = {.layout = pblockLayout, .parameters = {.textures = {tex2}}};
    obj2.materialParameters = m_device->CreateParameterBlock(pblock, "matParams");

    VulkanGraph graph;
    graph.renderNodes.push_back({.renderList = render1});
    graph.renderNodes.push_back({.renderList = render2});
    graph.renderNodes.push_back({.renderList = render3});
    graph.textureNodes.emplace_back(tex1, 0);
    graph.textureNodes.emplace_back(tex2, 1);
    graph.textureNodes.emplace_back(tex3, 2);

    BuildGraph(graph, *m_device);

    ASSERT_THAT(graph.renderNodes, ElementsAre(HasNoDependencies(), HasDependency(0), HasDependency(1)));
    ASSERT_THAT(graph.textureNodes, ElementsAre(HasSource(0), HasSource(1), HasSource(2)));
}
} // namespace
