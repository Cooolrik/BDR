#include "Vlk_Common.inl"
#include "Vlk_Image.h"
#include "Vlk_Buffer.h"
#include "Vlk_Helpers.h"

Vlk::Image::~Image()
	{
	if( this->ImageView != nullptr )
		{
		vkDestroyImageView( this->Module->GetDevice(), this->ImageView, nullptr );
		}
	if( this->Allocation != nullptr )
		{
		vmaDestroyImage( this->Module->GetMemoryAllocator(), this->ImageHandle, this->Allocation );
		}
	}

void Vlk::Image::TransitionLayout( VkImageMemoryBarrier& imageMemoryBarrier, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags destStageMask )
	{
	this->Module->RunBlockingCommandBuffer( 
		[&]( VkCommandBuffer cmd ) 
			{
			vkCmdPipelineBarrier( cmd, srcStageMask, destStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier );
			}
		);
	}

void Vlk::Image::TransitionLayout( VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags destStageMask, VkImageAspectFlags aspectMask )
	{
	VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.oldLayout = oldLayout;
	imageMemoryBarrier.newLayout = newLayout;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; 
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; 
	imageMemoryBarrier.image = this->ImageHandle;
	imageMemoryBarrier.subresourceRange.aspectMask = aspectMask;
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0; 
	imageMemoryBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS; 
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0; 
	imageMemoryBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS; 
	imageMemoryBarrier.srcAccessMask = srcAccessMask;
	imageMemoryBarrier.dstAccessMask = dstAccessMask;

	this->TransitionLayout( imageMemoryBarrier , srcStageMask , destStageMask );
	}

void Vlk::Image::CopyToBuffer( Buffer* destBuffer, uint32_t width, uint32_t height, uint32_t depth, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags destStageMask, VkImageAspectFlags aspectMask )
	{
	VkBufferImageCopy bufferImageCopy;

	bufferImageCopy.bufferOffset = 0; 
	bufferImageCopy.bufferRowLength = 0; 
	bufferImageCopy.bufferImageHeight = 0; 
	bufferImageCopy.imageSubresource.aspectMask = aspectMask;
	bufferImageCopy.imageSubresource.mipLevel = 0; 
	bufferImageCopy.imageSubresource.baseArrayLayer = 0; 
	bufferImageCopy.imageSubresource.layerCount = 1; 
	bufferImageCopy.imageOffset = { 0, 0, 0 }; 
	bufferImageCopy.imageExtent.width = width;
	bufferImageCopy.imageExtent.height = height;
	bufferImageCopy.imageExtent.depth = depth;

	this->CopyToBuffer( destBuffer , &bufferImageCopy , oldLayout, newLayout, srcAccessMask, dstAccessMask, srcStageMask, destStageMask, aspectMask	);
	}

void Vlk::Image::CopyToBuffer( Buffer* destBuffer, const VkBufferImageCopy* region, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags destStageMask, VkImageAspectFlags aspectMask )
	{
	if(oldLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL || srcAccessMask != VK_ACCESS_TRANSFER_READ_BIT)
		{
		// need to transition layout
		this->TransitionLayout(
			oldLayout,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			srcAccessMask,
			VK_ACCESS_TRANSFER_READ_BIT,
			srcStageMask,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			aspectMask
			);
		}

	// now copy to buffer
	this->Module->RunBlockingCommandBuffer(
		[&]( VkCommandBuffer cmd )
			{
			vkCmdCopyImageToBuffer( cmd, this->ImageHandle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destBuffer->GetBuffer(), 1, region );
			}
		);

	if(newLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL || dstAccessMask != VK_ACCESS_TRANSFER_READ_BIT)
		{
		// need to transition layout again
		this->TransitionLayout(
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			newLayout,
			VK_ACCESS_TRANSFER_READ_BIT,
			dstAccessMask,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			destStageMask,
			aspectMask
			);
		}
	}

///////////////////////////////////////////

#define IMAGE_CREATE_INFO_2D( _format , _usage , _mipLevels )\
	ret.ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;\
	ret.ImageCreateInfo.extent.width = width;\
	ret.ImageCreateInfo.extent.height = height;\
	ret.ImageCreateInfo.extent.depth = 1;\
	ret.ImageCreateInfo.mipLevels = _mipLevels;\
	ret.ImageCreateInfo.arrayLayers = 1;\
	ret.ImageCreateInfo.format = _format;\
	ret.ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;\
	ret.ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;\
	ret.ImageCreateInfo.usage = _usage;\
	ret.ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;\
	ret.ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

#define UPLOAD_BUFFER_IMAGE_COPY( _index , _bufferOffset , _aspectMask , _mipMapLevel , _extent )\
	ret.UploadBufferImageCopies[_index].bufferOffset = _bufferOffset;\
	ret.UploadBufferImageCopies[_index].bufferRowLength = 0;\
	ret.UploadBufferImageCopies[_index].bufferImageHeight = 0;\
	ret.UploadBufferImageCopies[_index].imageSubresource.aspectMask = _aspectMask;\
	ret.UploadBufferImageCopies[_index].imageSubresource.mipLevel = _mipMapLevel;\
	ret.UploadBufferImageCopies[_index].imageSubresource.baseArrayLayer = 0;\
	ret.UploadBufferImageCopies[_index].imageSubresource.layerCount = 1;\
	ret.UploadBufferImageCopies[_index].imageOffset = { 0, 0, 0 };\
	ret.UploadBufferImageCopies[_index].imageExtent = _extent;

