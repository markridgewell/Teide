
#include "GeoLib/Matrix.h"
#include "GeoLib/Vector.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/Buffer.h"
#include "Teide/Definitions.h"
#include "Teide/GraphicsDevice.h"
#include "Teide/Renderer.h"
#include "Teide/Shader.h"
#include "Teide/Surface.h"
#include "Teide/Texture.h"
#include "Types/TextureData.h"

#include <SDL.h>
#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/spdlog.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <optional>
#include <ranges>
#include <span>

#ifdef _WIN32
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <windows.h>
#endif

using namespace Geo::Literals;

namespace
{
constexpr bool UseMSAA = true;

class ApplicationError : public std::exception
{
public:
	explicit ApplicationError(std::string message) : m_what{std::move(message)} {}

	virtual const char* what() const noexcept { return m_what.c_str(); }

private:
	std::string m_what;
};

using Type = ShaderVariableType::BaseType;

const ShaderSourceData ModelShader = {
    .language = ShaderLanguage::Glsl,
    .scenePblock = {
        .parameters = {
            {"lightDir", Type::Vector3},
            {"lightColor", Type::Vector3},
            {"ambientColorTop", Type::Vector3},
            {"ambientColorBottom", Type::Vector3},
            {"shadowMatrix", Type::Matrix4}
        },
    },
    .viewPblock = {
        .parameters = {
            {"viewProj", Type::Matrix4},
            {"shadowMapSampler", Type::Texture2DShadow},
        },
    },
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
            {"inPosition", Type::Vector3},
            {"inTexCoord", Type::Vector2},
            {"inNormal", Type::Vector3},
            {"inColor", Type::Vector3},
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
    outPosition = mul(object.model, vec4(inPosition, 1.0)).xyz;
    gl_Position = mul(view.viewProj, vec4(outPosition, 1.0));
    outTexCoord = inTexCoord;
    outNormal = mul(object.model, vec4(inNormal, 0.0)).xyz;
    outColor = inColor;
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

struct Vertex
{
	Geo::Vector3 position;
	Geo::Vector2 texCoord;
	Geo::Vector3 normal;
	Geo::Vector3 color;
};

constexpr auto QuadVertices = std::array<Vertex, 4>{{
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}},
}};

constexpr auto QuadIndices = std::array<uint16_t, 6>{{0, 1, 2, 2, 3, 0}};

const auto VertexLayoutDesc = VertexLayout
{
	.inputAssembly = {
	    .topology = vk::PrimitiveTopology::eTriangleList,
	    .primitiveRestartEnable = false,
	},
    .vertexInputBindings = {{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = vk::VertexInputRate::eVertex,
    }},
    .vertexInputAttributes = {{
        .location = 0,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(Vertex, position),
    },
    {
        .location = 1,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(Vertex, texCoord),
    },
    {
        .location = 2,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(Vertex, normal),
    },
    {
        .location = 3,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(Vertex, color),
    }},
};

struct GlobalUniforms
{
	Geo::Vector3 lightDir;
	float pad0 [[maybe_unused]];
	Geo::Vector3 lightColor;
	float pad1 [[maybe_unused]];
	Geo::Vector3 ambientColorTop;
	float pad2 [[maybe_unused]];
	Geo::Vector3 ambientColorBottom;
	float pad3 [[maybe_unused]];
	Geo::Matrix4 shadowMatrix;
};

struct ViewUniforms
{
	Geo::Matrix4 viewProj;
};

struct ObjectUniforms
{
	Geo::Matrix4 model;
};

constexpr auto ShaderLang = ShaderLanguage::Glsl;

static const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

std::vector<std::byte> CopyBytes(BytesView src)
{
	return std::vector<std::byte>(src.begin(), src.end());
}

