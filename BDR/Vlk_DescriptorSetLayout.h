#pragma once

#include "Vlk_Renderer.h"

namespace Vlk
    {
    class DescriptorSetLayout : public RendererSubmodule
        {
        BDSubmoduleMacro( DescriptorSetLayout, RendererSubmodule , Renderer )

        private:
            VkDescriptorSetLayout DescriptorSetLayoutHandle = nullptr;
            std::vector<VkDescriptorSetLayoutBinding> Bindings;

        public:
            BDGetConstRefMacro( std::vector<VkDescriptorSetLayoutBinding> , Bindings );
            BDGetCustomNameMacro( VkDescriptorSetLayout, DescriptorSetLayout, DescriptorSetLayoutHandle );
        };

    class DescriptorSetLayoutTemplate
        {
        public:
            std::vector<VkDescriptorSetLayoutBinding> Bindings;

            ////////////////////

            // Adds a uniform buffer binding, returns index of binding 
            uint AddUniformBufferBinding( VkShaderStageFlags stageFlags, uint arrayCount = 1 );

            // Adds a storage buffer binding, returns index of binding 
            uint AddStorageBufferBinding( VkShaderStageFlags stageFlags, uint arrayCount = 1 );

            // Adds a combined sampler buffer binding, returns index of binding 
            uint AddSamplerBinding( VkShaderStageFlags stageFlags, uint arrayCount = 1 );

            // Adds an acceleration structure (for ray tracing) binding, returns index of binding 
            uint AddAccelerationStructureBinding( VkShaderStageFlags stageFlags, uint arrayCount = 1 );

            // Adds a stored image binding, returns index of binding 
            uint AddStoredImageBinding( VkShaderStageFlags stageFlags, uint arrayCount = 1 );
        };

    };