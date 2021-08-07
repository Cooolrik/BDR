#pragma once

#include "Vlk_Renderer.h"
#include "Vlk_Buffer.h"

namespace Vlk
    {
    class RayTracingAccBuffer : public BufferBase
        {
        private:
            RayTracingAccBuffer() = default;
            RayTracingAccBuffer( const RayTracingAccBuffer& other );
            friend class RayTracingExtension;

            VkAccelerationStructureKHR AccelerationStructure = nullptr;

        public:

            // dtor
            ~RayTracingAccBuffer();

            BDGetMacro( VkAccelerationStructureKHR, AccelerationStructure );
        };
    };
