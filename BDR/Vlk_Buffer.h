#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"

namespace Vlk
    {
    class BufferBase
        {
        protected:
            BufferBase() = default;
            BufferBase( const BufferBase& other );
            friend class Renderer;

            const Renderer* Parent = nullptr;
            VkBuffer BufferHandle = nullptr;
            VmaAllocation DeviceMemory = nullptr;
            VkDeviceSize BufferSize = 0;

        public:

            // returns the device address of the buffer.
            VkDeviceAddress GetDeviceAddress() const;

            // dtor, removes any allocated buffer
            ~BufferBase();

            // map/unmap (only host-visible buffers)
            void* MapMemory();
            void UnmapMemory();

            BDGetMacro( Renderer*, Parent );
            BDGetCustomNameMacro( VkBuffer, Buffer, BufferHandle );
            BDGetMacro( VmaAllocation, DeviceMemory );
            BDGetMacro( VkDeviceSize, BufferSize );
        };
    };
