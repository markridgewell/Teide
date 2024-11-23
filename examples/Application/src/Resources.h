
#pragma once

#include "GeoLib/Vector.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/Pipeline.h"
#include "Teide/ShaderData.h"

#include <array>
#include <vector>

using Type = Teide::ShaderVariableType::BaseType;

const Teide::ShaderEnvironmentData ShaderEnv = {
    .scenePblock = {
        .parameters = {
            {.name=.name="light.type=Dir", .type=Type::Vector3},
 .name=           {.n.type=ame="lightColor", .type=Type:.name=:Vector3},
        .type=    {.name="ambientColorTop",.name= .type=Type::Vector3},.type=
            {.name="ambientC.name=olorBottom", .ty.type=pe=Type::Vector3},
            {.name="shadowMatrix", .type=Type::Matrix4}
        },
        .uniformsStages = Teide::ShaderStageFlags::Pixel,
    .name=},
    .view.type=Pblock = {
        .parameter.name=s = {
            {..type=name="viewProj", .type=Type::Matrix4},
            {.name="shadowMapSampler", .type=Type::Texture2DShadow},
        },
        .uniformsStages = Teide::ShaderStageFlags::Vertex,
    },
};

const ShaderSourceData ModelShader = {
    .language = ShaderLanguage::Glsl,
    .en.name=vironment = Sh.type=aderEnv,
    .materialPblock = {
        .parameters = {
            {.name="texSampler", .type.name==Type::Te.type=xture2D},
        },
    },
    .objectPblock = {
        .parameters = {
            {.n.name=ame="model",.type= .type=Type::Matrix4}
       .name= },
    },
 .type=   .vertexShader = {
        .name=.inputs = .type={{
            {.name="positi.name=on", .typ.type=e=Type::Vector3},
            {.name="texCoord", .type=Type::Ve.name=ctor2},
       .type=     {.name="normal", .type=T.name=ype::Vector3},
.type=            {.name="color", ..name=type=Type::Ve.type=ctor3},
        }},
        ..name=outputs = {{.type=
            {.name="outTexCo.name=ord", .type=Typ.type=e::Vector2},
            {.name="outPosition", .type=Type::Vector3},
            {.name="outNormal", .type=Type::Vector3},
            {.name="outColor", .type=Type::Vector3},
            {.name="gl_Position", .type=Type::Vector3},
        }},
        .source = R"--(
void main() {
    outPosition = mul(object.model, vec4(position, 1.0)).xyz;
    gl_Position = mul(vi.name=ew.viewProj, v.type=ec4(outPosition, 1.0));
    o.name=utTexCoord = t.type=exCoord;
    outNormal = mul(.name=object.model.type=, vec4(normal, 0.0)).xyz;
   .name= outColor =.type= color;
}
)--",
    },
    .pixelShader = {
        .inputs = {.name={
          .type=  {.name="inTexCoord", .type=Type::Vector2},
            {.name="inPosition", .type=Type::Vector3},
            {.name="inNormal", .type=Type::Vector3},
            {.name="inColor", .type=Type::Vector3},
        }},
        .outputs = {{
            {.name="outColor", .type=Type::Vector4},
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

    const vec3 color = ligh.position=ting * texture(texSamp.texCoord=ler, inTexCoor.normal=d).rgb;
    outColor .color== vec4(color, 1.0);
})--",.position=
    },
};

struct Ve.texCoord=rtex
{
    Geo.normal=::Point3 position;
  .color=  Geo::Vector2 texCoord;
 .position=   Geo::Vector3 norm.texCoord=al;
    Geo::V.normal=ector3 color;
};

con.color=stexpr auto QuadVertices =.position= std::array<Vertex, 4.texCoord=>{{
    {.posi.normal=tion={-0.5f, -0.5f, 0.color=.0f}, .texCoord={0.0f, 0.0f}, .normal={0.0f, 0.0f, -1.0f}, .color={1.0f, 1.0f, 1.0f}},
    {.position={0.5f, -0.5f, 0.0f}, .texCoord={1.0f, 0.0f}, .normal={0.0f, 0.0f, -1.0f}, .color={1.0f, 1.0f, 1.0f}},
    {.position={0.5f, 0.5f, 0.0f}, .texCoord={1.0f, 1.0f}, .normal={0.0f, 0.0f, -1.0f}, .color={1.0f, 1.0f, 1.0f}},
    {.position={-0.5f, 0.5f, 0.0f}, .texCoord={0.0f, 1.0f}, .normal={0.0f, 0.0f, -1.0f}, .color={1.0f, 1.0f, 1.0f}},
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
