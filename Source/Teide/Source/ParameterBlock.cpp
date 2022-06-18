
#include "Teide/ParameterBlock.h"

#include "Teide/Buffer.h"

void ParameterBlock::SetUniformData(int currentFrame, BytesView data)
{
	uniformBuffer->SetData(currentFrame, data);
}