RenderStates MakeRenderStates(float depthBiasConstant = 0.0f, float depthBiasSlope = 0.0f)
{
	return {
		// Viewport and scissor will be dynamic states, so their initial values don't matter
		.viewport = vk::Viewport{},
		.scissor = vk::Rect2D{},

		.rasterizationState = vk::PipelineRasterizationStateCreateInfo{
			.depthClampEnable = false,
			.rasterizerDiscardEnable = false,
			.polygonMode = vk::PolygonMode::eFill,
			.cullMode = vk::CullModeFlagBits::eNone,
			.frontFace = vk::FrontFace::eClockwise,
			.depthBiasEnable = depthBiasConstant != 0.0f || depthBiasSlope != 0.0f,
			.depthBiasConstantFactor = depthBiasConstant,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = depthBiasSlope,
			.lineWidth = 1.0f,
		},
		.depthStencilState = {
			.depthTestEnable = true,
			.depthWriteEnable = true,
			.depthCompareOp = vk::CompareOp::eLess,
			.depthBoundsTestEnable = false,
			.stencilTestEnable = false,
		},
		.colorBlendAttachment = {
			.blendEnable = false,
			.srcColorBlendFactor = vk::BlendFactor::eOne,
			.dstColorBlendFactor = vk::BlendFactor::eZero,
			.colorBlendOp = vk::BlendOp::eAdd,
			.srcAlphaBlendFactor = vk::BlendFactor::eOne,
			.dstAlphaBlendFactor = vk::BlendFactor::eZero,
			.alphaBlendOp = vk::BlendOp::eAdd,
			.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
				| vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
		},
		.dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor}
	};
}

} // namespace

class Application
{
public:
	static constexpr Geo::Angle CameraRotateSpeed = 0.1_deg;
	static constexpr float CameraZoomSpeed = 0.002f;
	static constexpr float CameraMoveSpeed = 0.001f;

	explicit Application(SDL_Window* window, const char* imageFilename, const char* modelFilename) :
	    m_window{window},
	    m_device{CreateGraphicsDevice(window)},
	    m_surface{m_device->CreateSurface(window, UseMSAA)},
	    m_shader{m_device->CreateShader(CompileShader(ModelShader), "ModelShader")},
	    m_renderer{m_device->CreateRenderer(m_shader)}
	{
		const auto pipelineData = PipelineData{
		    .shader = m_shader,
		    .vertexLayout = VertexLayoutDesc,
		    .renderStates = MakeRenderStates(),
		    .framebufferLayout = {
		        .colorFormat = m_surface->GetColorFormat(),
		        .depthStencilFormat = m_surface->GetDepthFormat(),
		        .sampleCount = m_surface->GetSampleCount(),
		    },
		};
		m_pipeline = m_device->CreatePipeline(pipelineData);

		CreateMesh(modelFilename);
		LoadTexture(imageFilename);
		CreateShadowMap();
		CreateParameterBlocks();

		spdlog::info("Vulkan initialised successfully");
	}

