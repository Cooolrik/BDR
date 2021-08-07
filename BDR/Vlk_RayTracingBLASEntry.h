#pragma once

namespace Vlk
    {
    class RayTracingBLASEntry
        {
        private:
            std::vector<VkAccelerationStructureGeometryKHR> AccelerationStructureGeometries;
            std::vector<VkAccelerationStructureBuildRangeInfoKHR> AccelerationStructureBuildRangeInfos;
    
        public:

            // returns total number of geoms
            unsigned int GetGeometryCount() const;

            // returns index of added geometry in the lists
            unsigned int AddGeometry(
                IndexBuffer* indexBuffer, // index buffer to use
                VertexBuffer* vertexBuffer, // vertex buffer to use
                unsigned int vertexCoordAttributeIndex = 0// index of the 3d coordinate data attribute in the vertex buffer (usually it is index 0)
            );

            // get const references on the full list
            BDGetConstRefMacro( std::vector<VkAccelerationStructureGeometryKHR>, AccelerationStructureGeometries );
            BDGetConstRefMacro( std::vector<VkAccelerationStructureBuildRangeInfoKHR>, AccelerationStructureBuildRangeInfos );
        };
    };
