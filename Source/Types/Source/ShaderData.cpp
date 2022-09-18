
#include "Types/ShaderData.h"

#ifdef __GNUC__ // GCC 4.8+, Clang, Intel and other compilers compatible with GCC (-std=c++0x or above)
[[noreturn]] inline __attribute__((always_inline)) void Unreachable()
{
	__builtin_unreachable();
}
#elif defined(_MSC_VER) // MSVC
[[noreturn]] __forceinline void Unreachable()
{
	__assume(false);
}
#endif

namespace
{
constexpr auto RoundUp(std::integral auto a, std::integral auto b)
{
	return ((a - 1) / b + 1) * b;
}
} // namespace

bool IsResourceType(ShaderVariableType::BaseType type)
{
	switch (type)
	{
		using enum ShaderVariableType::BaseType;
		case Float:
		case Vector2:
		case Vector3:
		case Vector4:
		case Matrix4:
			return false;
		case Texture2D:
		case Texture2DShadow:
			return true;
	}
	Unreachable();
}

SizeAndAlignment GetSizeAndAlignment(ShaderVariableType::BaseType type)
{
	switch (type)
	{
		using enum ShaderVariableType::BaseType;
		case Float:
			return {.size = sizeof(float), .alignment = sizeof(float)};

		case Vector2:
			return {.size = sizeof(float) * 2, .alignment = sizeof(float) * 2};
		case Vector3:
			return {.size = sizeof(float) * 4, .alignment = sizeof(float) * 4};
		case Vector4:
			return {.size = sizeof(float) * 4, .alignment = sizeof(float) * 4};
		case Matrix4:
			return {.size = sizeof(float) * 4 * 4, .alignment = sizeof(float) * 4};

		case Texture2D:
		case Texture2DShadow:
			return {};
	}
	Unreachable();
}

SizeAndAlignment GetSizeAndAlignment(ShaderVariableType type)
{
	if (type.arraySize != 0 && type.baseType == ShaderVariableType::BaseType::Vector3)
	{
		const std::uint32_t arraySize = sizeof(float) * 3 * type.arraySize;
		return {.size = arraySize, .alignment = arraySize};
	}
	const auto [size, alignment] = GetSizeAndAlignment(type.baseType);
	return {size * std::max(1u, type.arraySize), alignment};
}

std::string ToString(ShaderVariableType::BaseType type)
{
	switch (type)
	{
		using enum ShaderVariableType::BaseType;
		case Float:
			return "float";
		case Vector2:
			return "vec2";
		case Vector3:
			return "vec3";
		case Vector4:
			return "vec4";
		case Matrix4:
			return "mat4";
		case Texture2D:
			return "sampler2D";
		case Texture2DShadow:
			return "sampler2DShadow";
	}
	Unreachable();
}

std::string ToString(ShaderVariableType type)
{
	std::string ret = ToString(type.baseType);
	if (type.arraySize != 0)
	{
		ret += '[';
		ret += std::to_string(type.arraySize);
		ret += ']';
	}
	return ret;
}

ParameterBlockLayout BuildParameterBlockLayout(const ParameterBlockDesc& pblock, int set)
{
	ParameterBlockLayout bindings;
	bindings.uniformsStages = pblock.uniformsStages;

	for (const auto& parameter : pblock.parameters)
	{
		if (IsResourceType(parameter.type.baseType))
		{
			bindings.textureCount++;
		}
		else
		{
			const auto [size, alignment] = GetSizeAndAlignment(parameter.type);
			const auto offset = RoundUp(bindings.uniformsSize, alignment);
			bindings.uniformDescs.push_back(UniformDesc{.name = parameter.name, .type = parameter.type, .offset = offset});
			bindings.uniformsSize = offset + size;
		}
	}

	if (set == 3 && bindings.uniformsSize <= 128u)
	{
		bindings.isPushConstant = true;
	}

	return bindings;
}
