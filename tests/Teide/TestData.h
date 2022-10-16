
#include "ShaderCompiler/ShaderCompiler.h"
#include "Teide/ShaderData.h"

#include <string>

inline const std::string SimpleVertexShader = R"--(
void main() {
    gl_Position = inPosition;
})--";

inline const std::string SimplePixelShader = R"--(
void main() {
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
})--";

inline const std::string PixelShaderWithMaterialParams = R"--(
void main() {
    outColor = material.color;
})--";

inline const std::string VertexShaderWithObjectParams = R"--(
void main() {
    gl_Position = object.mvp * inPosition;
})--";

using Type = ShaderVariableType::BaseType;

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
