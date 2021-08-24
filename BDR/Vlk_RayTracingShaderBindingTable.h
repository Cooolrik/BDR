#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_RayTracingExtension.h"
#include "Vlk_Buffer.h"

namespace Vlk
    {
    class RayTracingShaderBindingTable : public RayTracingExtensionSubmodule
        {
        BDSubmoduleMacro( RayTracingShaderBindingTable, RayTracingExtensionSubmodule, RayTracingExtension );
        
        private:
            std::unique_ptr<Buffer> BufferPtr;

            VkStridedDeviceAddressRegionKHR RaygenDeviceAddress = {};
            VkStridedDeviceAddressRegionKHR MissDeviceAddress = {};
            VkStridedDeviceAddressRegionKHR ClosestHitDeviceAddress = {};
            VkStridedDeviceAddressRegionKHR CallableDeviceAddress = {};

        public:

            BDGetConstRefMacro( VkStridedDeviceAddressRegionKHR, RaygenDeviceAddress );
            BDGetConstRefMacro( VkStridedDeviceAddressRegionKHR, MissDeviceAddress );
            BDGetConstRefMacro( VkStridedDeviceAddressRegionKHR, ClosestHitDeviceAddress );
            BDGetConstRefMacro( VkStridedDeviceAddressRegionKHR, CallableDeviceAddress );

        };
    };
