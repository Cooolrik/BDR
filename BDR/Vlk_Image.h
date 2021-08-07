#pragma once

#include "Vlk_Renderer.h"

namespace Vlk
    {
    class BufferBase;

    class Image
        {
        private:
            BDClassHeader( Image, Renderer );

            VkImage ImageHandle = nullptr;
            VkImageView ImageView = nullptr;
            VmaAllocation Allocation = nullptr;

        public:

            // transition the image layout using a memory barrier. Note that this method is synchronized and much slower than using a 
            // memory barrier in a command buffer, and should only be used during setup stages.
            void TransitionLayout( 
                VkImageMemoryBarrier &imageMemoryBarrier,
                VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_NONE_KHR , 
                VkPipelineStageFlags destStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT 
                );
            void TransitionLayout(
                VkImageLayout oldLayout,
                VkImageLayout newLayout,
                VkAccessFlags srcAccessMask,
                VkAccessFlags dstAccessMask,
                VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_NONE_KHR,
                VkPipelineStageFlags destStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
                );

            // copy the image to a buffer. The image will be put into transfer mode unless it is already in it,
            // which is specified in oldLayout/srcAccessMask
            void CopyToBuffer( 
                BufferBase *destBuffer,
                uint32_t width, 
                uint32_t height, 
                uint32_t depth = 1,
                VkImageLayout oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VkImageLayout newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VkAccessFlags srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                VkAccessFlags dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                VkPipelineStageFlags destStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
                );
            void CopyToBuffer(
                BufferBase* destBuffer,
                const VkBufferImageCopy* region = nullptr,
                VkImageLayout oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VkImageLayout newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VkAccessFlags srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                VkAccessFlags dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                VkPipelineStageFlags destStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
            );


            BDGetCustomNameMacro( VkImage, Image, ImageHandle );
            BDGetMacro( VkImageView, ImageView );
            BDGetMacro( VmaAllocation, Allocation );
        };

    // template used to create an image
    class ImageTemplate
        {
        public:
            // initial create information
            VkImageCreateInfo ImageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

            // vma allocation object
            VmaAllocationCreateInfo AllocationCreateInfo = {};

            // if an upload is to be made, here is the layout transition before the upload and the source pointer to copy from
            const void* UploadSourcePtr = nullptr;
            VkDeviceSize UploadSourceSize = 0;
            VkImageMemoryBarrier UploadLayoutTransition = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            std::vector<VkBufferImageCopy> UploadBufferImageCopies; // one per mip-map transfer
           
            // if TransitionImageLayout is set, apply a layout transition after creation (and optional upload)
            // the FinalLayoutStageMask is the stage where the image is to be used and has to match the layout 
            bool TransitionImageLayout = true;
            VkPipelineStageFlags FinalLayoutStageMask = VK_PIPELINE_STAGE_NONE_KHR;
            VkImageMemoryBarrier FinalLayoutTransition = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };

            // image view create info
            VkImageViewCreateInfo ImageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };

            /////////////////////////////////

            // create a default template
            static ImageTemplate Default();

            // create an 2d color image which is optimized for texture sampling. If source ptr is set, uploads image. If mipmap_offsets is set it is assumed to be at least mipmap_levels long, sets up transfers of mipmaps from source as well.
            static ImageTemplate Texture2D( VkFormat format , uint32_t width , uint32_t height , uint32_t mipmap_levels , const void* source_ptr = nullptr , VkDeviceSize source_size = 0 , const VkDeviceSize* source_mipmap_offsets = nullptr );

            // create a 2d general layout color image that can be used for storage and sampling in shaders
            static ImageTemplate General2D( VkFormat format, uint32_t width , uint32_t height, uint32_t mipmap_levels );
            
        };
    };
