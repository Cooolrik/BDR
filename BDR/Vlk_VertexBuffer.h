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

            BDGetMacro( VkVertexInputBindingDescription, VertexInputBindingDescription );
            BDGetMacro( std::vector<VkVertexInputAttributeDescription>, VertexInputAttributeDescriptions );
        };

    class VertexBuffer : public Buffer
        {
        BDSubmoduleMacro( VertexBuffer, Buffer, Renderer );

        private:
            VertexBufferDescription Description;

        public:
            unsigned int GetVertexCount() const
                {
                return (unsigned int)( this->BufferSize / this->GetVertexBufferBindingDescription().stride );
                }

            VkVertexInputBindingDescription GetVertexBufferBindingDescription() const
                {
                return this->Description.GetVertexInputBindingDescription();
                }

            std::vector<VkVertexInputAttributeDescription> GetVertexAttributeDescriptions() const
                {
                return this->Description.GetVertexInputAttributeDescriptions();
                }
        };
    };
