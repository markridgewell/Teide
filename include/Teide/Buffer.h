
#pragma once

#include "Teide/BytesView.h"
#include "Teide/ForwardDeclare.h"

#include <cstddef>
#include <span>

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
	BytesView data;
};

class Buffer
{
public:
	virtual ~Buffer() = default;

	virtual std::size_t GetSize() const = 0;
	virtual BytesView GetData() const = 0;
};

} // namespace Teide
