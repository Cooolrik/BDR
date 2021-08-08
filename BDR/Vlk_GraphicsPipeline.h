#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"

namespace Vlk
    {
    class ShaderModule;
    class VertexBuffer;
    class DescriptorSetLayout;

    class GraphicsPipeline
        {
        private:
            GraphicsPipeline() = default;
            GraphicsPipeline( const GraphicsPipeline& other );
            friend class Renderer;

            Renderer* Parent = nullptr;
            std::set<const ShaderModule*> ShaderModules{};
            VkPipeline Pipeline = nullptr;
            VkPipelineLayout PipelineLayout = nullptr;

            VkVertexInputBindingDescription VertexInputBindingDescription{};
            std::vector<VkVertexInputAttributeDescription> VertexInputAttributeDescriptions{};
            VkDescriptorSetLayout DescriptorSetLayoutHandle = nullptr;
            std::vector<VkPushConstantRange> PushConstantRanges{};

        public:

            // add a shader to the pipeline
            void AddShaderModule( const ShaderModule* shader );

            // remove a shader from the pipeline
            void RemoveShaderModule( const ShaderModule* shader );

            // sets the template to use for attribute description and vertex binding 
            void SetVertexDataTemplate( VkVertexInputBindingDescription bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescriptions );
            void SetVertexDataTemplateFromVertexBuffer( const VertexBuffer* buffer );

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

            ~GraphicsPipeline();

            BDGetMacro( Renderer*, Parent );
            BDGetMacro( VkPipeline, Pipeline );
            BDGetMacro( VkPipelineLayout, PipelineLayout );
            BDGetMacro( std::set<const ShaderModule*>, ShaderModules );
        };
    };

