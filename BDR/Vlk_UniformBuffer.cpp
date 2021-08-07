#include "Vlk_Common.inl"

#include "Vlk_UniformBuffer.h"

void Vlk::UniformBuffer::UpdateBuffer( void* data )
	{
	void* pbufferdata;

	VLK_CALL( vmaMapMemory( this->Parent->GetMemoryAllocator(), this->DeviceMemory, &pbufferdata ) );
	memcpy( pbufferdata, data, this->BufferSize );
	vmaUnmapMemory( this->Parent->GetMemoryAllocator(), this->DeviceMemory );
	}
