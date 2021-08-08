
#include "Vlk_Common.inl"

#include "Vlk_Buffer.h"

VkDeviceAddress Vlk::Buffer::GetDeviceAddress() const
	{
	VkBufferDeviceAddressInfo addressInfo{};
	addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	addressInfo.buffer = this->BufferHandle;
	return vkGetBufferDeviceAddress( this->Module->GetDevice(), &addressInfo );
	}

void* Vlk::Buffer::MapMemory()
	{
	void* memoryPtr;
	VLK_CALL( vmaMapMemory( this->Module->GetMemoryAllocator(), this->DeviceMemory, &memoryPtr ) );
	return memoryPtr;
	}

void Vlk::Buffer::UnmapMemory()
	{
	vmaUnmapMemory( this->Module->GetMemoryAllocator(), this->DeviceMemory );
	}

Vlk::Buffer::~Buffer()
	{
	if( this->DeviceMemory != nullptr )
		{
		vmaDestroyBuffer( this->Module->GetMemoryAllocator(), this->BufferHandle, this->DeviceMemory );
		}
	}
