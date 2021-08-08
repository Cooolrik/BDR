#include "Vlk_Common.inl"

#include "Vlk_GraphicsPipeline.h"
#include "Vlk_ShaderModule.h"
#include "Vlk_VertexBuffer.h"
#include "Vlk_DescriptorSetLayout.h"

using namespace std;

void Vlk::GraphicsPipeline::AddShaderModule( const Vlk::ShaderModule* shader )
	{
	auto it = this->ShaderModules.find( shader );
	if( it != this->ShaderModules.end() )
		{
		throw runtime_error( "Error: AddShaderModule() shader module already exists in pipeline" );
		}

	// add it
	this->ShaderModules.insert( shader );
	}

void Vlk::GraphicsPipeline::RemoveShaderModule( const Vlk::ShaderModule* shader )
	{
	auto it = this->ShaderModules.find( shader );
	if( it == this->ShaderModules.end() )
		{
		throw runtime_error( "Error: RemoveShaderModule() shader module does not exist in pipeline" );
		}
	// remove it
	this->ShaderModules.erase( it );
	}

void Vlk::GraphicsPipeline::SetVertexDataTemplate( VkVertexInputBindingDescription bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescriptions )
	{
	this->VertexInputBindingDescription = bindingDescription;
	this->VertexInputAttributeDescriptions = attributeDescriptions;
	}

void Vlk::GraphicsPipeline::SetVertexDataTemplateFromVertexBuffer( const VertexBuffer* buffer )
	{
	this->SetVertexDataTemplate( buffer->GetDescription().GetVertexInputBindingDescription(), buffer->GetDescription().GetVertexInputAttributeDescriptions() );
	}

void Vlk::GraphicsPipeline::SetVkDescriptorSetLayout( VkDescriptorSetLayout descriptorSetLayout )
	{
	this->DescriptorSetLayoutHandle = descriptorSetLayout;
	}

void Vlk::GraphicsPipeline::SetDescriptorSetLayout( const DescriptorSetLayout* descriptorLayout )
	{
	this->DescriptorSetLayoutHandle = descriptorLayout->GetDescriptorSetLayout();;
	}

void Vlk::GraphicsPipeline::SetSinglePushConstantRange( uint32_t buffersize, VkShaderStageFlags stageFlags )
	{
	std::vector<VkPushConstantRange> ranges;
	ranges.resize( 1 );
	ranges[0].stageFlags = stageFlags;
	ranges[0].offset = 0;
	ranges[0].size = buffersize;
	this->SetPushConstantRanges( ranges );
	}

void Vlk::GraphicsPipeline::SetPushConstantRanges( const std::vector<VkPushConstantRange> &ranges )
	{
	this->PushConstantRanges = ranges;
	}

void Vlk::GraphicsPipeline::BuildPipeline()
	{
	if( this->Pipeline != nullptr )
		{
		return; 
		}

	// setup the shader stages
	uint shaderStageCount = (uint)this->ShaderModules.size();
	vector<VkShaderModule> shaderStages( shaderStageCount );
	vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos( shaderStageCount );
	uint shaderIndex = 0;
	for( const ShaderModule* shader : this->ShaderModules )
		{
		// create a shader module wrap around the shader
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shader->Shader.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>( shader->Shader.data() );
		VLK_CALL( vkCreateShaderModule( this->Parent->Device, &createInfo, nullptr, &shaderStages[shaderIndex] ) );

		// add module to stage info
		pipelineShaderStageCreateInfos[shaderIndex] = {};
		pipelineShaderStageCreateInfos[shaderIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineShaderStageCreateInfos[shaderIndex].stage = shader->Stage;
		pipelineShaderStageCreateInfos[shaderIndex].module = shaderStages[shaderIndex];
		pipelineShaderStageCreateInfos[shaderIndex].pName = shader->Entrypoint;

		++shaderIndex;
		}

	// bind the vertex attributes descriptions
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &this->VertexInputBindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>( this->VertexInputAttributeDescriptions.size() );
	vertexInputInfo.pVertexAttributeDescriptions = this->VertexInputAttributeDescriptions.data();

	// define the input assembly 
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// set up the viewport and scissor rectangle
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)this->Parent->RenderExtent.width;
	viewport.height = (float)this->Parent->RenderExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = this->Parent->RenderExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; //Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; //Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; //Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; //Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 0;
	dynamicState.pDynamicStates = nullptr;
	dynamicState.dynamicStateCount = 0;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	if( this->DescriptorSetLayoutHandle != nullptr )
		{
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &this->DescriptorSetLayoutHandle;
		}
	else
		{
		pipelineLayoutInfo.setLayoutCount = 0;
		}
	if( this->PushConstantRanges.size() != 0 )
		{
		pipelineLayoutInfo.pushConstantRangeCount = (uint32_t)this->PushConstantRanges.size();
		pipelineLayoutInfo.pPushConstantRanges = this->PushConstantRanges.data();
		}
	VLK_CALL( vkCreatePipelineLayout( this->Parent->Device, &pipelineLayoutInfo, nullptr, &this->PipelineLayout ) );

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = shaderStageCount;
	pipelineInfo.pStages = pipelineShaderStageCreateInfos.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.layout = this->PipelineLayout;
	pipelineInfo.renderPass = this->Parent->RenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	VLK_CALL( vkCreateGraphicsPipelines( this->Parent->Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->Pipeline ) );

	// remove the temporary shader module objects
	for( shaderIndex = 0; shaderIndex < shaderStageCount; ++shaderIndex )
		{
		vkDestroyShaderModule( this->Parent->Device, shaderStages[shaderIndex], nullptr );
		}
	}

void Vlk::GraphicsPipeline::CleanupPipeline()
	{
	if( this->Pipeline == nullptr )
		{
		return;
		}

	// destroy vulkan objects
	vkDestroyPipeline( this->Parent->Device, this->Pipeline, nullptr );
	this->Pipeline = nullptr;
	vkDestroyPipelineLayout( this->Parent->Device, this->PipelineLayout, nullptr );
	this->PipelineLayout = nullptr;
	}

Vlk::GraphicsPipeline::~GraphicsPipeline()
	{
	// remove objects
	this->CleanupPipeline();

	// unregister from parent
	this->Parent->RemoveGraphicsPipeline( this );
	}