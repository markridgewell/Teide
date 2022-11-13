
#pragma once

#include "GeoLib/Angle.h"
#include "GeoLib/Box.h"
#include "GeoLib/Matrix.h"
#include "GeoLib/Vector.h"
#include "Teide/ForwardDeclare.h"
#include "Teide/GraphicsDevice.h"
#include "Teide/Surface.h"

#include <SDL.h>

#include <chrono>
#include <exception>
#include <string>

using namespace Geo::Literals;

class ApplicationError : public std::exception
{
public:
    explicit ApplicationError(std::string message) : m_what{std::move(message)} {}

    virtual const char* what() const noexcept { return m_what.c_str(); }

private:
    std::string m_what;
};

class Application
{
public:
    static constexpr Geo::Angle CameraRotateSpeed = 0.1_deg;
    static constexpr float CameraZoomSpeed = 0.002f;
    static constexpr float CameraMoveSpeed = 0.001f;

    explicit Application(SDL_Window* window, const char* imageFilename, const char* modelFilename);

    void OnRender();
    void OnResize();
    bool OnEvent(const SDL_Event& event);
    bool OnUpdate();
    void MainLoop();

private:
    void CreateMesh(const char* filename);
    void CreateParameterBlocks();
    void LoadTexture(const char* filename);
    void CreatePipelines();

    SDL_Window* m_window;

    std::chrono::high_resolution_clock::time_point m_startTime = std::chrono::high_resolution_clock::now();

    Teide::GraphicsDevicePtr m_device;
    Teide::SurfacePtr m_surface;
    Teide::ShaderEnvironmentPtr m_shaderEnvironment;

    // Object setup
    Teide::ShaderPtr m_shader;
    Teide::PipelinePtr m_pipeline;
    Geo::Box3 m_meshBounds;
    Teide::MeshPtr m_mesh;
    Teide::TexturePtr m_texture;

    Teide::ParameterBlockPtr m_materialParams;

    // Lights
    Geo::Angle m_lightYaw = 45.0_deg;
    Geo::Angle m_lightPitch = -45.0_deg;
    Teide::PipelinePtr m_shadowMapPipeline;
    Geo::Matrix4 m_shadowMatrix;

    // Camera
    Geo::Point3 m_cameraTarget = {0.0f, 0.0f, 0.0f};
    Geo::Angle m_cameraYaw = 45.0_deg;
    Geo::Angle m_cameraPitch = -45.0_deg;
    float m_cameraDistance = 3.0f;

    // Rendering
    Teide::RendererPtr m_renderer;
};
