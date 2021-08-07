#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"
#include "Vlk_Buffer.h"

namespace Vlk
    {
    class UniformBuffer : public BufferBase
        {
        private:
            UniformBuffer() = default;
            UniformBuffer( const UniformBuffer& other );
            friend class Renderer;

        public:

            // update buffer with data. The size of the data must match the size when the buffer was created.
            void UpdateBuffer( void* data );

        };
    };
