#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"
#include "Vlk_Image.h"

namespace Vlk
    {
    class RayTracingShaderBindingTable;
    class Pipeline;
    class VertexBuffer;
    class IndexBuffer;

    class CommandPool : public RendererSubmodule
        {
        BDSubmoduleMacro( CommandPool, RendererSubmodule, Renderer );

        private:
            VkCommandPool Pool = nullptr; 
            std::vector<VkCommandBuffer> Buffers;
            int CurrentBufferIndex = -1;
            bool IsRecordingBuffer = false;

            std::vector<VkBufferMemoryBarrier> BufferMemoryBarriers;
            std::vector<VkImageMemoryBarrier> ImageMemoryBarriers;

        public:

            void ResetCommandPool();

            VkCommandBuffer BeginCommandBuffer();
            void EndCommandBuffer();

            void BeginRenderPass( VkFramebuffer destFramebuffer );
            void EndRenderPass();

            void BindPipeline( Pipeline* pipeline );

            void BindVertexBuffer( VertexBuffer* buffer );
            void BindIndexBuffer( IndexBuffer* buffer );

            void BindDescriptorSet( Pipeline* pipeline, VkDescriptorSet set );
 
            void PushConstants( Pipeline* pipeline, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues );
 
            void SetViewport( VkViewport viewport );
            void SetViewport( float x, float y, float width, float height, float minDepth = 0.f, float maxDepth = 1.f );

            void SetScissorRectangle( VkRect2D scissorRectangle );
            void SetScissorRectangle( int32_t x, int32_t y, uint32_t width, uint32_t height );

            void UpdateBuffer( Buffer* buffer, VkDeviceSize dstOffset, uint32_t dataSize, const void* pData );

            void Draw( uint vertexCount );
            
            void DrawIndexed(
                uint indexCount,
                uint instanceCount,
                uint firstIndex,
                int vertexOffset,
                uint firstInstance
                );

            void DrawIndexedIndirect(
                const Buffer *buffer,
                VkDeviceSize offset,
                uint drawCount,
                uint stride
                );

            void QueueUpBufferMemoryBarrier( 
                VkBuffer      buffer,
                VkAccessFlags srcAccessMask,
                VkAccessFlags dstAccessMask,
                VkDeviceSize  offset,
                VkDeviceSize  size
                );

            void QueueUpBufferMemoryBarrier(
                const Buffer* buffer,
                VkAccessFlags srcAccessMask,
                VkAccessFlags dstAccessMask,
                VkDeviceSize  offset = 0,
                VkDeviceSize  size = VkDeviceSize( ~0 )
              );

            void QueueUpImageMemoryBarrier(
                VkImage image,
                VkImageLayout oldLayout,
                VkImageLayout newLayout,
                VkAccessFlags srcAccessMask,
                VkAccessFlags dstAccessMask,
                VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
                );

            void QueueUpImageMemoryBarrier(
                const Image* image,
                VkImageLayout oldLayout,
                VkImageLayout newLayout,
                VkAccessFlags srcAccessMask,
                VkAccessFlags dstAccessMask,
                VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
                );

            // creates a command barries with queued up barriers
            void PipelineBarrier(
                VkPipelineStageFlags srcStageMask,
                VkPipelineStageFlags dstStageMask
                );

            void DispatchCompute( 
                uint32_t groupCountX, 
                uint32_t groupCountY = 1, 
                uint32_t groupCountZ = 1 
                );

            // Ray tracing extension
            void TraceRays( RayTracingShaderBindingTable* sbt , uint width, uint height );

            // Mesh shader extension
            void DrawMeshTasks( uint taskCount, uint firstTask );

            BDGetMacro( int, CurrentBufferIndex );
            BDGetMacro( bool, IsRecordingBuffer );
            BDGetMacro( std::vector<VkCommandBuffer>, Buffers );
        };
    };