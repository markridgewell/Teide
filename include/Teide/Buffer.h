
#pragma once

#include "Teide/AbstractBase.h"
#include "Teide/BasicTypes.h"
#include "Teide/BytesView.h"
#include "Teide/ForwardDeclare.h"

#include <algorithm>
#include <cstring>
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
    std::vector<byte> data;
};

template <TrivialObject T>
void AppendBytes(std::vector<byte>& bytes, const T& elem)
{
    const auto pos = bytes.size();
    bytes.resize(pos + sizeof(T));
    const auto source = std::as_bytes(std::span(&elem, 1));
    const auto dest = std::span(bytes).subspan(pos);
    std::ranges::copy(source, dest.begin());
}

template <TrivialObject T>
std::vector<byte> MakeBytes(std::initializer_list<T> init)
{
    std::vector<byte> ret;
    ret.reserve(init.size() * sizeof(T));
    for (const T& elem : init)
    {
        AppendBytes(ret, elem);
    }
    return ret;
}

class Buffer : AbstractBase
{
public:
    virtual usize GetSize() const = 0;
    virtual BytesView GetData() const = 0;
};

} // namespace Teide
