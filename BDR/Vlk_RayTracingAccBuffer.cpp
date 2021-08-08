#include "Vlk_RayTracingAccBuffer.h"
#include "Vlk_Buffer.h"

Vlk::RayTracingAccBuffer::~RayTracingAccBuffer()
	{
	if( this->AccelerationStructure != nullptr )
		{
		RayTracingExtension::vkDestroyAccelerationStructureKHR( this->Module->GetParent()->GetDevice(), this->AccelerationStructure, nullptr );
		}
	}
