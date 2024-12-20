
#include "RenderTest.h"

#include <SDL.h>
#include <SDL_image.h>

#include <cstring>
#include <utility>

using namespace Teide::BasicTypes;

namespace
{

void WritePng(const std::filesystem::path& path, Geo::Size2i size, Teide::BytesView pixels)
{
    SDL_Surface* surface
        = SDL_CreateRGBSurfaceWithFormat(0, static_cast<int>(size.x), static_cast<int>(size.y), 32, SDL_PIXELFORMAT_RGBA32);
    std::memcpy(surface->pixels, pixels.data(), pixels.size());
    IMG_SavePNG(surface, path.string().c_str());
    SDL_FreeSurface(surface);
}

struct ReadPngResult
{
    Geo::Size2i size;
    std::vector<Teide::byte> pixels;
};

ReadPngResult ReadPng(const std::filesystem::path& path)
{
    ReadPngResult result;
    if (SDL_Surface* image = IMG_Load(path.string().c_str()))
    {
        result.size = {static_cast<Teide::uint32>(image->w), static_cast<Teide::uint32>(image->h)};
        result.pixels = Teide::ToBytes(std::span{
            static_cast<const Teide::uint8*>(image->pixels), Teide::usize{result.size.x} * Teide::usize{result.size.y} * 4});
        SDL_FreeSurface(image);
    }
    return result;
}


constexpr auto QuadVertices = std::array<Vertex, 4>{{
    {.position = {-0.5f, -0.5f, 0.0f}, .texCoord = {0.0f, 0.0f}, .normal = {0.0f, 1.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}},
    {.position = {0.5f, -0.5f, 0.0f}, .texCoord = {1.0f, 0.0f}, .normal = {0.0f, 1.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}},
    {.position = {0.5f, 0.5f, 0.0f}, .texCoord = {1.0f, 1.0f}, .normal = {0.0f, 1.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}},
    {.position = {-0.5f, 0.5f, 0.0f}, .texCoord = {0.0f, 1.0f}, .normal = {0.0f, 1.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}},
}};

constexpr auto QuadIndices = std::array<uint16, 6>{{0, 1, 2, 2, 3, 0}};

using Type = Teide::ShaderVariableType::BaseType;

const Teide::ShaderEnvironmentData DefaultShaderEnv = {
    .scenePblock = {
        .parameters = {
            {"lightDir", Type::Vector3},
            {"lightColor", Type::Vector3},
            {"ambientColorTop", Type::Vector3},
            {"ambientColorBottom", Type::Vector3},
            {"shadowMatrix", Type::Matrix4}
        },
        .uniformsStages = Teide::ShaderStageFlags::Pixel,
    },
    .viewPblock = {
        .parameters = {
            {"viewProj", Type::Matrix4},
            {"shadowMapSampler", Type::Texture2DShadow},
        },
        .uniformsStages = Teide::ShaderStageFlags::Vertex,
    },
};

const ShaderSourceData ModelShader = {
    .language = ShaderLanguage::Glsl,
    .environment = DefaultShaderEnv,
    .materialPblock = {
        .parameters = {
            {"texSampler", Type::Texture2D},
        },
    },
    .objectPblock = {
        .parameters = {
            {"model", Type::Matrix4}
        },
    },
    .vertexShader = {
        .inputs = {{
            {"position", Type::Vector3},
            {"texCoord", Type::Vector2},
            {"normal", Type::Vector3},
            {"color", Type::Vector3},
        }},
        .outputs = {{
            {"outTexCoord", Type::Vector2},
            {"outPosition", Type::Vector3},
            {"outNormal", Type::Vector3},
            {"outColor", Type::Vector3},
            {"gl_Position", Type::Vector3},
        }},
        .source = R"--(
            void main() {
                outPosition = mul(object.model, vec4(position, 1.0)).xyz;
                gl_Position = mul(view.viewProj, vec4(outPosition, 1.0));
                outTexCoord = texCoord;
                outNormal = mul(object.model, vec4(normal, 0.0)).xyz;
                outColor = color;
            }
            )--",
    },
    .pixelShader = {
        .inputs = {{
            {"inTexCoord", Type::Vector2},
            {"inPosition", Type::Vector3},
            {"inNormal", Type::Vector3},
            {"inColor", Type::Vector3},
        }},
        .outputs = {{
            {"outColor", Type::Vector4},
        }},
        .source = R"--(
            float textureProj(sampler2DShadow shadowMap, vec4 shadowCoord, vec2 off) {
                return texture(shadowMap, shadowCoord.xyz + vec3(off, 0.0)).r;
            }

            void main() {
                vec4 shadowCoord = mul(scene.shadowMatrix, vec4(inPosition, 1.0));
                shadowCoord /= shadowCoord.w;
                shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;

                ivec2 texDim = textureSize(shadowMapSampler, 0);
                float scale = 1.5;
                float dx = scale * 1.0 / float(texDim.x);
                float dy = scale * 1.0 / float(texDim.y);

                const int range = 1;
                int count = 0;
                float shadowFactor = 0.0;
                for (int x = -range; x <= range; x++) {
                    for (int y = -range; y <= range; y++) {
                        count++;
                        shadowFactor += textureProj(shadowMapSampler, shadowCoord, vec2(dx*x, dy*y));
                    }
                }
                const float lit = shadowFactor / count;

                const vec3 dirLight = clamp(dot(inNormal, -scene.lightDir), 0.0, 1.0) * scene.lightColor;
                const vec3 ambLight = mix(scene.ambientColorBottom, scene.ambientColorTop, inNormal.z * 0.5 + 0.5);
                const vec3 lighting = lit * dirLight + ambLight;

                const vec3 color = lighting * texture(texSampler, inTexCoord).rgb;
                outColor = vec4(color, 1.0);
            })--",
    },
};

