#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"

namespace Vlk
    {
    class Texture;
    class RayTracingAccBuffer;

    class DescriptorPool
        {
        private:
            DescriptorPool() = default;
            DescriptorPool( const DescriptorPool& other );
            friend class Renderer;

            Renderer* Parent = nullptr;
            VkDescriptorPool Pool = nullptr;

            class DescriptorInfo
                {
                public:
                    // standard vulkan stuff
                    vector<VkDescriptorBufferInfo> DescriptorBufferInfo;
                    vector<VkDescriptorImageInfo> DescriptorImageInfo;

                    // extensions
                    VkWriteDescriptorSetAccelerationStructureKHR WriteDescriptorSetAccelerationStructureKHR;
                    vector<VkAccelerationStructureKHR> AccelerationStructureKHR;
                };

            std::vector<DescriptorInfo> WriteDescriptorInfos;

            std::vector<VkWriteDescriptorSet> WriteDescriptorSets;
            
        public:

            // begin create descriptor set 
            VkDescriptorSet BeginDescriptorSet( DescriptorLayout* descriptorLayout );

            // Sets a buffer for the descriptor
            void SetBuffer( uint bindingIndex, BufferBase* buffer, uint byteOffset = 0 );
            void SetBufferInArray( uint bindingIndex, uint arrayIndex, BufferBase* buffer, uint byteOffset = 0 );

            // Sets an image for the descriptor
            void SetImage( uint bindingIndex, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout );
            void SetImageInArray( uint bindingIndex, uint arrayIndex, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout );

            void SetAccelerationStructureInArray( uint bindingIndex, uint arrayIndex, RayTracingAccBuffer* accelerationStructure );

            // Sets the acceleration structure bound to the descriptor
            void SetAccelerationStructure( uint bindingIndex, RayTracingAccBuffer* accelerationStructure );

            // update and finalize the descriptor set
            void EndDescriptorSet();

            // resets the pool
            void ResetDescriptorPool();

            ~DescriptorPool();
        };
    };