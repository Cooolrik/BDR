#pragma once

#include "Vlk_RayTracingExtension.h"

namespace Vlk
    {
    class ShaderModule;
    class RayTracingShaderBindingTable;

    class RayTracingPipeline
        {
        private:
            RayTracingPipeline() = default;
            RayTracingPipeline( const RayTracingPipeline& other );
            friend class RayTracingExtension;

            RayTracingExtension* Parent = nullptr;

            const ShaderModule* RaygenShader = nullptr;
            std::vector<const ShaderModule*> MissShaders{};
            std::vector<const ShaderModule*> ClosestHitShaders{};
            

            //VkStridedDeviceAddressRegionKHR

            VkPipeline Pipeline = nullptr;
            VkPipelineLayout PipelineLayout = nullptr;
            VkDescriptorSetLayout DescriptorSetLayout = nullptr;
            std::vector<VkPushConstantRange> PushConstantRanges{};

        public:

            // add a shader module to the pipeline
            void SetRaygenShader( const ShaderModule* shader );
            uint AddMissShader( const ShaderModule* shader );
            uint AddClosestHitShader( const ShaderModule* shader );

            // sets the descriptor set layout for the uniform buffers and texture samplers
            void SetVkDescriptorSetLayout( VkDescriptorSetLayout descriptorSetLayout );
            void SetDescriptorLayout( const DescriptorLayout* descriptorLayout );

            // sets the push constant ranges
            void SetSinglePushConstantRange( uint32_t buffersize, VkShaderStageFlags stageFlags );
            void SetPushConstantRanges( const std::vector<VkPushConstantRange>& ranges );

            // returns true if pipeline needs to be built
            bool IsPipelineBuilt() const { return (this->Pipeline != nullptr); };

            // build vulkan pipeline
            void BuildPipeline();

            // cleanup allocated pipeline, remove vulkan objects
            void CleanupPipeline();

            // create a Shader Binding Table for the built pipeline
            RayTracingShaderBindingTable* CreateShaderBindingTable() const;

            ~RayTracingPipeline();

            BDGetMacro( RayTracingExtension*, Parent );
            BDGetMacro( VkPipeline, Pipeline );
            BDGetMacro( VkPipelineLayout, PipelineLayout );
            BDGetMacro( ShaderModule*, RaygenShader );
            BDGetMacro( std::vector<const ShaderModule*>, MissShaders );
            BDGetMacro( std::vector<const ShaderModule*>, ClosestHitShaders );
        };
    };