const Teide::VertexLayout VertexLayoutDesc = {
    .topology = Teide::PrimitiveTopology::TriangleList,
    .bufferBindings = {{
        .stride = sizeof(Vertex),
    }},
    .attributes
    = {{
           .name = "position",
           .format = Teide::Format::Float3,
           .offset = offsetof(Vertex, position),
       },
       {
           .name = "texCoord",
           .format = Teide::Format::Float2,
           .offset = offsetof(Vertex, texCoord),
       },
       {
           .name = "normal",
           .format = Teide::Format::Float3,
           .offset = offsetof(Vertex, normal),
       },
       {
           .name = "color",
           .format = Teide::Format::Float3,
           .offset = offsetof(Vertex, color),
       }},
};


constexpr Teide::RenderTargetInfo RenderTarget = {
    .size = RenderSize,
    .framebufferLayout = {
        .colorFormat = Teide::Format::Byte4Srgb,
        .depthStencilFormat = std::nullopt,
        .sampleCount = 1,
        .captureColor = true,
        .captureDepthStencil = false,
    },
    .samplerState = {},
};

} // namespace

void RenderTest::SetUpdateReferences(bool set)
{
    s_updateReferences = set;
}

void RenderTest::SetReferenceDir(const std::filesystem::path& dir)
{
    s_referenceDir = dir;
}

void RenderTest::SetOutputDir(const std::filesystem::path& dir)
{
    s_outputDir = dir;

    remove_all(s_outputDir);
    create_directory(s_outputDir);
}

RenderTest::RenderTest() : RenderTest(DefaultShaderEnv)
{}

RenderTest::RenderTest(const Teide::ShaderEnvironmentData& shaderEnv) :
    m_device{Teide::CreateHeadlessDevice()},
    m_shaderEnv{m_device->CreateShaderEnvironment(shaderEnv, "ShaderEnv")},
    m_renderer{m_device->CreateRenderer(m_shaderEnv)}
{}

void RenderTest::RunTest(const SceneUniforms& sceneUniforms, Teide::RenderList renderList)
{
    m_renderDoc.StartFrameCapture();
    m_renderer->BeginFrame({.uniformData = Teide::ToBytes(sceneUniforms)});

    const Teide::Texture output = m_renderer->RenderToTexture(RenderTarget, std::move(renderList)).colorTexture.value();

    const Teide::TextureData outputData = m_renderer->CopyTextureData(output).get();

    m_renderer->EndFrame();
    m_renderDoc.EndFrameCapture();

    const auto& currentTest = *testing::UnitTest::GetInstance()->current_test_info();
    const std::string testName = currentTest.name();
    const std::filesystem::path outputFile = (s_outputDir / testName).replace_extension(".out.png");
    const std::filesystem::path referenceFile = s_referenceDir / (testName + ".png");

    CompareImageToReference(outputData, referenceFile);

    if (currentTest.result()->Failed())
    {
        const std::filesystem::path testOutputDir = s_outputDir;
        create_directories(testOutputDir);
        WritePng(outputFile, outputData.size, outputData.pixels);
        copy_file(referenceFile, testOutputDir / (testName + ".ref.png"));
    }
}

Teide::ShaderPtr RenderTest::CreateModelShader()
{
    return m_device->CreateShader(CompileShader(ModelShader), "ModelShader");
}