#define IMAGE_MEMORY_BARRIER( _object , _oldLayout , _newLayout , _aspectMask , _srcAccessMask , _dstAccessMask )\
	_object.oldLayout = _oldLayout;\
	_object.newLayout = _newLayout;\
	_object.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;\
	_object.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;\
	_object.subresourceRange.aspectMask = _aspectMask;\
	_object.subresourceRange.baseMipLevel = 0;\
	_object.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;\
	_object.subresourceRange.baseArrayLayer = 0;\
	_object.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;\
	_object.srcAccessMask = _srcAccessMask;\
	_object.dstAccessMask = _dstAccessMask;

#define IMAGE_VIEW_CREATE_INFO_2D( _format , _aspectMask , _mipmap_levels )\
	ret.ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;\
	ret.ImageViewCreateInfo.format = _format;\
	ret.ImageViewCreateInfo.subresourceRange.aspectMask = _aspectMask;\
	ret.ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;\
	ret.ImageViewCreateInfo.subresourceRange.levelCount = _mipmap_levels;\
	ret.ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;\
	ret.ImageViewCreateInfo.subresourceRange.layerCount = 1;

static Vlk::ImageTemplate Standard2DImage( 
	VkFormat format, 
	VkImageUsageFlags usage, 
	VkImageAspectFlags aspectMask, 
	uint32_t width, uint32_t height, 
	uint32_t mipmap_levels,
	const void* source_ptr,
	VkDeviceSize source_size,
	const VkDeviceSize* source_mipmap_offsets,
	bool transitionImageLayout,
	VkImageLayout finalLayout,
	VkAccessFlags finalAccessMask,
	VkPipelineStageFlags finalLayoutStageMask
	)
	{
	Vlk::ImageTemplate ret;

	// 2d image setup
	IMAGE_CREATE_INFO_2D( format, usage, mipmap_levels );

	// vma allocation
	ret.AllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	// setup image view
	IMAGE_VIEW_CREATE_INFO_2D( format, aspectMask , mipmap_levels );

	// should we upload?
	if(source_ptr)
		{
		// if source_size is not set, assume per-pixel and multiply (will not work with compressed formats)
		if(source_size == 0)
			{
			source_size = (VkDeviceSize)
				ret.ImageCreateInfo.extent.width *
				ret.ImageCreateInfo.extent.height *
				ret.ImageCreateInfo.extent.depth *
				Vlk::GetVulkanFormatByteSize( format );
			}

		ret.UploadSourcePtr = source_ptr;
		ret.UploadSourceSize = source_size;

		if(source_mipmap_offsets)
			{
			// upload mipmaps
			ret.UploadBufferImageCopies.resize( mipmap_levels );

			// set up mipmap transfers
			VkExtent3D extent = ret.ImageCreateInfo.extent;
			for(uint32_t m = 0; m < mipmap_levels; ++m)
				{
				UPLOAD_BUFFER_IMAGE_COPY( m, source_mipmap_offsets[m] , aspectMask, m, extent );
				extent.width = (extent.width > 1) ? extent.width / 2 : 1;
				extent.height = (extent.height > 1) ? extent.height / 2 : 1;
				extent.depth = (extent.depth > 1) ? extent.depth / 2 : 1;
				}
			}
		else
			{
			// only one transfer 
			ret.UploadBufferImageCopies.resize( 1 );
			UPLOAD_BUFFER_IMAGE_COPY( 0 , 0 , aspectMask , 0 , ret.ImageCreateInfo.extent );
			}

		IMAGE_MEMORY_BARRIER( ret.UploadLayoutTransition, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, aspectMask, VK_ACCESS_NONE_KHR, VK_ACCESS_TRANSFER_WRITE_BIT );
		}

	// tell the image setup to do a final transition of the image
	ret.TransitionImageLayout = transitionImageLayout;
	if(transitionImageLayout)
		{
		// setup the final layout
		if(source_ptr)
			ret.FinalLayoutTransition.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		else
			ret.FinalLayoutTransition.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ret.FinalLayoutTransition.newLayout = finalLayout;
		ret.FinalLayoutTransition.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ret.FinalLayoutTransition.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ret.FinalLayoutTransition.subresourceRange.aspectMask = aspectMask;
		ret.FinalLayoutTransition.subresourceRange.baseMipLevel = 0;
		ret.FinalLayoutTransition.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		ret.FinalLayoutTransition.subresourceRange.baseArrayLayer = 0;
		ret.FinalLayoutTransition.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		if(source_ptr)
			ret.FinalLayoutTransition.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		else
			ret.FinalLayoutTransition.srcAccessMask = VK_ACCESS_NONE_KHR;
		ret.FinalLayoutTransition.dstAccessMask = finalAccessMask;
		ret.FinalLayoutStageMask = finalLayoutStageMask;
		}

	return ret;
	}


Vlk::ImageTemplate Vlk::ImageTemplate::Texture2D( VkFormat format, uint32_t width , uint32_t height, uint32_t mipmap_levels , const void *source_ptr, VkDeviceSize source_size, const VkDeviceSize* source_mipmap_offsets )
	{
	// setup texture 2d image, optimized for sampling
	return Standard2DImage( 
		format,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT, 
		width, height, 
		mipmap_levels,
		source_ptr,
		source_size,
		source_mipmap_offsets,
		true,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_SHADER_READ_BIT ,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		);
	}

Vlk::ImageTemplate Vlk::ImageTemplate::General2D( VkFormat format, uint32_t width, uint32_t height, uint32_t mipmap_levels )
	{
	// setup general 2d image
	return Standard2DImage(
		format,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		width, height, 
		mipmap_levels,
		nullptr,
		0,
		nullptr,
		true,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT
		);
	}
