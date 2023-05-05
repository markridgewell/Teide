
#include "Application.h"

#include "Loaders.h"
#include "Resources.h"

#include "GeoLib/Matrix.h"
#include "GeoLib/Vector.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/Buffer.h"
#include "Teide/Definitions.h"
#include "Teide/GraphicsDevice.h"
#include "Teide/Mesh.h"
#include "Teide/Renderer.h"
#include "Teide/Shader.h"
#include "Teide/Surface.h"
#include "Teide/Texture.h"
#include "Teide/TextureData.h"

#include <SDL.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <optional>
#include <ranges>
#include <span>

namespace
{
constexpr bool UseMSAA = true;

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

constexpr Teide::FramebufferLayout ShadowFramebufferLayout = {
    .depthStencilFormat = Teide::Format::Depth16,
    .sampleCount = 1,
};

constexpr Teide::RenderOverrides ShadowRenderOverrides = {
    // Depth bias (and slope) are used to avoid shadowing artifacts
    // Constant depth bias factor (always applied)
    .depthBiasConstant = 1.25f,
    // Slope depth bias factor, applied depending on polygon's slope
    .depthBiasSlope = 2.75f,
};

Teide::RenderStates MakeRenderStates(float depthBiasConstant = 0.0f, float depthBiasSlope = 0.0f)
{
    return {
        .blendState = Teide::BlendState{
            .blendFunc = { Teide::BlendFactor::One, Teide::BlendFactor::Zero, Teide::BlendOp::Add },
        },
        .depthState = {
            .depthTest = true,
            .depthWrite = true,
            .depthFunc = Teide::CompareOp::Less,
        },
        .rasterState = {
            .fillMode = Teide::FillMode::Solid,
            .cullMode = Teide::CullMode::None,
            .depthBiasConstant = depthBiasConstant,
            .depthBiasSlope = depthBiasSlope,
        },
    };
}

} // namespace

Application::Application(SDL_Window* window, const char* imageFilename, const char* modelFilename) :
    Application(window, Teide::CreateDeviceAndSurface(window, UseMSAA), imageFilename, modelFilename)
{}

Application::Application(
    SDL_Window* window, Teide::DeviceAndSurface deviceAndSurface, const char* imageFilename, const char* modelFilename) :
    m_window{window},
    m_device{std::move(deviceAndSurface.device)},
    m_surface{std::move(deviceAndSurface.surface)},
    m_shaderEnvironment{m_device->CreateShaderEnvironment(ShaderEnv, "ShaderEnv")},
    m_renderer{m_device->CreateRenderer(m_shaderEnvironment)}
{
    CreateMesh(modelFilename);
    CreateMaterial(imageFilename);
    CreatePipelines();

    spdlog::info("Vulkan initialised successfully");
}

