
#include "RenderTest.h"

using namespace Teide;

TEST_F(RenderTest, BasicLighting)
{
    using namespace Geo::Literals;
    const Geo::Point3 cameraTarget = {0.0f, 0.0f, 0.0f};
    const Geo::Angle cameraYaw = 45.0_deg;
    const Geo::Angle cameraPitch = -45.0_deg;
    const float cameraDistance = 3.0f;

    const Geo::Matrix4 rotation = Geo::Matrix4::RotationZ(cameraYaw) * Geo::Matrix4::RotationX(cameraPitch);
    const Geo::Vector3 cameraDirection = Geo::Normalise(rotation * Geo::Vector3{0.0f, 1.0f, 0.0f});
    const Geo::Vector3 cameraUp = Geo::Normalise(rotation * Geo::Vector3{0.0f, 0.0f, 1.0f});

    const Geo::Point3 cameraPosition = cameraTarget - cameraDirection * cameraDistance;

    const Geo::Matrix view = Geo::LookAt(cameraPosition, cameraTarget, cameraUp);
    const Geo::Matrix proj = Geo::Perspective(45.0_deg, RenderAspectRatio, 0.1f, 10.0f);

    const SceneUniforms sceneUniforms = {
        .lightDir = {0.0f, -1.0f, 0.0f},
        .lightColor = {1.0f, 1.0f, 1.0f},
        .ambientColorTop = {0.08f, 0.1f, 0.1f},
        .ambientColorBottom = {0.003f, 0.003f, 0.002f},
    };
    const ViewUniforms viewUniforms = {.viewProj = proj * view};
    const ObjectUniforms objectUniforms = {};

    const auto quadMesh = CreateQuadMesh();
    const auto checkerTexture = CreateCheckerTexture(Filter::Nearest, false);
    const auto shadowTexture = CreateNullShadowmapTexture();
    const auto shader = CreateModelShader();
    const auto pipeline = CreatePipeline(shader, quadMesh);

    const ParameterBlockData materialData = {
        .layout = shader->GetMaterialPblockLayout(),
        .parameters = {
            .textures = {checkerTexture},
        },
    };
    const auto materialParams = GetDevice().CreateParameterBlock(materialData, "Material");

    RenderList renderList = {
        .clearState = {.colorValue = Color{0.0f, 0.0f, 0.0f, 1.0f}},
        .viewParameters = {
            .uniformData = ToBytes(viewUniforms),
            .textures = {shadowTexture},
        },
        .objects = {{
            .mesh = quadMesh,
            .pipeline = pipeline,
            .materialParameters = materialParams,
            .objectParameters = {.uniformData = Teide::ToBytes(objectUniforms)},
        }},
    };

    RunTest(sceneUniforms, std::move(renderList));
}

TEST_F(RenderTest, MipMapping)
{
    const float planeSize = 20.0f;

    using namespace Geo::Literals;
    const Geo::Point3 cameraTarget = {0.0f, 0.0f, 0.0f};
    const Geo::Angle cameraYaw = 45.0_deg;
    const Geo::Angle cameraPitch = -10.0_deg;
    const float cameraDistance = 3.0f;

    const Geo::Matrix4 rotation = Geo::Matrix4::RotationZ(cameraYaw) * Geo::Matrix4::RotationX(cameraPitch);
    const Geo::Vector3 cameraDirection = Geo::Normalise(rotation * Geo::Vector3{0.0f, 1.0f, 0.0f});
    const Geo::Vector3 cameraUp = Geo::Normalise(rotation * Geo::Vector3{0.0f, 0.0f, 1.0f});

    const Geo::Point3 cameraPosition = cameraTarget - cameraDirection * cameraDistance;

    const Geo::Matrix view = Geo::LookAt(cameraPosition, cameraTarget, cameraUp);
    const Geo::Matrix proj = Geo::Perspective(45.0_deg, RenderAspectRatio, 0.1f, 10.0f);

    const SceneUniforms sceneUniforms = {
        .lightDir = {0.0f, -1.0f, 0.0f},
        .lightColor = {1.0f, 1.0f, 1.0f},
        .ambientColorTop = {0.08f, 0.1f, 0.1f},
        .ambientColorBottom = {0.003f, 0.003f, 0.002f},
    };
    const ViewUniforms viewUniforms = {.viewProj = proj * view};
    const ObjectUniforms objectUniforms = {};

    const auto planeMesh = CreatePlaneMesh({planeSize, planeSize}, {planeSize, planeSize});
    const auto checkerTexture = CreateCheckerTexture(Filter::Linear, true);
    const auto shadowTexture = CreateNullShadowmapTexture();
    const auto shader = CreateModelShader();
    const auto pipeline = CreatePipeline(shader, planeMesh);

    const ParameterBlockData materialData = {
        .layout = shader->GetMaterialPblockLayout(),
        .parameters = {
            .textures = {checkerTexture},
        },
    };
    const auto materialParams = GetDevice().CreateParameterBlock(materialData, "Material");

    RenderList renderList = {
        .clearState = {.colorValue = Color{0.0f, 0.0f, 0.0f, 1.0f}},
        .viewParameters = {
            .uniformData = ToBytes(viewUniforms),
            .textures = {shadowTexture},
        },
        .objects = {{
            .mesh = planeMesh ,
            .pipeline = pipeline,
            .materialParameters = materialParams,
            .objectParameters = {.uniformData = Teide::ToBytes(objectUniforms)},
        }},
    };

    RunTest(sceneUniforms, std::move(renderList));
}
