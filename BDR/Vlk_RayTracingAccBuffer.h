#pragma once

#include "Vlk_RayTracingExtension.h"

namespace Vlk
    {
    class RayTracingAccBuffer : public RayTracingExtensionSubmodule
        {
        BDSubmoduleMacro( RayTracingAccBuffer , RayTracingExtensionSubmodule, RayTracingExtension );

        private:
            std::unique_ptr<Buffer> BufferPtr;
            VkAccelerationStructureKHR AccelerationStructure = nullptr;

        public:
            BDGetMacro( VkAccelerationStructureKHR, AccelerationStructure );
        };
    };