void Application::OnRender()
{
    const Geo::Matrix4 lightRotation = Geo::Matrix4::RotationZ(m_lightYaw) * Geo::Matrix4::RotationX(m_lightPitch);
    const Geo::Vector3 lightDirection = Geo::Normalise(lightRotation * Geo::Vector3{0.0f, 1.0f, 0.0f});
    const Geo::Vector3 lightUp = Geo::Normalise(lightRotation * Geo::Vector3{0.0f, 0.0f, 1.0f});
    const Geo::Point3 shadowCameraPosition = Geo::Point3{} - lightDirection;

    const auto modelMatrix = Geo::Matrix4::RotationX(180.0_deg);

    const auto shadowCamView = Geo::LookAt(shadowCameraPosition, Geo::Point3{}, lightUp);
    const auto shadowCamProj = Geo::Orthographic(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);

    const auto shadowMVP = shadowCamProj * shadowCamView * modelMatrix;

    const auto bbox = m_mesh->GetBoundingBox();
    const std::array<Geo::Point3, 8> bboxCorners = {
        shadowMVP * Geo::Point3(bbox.min.x, bbox.min.y, bbox.min.z),
        shadowMVP * Geo::Point3(bbox.min.x, bbox.min.y, bbox.max.z),
        shadowMVP * Geo::Point3(bbox.min.x, bbox.max.y, bbox.min.z),
        shadowMVP * Geo::Point3(bbox.min.x, bbox.max.y, bbox.max.z),
        shadowMVP * Geo::Point3(bbox.max.x, bbox.min.y, bbox.min.z),
        shadowMVP * Geo::Point3(bbox.max.x, bbox.min.y, bbox.max.z),
        shadowMVP * Geo::Point3(bbox.max.x, bbox.max.y, bbox.min.z),
        shadowMVP * Geo::Point3(bbox.max.x, bbox.max.y, bbox.max.z),
    };

    const auto [minX, maxX] = std::ranges::minmax(std::views::transform(bboxCorners, &Geo::Point3::x));
    const auto [minY, maxY] = std::ranges::minmax(std::views::transform(bboxCorners, &Geo::Point3::y));
    const auto [minZ, maxZ] = std::ranges::minmax(std::views::transform(bboxCorners, &Geo::Point3::z));

    const auto shadowCamProjFitted = Geo::Orthographic(minX, maxX, maxY, minY, minZ, maxZ);

    m_shadowMatrix = shadowCamProjFitted * shadowCamView;

    // Update scene uniforms
    const SceneUniforms sceneUniforms = {
        .lightDir = Geo::Normalise(lightDirection),
        .lightColor = {1.0f, 1.0f, 1.0f},
        .ambientColorTop = {0.08f, 0.1f, 0.1f},
        .ambientColorBottom = {0.003f, 0.003f, 0.002f},
        .shadowMatrix = m_shadowMatrix,
    };

    m_renderer->BeginFrame(Teide::ShaderParameters{
        .uniformData = Teide::ToBytes(sceneUniforms),
    });

    // Update object uniforms
    const ObjectUniforms objectUniforms = {
        .model = modelMatrix,
    };

    //
    // Pass 0. Draw shadows
    //
    Teide::TexturePtr shadowMap;
    {
        // Update view uniforms
        const ViewUniforms viewUniforms = {
            .viewProj = m_shadowMatrix,
        };
        const Teide::ShaderParameters viewParams = {
            .uniformData = Teide::ToBytes(viewUniforms),
            .textures = {},
        };

        Teide::RenderList renderList = {
            .name = "ShadowPass",
            .clearState = {.depthValue = 1.0f},
            .viewParameters = viewParams,
            .renderOverrides = ShadowRenderOverrides,
            .objects = {{
                .mesh = m_mesh,
                .pipeline = m_pipeline,
                .materialParameters = m_material.params,
                .objectParameters = {.uniformData = Teide::ToBytes(objectUniforms)},
            }},
        };

        constexpr uint32_t shadowMapSize = 2048;

        const Teide::RenderTargetInfo shadowRenderTarget = {
            .size = {shadowMapSize, shadowMapSize},
            .framebufferLayout = ShadowFramebufferLayout,
            .samplerState = {
                .magFilter = Teide::Filter::Linear,
                .minFilter = Teide::Filter::Linear,
                .mipmapMode = Teide::MipmapMode::Nearest,
                .addressModeU = Teide::SamplerAddressMode::Repeat,
                .addressModeV = Teide::SamplerAddressMode::Repeat,
                .addressModeW = Teide::SamplerAddressMode::Repeat,
                .maxAnisotropy = 8.0f,
                .compareOp = Teide::CompareOp::Less,
            },
            .captureDepthStencil = true,
        };

        const auto shadowResult = m_renderer->RenderToTexture(shadowRenderTarget, std::move(renderList));
        shadowMap = shadowResult.depthStencilTexture;
    }

    //
    // Pass 1. Draw scene
    //
    {
        // Update view uniforms
        const auto extent = m_surface->GetExtent();
        const float aspectRatio = static_cast<float>(extent.x) / static_cast<float>(extent.y);

        const Geo::Matrix4 rotation = Geo::Matrix4::RotationZ(m_cameraYaw) * Geo::Matrix4::RotationX(m_cameraPitch);
        const Geo::Vector3 cameraDirection = Geo::Normalise(rotation * Geo::Vector3{0.0f, 1.0f, 0.0f});
        const Geo::Vector3 cameraUp = Geo::Normalise(rotation * Geo::Vector3{0.0f, 0.0f, 1.0f});

        const Geo::Point3 cameraPosition = m_cameraTarget - cameraDirection * m_cameraDistance;

        const Geo::Matrix view = Geo::LookAt(cameraPosition, m_cameraTarget, cameraUp);
        const Geo::Matrix proj = Geo::Perspective(45.0_deg, aspectRatio, 0.1f, 10.0f);
        const Geo::Matrix viewProj = proj * view;

        const ViewUniforms viewUniforms = {
            .viewProj = viewProj,
        };
        const Teide::ShaderParameters viewParams = {
            .uniformData = Teide::ToBytes(viewUniforms),
            .textures = {shadowMap},
        };

        Teide::RenderList renderList = {
            .name = "MainPass",
            .clearState = {
                .colorValue = Teide::Color{0.0f, 0.0f, 0.0f, 1.0f},
                .depthValue = 1.0f,
            },
            .viewParameters = viewParams,
            .objects = {{
                .mesh = m_mesh,
                .pipeline = m_pipeline,
                .materialParameters = m_material.params,
                .objectParameters = {.uniformData = Teide::ToBytes(objectUniforms)},
            }},
        };

        m_renderer->RenderToSurface(*m_surface, std::move(renderList));
    }

    m_renderer->EndFrame();
}

