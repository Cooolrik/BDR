#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"

namespace Vlk
    {
    class RayTracingShaderBindingTable;
    class GraphicsPipeline;
    class VertexBuffer;
    class IndexBuffer;
    class RayTracingPipeline;

    class CommandPool
        {
        private:
            CommandPool() = default;
            CommandPool( const CommandPool& other );
            friend class Renderer;

            Renderer* Parent = nullptr;
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

            void BindGraphicsPipeline( GraphicsPipeline* pipeline );

            void BindComputePipeline( ComputePipeline* pipeline );

            void BindRayTracingPipeline( RayTracingPipeline* pipeline );

            void BindVertexBuffer( VertexBuffer* buffer );

            void BindIndexBuffer( IndexBuffer* buffer );

            void BindDescriptorSet( GraphicsPipeline* pipeline, VkDescriptorSet set );
            void BindDescriptorSet( RayTracingPipeline* pipeline, VkDescriptorSet set );
            void BindDescriptorSet( ComputePipeline* pipeline, VkDescriptorSet set );


            void PushConstants( GraphicsPipeline* pipeline, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues );

            void PushConstants( RayTracingPipeline* pipeline, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues );

            void PushConstants( ComputePipeline* pipeline, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues );

            void Draw( uint vertexCount );

            void DrawIndexed( uint firstIndex, uint indexCount );

            void DrawIndexedInstances(
                uint indexCount,
                uint instanceCount,
                uint firstIndex,
                int vertexOffset,
                uint firstInstance
            );

            void DrawIndexedIndirect(
                BufferBase *buffer,
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
                BufferBase*   buffer,
                VkAccessFlags srcAccessMask,
                VkAccessFlags dstAccessMask,
                VkDeviceSize  offset = 0,
                VkDeviceSize  size = VkDeviceSize(~0)
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
                Image *image,
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

            void TraceRays( RayTracingShaderBindingTable* sbt , uint width, uint height );

            BDGetMacro( int, CurrentBufferIndex );
            BDGetMacro( bool, IsRecordingBuffer );
            BDGetMacro( std::vector<VkCommandBuffer>, Buffers );

            ~CommandPool();
        };
    };