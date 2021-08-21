#include "Vlk_CommandPool.h"
#include "Vlk_GraphicsPipeline.h"
#include "Vlk_ComputePipeline.h"
#include "Vlk_VertexBuffer.h"
#include "Vlk_IndexBuffer.h"
#include "Vlk_Image.h"

#include "Vlk_RayTracingExtension.h"
#include "Vlk_RayTracingPipeline.h"
#include "Vlk_RayTracingShaderBindingTable.h"

#include <stdexcept>
#include <algorithm>

using std::runtime_error;

// makes sure the return value is VK_SUCCESS or throws an exception
#define VLK_CALL( s ) if( s != VK_SUCCESS ) { throw runtime_error( "Vulcan call " #s " failed (did not return VK_SUCCESS)"); }
#define ASSERT_RECORDING() if( !this->IsRecordingBuffer ) { throw runtime_error( "Error: " __FUNCTION__ "(): currently not recording buffer."); }

Vlk::CommandPool::~CommandPool()
	{
	// the allocated buffers will be automatically deallocated
	vkDestroyCommandPool( this->Parent->GetDevice(), Pool, nullptr );

	// unregister from parent
	this->Parent->RemoveCommandPool( this );
	}

void Vlk::CommandPool::ResetCommandPool()
	{
	// make sure we are not recording
	if( this->IsRecordingBuffer )
		{
		throw runtime_error( "Error: ResetCommandPool(), currently recording buffer. End buffer before begin new or resetting." );
		}

	VLK_CALL( vkResetCommandPool( this->Parent->GetDevice(), this->Pool, 0 ) );

	// reset index
	this->CurrentBufferIndex = -1;
	}

VkCommandBuffer Vlk::CommandPool::BeginCommandBuffer()
	{
	// make sure we are not already recording
	if( this->IsRecordingBuffer )
		{
		throw runtime_error( "Error: BeginCommandBuffer(), currently recording buffer. End buffer before begin new or resetting." );
		}
	this->IsRecordingBuffer = true;

	// step up buffer index, make sure we are not out of buffers
	++this->CurrentBufferIndex;
	if( (size_t)this->CurrentBufferIndex >= this->Buffers.size() )
		{
		throw runtime_error( "Error: BeginCommandBuffer(), out of buffers. Allocate enough buffers when allocating pool." );
		}

	// begin the buffer
	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = 0; 
	commandBufferBeginInfo.pInheritanceInfo = nullptr; 
	VLK_CALL( vkBeginCommandBuffer( this->Buffers[this->CurrentBufferIndex], &commandBufferBeginInfo ) );

	// return buffer handle 
	return this->Buffers[this->CurrentBufferIndex];
	}

