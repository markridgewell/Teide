
#pragma once

namespace Teide
{

struct MoveOnly
{
    MoveOnly() = default;
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) = default;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly& operator=(MoveOnly&&) = default;
    ~MoveOnly() = default;
};

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
