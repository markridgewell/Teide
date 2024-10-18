
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
            {"inPosition", Type::Vector4},
        }},
        .outputs = {{
            {"gl_Position", Type::Vector3},
        }},
        .source = SimpleVertexShader,
    },
    .pixelShader = {
        .outputs = {{
            {"outColor", Type::Vector4},
        }},
        .source = SimplePixelShader,
    },
};

inline const ShaderSourceData ShaderWithMaterialParams = {
    .language = ShaderLanguage::Glsl,
    .materialPblock = {
        .parameters = {
            {"color", Type::Vector4},
        },
    },
    .vertexShader = {
        .inputs = {{
            {"inPosition", Type::Vector4},
        }},
        .outputs = {{
            {"gl_Position", Type::Vector3},
        }},
        .source = SimpleVertexShader,
    },
    .pixelShader = {
        .outputs = {{
            {"outColor", Type::Vector4},
        }},
        .source = PixelShaderWithMaterialParams,
    },
};

inline const ShaderSourceData ShaderWithObjectParams = {
    .language = ShaderLanguage::Glsl,
    .objectPblock = {
        .parameters = {
            {"mvp", Type::Matrix4},
        },
    },
    .vertexShader = {
        .inputs = {{
            {"inPosition", Type::Vector4},
        }},
        .outputs = {{
            {"gl_Position", Type::Vector3},
        }},
        .source = VertexShaderWithObjectParams,
    },
    .pixelShader = {
        .outputs = {{
            {"outColor", Type::Vector4},
        }},
        .source = SimplePixelShader,
    },
};

inline const Teide::ShaderEnvironmentData ViewTextureEnvironment = {
    .viewPblock = {
        .parameters = {
            {"tex", Type::Texture2D},
        },
    }
};

inline const ShaderSourceData ViewTextureShader = {
    .language = ShaderLanguage::Glsl,
    .environment = ViewTextureEnvironment,
    .vertexShader = {
        .inputs = {{
            {"inPosition", Type::Vector4},
        }},
        .outputs = {{
            {"gl_Position", Type::Vector3},
        }},
        .source = SimpleVertexShader,
    },
    .pixelShader = {
        .outputs = {{
            {"outColor", Type::Vector4},
        }},
        .source = ViewTexturePixelShader,
    },
};

const KernelSourceData SimpleKernel = {
    .language = ShaderLanguage::Glsl,
    .kernelShader = {
        .outputs = {{
            {"result", Type::Float},
        }},
        .source = R"--(
            void main() {
                result = 42.0f;
            }
        )--",
    },
};

inline const Teide::TextureData OnePixelWhiteTexture = {
    .size = {1, 1},
    .format = Teide::Format::Byte4Norm,
    .mipLevelCount = 1,
    .sampleCount = 1,
    .pixels = {std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff}},
};
