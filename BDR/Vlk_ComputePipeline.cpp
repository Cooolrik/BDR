#include "Vlk_Common.inl"

#include "Vlk_ComputePipeline.h"
#include "Vlk_ShaderModule.h"
#include "Vlk_VertexBuffer.h"
#include "Vlk_DescriptorSetLayout.h"

using std::vector;


Vlk::ComputePipelineTemplate::ComputePipelineTemplate()
	{
	// pipeline layout is initially empty
	this->PipelineLayoutCreateInfo.setLayoutCount = 0;
	this->PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	}

void Vlk::ComputePipelineTemplate::SetShaderModule( const ShaderModule* shader )
	{
	this->Shader = shader;
	}

unsigned int Vlk::ComputePipelineTemplate::AddDescriptorSetLayout( const DescriptorSetLayout* descriptorLayout )
	{
	uint index = (uint)this->DescriptorSetLayouts.size();
	this->DescriptorSetLayouts.emplace_back( descriptorLayout->GetDescriptorSetLayout() );
	this->PipelineLayoutCreateInfo.setLayoutCount = (uint32_t)this->DescriptorSetLayouts.size();
	this->PipelineLayoutCreateInfo.pSetLayouts = this->DescriptorSetLayouts.data();
	return index;
	}

unsigned int Vlk::ComputePipelineTemplate::AddPushConstantRange( VkPushConstantRange range )
	{
	uint index = (uint)this->PushConstantRanges.size();
	this->PushConstantRanges.emplace_back( range );
	this->PipelineLayoutCreateInfo.pushConstantRangeCount = (uint32_t)this->PushConstantRanges.size();
	this->PipelineLayoutCreateInfo.pPushConstantRanges = this->PushConstantRanges.data();
	return index;
	}

unsigned int Vlk::ComputePipelineTemplate::AddPushConstantRange( VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size )
	{
	VkPushConstantRange range = {};
	range.stageFlags = stageFlags;
	range.offset = offset;
	range.size = size;
	return AddPushConstantRange( range );
	}

