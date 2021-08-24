#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"
#include "Vlk_Pipeline.h"

namespace Vlk
    {
    class ShaderModule;
    class VertexBuffer;
    class DescriptorSetLayout;

    class GraphicsPipelineTemplate
        {
        private:
            // dont allow copy-by-value, because of inter-struct links
            GraphicsPipelineTemplate( const GraphicsPipelineTemplate& other );
            const GraphicsPipelineTemplate& operator = ( const GraphicsPipelineTemplate& other );

        public:
            // the shader modules to use
            std::vector<const ShaderModule*> ShaderModules{};

            // pipeline layout structures
            std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
            std::vector<VkPushConstantRange> PushConstantRanges;
            VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

            // vertex input binding
            std::vector<VkVertexInputBindingDescription> VertexInputBindingDescriptions;
            std::vector<VkVertexInputAttributeDescription> VertexInputAttributeDescriptions;
            VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

            // input asseblmbly 
            VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

            // viewport & scissor states
            std::vector<VkViewport> Viewports;
            std::vector<VkRect2D> ScissorRectangles;
            VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };

            // rasterization
            VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

            // multisampling
            VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
            
            // color attatchments' blend states
            std::vector<VkPipelineColorBlendAttachmentState> PipelineColorBlendAttachmentStates;
            VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
            
            // depth and stencil states
            VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

            // list of dynamic states
            std::vector<VkDynamicState> DynamicStates;
            VkPipelineDynamicStateCreateInfo PipelineDynamicStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };

            // pipeline create info
            VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

            ////////////////////////////////////

            // creates an initial pipeline. Assumes dynamic viewport and scissor rectangle structures, and triangle lists as primitive type.
            GraphicsPipelineTemplate();

            // add a shader stage to the pipeline
            void AddShaderModule( const ShaderModule* shader );

            // set or replace the template to use for attribute description and vertex binding 
            void SetVertexDataTemplate( VkVertexInputBindingDescription bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescriptions );
            void SetVertexDataTemplateFromVertexBuffer( const VertexBuffer* buffer );
            void SetVertexDataTemplateFromVertexBufferDescription( const VertexBufferDescription& bufferdesc );

            // adds a descriptor set layout. returns the index of the set in the list of layouts
            unsigned int AddDescriptorSetLayout( const DescriptorSetLayout* descriptorLayout );

            // adds a push constant range. returns the index of the range in the list of layouts
            unsigned int AddPushConstantRange( VkPushConstantRange range );
            unsigned int AddPushConstantRange( VkShaderStageFlags stageFlags, uint32_t offset , uint32_t size );

            // set a single static viewport value, and removes the viewport data from the list of dynamic values
            // this renders faster, but requires the pipeline to be rebuild if the screen is resized
            void SetStaticViewport( VkViewport viewport );
            void SetStaticViewport( float x, float y, float width, float height, float minDepth = 0.f, float maxDepth = 1.f );

            // set a single static scissor rectangle value, and removes the scissor rectangle data from the list of dynamic values
            // this renders faster, but requires the pipeline to be rebuild if the screen is resized
            void SetStaticScissorRectangle( VkRect2D scissorRectangle );
            void SetStaticScissorRectangle( int32_t x, int32_t y, uint32_t width, uint32_t height );

            // add/removes a dynamic state from the DynamicStates vector. Also updates the PipelineDynamicStateCreateInfo struct
            void AddDynamicState( VkDynamicState state );
            void RemoveDynamicState( VkDynamicState state );
        };


    };

