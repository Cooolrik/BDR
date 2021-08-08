#include "Vlk_Common.inl"

#include "Vlk_RayTracingBLASEntry.h"
#include "Vlk_VertexBuffer.h"
#include "Vlk_IndexBuffer.h"

unsigned int Vlk::RayTracingBLASEntry::GetGeometryCount() const
	{
	return (uint)AccelerationStructureGeometries.size();
	}

unsigned int Vlk::RayTracingBLASEntry::AddGeometry(
	IndexBuffer* indexBuffer, // index buffer to use
	VertexBuffer* vertexBuffer, // vertex buffer to use
	unsigned int vertexCoordAttributeIndex // index of the 3d coordinate data attribute in the vertex buffer (usually it is index 0)
	)
	{
	uint index = (uint)this->AccelerationStructureBuildRangeInfos.size();

	// get memory addresses of index and vertex buffers
	VkDeviceAddress vertexAddress = vertexBuffer->GetDeviceAddress();
	VkDeviceAddress indexAddress = indexBuffer->GetDeviceAddress();

	// basic geometry structure with flags
	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

	// setup triangles of geometry
	accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	accelerationStructureGeometry.geometry.triangles.vertexFormat = vertexBuffer->GetDescription().GetVertexInputAttributeDescriptions()[vertexCoordAttributeIndex].format;
	accelerationStructureGeometry.geometry.triangles.vertexStride = vertexBuffer->GetDescription().GetVertexInputBindingDescription().stride;
	accelerationStructureGeometry.geometry.triangles.vertexData.deviceAddress = vertexAddress;
	accelerationStructureGeometry.geometry.triangles.maxVertex = vertexBuffer->GetVertexCount();
	accelerationStructureGeometry.geometry.triangles.indexType = indexBuffer->GetIndexType();
	accelerationStructureGeometry.geometry.triangles.indexData.deviceAddress = indexAddress;

	// setup build range, the struct that defines which range of triangles in the input geometry to use
	// (for now, use all input triangles)
	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.primitiveCount = indexBuffer->GetIndexCount()/3;
	accelerationStructureBuildRangeInfo.transformOffset = 0;

	// add to blas array
	this->AccelerationStructureGeometries.push_back( accelerationStructureGeometry );
	this->AccelerationStructureBuildRangeInfos.push_back( accelerationStructureBuildRangeInfo );

	return index;
	}