	void OnRender()
	{
		const Geo::Matrix4 lightRotation = Geo::Matrix4::RotationZ(m_lightYaw) * Geo::Matrix4::RotationX(m_lightPitch);
		const Geo::Vector3 lightDirection = Geo::Normalise(lightRotation * Geo::Vector3{0.0f, 1.0f, 0.0f});
		const Geo::Vector3 lightUp = Geo::Normalise(lightRotation * Geo::Vector3{0.0f, 0.0f, 1.0f});
		const Geo::Point3 shadowCameraPosition = Geo::Point3{} - lightDirection;

		const auto modelMatrix = Geo::Matrix4::RotationX(180.0_deg);

		const auto shadowCamView = Geo::LookAt(shadowCameraPosition, Geo::Point3{}, lightUp);
		const auto shadowCamProj = Geo::Orthographic(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);

		const auto shadowMVP = shadowCamProj * shadowCamView * modelMatrix;

		const std::array<Geo::Point3, 8> bboxCorners = {
		    shadowMVP * Geo::Point3(m_meshBoundsMin.x, m_meshBoundsMin.y, m_meshBoundsMin.z),
		    shadowMVP * Geo::Point3(m_meshBoundsMin.x, m_meshBoundsMin.y, m_meshBoundsMax.z),
		    shadowMVP * Geo::Point3(m_meshBoundsMin.x, m_meshBoundsMax.y, m_meshBoundsMin.z),
		    shadowMVP * Geo::Point3(m_meshBoundsMin.x, m_meshBoundsMax.y, m_meshBoundsMax.z),
		    shadowMVP * Geo::Point3(m_meshBoundsMax.x, m_meshBoundsMin.y, m_meshBoundsMin.z),
		    shadowMVP * Geo::Point3(m_meshBoundsMax.x, m_meshBoundsMin.y, m_meshBoundsMax.z),
		    shadowMVP * Geo::Point3(m_meshBoundsMax.x, m_meshBoundsMax.y, m_meshBoundsMin.z),
		    shadowMVP * Geo::Point3(m_meshBoundsMax.x, m_meshBoundsMax.y, m_meshBoundsMax.z),
		};

		const auto [minX, maxX] = std::ranges::minmax(std::views::transform(bboxCorners, &Geo::Point3::x));
		const auto [minY, maxY] = std::ranges::minmax(std::views::transform(bboxCorners, &Geo::Point3::y));
		const auto [minZ, maxZ] = std::ranges::minmax(std::views::transform(bboxCorners, &Geo::Point3::z));

		const auto shadowCamProjFitted = Geo::Orthographic(minX, maxX, maxY, minY, minZ, maxZ);

		m_shadowMatrix = shadowCamProjFitted * shadowCamView;

		// Update global uniforms
		const auto globalUniforms = GlobalUniforms{
		    .lightDir = Geo::Normalise(lightDirection),
		    .lightColor = {1.0f, 1.0f, 1.0f},
		    .ambientColorTop = {0.08f, 0.1f, 0.1f},
		    .ambientColorBottom = {0.003f, 0.003f, 0.002f},
		    .shadowMatrix = m_shadowMatrix,
		};
		const auto sceneParams = ShaderParameters{
		    .uniformBufferData = ToBytes(globalUniforms),
		};

		m_renderer->BeginFrame(sceneParams);

		// Update object uniforms
		m_objectUniforms = {
		    .model = modelMatrix,
		};

		//
		// Pass 0. Draw shadows
		//
		{
			// Update view uniforms
			const auto viewUniforms = ViewUniforms{
			    .viewProj = m_shadowMatrix,
			};
			const auto viewParams = ShaderParameters{
			    .uniformBufferData = ToBytes(viewUniforms),
			    .textures = {},
			};

			RenderList renderList = {
			    .name = "ShadowPass",
			    .clearDepthValue = 1.0f,
			    .viewParameters = viewParams,
			    .objects = {{
			        .vertexBuffer = m_vertexBuffer,
			        .indexBuffer = m_indexBuffer,
			        .indexCount = m_indexCount,
			        .pipeline = m_shadowMapPipeline,
			        .materialParameters = m_materialParams,
			        .pushConstants = m_objectUniforms,
			    }},
			};

			m_renderer->RenderToTexture(m_shadowMap, std::move(renderList));
		}

		//
		// Pass 1. Draw scene
		//
		{
			// Update view uniforms
			const auto extent = m_surface->GetExtent();
			const float aspectRatio = static_cast<float>(extent.width) / static_cast<float>(extent.height);

			const Geo::Matrix4 rotation = Geo::Matrix4::RotationZ(m_cameraYaw) * Geo::Matrix4::RotationX(m_cameraPitch);
			const Geo::Vector3 cameraDirection = Geo::Normalise(rotation * Geo::Vector3{0.0f, 1.0f, 0.0f});
			const Geo::Vector3 cameraUp = Geo::Normalise(rotation * Geo::Vector3{0.0f, 0.0f, 1.0f});

			const Geo::Point3 cameraPosition = m_cameraTarget - cameraDirection * m_cameraDistance;

			const Geo::Matrix view = Geo::LookAt(cameraPosition, m_cameraTarget, cameraUp);
			const Geo::Matrix proj = Geo::Perspective(45.0_deg, aspectRatio, 0.1f, 10.0f);
			const Geo::Matrix viewProj = proj * view;

			const auto viewUniforms = ViewUniforms{
			    .viewProj = viewProj,
			};
			const auto viewParams = ShaderParameters{
			    .uniformBufferData = ToBytes(viewUniforms),
			    .textures = {m_shadowMap.get()},
			};

			RenderList renderList = {
			    .name = "MainPass",
			    .clearColorValue = Color{0.0f, 0.0f, 0.0f, 1.0f},
			    .clearDepthValue = 1.0f,
			    .viewParameters = viewParams,
			    .objects = {{
			        .vertexBuffer = m_vertexBuffer,
			        .indexBuffer = m_indexBuffer,
			        .indexCount = m_indexCount,
			        .pipeline = m_pipeline,
			        .materialParameters = m_materialParams,
			        .pushConstants = m_objectUniforms,
			    }},
			};

			m_renderer->RenderToSurface(*m_surface, std::move(renderList));
		}

		m_renderer->EndFrame();
	}

