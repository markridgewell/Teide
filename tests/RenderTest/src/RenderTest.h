
#pragma once

#include "RenderDocHooks.h"

#include "GeoLib/Matrix.h"
#include "GeoLib/Vector.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/Buffer.h"
#include "Teide/GraphicsDevice.h"
#include "Teide/Mesh.h"
#include "Teide/Renderer.h"

#include <gtest/gtest.h>

#include <filesystem>

static constexpr Geo::Size2i RenderSize = {1024, 1024};

struct Vertex
{
    Geo::Point3 position;
    Geo::Vector2 texCoord;
    Geo::Vector3 normal;
    Geo::Vector3 color;
};

struct SceneUniforms
{
    alignas(16) Geo::Vector3 lightDir;
    alignas(16) Geo::Vector3 lightColor;
    alignas(16) Geo::Vector3 ambientColorTop;
    alignas(16) Geo::Vector3 ambientColorBottom;
    alignas(16) Geo::Matrix4 shadowMatrix;
};

struct ViewUniforms
{
    Geo::Matrix4 viewProj;
};

struct ObjectUniforms
{
    Geo::Matrix4 model;
};

class RenderTest : public testing::Test
{
public:
    static void SetUpdateReferences(bool set);
    static void SetReferenceDir(const std::filesystem::path& dir);
    static void SetOutputDir(const std::filesystem::path& dir);

protected:
    RenderTest();
    explicit RenderTest(const Teide::ShaderEnvironmentData& shaderEnv);

    void RunTest(const SceneUniforms& sceneUniforms, Teide::RenderList renderList);

    Teide::ShaderPtr CreateModelShader();
    Teide::MeshPtr CreateQuadMesh();
    Teide::TexturePtr CreateNullShadowmapTexture();
    Teide::TexturePtr CreateCheckerTexture();
    Teide::PipelinePtr CreatePipeline(Teide::ShaderPtr shader, const Teide::MeshPtr& mesh);

    Teide::GraphicsDevice& GetDevice();

private:
    void CompareImageToReference(const Teide::TextureData& image, const testing::TestInfo& testInfo);

    static bool s_updateReferences;
    static std::filesystem::path s_referenceDir;
    static std::filesystem::path s_outputDir;

    RenderDoc m_renderDoc;
    Teide::GraphicsDevicePtr m_device;
    Teide::ShaderEnvironmentPtr m_shaderEnv;
    Teide::RendererPtr m_renderer;
};
