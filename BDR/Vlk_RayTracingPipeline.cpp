#include "Vlk_Common.inl"

#include "Vlk_RayTracingPipeline.h"
#include "Vlk_ShaderModule.h"
#include "Vlk_VertexBuffer.h"
#include "Vlk_DescriptorSetLayout.h"
#include "Vlk_RayTracingShaderBindingTable.h"

using std::vector;

void Vlk::RayTracingPipelineTemplate::SetRaygenShaderModule( const ShaderModule* shader )
	{
	this->RaygenShader = shader;
	}

uint Vlk::RayTracingPipelineTemplate::AddMissShaderModule( const ShaderModule* shader )
	{
	uint index = (uint)this->MissShaders.size();
	this->MissShaders.push_back( shader );
	return index;
	}

uint Vlk::RayTracingPipelineTemplate::AddClosestHitShaderModule( const ShaderModule* shader )
	{
	uint index = (uint)this->ClosestHitShaders.size();
	this->ClosestHitShaders.push_back( shader );
	return index;
	}

Vlk::RayTracingPipelineTemplate::RayTracingPipelineTemplate()
	{
	// pipeline layout is initially empty
	this->PipelineLayoutCreateInfo.setLayoutCount = 0;
	this->PipelineLayoutCreateInfo.pushConstantRangeCount = 0;

	// set the base values
	this->RayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 2;

	}


uint Vlk::RayTracingPipelineTemplate::AddDescriptorSetLayout( const DescriptorSetLayout* descriptorLayout )
	{
	uint index = (uint)this->DescriptorSetLayouts.size();
	this->DescriptorSetLayouts.emplace_back( descriptorLayout->GetDescriptorSetLayout() );
	this->PipelineLayoutCreateInfo.setLayoutCount = (uint32_t)this->DescriptorSetLayouts.size();
	this->PipelineLayoutCreateInfo.pSetLayouts = this->DescriptorSetLayouts.data();
	return index;
	}

uint Vlk::RayTracingPipelineTemplate::AddPushConstantRange( VkPushConstantRange range )
	{
	uint index = (uint)this->PushConstantRanges.size();
	this->PushConstantRanges.emplace_back( range );
	this->PipelineLayoutCreateInfo.pushConstantRangeCount = (uint32_t)this->PushConstantRanges.size();
	this->PipelineLayoutCreateInfo.pPushConstantRanges = this->PushConstantRanges.data();
	return index;
	}

uint Vlk::RayTracingPipelineTemplate::AddPushConstantRange( VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size )
	{
	VkPushConstantRange range = {};
	range.stageFlags = stageFlags;
	range.offset = offset;
	range.size = size;
	return AddPushConstantRange( range );
	}

Vlk::RayTracingPipeline::~RayTracingPipeline() 
	{
	};