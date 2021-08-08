#include "Vlk_Common.inl"

#include "Vlk_VertexBuffer.h"

Vlk::VertexBuffer::~VertexBuffer()
	{
	}

void Vlk::VertexBufferDescription::SetVertexInputBindingDescription( uint32_t binding, uint32_t stride, VkVertexInputRate inputRate )
	{
	this->VertexInputBindingDescription.binding = binding;
	this->VertexInputBindingDescription.stride = stride;
	this->VertexInputBindingDescription.inputRate = inputRate;
	}

void Vlk::VertexBufferDescription::AddVertexInputAttributeDescription( uint32_t binding, uint32_t location, VkFormat format, uint32_t offset )
	{
	VkVertexInputAttributeDescription desc;

	desc.binding = binding;
	desc.location = location;
	desc.format = format;
	desc.offset = offset;

	this->VertexInputAttributeDescriptions.push_back( desc );
	}


