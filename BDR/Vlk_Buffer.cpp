
#include "Vlk_Common.inl"

#include "Vlk_Buffer.h"

VkDeviceAddress Vlk::BufferBase::GetDeviceAddress() const
	{
	VkBufferDeviceAddressInfo addressInfo{};
	addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	addressInfo.buffer = this->BufferHandle;
	return vkGetBufferDeviceAddress( this->Parent->GetDevice(), &addressInfo );
	}

void* Vlk::BufferBase::MapMemory()
	{
	void* memoryPtr;
	VLK_CALL( vmaMapMemory( this->Parent->GetMemoryAllocator(), this->DeviceMemory, &memoryPtr ) );
	return memoryPtr;
	}

void Vlk::BufferBase::UnmapMemory()
	{
	vmaUnmapMemory( this->Parent->GetMemoryAllocator(), this->DeviceMemory );
	}

Vlk::BufferBase::~BufferBase()
	{
	if( this->DeviceMemory != nullptr )
		{
		vmaDestroyBuffer( this->Parent->GetMemoryAllocator(), this->BufferHandle, this->DeviceMemory );
		}
	}
