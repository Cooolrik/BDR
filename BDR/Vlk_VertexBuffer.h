#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"
#include "Vlk_Buffer.h"

namespace Vlk
    {
    class VertexBufferDescription
        {
        private:
            VkVertexInputBindingDescription VertexInputBindingDescription{};
            std::vector<VkVertexInputAttributeDescription> VertexInputAttributeDescriptions;

        public:
            void SetVertexInputBindingDescription( uint32_t binding, uint32_t stride, VkVertexInputRate inputRate );
            void AddVertexInputAttributeDescription( uint32_t binding, uint32_t location, VkFormat format , uint32_t offset );

            BDGetConstRefMacro( VkVertexInputBindingDescription, VertexInputBindingDescription );
            BDGetConstRefMacro( std::vector<VkVertexInputAttributeDescription>, VertexInputAttributeDescriptions );
        };

    class VertexBuffer : public Buffer
        {
        BDSubmoduleMacro( VertexBuffer, Buffer, Renderer );

        private:
            VertexBufferDescription Description;

        public:
            unsigned int GetVertexCount() const
                {
                return (unsigned int)( this->BufferSize / Description.GetVertexInputBindingDescription().stride );
                }

            BDGetConstRefMacro( VertexBufferDescription , Description );
        };

    class VertexBufferTemplate : public BufferTemplate
        {
        public:
            // the vertex description to use for constructing the buffer
            VertexBufferDescription Description;

            /////////////////////////////////

            // create a vertex buffer in GPU memory
            static VertexBufferTemplate VertexBuffer(
                const VertexBufferDescription& description, 
                uint vertexCount, 
                const void* src_data = nullptr,
                VkBufferUsageFlags additionalBufferUsageFlags = 0 // eg VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            );

        };

    };
