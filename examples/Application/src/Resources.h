
#pragma once

#include "GeoLib/Vector.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/Pipeline.h"
#include "Teide/ShaderData.h"

#include <array>

using Type = Teide::ShaderVariableType::BaseType;

const Teide::ShaderEnvironmentData ShaderEnv = {
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
    .environment = ShaderEnv,
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

struct Vertex
{
    Geo::Point3 position;
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