void Application::OnResize()
{
    m_surface->OnResize();
}

bool Application::OnEvent(const SDL_Event& event)
{
    switch (event.type)
    {
        case SDL_QUIT: return false;

        case SDL_WINDOWEVENT:
            switch (event.window.event)
            {
                case SDL_WINDOWEVENT_SIZE_CHANGED: {
                    OnResize();
                    break;
                }
            }
            break;

        case SDL_MOUSEBUTTONDOWN: {
            SDL_SetRelativeMouseMode(SDL_TRUE);
            break;
        }

        case SDL_MOUSEBUTTONUP: {
            SDL_SetRelativeMouseMode(SDL_FALSE);
            break;
        }
    }
    return true;
}

bool Application::OnUpdate()
{
    int mouseX = 0;
    int mouseY = 0;
    const uint32_t mouseButtonMask = SDL_GetRelativeMouseState(&mouseX, &mouseY);

    if (SDL_GetModState() & KMOD_CTRL)
    {
        if (mouseButtonMask & SDL_BUTTON_LMASK)
        {
            m_lightYaw += static_cast<float>(mouseX) * CameraRotateSpeed;
            m_lightPitch -= static_cast<float>(mouseY) * CameraRotateSpeed;
        }
    }
    else
    {
        if (mouseButtonMask & SDL_BUTTON_LMASK)
        {
            m_cameraYaw += static_cast<float>(mouseX) * CameraRotateSpeed;
            m_cameraPitch -= static_cast<float>(mouseY) * CameraRotateSpeed;
        }
        if (mouseButtonMask & SDL_BUTTON_RMASK)
        {
            m_cameraDistance -= static_cast<float>(mouseX) * CameraZoomSpeed;
        }
        if (mouseButtonMask & SDL_BUTTON_MMASK)
        {
            const auto rotation = Geo::Matrix4::RotationZ(m_cameraYaw) * Geo::Matrix4::RotationX(m_cameraPitch);
            const auto cameraDirection = Geo::Normalise(rotation * Geo::Vector3{0.0f, 1.0f, 0.0f});
            const auto cameraUp = Geo::Normalise(rotation * Geo::Vector3{0.0f, 0.0f, 1.0f});
            const auto cameraRight = Geo::Normalise(Geo::Cross(cameraUp, cameraDirection));

            m_cameraTarget += cameraRight * static_cast<float>(-mouseX) * CameraMoveSpeed;
            m_cameraTarget += cameraUp * static_cast<float>(mouseY) * CameraMoveSpeed;
        }
    }
    return true;
}

void Application::MainLoop()
{
    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            running &= OnEvent(event);
        }

        running &= OnUpdate();
        OnRender();
    }
}

void Application::CreateMesh(const char* filename)
{
    if (filename == nullptr)
    {
        const Teide::MeshData meshData = {
            .vertexLayout = VertexLayoutDesc,
            .vertexData = Teide::ToBytes(QuadVertices),
            .indexData = Teide::ToBytes(QuadIndices),
            .aabb = {{-0.5f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f}},
        };

        m_mesh = m_device->CreateMesh(meshData, "Quad");
    }
    else
    {
        const auto meshData = LoadMesh(filename);
        m_mesh = m_device->CreateMesh(meshData, filename);
    }
}

void Application::CreateMaterial(const char* imageFilename)
{
    m_material.shader = m_device->CreateShader(CompileShader(ModelShader), "ModelShader");

    const auto* const textureName = imageFilename ? imageFilename : "DefaultTexture";
    const auto texture = m_device->CreateTexture(LoadTexture(imageFilename), textureName);

    const Teide::ParameterBlockData materialData = {
        .layout = m_material.shader->GetMaterialPblockLayout(),
        .parameters = {
            .textures = {texture},
        },
    };
    m_material.params = m_device->CreateParameterBlock(materialData, "Material");
}

void Application::CreatePipelines()
{
    m_pipeline = m_device->CreatePipeline({
        .shader = m_material.shader,
        .vertexLayout = m_mesh->GetVertexLayout(),
        .renderStates = MakeRenderStates(),
        .renderPasses = {
            {
                .framebufferLayout = {
                    .colorFormat = m_surface->GetColorFormat(),
                    .depthStencilFormat = m_surface->GetDepthFormat(),
                    .sampleCount = m_surface->GetSampleCount(),
                },
            },
            {
                .framebufferLayout = ShadowFramebufferLayout,
                .renderOverrides = ShadowRenderOverrides,
            }
        },
    });
}
