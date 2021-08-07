#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"
#include "Vlk_Buffer.h"

namespace Vlk
    {
    class RayTracingShaderBindingTable : public BufferBase
        {
        private:
            RayTracingShaderBindingTable() = default;
            RayTracingShaderBindingTable( const RayTracingShaderBindingTable& other );
            friend class Renderer;
            friend class RayTracingPipeline;

            VkStridedDeviceAddressRegionKHR RaygenDeviceAddress;
            VkStridedDeviceAddressRegionKHR MissDeviceAddress;
            VkStridedDeviceAddressRegionKHR ClosestHitDeviceAddress;
            VkStridedDeviceAddressRegionKHR CallableDeviceAddress;

        public:

            BDGetConstRefMacro( VkStridedDeviceAddressRegionKHR, RaygenDeviceAddress );
            BDGetConstRefMacro( VkStridedDeviceAddressRegionKHR, MissDeviceAddress );
            BDGetConstRefMacro( VkStridedDeviceAddressRegionKHR, ClosestHitDeviceAddress );
            BDGetConstRefMacro( VkStridedDeviceAddressRegionKHR, CallableDeviceAddress );

        };
    };
