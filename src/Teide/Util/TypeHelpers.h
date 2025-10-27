
#pragma once

namespace Teide
{

struct Immovable
{
    Immovable() = default;
    Immovable(const Immovable&) = delete;
    Immovable(Immovable&&) = delete;
    Immovable& operator=(const Immovable&) = delete;
    Immovable& operator=(Immovable&&) = delete;
    ~Immovable() = default;
};

} // namespace Teide