	void OnResize() { m_surface->OnResize(); }

	bool OnEvent(const SDL_Event& event)
	{
		switch (event.type)
		{
			case SDL_QUIT:
				return false;

			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_SIZE_CHANGED:
						OnResize();
						break;
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
				SDL_SetRelativeMouseMode(SDL_TRUE);
				break;

			case SDL_MOUSEBUTTONUP:
				SDL_SetRelativeMouseMode(SDL_FALSE);
				break;
		}
		return true;
	}

	bool OnUpdate()
	{
		int mouseX{}, mouseY{};
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

	void MainLoop()
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

private:
	void CreateVertexBuffer(BytesView data)
	{
		m_vertexBuffer = m_device->CreateBuffer(
		    {.usage = vk::BufferUsageFlagBits::eVertexBuffer,
		     .memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal,
		     .data = data},
		    "VertexBuffer");
	}

	void CreateIndexBuffer(std::span<const uint16_t> data)
	{
		m_indexBuffer = m_device->CreateBuffer(
		    {.usage = vk::BufferUsageFlagBits::eIndexBuffer, .memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal, .data = data},
		    "IndexBuffer");
		m_indexCount = static_cast<std::uint32_t>(size(data));
	}

	void CreateMesh(const char* filename)
	{
		if (filename == nullptr)
		{
			CreateVertexBuffer(QuadVertices);
			CreateIndexBuffer(QuadIndices);

			m_meshBoundsMin = {-0.5f, -0.5f, 0.0f};
			m_meshBoundsMax = {0.5f, 0.5f, 0.0f};
		}
		else
		{
			Assimp::Importer importer;

			const auto importFlags = aiProcess_CalcTangentSpace | aiProcess_Triangulate
			    | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType | aiProcess_RemoveComponent
			    | aiProcess_RemoveRedundantMaterials | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph
			    | aiProcess_ConvertToLeftHanded | aiProcess_FlipWindingOrder;

			const aiScene* scene = importer.ReadFile(filename, importFlags);
			if (!scene)
			{
				throw ApplicationError(importer.GetErrorString());
			}

			if (scene->mNumMeshes == 0)
			{
				throw ApplicationError(fmt::format("Model file '{}' contains no meshes", filename));
			}
			if (scene->mNumMeshes > 1)
			{
				throw ApplicationError(fmt::format("Model file '{}' contains too many meshes", filename));
			}

			const aiMesh& mesh = **scene->mMeshes;

			std::vector<Vertex> vertices;
			vertices.reserve(mesh.mNumVertices);

			m_meshBoundsMin
			    = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
			m_meshBoundsMax
			    = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
			       std::numeric_limits<float>::lowest()};
			for (unsigned int i = 0; i < mesh.mNumVertices; i++)
			{
				const auto pos = mesh.mVertices[i];
				const auto uv = mesh.HasTextureCoords(0) ? mesh.mTextureCoords[0][i] : aiVector3D{};
				const auto norm = mesh.HasNormals() ? mesh.mNormals[i] : aiVector3D{};
				const auto color = mesh.HasVertexColors(0) ? mesh.mColors[0][i] : aiColor4D{1, 1, 1, 1};
				vertices.push_back(Vertex{
				    .position = {pos.x, pos.y, pos.z},
				    .texCoord = {uv.x, uv.y},
				    .normal = {norm.x, norm.y, norm.z},
				    .color = {color.r, color.g, color.b},
				});

				m_meshBoundsMin.x = std::min(m_meshBoundsMin.x, pos.x);
				m_meshBoundsMin.y = std::min(m_meshBoundsMin.y, pos.y);
				m_meshBoundsMin.z = std::min(m_meshBoundsMin.z, pos.z);
				m_meshBoundsMax.x = std::max(m_meshBoundsMax.x, pos.x);
				m_meshBoundsMax.y = std::max(m_meshBoundsMax.y, pos.y);
				m_meshBoundsMax.z = std::max(m_meshBoundsMax.z, pos.z);
			}

			CreateVertexBuffer(vertices);

			std::vector<uint16_t> indices;
			indices.reserve(static_cast<std::size_t>(mesh.mNumFaces) * 3);

			for (unsigned int i = 0; i < mesh.mNumFaces; i++)
			{
				const auto& face = mesh.mFaces[i];
				assert(face.mNumIndices == 3);
				for (int j = 0; j < 3; j++)
				{
					if (face.mIndices[j] > std::numeric_limits<uint16_t>::max())
					{
						throw ApplicationError("Too many vertices for 16-bit indices");
					}
					indices.push_back(static_cast<uint16_t>(face.mIndices[j]));
				}
			}

			CreateIndexBuffer(indices);
		}
	}

