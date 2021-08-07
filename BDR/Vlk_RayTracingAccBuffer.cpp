#include "Vlk_RayTracingAccBuffer.h"
#include "Vlk_RayTracingExtension.h"

Vlk::RayTracingAccBuffer::~RayTracingAccBuffer()
	{
	if( this->AccelerationStructure != nullptr )
		{
		RayTracingExtension::vkDestroyAccelerationStructureKHR( this->Parent->GetDevice(), this->AccelerationStructure, nullptr );
		}
	}
