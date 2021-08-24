#include "Vlk_Common.inl"

#include "Vlk_GraphicsPipeline.h"
#include "Vlk_ShaderModule.h"
#include "Vlk_VertexBuffer.h"
#include "Vlk_DescriptorSetLayout.h"

using std::vector;

Vlk::GraphicsPipeline::~GraphicsPipeline()
	{
	if( this->Pipeline )
		{
		vkDestroyPipeline( this->Module->GetDevice(), this->Pipeline, nullptr );
		}
	if( this->PipelineLayout )
		{
		vkDestroyPipelineLayout( this->Module->GetDevice(), this->PipelineLayout, nullptr );
		}
	}

Vlk::GraphicsPipelineTemplate::GraphicsPipelineTemplate()
	{
	// pipeline layout is initially empty
	this->PipelineLayoutCreateInfo.setLayoutCount = 0;
	this->PipelineLayoutCreateInfo.pushConstantRangeCount = 0;

	// define the input assembly, assume triangle list 
	this->PipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	this->PipelineInputAssemblyStateCreateInfo.flags = 0; // reserved 
	this->PipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	// set up one empty viewport and one empty scissor (assume dynamic)
	this->Viewports.push_back( {} );
	this->ScissorRectangles.push_back( {} );
	this->PipelineViewportStateCreateInfo.viewportCount = (uint32_t)this->Viewports.size();
	this->PipelineViewportStateCreateInfo.pViewports = this->Viewports.data();
	this->PipelineViewportStateCreateInfo.scissorCount = (uint32_t)this->ScissorRectangles.size();;
	this->PipelineViewportStateCreateInfo.pScissors = this->ScissorRectangles.data();

	// setup rasterization, assume filled triangles, and cull backfacing
	this->PipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	this->PipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	this->PipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	this->PipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
	this->PipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	this->PipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	this->PipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	this->PipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f; 
	this->PipelineRasterizationStateCreateInfo.depthBiasClamp = 0.0f; 
	this->PipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f; 

	// set up no multisampling
	this->PipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	this->PipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	this->PipelineMultisampleStateCreateInfo.minSampleShading = 1.0f; 
	this->PipelineMultisampleStateCreateInfo.pSampleMask = nullptr; 
	this->PipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE; 
	this->PipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE; 

	// set up one color attachment, no blending
	this->PipelineColorBlendAttachmentStates.push_back( {} );
	this->PipelineColorBlendAttachmentStates.back().colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	this->PipelineColorBlendAttachmentStates.back().blendEnable = VK_FALSE;
	this->PipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	this->PipelineColorBlendStateCreateInfo.attachmentCount = (uint32_t)this->PipelineColorBlendAttachmentStates.size();
	this->PipelineColorBlendStateCreateInfo.pAttachments = this->PipelineColorBlendAttachmentStates.data();

	// depth stencil, assue depth writes, and compare operator "less"
	this->PipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	this->PipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	this->PipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	this->PipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	this->PipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;

	// add dynamic states for viewport and scissor rects
	this->DynamicStates =
		{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
		};
	this->PipelineDynamicStateCreateInfo.dynamicStateCount = (uint32_t)this->DynamicStates.size();
	this->PipelineDynamicStateCreateInfo.pDynamicStates = this->DynamicStates.data();

	// set up the create info pointers
	this->GraphicsPipelineCreateInfo.pVertexInputState = &this->PipelineVertexInputStateCreateInfo;
	this->GraphicsPipelineCreateInfo.pInputAssemblyState = &this->PipelineInputAssemblyStateCreateInfo;
	this->GraphicsPipelineCreateInfo.pViewportState = &this->PipelineViewportStateCreateInfo;
	this->GraphicsPipelineCreateInfo.pRasterizationState = &this->PipelineRasterizationStateCreateInfo;
	this->GraphicsPipelineCreateInfo.pMultisampleState = &this->PipelineMultisampleStateCreateInfo;
	this->GraphicsPipelineCreateInfo.pDepthStencilState = &this->PipelineDepthStencilStateCreateInfo;
	this->GraphicsPipelineCreateInfo.pColorBlendState = &this->PipelineColorBlendStateCreateInfo;
	this->GraphicsPipelineCreateInfo.pDynamicState = &this->PipelineDynamicStateCreateInfo;
	}

void Vlk::GraphicsPipelineTemplate::AddShaderModule( const ShaderModule* shader )
	{
	this->ShaderModules.emplace_back( shader );
	}

void Vlk::GraphicsPipelineTemplate::SetVertexDataTemplate( VkVertexInputBindingDescription bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescriptions )
	{
	this->VertexInputBindingDescriptions = { bindingDescription };
	this->VertexInputAttributeDescriptions = attributeDescriptions;

	// update pointers in the struct
	this->PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = (uint32_t)this->VertexInputBindingDescriptions.size();
	this->PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = this->VertexInputBindingDescriptions.data();
	this->PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = (uint32_t)this->VertexInputAttributeDescriptions.size();
	this->PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = this->VertexInputAttributeDescriptions.data();
	}

