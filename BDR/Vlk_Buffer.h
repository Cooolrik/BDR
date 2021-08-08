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
            VmaAllocation Allocation = nullptr;
            VkDeviceSize BufferSize = 0;

        public:

            // returns the device address of the buffer.
            VkDeviceAddress GetDeviceAddress() const;

            // map/unmap (only host-visible buffers)
            void* MapMemory();
            void UnmapMemory();

            BDGetCustomNameMacro( VkBuffer, Buffer, BufferHandle );
            BDGetMacro( VmaAllocation, Allocation );
            BDGetMacro( VkDeviceSize, BufferSize );
        };

    class BufferTemplate
        {
        public:
            // initial create information
            VkBufferCreateInfo BufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };

            // vma allocation object
            VmaAllocationCreateInfo AllocationCreateInfo = {};

            // if an upload is to be made (either using a staging buffer, or mapping the memory directly)
            const void* UploadSourcePtr = nullptr;
            VkDeviceSize UploadSourceSize = 0;
            std::vector<VkBufferCopy> UploadBufferCopies = {};

            /////////////////////////////////

            // create a buffer manually, and (optionally) copy the whole data size from a memory address
            static BufferTemplate ManualBuffer(
                VkBufferUsageFlags bufferUsageFlags, 
                VmaMemoryUsage memoryPropertyFlags, 
                VkDeviceSize bufferSize, 
                const void* src_data = nullptr
                );

            // create a uniform buffer, CPU side but GPU readable
            static BufferTemplate UniformBuffer(
                VkDeviceSize bufferSize,
                const void* src_data = nullptr
            );

        };

    };
