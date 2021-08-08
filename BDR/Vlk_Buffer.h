#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"

namespace Vlk
    {
    class Buffer : public RendererSubmodule
        {
        BDSubmoduleMacro( Buffer, RendererSubmodule, Renderer );

        protected:
            VkBuffer BufferHandle = nullptr;
            VmaAllocation DeviceMemory = nullptr;
            VkDeviceSize BufferSize = 0;

        public:

            // returns the device address of the buffer.
            VkDeviceAddress GetDeviceAddress() const;

            // map/unmap (only host-visible buffers)
            void* MapMemory();
            void UnmapMemory();

            BDGetCustomNameMacro( VkBuffer, Buffer, BufferHandle );
            BDGetMacro( VmaAllocation, DeviceMemory );
            BDGetMacro( VkDeviceSize, BufferSize );
        };

    };