	void CreateParameterBlocks()
	{
		const auto materialData = ParameterBlockData{
		    .shader = m_shader,
		    .blockType = ParameterBlockType::Material,
		    .parameters = {
		        .textures = {m_texture.get()},
		    },
		};
		m_materialParams = m_device->CreateParameterBlock(materialData, "Material");
	}

	void LoadTexture(const char* filename)
	{
		if (filename == nullptr)
		{
			// Create default checkerboard texture
			constexpr auto c0 = std::uint32_t{0xffffffff};
			constexpr auto c1 = std::uint32_t{0xffff00ff};
			constexpr auto pixels = std::array{
			    c0, c1, c0, c1, c0, c1, c0, c1, //
			    c1, c0, c1, c0, c1, c0, c1, c0, //
			    c0, c1, c0, c1, c0, c1, c0, c1, //
			    c1, c0, c1, c0, c1, c0, c1, c0, //
			    c0, c1, c0, c1, c0, c1, c0, c1, //
			    c1, c0, c1, c0, c1, c0, c1, c0, //
			    c0, c1, c0, c1, c0, c1, c0, c1, //
			    c1, c0, c1, c0, c1, c0, c1, c0,
			};
			const auto data = TextureData{
			    .size = {8, 8},
			    .format = vk::Format::eR8G8B8A8Srgb,
			    .mipLevelCount = static_cast<uint32_t>(std::floor(std::log2(8))) + 1,
			    .samplerInfo={.magFilter = vk::Filter::eNearest,.minFilter = vk::Filter::eNearest,},
			    .pixels = CopyBytes(pixels),
			};

			m_texture = m_device->CreateTexture(data, "DefaultTexture");
			return;
		}

		struct StbiDeleter
		{
			void operator()(stbi_uc* p) { stbi_image_free(p); }
		};
		using StbiPtr = std::unique_ptr<stbi_uc, StbiDeleter>;

		// Load image
		int width{}, height{}, channels{};
		const auto pixels = StbiPtr(stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha));
		if (!pixels)
		{
			throw ApplicationError(fmt::format("Error loading texture '{}'", filename));
		}

		const vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(width) * height * 4;

		const auto data = TextureData{
		    .size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
		    .format = vk::Format::eR8G8B8A8Srgb,
		    .mipLevelCount = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1,
			.samplerInfo={.magFilter = vk::Filter::eLinear,.minFilter = vk::Filter::eLinear,},
		    .pixels = CopyBytes(std::span(pixels.get(), imageSize)),
		};

