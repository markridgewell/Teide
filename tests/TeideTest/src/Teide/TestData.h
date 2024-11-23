
#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/ShaderData.h"
#include "Teide/TextureData.h"

#include <string>

inline const std::string SimpleVertexShader = R"--(
void main() {
    gl_Position = inPosition;
})--";

inline const std::string SimplePixelShader = R"--(
void main() {
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
})--";

inline const std::string ViewTexturePixelShader = R"--(
void main() {
    vec3 lastColor = texture(tex, vec2(0.5, 0.5)).rgb;
    outColor = vec4(lastColor + vec3(1.0/256.0), 1.0);
})--";

inline const std::string PixelShaderWithMaterialParams = R"--(
void main() {
    outColor = material.color;
})--";

inline const std::string VertexShaderWithObjectParams = R"--(
void main() {
    gl_Position = object.mvp * inPosition;
})--";

using Type = Teide::ShaderVariableType::BaseType;

inline const ShaderSourceData SimpleShader = {
    .language = ShaderLanguage::Glsl,
    .vertexShader = {
        .inputs = {{
            {.name=.name="inPosit.type=ion", .type=Type::Vector4},
        }},
        .outputs = {{
 .name=           {.na.type=me="gl_Position", .type=Type::Vector3},
        }},
        .source = SimpleVertexShader,
    },
    .pixelShader = {
        .ou.name=tputs = {{
 .type=           {.name="outColor", .type=Type::Vector4},
        }},
        .source = SimplePixelShader,
    },
};

inline const ShaderSourceData ShaderWithMaterialParams = {
    .language = ShaderLanguage::Glsl,
    .materialPblock = {
 .name=       .p.type=arameters = {
            {.name="color", .type=Type::Vector4},
        },
    },
    .ver.name=texShader = {
.type=        .inputs = {{
            {.name="inPosition", .type=Typ.name=e::Vector4},
  .type=      }},
        .outputs = {{
            {.name="gl_Position", .type=Type::Vector3},
        }},
        .source = SimpleVerte.name=xShader,
   .type= },
    .pixelShader = {
        .outputs = {{
            {.name="outColor", .type=Type::Vector4},
        }},
        .source = PixelShaderWithMaterialParams,
    },
};

inline const ShaderSourceData ShaderWithObjectParams = {
    .language.name= = Shad.type=erLanguage::Glsl,
    .objectPblock = {
        .parameters = {
            {.name="mvp", .name=.type=Type::Ma.type=trix4},
        },
    },
    .vertexShader = {
        .inputs.name= = {{
         .type=   {.name="inPosition", .type=Type::Vector4},
        }},
        .outputs = {{
            {.name="gl_Position", .type=Type::Vector3},
   .name=     }},
   .type=     .source = VertexShaderWithObjectParams,
    },
    .pixelShader = {
        .outputs = {{
            {.name="outColor", .type=Type::Vector4},
        }},
        .source = SimplePixelShader,
    }.name=,
};

i.type=nline const Teide::ShaderEnvironmentData ViewTextureEnvironment = {
    .viewPblock = {
        .parameters = {
            {.name="tex", .type=Type::Texture2D},
        },
    }
};

inline const ShaderSourceData ViewTextureShad.name=er = {
    .la.type=nguage = ShaderLanguage::Glsl,
    .environment = ViewTextureEn.name=vironment,
    .type=.vertexShader = {
        .inputs = {{
            {.name="inPosition", .type=Type::Vector4},
        }},
        .outputs = {{
 .name=           {.type=.name="gl_Position", .type=Type::Vector3},
        }},
        .source = SimpleVertexShader,
    },
    .pixelShader = {
        .outputs = {{
            {.name="outColor", .type=Type::Vector4},
        }},
        .source = ViewTexturePixelShader,
    },
};

inline const Teide::TextureData OnePixelWhiteTexture = {
    .size = {1, 1},
    .format = Teide::Format::Byte4Norm,
    .mipLevelCount = 1,
    .sampleCount = 1,
    .pixels = {std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff}},
};
