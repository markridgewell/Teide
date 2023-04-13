
#pragma once

namespace Teide
{
struct AbstractBase
{
    AbstractBase() = default;
    virtual ~AbstractBase() = default;

    AbstractBase(const AbstractBase&) = default;
    AbstractBase(AbstractBase&&) = default;
    AbstractBase& operator=(const AbstractBase&) = default;
    AbstractBase& operator=(AbstractBase&&) = default;
};
} // namespace Teide
