#include "Vlk_Common.inl"

#include "Vlk_UniformBuffer.h"

Vlk::UniformBuffer::~UniformBuffer()
	{
	}

void Vlk::UniformBuffer::UpdateBuffer( void* data )
	{
	void* pbufferdata;

	VLK_CALL( vmaMapMemory( this->Module->GetMemoryAllocator(), this->Allocation, &pbufferdata));
	memcpy( pbufferdata, data, this->BufferSize );
	vmaUnmapMemory( this->Module->GetMemoryAllocator(), this->Allocation );
	}
