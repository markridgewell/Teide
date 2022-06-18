
#include "Framework/ParameterBlock.h"

#include "Framework/Buffer.h"

void ParameterBlock::SetUniformData(int currentFrame, BytesView data)
{
	uniformBuffer->SetData(currentFrame, data);
}
