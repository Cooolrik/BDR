#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"

namespace Vlk
    {
    class ShaderModule;

    class ComputePipeline
        {
        private:
            ComputePipeline() = default;
            ComputePipeline( const ComputePipeline& other );
            friend class Renderer;

            Renderer* Parent = nullptr;
            const ShaderModule* Shader = nullptr;
            VkPipeline Pipeline = nullptr;
            VkPipelineLayout PipelineLayout = nullptr;

            VkDescriptorSetLayout DescriptorSetLayoutHandle = nullptr;
            std::vector<VkPushConstantRange> PushConstantRanges{};

        public:

            // set the shader of the pipeline
            void SetShaderModule( const ShaderModule* shader );

            // sets the descriptor set layout for the uniform buffers and texture samplers
            void SetVkDescriptorSetLayout( VkDescriptorSetLayout descriptorSetLayout );
            void SetDescriptorSetLayout( const DescriptorSetLayout* descriptorLayout );

            // sets the push constant ranges
            void SetSinglePushConstantRange( uint32_t buffersize, VkShaderStageFlags stageFlags );
            void SetPushConstantRanges( const std::vector<VkPushConstantRange>& ranges );

            // returns true if pipeline needs to be built
            bool PipelineNotBuilt() const { return (this->Pipeline == nullptr); };

            // build vulkan pipeline
            void BuildPipeline();

            // cleanup allocated pipeline, remove vulkan objects
            void CleanupPipeline();

            ~ComputePipeline();

            BDGetMacro( Renderer*, Parent );
            BDGetMacro( VkPipeline, Pipeline );
            BDGetMacro( VkPipelineLayout, PipelineLayout );
            BDGetMacro( ShaderModule*, Shader );
        };
    };

