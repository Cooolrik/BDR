#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"
#include "Vlk_Buffer.h"

namespace Vlk
    {
    class IndexBuffer : public Buffer
        {
        BDSubmoduleMacro( IndexBuffer, Buffer, Renderer );

        private:
            VkIndexType IndexType = {};

        public:

            unsigned int GetIndexCount() const
                {
                if( this->IndexType == VK_INDEX_TYPE_UINT32 )
                    return (unsigned int) (this->BufferSize / sizeof( uint32_t ));
                else if(this->IndexType == VK_INDEX_TYPE_UINT16)
                    return (unsigned int)( this->BufferSize / sizeof( uint16_t ) );
                else // assume 8-bit indices
                    return (unsigned int)(this->BufferSize);
                }

            BDGetMacro( VkIndexType, IndexType );
        };

    class IndexBufferTemplate : public BufferTemplate
        {
        public:
            // the index type to use for the buffer
            VkIndexType IndexType = {};

            /////////////////////////////////

            // create a vertex buffer in GPU memory
            static IndexBufferTemplate IndexBuffer(
                VkIndexType indexType,
                uint indexCount,
                const void* src_data = nullptr,
                VkBufferUsageFlags additionalBufferUsageFlags = 0 // eg VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            );

        };
    };
