
#include "Framework/Buffer.h"

#include <cassert>

void Buffer::SetData(BytesView data)
{
	assert(allocation.mappedData);
	memcpy(allocation.mappedData, data.data(), data.size());
}
