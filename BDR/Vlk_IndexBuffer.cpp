#include "Vlk_Common.inl"
#include "Vlk_IndexBuffer.h"

Vlk::IndexBuffer::~IndexBuffer()
	{
	}

Vlk::IndexBufferTemplate Vlk::IndexBufferTemplate::IndexBuffer( VkIndexType indexType, uint indexCount, const void* src_data, VkBufferUsageFlags additionalBufferUsageFlags )
	{
	// calc the size of the buffer
	VkDeviceSize bufferSize;
	if(indexType == VK_INDEX_TYPE_UINT32)
		bufferSize = sizeof( uint32_t ) * indexCount;
	else if(indexType == VK_INDEX_TYPE_UINT16)
		bufferSize = sizeof( uint16_t ) * indexCount;
	else if(indexType == VK_INDEX_TYPE_UINT8_EXT)
		bufferSize = sizeof( uint8_t ) * indexCount;
	else
		{
		throw runtime_error( "Error: IndexBufferTemplate::IndexBuffer() indexType is unrecognized." );
		}

	// init the template with the base buffer
	IndexBufferTemplate ret = {
		BufferTemplate::ManualBuffer(
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | additionalBufferUsageFlags,
			VMA_MEMORY_USAGE_GPU_ONLY,
			bufferSize,
			src_data
			)
		};

	// set the additional info: index type 
	ret.IndexType = indexType;

	return ret;
	}
