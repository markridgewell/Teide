
#include "Teide/Handle.h"

#include <gmock/gmock.h>

namespace Teide
{

class MockRefCounter : public RefCounter
{
public:
    MOCK_METHOD(void, AddRef, (uint64 index), (noexcept));
    MOCK_METHOD(void, DecRef, (uint64 index), (noexcept));
};

} // namespace Teide