		m_texture = m_device->CreateTexture(data, filename);
	}

	void CreateShadowMap()
	{
		constexpr uint32_t shadowMapSize = 2048;

		const auto data = TextureData{
		    .size = {shadowMapSize, shadowMapSize},
		    .format = vk::Format::eD16Unorm,
		    .samplerInfo = {
		        .magFilter = vk::Filter::eLinear,
		        .minFilter = vk::Filter::eLinear,
		        .mipmapMode = vk::SamplerMipmapMode::eNearest,
		        .addressModeU = vk::SamplerAddressMode::eRepeat,
		        .addressModeV = vk::SamplerAddressMode::eRepeat,
		        .addressModeW = vk::SamplerAddressMode::eRepeat,
		        .anisotropyEnable = true,
		        .maxAnisotropy = 8.0f,
		        .compareEnable = true,
		        .compareOp = vk::CompareOp::eLess,
		        .borderColor = vk::BorderColor::eFloatOpaqueWhite,
		    },
		};

		m_shadowMap = m_device->CreateRenderableTexture(data, "ShadowMap");

		// Depth bias (and slope) are used to avoid shadowing artifacts
		// Constant depth bias factor (always applied)
		float depthBiasConstant = 1.25f;
		// Slope depth bias factor, applied depending on polygon's slope
		float depthBiasSlope = 2.75f;

		// Create pipeline
		const auto pipelineData = PipelineData{
		    .shader = m_shader,
		    .vertexLayout = VertexLayoutDesc,
		    .renderStates = MakeRenderStates(depthBiasConstant, depthBiasSlope),
		    .framebufferLayout = {
		        .colorFormat = vk::Format::eUndefined,
		        .depthStencilFormat = m_shadowMap->GetFormat(),
		        .sampleCount = m_shadowMap->GetSampleCount(),
		    },
		};
		m_shadowMapPipeline = m_device->CreatePipeline(pipelineData);
	}

	//
	// Device functions
	//

	SDL_Window* m_window;

	std::chrono::high_resolution_clock::time_point m_startTime = std::chrono::high_resolution_clock::now();

	GraphicsDevicePtr m_device;
	SurfacePtr m_surface;

	// Object setup
	ShaderPtr m_shader;
	PipelinePtr m_pipeline;
	Geo::Point3 m_meshBoundsMin;
	Geo::Point3 m_meshBoundsMax;
	BufferPtr m_vertexBuffer;
	BufferPtr m_indexBuffer;
	uint32_t m_indexCount;
	TexturePtr m_texture;

	const uint32_t m_passCount = 2;
	ObjectUniforms m_objectUniforms;
	ParameterBlockPtr m_materialParams;

	// Lights
	Geo::Angle m_lightYaw = 45.0_deg;
	Geo::Angle m_lightPitch = -45.0_deg;
	DynamicTexturePtr m_shadowMap;
	PipelinePtr m_shadowMapPipeline;
	Geo::Matrix4 m_shadowMatrix;

	// Camera
	Geo::Point3 m_cameraTarget = {0.0f, 0.0f, 0.0f};
	Geo::Angle m_cameraYaw = 45.0_deg;
	Geo::Angle m_cameraPitch = -45.0_deg;
	float m_cameraDistance = 3.0f;

	// Rendering
	RendererPtr m_renderer;
};

struct SDLWindowDeleter
{
	void operator()(SDL_Window* window) { SDL_DestroyWindow(window); }
};

using UniqueSDLWindow = std::unique_ptr<SDL_Window, SDLWindowDeleter>;

int Run(int argc, char* argv[])
{
	const char* const imageFilename = (argc >= 2) ? argv[1] : nullptr;
	const char* const modelFilename = (argc >= 3) ? argv[2] : nullptr;

	const auto windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
	const auto window = UniqueSDLWindow(
	    SDL_CreateWindow("Teide", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 768, windowFlags), {});
	if (!window)
	{
		spdlog::critical("SDL error: {}", SDL_GetError());
		std::string message = fmt::format("The following error occurred when initializing SDL: {}", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), window.get());
		return 1;
	}

	try
	{
		auto application = Application(window.get(), imageFilename, modelFilename);
		application.MainLoop();
	}
	catch (const vk::Error& e)
	{
		spdlog::critical("Vulkan error: {}", e.what());
		std::string message = fmt::format("The following error occurred when initializing Vulkan:\n{}", e.what());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), window.get());
		return 1;
	}
	catch (const ApplicationError& e)
	{
		spdlog::critical("Application error: {}", e.what());
		std::string message = fmt::format("The following error occurred when initializing the application:\n{}", e.what());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), window.get());
		return 1;
	}
	catch (const CompileError& e)
	{
		spdlog::critical("Shader compilation error: {}", e.what());
		std::string message = fmt::format("The following error occurred when compiling shaders:\n{}", e.what());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), window.get());
		return 1;
	}


	return 0;
}

int SDL_main(int argc, char* argv[])
{
#ifdef _WIN32
	if (IsDebuggerPresent())
	{
		// Send log output to Visual Studio's Output window when running in the debugger.
		spdlog::set_default_logger(std::make_shared<spdlog::logger>("msvc", std::make_shared<spdlog::sinks::msvc_sink_mt>()));
	}
#endif

	spdlog::set_pattern("[%Y-%m-%D %H:%M:%S.%e] [%l] %v");

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		spdlog::critical("Couldn't initialise SDL: {}", SDL_GetError());
		return 1;
	}

	spdlog::info("SDL initialised successfully");

	int retcode = Run(argc, argv);

	SDL_Quit();

	return retcode;
}
