
#include "ShaderCompiler/ShaderCompiler.h"
#include "Types/ShaderData.h"

#include <string>

inline const std::string SimpleVertexShader = R"--(
void main() {
    gl_Position = inPosition;
})--";

inline const std::string SimplePixelShader = R"--(
void main() {
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
})--";

inline const std::string VertexShaderWithParams = R"--(
void main() {
    gl_Position = scene.mvp * inPosition;
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

inline const ShaderSourceData ShaderWithParams = {
    .language = ShaderLanguage::Glsl,
    .scenePblock = {
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
        .source = VertexShaderWithParams,
    },
    .pixelShader = {
        .outputs = {{
            {"outColor", Type::Vector4},
        }},
        .source = SimplePixelShader,
    },
};
