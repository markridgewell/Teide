
#pragma once

#include "Teide/BytesView.h"
#include "Teide/ForwardDeclare.h"

#include <cstddef>
#include <initializer_list>
#include <vector>

namespace Teide
{

enum class BufferUsage
{
	Generic,
	Vertex,
	Index,
	Uniform,
};

struct BufferData
{
	BufferUsage usage = BufferUsage::Generic;
	ResourceLifetime lifetime = ResourceLifetime::Permanent;
	std::vector<std::byte> data;
};

template <TrivialObject T>
void AppendBytes(std::vector<std::byte>& bytes, const T& elem)
{
	const auto pos = bytes.size();
	bytes.resize(pos + sizeof(T));
	std::memcpy(bytes.data() + pos, &elem, sizeof(T));
}

template <TrivialObject T>
std::vector<std::byte> MakeBytes(std::initializer_list<T> init)
{
	std::vector<std::byte> ret;
	ret.reserve(init.size() * sizeof(T));
	for (const T& elem : init)
	{
		AppendBytes(ret, elem);
	}
	return ret;
}

class Buffer
{
public:
	virtual ~Buffer() = default;

	virtual std::size_t GetSize() const = 0;
	virtual BytesView GetData() const = 0;
};

} // namespace Teide
