
#pragma once

#include "Teide/Handle.h"

namespace Teide
{

class Kernel final : public Handle<>
{
public:
    using Handle::Handle;

    bool operator==(const Kernel&) const = default;
};

} // namespace Teide
