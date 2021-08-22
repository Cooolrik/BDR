#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"

namespace Vlk
    {
    class Texture;
    class RayTracingAccelerationStructure;

    class DescriptorPool : public RendererSubmodule
        {
        private:
            BDSubmoduleMacro( DescriptorPool, RendererSubmodule, Renderer );

            VkDescriptorPool Pool = nullptr;

            class DescriptorInfo
                {
                public:
                    // standard vulkan stuff
                    std::vector<VkDescriptorBufferInfo> DescriptorBufferInfo;
                    std::vector<VkDescriptorImageInfo> DescriptorImageInfo;

                    // extensions
                    VkWriteDescriptorSetAccelerationStructureKHR WriteDescriptorSetAccelerationStructureKHR;
                    std::vector<VkAccelerationStructureKHR> AccelerationStructureKHR;
                };

            std::vector<DescriptorInfo> WriteDescriptorInfos;

            std::vector<VkWriteDescriptorSet> WriteDescriptorSets;
            
        public:

            // begin create descriptor set 
            VkDescriptorSet BeginDescriptorSet( DescriptorSetLayout* descriptorLayout );

            // Sets a buffer for the descriptor
            void SetBuffer( uint bindingIndex, Buffer* buffer, uint byteOffset = 0 );
            void SetBufferInArray( uint bindingIndex, uint arrayIndex, Buffer* buffer, uint byteOffset = 0 );

            // Sets an image for the descriptor
            void SetImage( uint bindingIndex, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout );
            void SetImageInArray( uint bindingIndex, uint arrayIndex, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout );

            // Sets the acceleration structure bound to the descriptor
            void SetAccelerationStructure( uint bindingIndex, RayTracingAccelerationStructure* accelerationStructure );
            void SetAccelerationStructureInArray( uint bindingIndex, uint arrayIndex, RayTracingAccelerationStructure* accelerationStructure );

            // update and finalize the descriptor set
            void EndDescriptorSet();

            // resets the pool
            void ResetDescriptorPool();
        };

    class DescriptorPoolTemplate
        {
        public:
            // initial create information
            VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };

            // the pool sizes of different descriptor types
            std::vector<VkDescriptorPoolSize> DescriptorPoolSizes;

            /////////////////////////////

            // create general descriptor pool that supports most standard rendering scenarios
            // and allocates descriptors for uniforms, storage images, storage buffers and combined samplers
            static DescriptorPoolTemplate General( 
                unsigned int maxDescriptorSets = 10, 
                unsigned int maxDescriptorCount = 10
                );

            // create a maximized descriptor pool that allocates all types of descriptors
            // dont overallocate! this is mainly used for GUIs such as ImGUI
            static DescriptorPoolTemplate Maximized(
                unsigned int maxDescriptorSets = 1000,
                unsigned int maxDescriptorCount = 1000
                );


        };

    };