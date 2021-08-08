#include "Vlk_Common.inl"

#include "Vlk_ComputePipeline.h"
#include "Vlk_ShaderModule.h"
#include "Vlk_VertexBuffer.h"
#include "Vlk_DescriptorSetLayout.h"

using namespace std;

void Vlk::ComputePipeline::SetShaderModule( const Vlk::ShaderModule* shader )
	{
	this->Shader = shader;
	}

void Vlk::ComputePipeline::SetVkDescriptorSetLayout( VkDescriptorSetLayout descriptorSetLayout )
	{
	this->DescriptorSetLayoutHandle = descriptorSetLayout;
	}

void Vlk::ComputePipeline::SetDescriptorSetLayout( const DescriptorSetLayout* descriptorLayout )
	{
	this->DescriptorSetLayoutHandle = descriptorLayout->GetDescriptorSetLayout();
	}

void Vlk::ComputePipeline::SetSinglePushConstantRange( uint32_t buffersize, VkShaderStageFlags stageFlags )
	{
	std::vector<VkPushConstantRange> ranges;
	ranges.resize( 1 );
	ranges[0].stageFlags = stageFlags;
	ranges[0].offset = 0;
	ranges[0].size = buffersize;
	this->SetPushConstantRanges( ranges );
	}

void Vlk::ComputePipeline::SetPushConstantRanges( const std::vector<VkPushConstantRange> &ranges )
	{
	this->PushConstantRanges = ranges;
	}

void Vlk::ComputePipeline::BuildPipeline()
	{
	if( this->Pipeline != nullptr )
		{
		return; 
		}
	if( this->Shader == nullptr )
		{
		throw runtime_error( "Error: No shader set in Compute Pipeline" );
		}
		
	
	// create a shader module wrap around the shader
	VkShaderModule shaderModule;
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = this->Shader->Shader.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>( this->Shader->Shader.data() );
	VLK_CALL( vkCreateShaderModule( this->Parent->Device, &createInfo, nullptr, &shaderModule ) );

	// add module to stage info
	VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo;
	pipelineShaderStageCreateInfo = {};
	pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pipelineShaderStageCreateInfo.stage = this->Shader->Stage;
	pipelineShaderStageCreateInfo.module = shaderModule;
	pipelineShaderStageCreateInfo.pName = this->Shader->Entrypoint;

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

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = pipelineShaderStageCreateInfo;
	pipelineInfo.layout = this->PipelineLayout;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	VLK_CALL( vkCreateComputePipelines( this->Parent->Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->Pipeline ) );

	// remove the temporary shader module objects
	vkDestroyShaderModule( this->Parent->Device, shaderModule, nullptr );

	}

void Vlk::ComputePipeline::CleanupPipeline()
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

Vlk::ComputePipeline::~ComputePipeline()
	{
	// remove objects
	this->CleanupPipeline();

	// unregister from parent
	this->Parent->RemoveComputePipeline( this );
	}