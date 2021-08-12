#include "Vlk_RayTracingAccelerationStructure.h"
#include "Vlk_Buffer.h"

Vlk::RayTracingAccelerationStructure::~RayTracingAccelerationStructure()
	{
	if( this->AccelerationStructure != nullptr )
		{
		RayTracingExtension::vkDestroyAccelerationStructureKHR( this->Module->GetParent()->GetDevice(), this->AccelerationStructure, nullptr );
		}
	}