Teide::MeshPtr RenderTest::CreateQuadMesh()
{
    const Teide::MeshData meshData = {
        .vertexLayout = VertexLayoutDesc,
        .vertexData = Teide::ToBytes(QuadVertices),
        .indexData = Teide::ToBytes(QuadIndices),
        .aabb = {{-0.5f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f}},
    };

    return m_device->CreateMesh(meshData, "Quad");
}

Teide::MeshPtr RenderTest::CreatePlaneMesh(Geo::Size2 size, Geo::Vector2 tiling)
{
    const float x = size.x / 2.0f;
    const float y = size.y / 2.0f;
    const auto planeVertices = std::array<Vertex, 4>{{
        {.position = {-x, -y, 0.0f}, .texCoord = {0.0f, 0.0f}, .normal = {0.0f, 1.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}},
        {.position = {x, -y, 0.0f}, .texCoord = {tiling.x, 0.0f}, .normal = {0.0f, 1.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}},
        {.position = {x, y, 0.0f}, .texCoord = {tiling.x, tiling.y}, .normal = {0.0f, 1.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}},
        {.position = {-x, y, 0.0f}, .texCoord = {0.0f, tiling.y}, .normal = {0.0f, 1.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}},
    }};

    const Teide::MeshData meshData = {
        .vertexLayout = VertexLayoutDesc,
        .vertexData = Teide::ToBytes(planeVertices),
        .indexData = Teide::ToBytes(QuadIndices),
        .aabb = {{-x, -x, 0.0f}, {y, y, 0.0f}},
    };

    return m_device->CreateMesh(meshData, "Plane");
}

Teide::Texture RenderTest::CreateNullShadowmapTexture()
{
    // Create checkerboard texture
    constexpr auto pixels = 1.0f;

    const Teide::TextureData textureData = {
        .size = {1, 1},
        .format = Teide::Format::Float1,
        .mipLevelCount = 1,
        .samplerState = {
            .magFilter = Teide::Filter::Nearest,
            .minFilter = Teide::Filter::Nearest,
            .compareOp = Teide::CompareOp::Less,
        },
        .pixels = Teide::ToBytes(pixels),
    };

    return m_device->CreateTexture(textureData, "NullShadowMap");
}

Teide::Texture RenderTest::CreateCheckerTexture(Teide::Filter filter, bool mipmaps)
{
    // Create checkerboard texture
    constexpr usize size = 256;
    constexpr auto checkSize = size / 8;
    constexpr auto color0 = uint32{0xffff00ff};
    constexpr auto color1 = uint32{0xffffffff};

    std::array<uint32, size * size> pixels{};
    for (usize i = 0; i < size * size; i++)
    {
        const auto x = i % size;
        const auto y = i / size;

        pixels[i] = ((x / checkSize) - (y / checkSize)) & 0x1 ? color0 : color1;
    }

    const Teide::TextureData textureData = {
        .size = {size, size},
        .format = Teide::Format::Byte4Srgb,
        .mipLevelCount = mipmaps ? static_cast<uint32>(std::floor(std::log2(size))) + 1 : 1,
        .samplerState = {
            .magFilter = filter,
            .minFilter = filter,
            .mipmapMode = Teide::MipmapMode::Linear,
        },
        .pixels = Teide::ToBytes(pixels),
    };

    return m_device->CreateTexture(textureData, "Checker");
}

Teide::PipelinePtr RenderTest::CreatePipeline(Teide::ShaderPtr shader, const Teide::MeshPtr& mesh)
{
    return m_device->CreatePipeline({
        .shader = std::move(shader),
        .vertexLayout = mesh->GetVertexLayout(),
        .renderPasses = {
            { .framebufferLayout = RenderTarget.framebufferLayout },
        },
    });
}

Teide::ShaderData RenderTest::CompileShader(const ShaderSourceData& data)
{
    return m_shaderCompiler.Compile(data);
}

Teide::Device& RenderTest::GetDevice()
{
    return *m_device;
}


void RenderTest::CompareImageToReference(const Teide::TextureData& image, const std::filesystem::path& referenceFile)
{
    if (s_updateReferences)
    {
        WritePng(referenceFile, image.size, image.pixels);

        GTEST_SKIP_("Updated");
    }

    if (!exists(referenceFile))
    {
        FAIL() << "Reference image missing";
    }

    const auto [referenceSize, referenceData] = ReadPng(referenceFile);
    if (referenceData.empty())
    {
        FAIL() << "Reference image could not be loaded";
    }

    if (image.size != referenceSize || image.pixels != referenceData)
    {
        FAIL() << "Rendered image does not match reference";
    }
}

bool RenderTest::s_updateReferences = false;
std::filesystem::path RenderTest::s_referenceDir;
std::filesystem::path RenderTest::s_outputDir;
