#pragma once

#include "Vlk_Renderer.h"

namespace Vlk
    {
    class RayTracingTLASEntry
        {
        public:
            // Index of the BLAS that this instance references
            uint32_t BlasId = 0;     

            // Application-custom Instance Index (will be presented in shader as gl_InstanceCustomIndexEXT, only lower 24 bits are valid)
            uint32_t InstanceCustomIndex = 0;

            // Hit group id in the acceleration structure
            uint32_t HitGroupId = 0;        

            // Instance Visibility mask, 8 bits, will be ANDed with ray mask 
            uint32_t Mask = 0xff;

            // Instance flags
            VkGeometryInstanceFlagsKHR Flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

            // Transformation matrix, set to identitiy by default
            float Transformation[3][4] = { {1,0,0,0} , {0,1,0,0} , {0,0,1,0} };
        };
    };