void Vlk::CommandPool::BeginRenderPass( VkFramebuffer destFramebuffer )
	{
	ASSERT_RECORDING();

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = this->Parent->RenderPass;
	renderPassBeginInfo.framebuffer = destFramebuffer;

	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = this->Parent->RenderExtent;

	VkClearValue clearValues[2]{};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass( this->Buffers[this->CurrentBufferIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
	}

void Vlk::CommandPool::EndRenderPass()
	{
	ASSERT_RECORDING();

	vkCmdEndRenderPass( this->Buffers[this->CurrentBufferIndex] );
	}

void Vlk::CommandPool::BindGraphicsPipeline( GraphicsPipeline* pipeline )
	{
	ASSERT_RECORDING();

	vkCmdBindPipeline( this->Buffers[this->CurrentBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline() );
	}

void Vlk::CommandPool::BindComputePipeline( ComputePipeline* pipeline )
	{
	ASSERT_RECORDING();

	vkCmdBindPipeline( this->Buffers[this->CurrentBufferIndex], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetPipeline() );
	}

void Vlk::CommandPool::BindRayTracingPipeline( RayTracingPipeline* pipeline )
	{
	ASSERT_RECORDING();

	vkCmdBindPipeline( this->Buffers[this->CurrentBufferIndex], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline->GetPipeline() );
	}


void Vlk::CommandPool::BindVertexBuffer( VertexBuffer* buffer )
	{
	ASSERT_RECORDING();

	VkBuffer vertexBuffers[] = { buffer->GetBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers( this->Buffers[this->CurrentBufferIndex], 0, 1, vertexBuffers, offsets );
	}

void Vlk::CommandPool::BindIndexBuffer( IndexBuffer* buffer )
	{
	ASSERT_RECORDING();

	vkCmdBindIndexBuffer( this->Buffers[this->CurrentBufferIndex], buffer->GetBuffer(), 0, buffer->GetIndexType() );
	}

void Vlk::CommandPool::BindDescriptorSet( GraphicsPipeline* pipeline , VkDescriptorSet set )
	{
	ASSERT_RECORDING();

	vkCmdBindDescriptorSets( this->Buffers[this->CurrentBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipelineLayout(), 0, 1, &set, 0, nullptr );
	}

void Vlk::CommandPool::BindDescriptorSet( RayTracingPipeline* pipeline, VkDescriptorSet set )
	{
	ASSERT_RECORDING();

	vkCmdBindDescriptorSets( this->Buffers[this->CurrentBufferIndex], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline->GetPipelineLayout(), 0, 1, &set, 0, nullptr );
	}

void Vlk::CommandPool::BindDescriptorSet( ComputePipeline* pipeline, VkDescriptorSet set )
	{
	ASSERT_RECORDING();

	vkCmdBindDescriptorSets( this->Buffers[this->CurrentBufferIndex], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetPipelineLayout(), 0, 1, &set, 0, nullptr );
	}

void Vlk::CommandPool::PushConstants( GraphicsPipeline* pipeline , VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues )
	{
	ASSERT_RECORDING();

	vkCmdPushConstants( this->Buffers[this->CurrentBufferIndex], pipeline->GetPipelineLayout(), stageFlags, offset, size, pValues );
	}

void Vlk::CommandPool::PushConstants( RayTracingPipeline* pipeline, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues )
	{
	ASSERT_RECORDING();

	vkCmdPushConstants( this->Buffers[this->CurrentBufferIndex], pipeline->GetPipelineLayout(), stageFlags, offset, size, pValues );
	}

void Vlk::CommandPool::PushConstants( ComputePipeline* pipeline, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues )
	{
	ASSERT_RECORDING();

	vkCmdPushConstants( this->Buffers[this->CurrentBufferIndex], pipeline->GetPipelineLayout(), stageFlags, offset, size, pValues );
	}

void Vlk::CommandPool::UpdateBuffer( Buffer* buffer, VkDeviceSize dstOffset, uint32_t dataSize, const void* pData )
	{
	ASSERT_RECORDING();

	vkCmdUpdateBuffer( this->Buffers[this->CurrentBufferIndex], buffer->GetBuffer(), dstOffset, dataSize, pData );
	}

void Vlk::CommandPool::Draw( uint vertexCount )
	{
	ASSERT_RECORDING();

	vkCmdDraw( this->Buffers[this->CurrentBufferIndex], vertexCount, 1, 0, 0 );
	}

void Vlk::CommandPool::DrawIndexed( uint indexCount, uint instanceCount, uint firstIndex, int vertexOffset, uint firstInstance )
	{
	ASSERT_RECORDING();

	vkCmdDrawIndexed( this->Buffers[this->CurrentBufferIndex], indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
	}

void Vlk::CommandPool::DrawIndexedIndirect( const Buffer *buffer, VkDeviceSize offset, uint drawCount, uint stride )
	{
	ASSERT_RECORDING();

	vkCmdDrawIndexedIndirect( this->Buffers[this->CurrentBufferIndex], buffer->GetBuffer(), offset, drawCount, stride );
	}

void Vlk::CommandPool::QueueUpBufferMemoryBarrier( VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkDeviceSize offset, VkDeviceSize size )
	{
	VkBufferMemoryBarrier bufferMemoryBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	bufferMemoryBarrier.srcAccessMask = srcAccessMask;
	bufferMemoryBarrier.dstAccessMask = dstAccessMask;
	bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferMemoryBarrier.buffer = buffer;
	bufferMemoryBarrier.offset = offset;
	bufferMemoryBarrier.size = size;
	this->BufferMemoryBarriers.push_back( bufferMemoryBarrier );
	}

void Vlk::CommandPool::QueueUpBufferMemoryBarrier( const Buffer* buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkDeviceSize offset, VkDeviceSize size )
	{
	this->QueueUpBufferMemoryBarrier(
		buffer->GetBuffer(),
		srcAccessMask,
		dstAccessMask,
		offset,
		(size == VkDeviceSize( ~0 )) ? buffer->GetBufferSize() : size
	);
	}

void Vlk::CommandPool::QueueUpImageMemoryBarrier( VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageAspectFlags aspectMask )
	{
	VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.srcAccessMask = srcAccessMask;
	imageMemoryBarrier.dstAccessMask = dstAccessMask;
	imageMemoryBarrier.oldLayout = oldLayout;
	imageMemoryBarrier.newLayout = newLayout;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange.aspectMask = aspectMask;
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0; 
	imageMemoryBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	this->ImageMemoryBarriers.push_back( imageMemoryBarrier );
	}

void Vlk::CommandPool::QueueUpImageMemoryBarrier( const Image* image, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageAspectFlags aspectMask )
	{
	this->QueueUpImageMemoryBarrier(
		image->GetImage(),
		oldLayout,
		newLayout,
		srcAccessMask,
		dstAccessMask,
		aspectMask
		);
	}

void Vlk::CommandPool::PipelineBarrier( VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask )
	{
	ASSERT_RECORDING();

	vkCmdPipelineBarrier( 
		this->Buffers[this->CurrentBufferIndex], 
		srcStageMask, 
		dstStageMask, 
		VK_DEPENDENCY_BY_REGION_BIT,
		0, nullptr, 
		(uint)this->BufferMemoryBarriers.size(), this->BufferMemoryBarriers.data(), 
		(uint)this->ImageMemoryBarriers.size(), this->ImageMemoryBarriers.data()
		);

	BufferMemoryBarriers.clear();
	ImageMemoryBarriers.clear();
	}

void Vlk::CommandPool::DispatchCompute( uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ )
	{
	ASSERT_RECORDING();

	vkCmdDispatch( this->Buffers[this->CurrentBufferIndex], groupCountX, groupCountY, groupCountZ );
	}

void Vlk::CommandPool::TraceRays( RayTracingShaderBindingTable* sbt , uint width , uint height )
	{
	ASSERT_RECORDING();

	RayTracingExtension::vkCmdTraceRaysKHR(
		this->Buffers[this->CurrentBufferIndex],
		&sbt->GetRaygenDeviceAddress(),
		&sbt->GetMissDeviceAddress(),
		&sbt->GetClosestHitDeviceAddress(),
		&sbt->GetCallableDeviceAddress(),
		width,
		height,
		1 );
	}

void Vlk::CommandPool::EndCommandBuffer()
	{
	ASSERT_RECORDING();

	// done with the buffer
	VLK_CALL( vkEndCommandBuffer( this->Buffers[this->CurrentBufferIndex] ) );
	this->IsRecordingBuffer = false;
	}