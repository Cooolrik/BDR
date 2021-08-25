#pragma once

#include "Vlk_RayTracingExtension.h"
#include "Vlk_Pipeline.h"

namespace Vlk
    {
    class ShaderModule;
    class RayTracingShaderBindingTable;

    class RayTracingPipeline : public Pipeline
        {
        BDExtensionSubmoduleMacro( RayTracingPipeline, Pipeline, RayTracingExtension );

        private:
            friend class RayTracingExtension;

            // number of miss and closest hit shaders (there is always a single "gen" shader, at slot 0)
            unsigned int MissShadersCount = 0;
            unsigned int ClosestHitShadersCount = 0;

        public:
            BDGetMacro( unsigned int, MissShadersCount );
            BDGetMacro( unsigned int, ClosestHitShadersCount );
        };

    class RayTracingPipelineTemplate
        {
        private:
            // dont allow copy-by-value, because of inter-struct links
            RayTracingPipelineTemplate( const RayTracingPipelineTemplate& other );
            const RayTracingPipelineTemplate& operator = ( const RayTracingPipelineTemplate& other );

        public:
            // the shader modules to use
            const ShaderModule* RaygenShader = nullptr;
            std::vector<const ShaderModule*> MissShaders{};
            std::vector<const ShaderModule*> ClosestHitShaders{};

            // pipeline layout structures
            std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
            std::vector<VkPushConstantRange> PushConstantRanges;
            VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

            // pipeline create info
            VkRayTracingPipelineCreateInfoKHR RayTracingPipelineCreateInfo = { VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };

            //////////////////////////////////////

             // creates an initial pipeline. 
            RayTracingPipelineTemplate();

            // add a shader module to the pipeline
            void SetRaygenShaderModule( const ShaderModule* shader );
            uint AddMissShaderModule( const ShaderModule* shader );
            uint AddClosestHitShaderModule( const ShaderModule* shader );

            // adds a descriptor set layout. returns the index of the set in the list of layouts
            unsigned int AddDescriptorSetLayout( const DescriptorSetLayout* descriptorLayout );

            // adds a push constant range. returns the index of the range in the list of layouts
            unsigned int AddPushConstantRange( VkPushConstantRange range );
            unsigned int AddPushConstantRange( VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size );

        };

    };