void Vlk::GraphicsPipelineTemplate::SetVertexDataTemplateFromVertexBuffer( const VertexBuffer* buffer )
	{
	this->SetVertexDataTemplateFromVertexBufferDescription( buffer->GetDescription() );
	}

void Vlk::GraphicsPipelineTemplate::SetVertexDataTemplateFromVertexBufferDescription( const VertexBufferDescription& bufferdesc )
	{
	this->SetVertexDataTemplate( bufferdesc.GetVertexInputBindingDescription(), bufferdesc.GetVertexInputAttributeDescriptions() );
	}

unsigned int Vlk::GraphicsPipelineTemplate::AddDescriptorSetLayout( const DescriptorSetLayout* descriptorLayout )
	{
	uint index = (uint)this->DescriptorSetLayouts.size();
	this->DescriptorSetLayouts.emplace_back( descriptorLayout->GetDescriptorSetLayout() );
	this->PipelineLayoutCreateInfo.setLayoutCount = (uint32_t)this->DescriptorSetLayouts.size();
	this->PipelineLayoutCreateInfo.pSetLayouts = this->DescriptorSetLayouts.data();
	return index;
	}

unsigned int Vlk::GraphicsPipelineTemplate::AddPushConstantRange( VkPushConstantRange range )
	{
	uint index = (uint)this->PushConstantRanges.size();
	this->PushConstantRanges.emplace_back( range );
	this->PipelineLayoutCreateInfo.pushConstantRangeCount = (uint32_t)this->PushConstantRanges.size();
	this->PipelineLayoutCreateInfo.pPushConstantRanges = this->PushConstantRanges.data();
	return index;
	}

unsigned int Vlk::GraphicsPipelineTemplate::AddPushConstantRange( VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size )
	{
	VkPushConstantRange range = {};
	range.stageFlags = stageFlags;
	range.offset = offset;
	range.size = size;
	return AddPushConstantRange( range );
	}

void Vlk::GraphicsPipelineTemplate::SetStaticViewport( VkViewport viewport )
	{
	this->RemoveDynamicState( VK_DYNAMIC_STATE_VIEWPORT );

	// set the viewport
	this->Viewports = { viewport };
	this->PipelineViewportStateCreateInfo.viewportCount = (uint32_t)this->Viewports.size();
	this->PipelineViewportStateCreateInfo.pViewports = this->Viewports.data();
	}

void Vlk::GraphicsPipelineTemplate::SetStaticViewport( float x, float y, float width, float height, float minDepth, float maxDepth )
	{
	VkViewport viewport;
	viewport.x = x;
	viewport.y = y;
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;
	this->SetStaticViewport( viewport );
	}

void Vlk::GraphicsPipelineTemplate::SetStaticScissorRectangle( VkRect2D scissorRectangle )
	{
	this->RemoveDynamicState( VK_DYNAMIC_STATE_SCISSOR );

	// set the rectangle
	this->ScissorRectangles = { scissorRectangle };
	this->PipelineViewportStateCreateInfo.scissorCount = (uint32_t)this->ScissorRectangles.size();;
	this->PipelineViewportStateCreateInfo.pScissors = this->ScissorRectangles.data();
	}

void Vlk::GraphicsPipelineTemplate::SetStaticScissorRectangle( int32_t x, int32_t y, uint32_t width, uint32_t height )
	{
	VkRect2D scissorRectangle;
	scissorRectangle.offset.x = x;
	scissorRectangle.offset.y = y;
	scissorRectangle.extent.width = width;
	scissorRectangle.extent.height = height;
	this->SetStaticScissorRectangle( scissorRectangle );
	}

void Vlk::GraphicsPipelineTemplate::AddDynamicState( VkDynamicState state )
	{
	// add dynamic value to vector
	this->DynamicStates.emplace_back( state );

	// update creation struct
	this->PipelineDynamicStateCreateInfo.dynamicStateCount = (uint32_t)this->DynamicStates.size();
	this->PipelineDynamicStateCreateInfo.pDynamicStates = this->DynamicStates.data();
	}

void Vlk::GraphicsPipelineTemplate::RemoveDynamicState( VkDynamicState state )
	{
	// remove dynamic value if it is vector
	std::vector<VkDynamicState>::iterator it = std::find( this->DynamicStates.begin(), this->DynamicStates.end(), state );
	if( it != this->DynamicStates.end() )
		this->DynamicStates.erase( it );

	// update creation struct
	this->PipelineDynamicStateCreateInfo.dynamicStateCount = (uint32_t)this->DynamicStates.size();
	this->PipelineDynamicStateCreateInfo.pDynamicStates = ( this->DynamicStates.empty()) ? nullptr : this->DynamicStates.data();
	}