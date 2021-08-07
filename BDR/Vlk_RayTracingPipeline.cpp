#include "Vlk_Common.inl"

#include "Vlk_RayTracingPipeline.h"
#include "Vlk_ShaderModule.h"
#include "Vlk_VertexBuffer.h"
#include "Vlk_DescriptorLayout.h"
#include "Vlk_RayTracingShaderBindingTable.h"

using namespace std;

void Vlk::RayTracingPipeline::SetRaygenShader( const ShaderModule* shader )
	{
	this->RaygenShader = shader;
	}

uint Vlk::RayTracingPipeline::AddMissShader( const ShaderModule* shader )
	{
	uint index = (uint)this->MissShaders.size();
	this->MissShaders.push_back( shader );
	return index;
	}

uint Vlk::RayTracingPipeline::AddClosestHitShader( const ShaderModule* shader )
	{
	uint index = (uint)this->ClosestHitShaders.size();
	this->ClosestHitShaders.push_back( shader );
	return index;
	}

void Vlk::RayTracingPipeline::SetSinglePushConstantRange( uint32_t buffersize, VkShaderStageFlags stageFlags )
	{
	std::vector<VkPushConstantRange> ranges;
	ranges.resize( 1 );
	ranges[0].stageFlags = stageFlags;
	ranges[0].offset = 0;
	ranges[0].size = buffersize;
	this->SetPushConstantRanges( ranges );
	}

void Vlk::RayTracingPipeline::SetPushConstantRanges( const std::vector<VkPushConstantRange> &ranges )
	{
	this->PushConstantRanges = ranges;
	}

void Vlk::RayTracingPipeline::SetVkDescriptorSetLayout( VkDescriptorSetLayout descriptorSetLayout )
	{
	this->DescriptorSetLayout = descriptorSetLayout;
	}

void Vlk::RayTracingPipeline::SetDescriptorLayout( const DescriptorLayout* descriptorLayout )
	{
	this->DescriptorSetLayout = descriptorLayout->GetDescriptorSetLayout();;
	}

namespace Vlk
	{
	class _RayTracingPipelineBuilder
		{
		public:
			_RayTracingPipelineBuilder() = default;

			vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroupCreateInfos;
			vector<VkPipelineShaderStageCreateInfo> stageCreateInfos;
			vector<VkShaderModule> shaderModules;

			VkDevice Device = nullptr;

			uint32_t AddShaderStage( const Vlk::ShaderModule* shader_module );

			void AddGeneralShader( const Vlk::ShaderModule* shader_module );
			void AddClosestHitShader( const Vlk::ShaderModule* shader_module );

			~_RayTracingPipelineBuilder();
		};
	}

// returns the index of the shader stage
uint32_t Vlk::_RayTracingPipelineBuilder::AddShaderStage( const Vlk::ShaderModule* shader_module )
	{
	size_t stage_index = stageCreateInfos.size();
	size_t module_index = shaderModules.size();

	// create a shader module wrap around the shader
	VkShaderModule shaderModule = nullptr;
	VkShaderModuleCreateInfo shaderModuleCreateInfo{};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = shader_module->GetShader().size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>( shader_module->GetShader().data() );
	VLK_CALL( vkCreateShaderModule( this->Device, &shaderModuleCreateInfo, nullptr, &shaderModule ) );
	this->shaderModules.push_back( shaderModule );

	// set up the stage info
	VkPipelineShaderStageCreateInfo stageInfo{};
	stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo.stage = shader_module->GetStage();
	stageInfo.module = this->shaderModules[module_index];
	stageInfo.pName = shader_module->GetEntrypoint();
	stageCreateInfos.push_back( stageInfo );

	return (uint)stage_index;
	}

void Vlk::_RayTracingPipelineBuilder::AddGeneralShader( const Vlk::ShaderModule* shader_module )
	{
	// set up the group create info, but leave the shader indices empty, and let the caller fill that in
	VkRayTracingShaderGroupCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	createInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	createInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	createInfo.generalShader = VK_SHADER_UNUSED_KHR;
	createInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
	createInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

	createInfo.generalShader = this->AddShaderStage( shader_module );

	shaderGroupCreateInfos.push_back( createInfo );
	}

void Vlk::_RayTracingPipelineBuilder::AddClosestHitShader( const Vlk::ShaderModule* shader_module )
	{
	// set up the group create info, but leave the shader indices empty, and let the caller fill that in
	VkRayTracingShaderGroupCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	createInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	createInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	createInfo.generalShader = VK_SHADER_UNUSED_KHR;
	createInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
	createInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

	createInfo.closestHitShader = this->AddShaderStage( shader_module );

	shaderGroupCreateInfos.push_back( createInfo );
	}

Vlk::_RayTracingPipelineBuilder::~_RayTracingPipelineBuilder()
	{
	// remove the temporary shader module objects
	for( size_t shaderIndex = 0; shaderIndex < shaderModules.size(); ++shaderIndex )
		{
		vkDestroyShaderModule( this->Device, this->shaderModules[shaderIndex], nullptr );
		}
	}

void Vlk::RayTracingPipeline::BuildPipeline()
	{
	if( this->Pipeline != nullptr )
		{
		return; 
		}

	_RayTracingPipelineBuilder pipelineBuilder;
	pipelineBuilder.Device = this->Parent->GetParent()->GetDevice();

	// add the raygen shader
	if( this->RaygenShader == nullptr || this->RaygenShader->GetStage() != VK_SHADER_STAGE_RAYGEN_BIT_KHR )
		{
		throw runtime_error( "RayTracingPipeline::BuildPipeline() Error: RaygenShader must be a raygen shader with raygen shader stage set" );
		}
	pipelineBuilder.AddGeneralShader( this->RaygenShader );

	// add the miss shaders
	for( size_t i = 0; i < this->MissShaders.size(); ++i )
		{
		if( this->MissShaders[i]->GetStage() != VK_SHADER_STAGE_MISS_BIT_KHR )
			{
			throw runtime_error( "RayTracingPipeline::BuildPipeline() Error: MissShader must be a miss shader with miss shader stage set" );
			}
		pipelineBuilder.AddGeneralShader( this->MissShaders[i] );
		}

	// add the closest hits shaders
	for( size_t i = 0; i < this->ClosestHitShaders.size(); ++i )
		{
		if( this->ClosestHitShaders[i]->GetStage() != VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR )
			{
			throw runtime_error( "RayTracingPipeline::BuildPipeline() Error: ClosestHitShaders must all be set as closest hit shader stage bit" );
			}
		pipelineBuilder.AddClosestHitShader( this->ClosestHitShaders[i] );
		}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	if( this->DescriptorSetLayout != nullptr )
		{
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &this->DescriptorSetLayout;
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
	VLK_CALL( vkCreatePipelineLayout( this->Parent->GetParent()->GetDevice(), &pipelineLayoutInfo, nullptr, &this->PipelineLayout ) );

	VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	pipelineInfo.stageCount = (uint32_t)pipelineBuilder.stageCreateInfos.size();
	pipelineInfo.pStages = pipelineBuilder.stageCreateInfos.data();
	pipelineInfo.groupCount = (uint32_t)pipelineBuilder.shaderGroupCreateInfos.size();
	pipelineInfo.pGroups = pipelineBuilder.shaderGroupCreateInfos.data();
	pipelineInfo.maxPipelineRayRecursionDepth = 2;
	pipelineInfo.layout = this->PipelineLayout;
	
	VLK_CALL( RayTracingExtension::vkCreateRayTracingPipelinesKHR( this->Parent->GetParent()->GetDevice(), nullptr, nullptr, 1, &pipelineInfo, nullptr, &this->Pipeline ) );
	}

void Vlk::RayTracingPipeline::CleanupPipeline()
	{
	if( this->Pipeline == nullptr )
		{
		return;
		}

	// destroy vulkan objects
	vkDestroyPipeline( this->Parent->GetParent()->GetDevice(), this->Pipeline, nullptr );
	this->Pipeline = nullptr;
	vkDestroyPipelineLayout( this->Parent->GetParent()->GetDevice(), this->PipelineLayout, nullptr );
	this->PipelineLayout = nullptr;
	}

static uint32_t alignUp( uint32_t value, uint32_t alignment )
	{
	return ( value + alignment - 1 ) & ~( alignment - 1 );
	}

// calculates the strided device address struct, and moves ptr to the end of the struct.
static VkStridedDeviceAddressRegionKHR GetGroupAddress( uint32_t& ptr, uint group_count, uint32_t alignedHandleSize, uint32_t alignedGroupBaseSize )
	{
	VkStridedDeviceAddressRegionKHR ret{};

	// if group_count is 0, skip this one
	if( group_count == 0 )
		{
		return ret;
		}

	// make sure ptr is group base aligned
	ptr = alignUp( ptr, alignedGroupBaseSize );

	// calculate the size of the range
	ret.deviceAddress = ptr;
	ret.stride = alignedHandleSize;
	ret.size = alignedHandleSize * group_count;

	// move ptr to beyond the range
	ptr = ptr + (uint32_t)ret.size;
	return ret;
	}

static void CopyGroupDataAndUpdateStridedRange( VkStridedDeviceAddressRegionKHR &strided_range, VkDeviceAddress buffer_address , vector<uint8_t> &shaderHandleStorage , void* mapptr , uint &base_index, uint group_count, uint32_t handleSize )
	{
	// if no groups, skip
	if( group_count == 0 )
		{
		return;
		}

	// get src ptr. just base pointer and then jump the number of handles in
	uint8_t* src_ptr = shaderHandleStorage.data() + ( base_index * handleSize );

	// get dst ptr. use mapped ptr, add the range start
	uint8_t* dst_ptr = (uint8_t*)mapptr + strided_range.deviceAddress;

	// update strided_range with buffer_address
	strided_range.deviceAddress += buffer_address;

	// now, copy the groups
	for( uint i = 0; i < group_count; ++i )
		{
		// copy the data
		memcpy(dst_ptr, src_ptr, handleSize );

		// step the poiters
		src_ptr += handleSize;
		dst_ptr += strided_range.stride;
		}

	// update base index with the group count
	base_index += group_count;
	}

Vlk::RayTracingShaderBindingTable* Vlk::RayTracingPipeline::CreateShaderBindingTable() const
	{
	if( this->Pipeline == nullptr )
		{
		throw runtime_error("RayTracingPipeline::CreateShaderBindingTable(): Error: Pipeline has not been built.");
		}
		
	RayTracingShaderBindingTable* buffer = new RayTracingShaderBindingTable();
	buffer->Parent = this->Parent->GetParent();

	// get size of each shader group handle, and also calculate the needed alignment of the handles in the binding table
	uint32_t handleSize = this->Parent->GetRayTracingPipelineProperties().shaderGroupHandleSize;
	uint32_t alignedHandleSize = alignUp( handleSize , this->Parent->GetRayTracingPipelineProperties().shaderGroupHandleAlignment );
	uint32_t alignedGroupBaseSize = this->Parent->GetRayTracingPipelineProperties().shaderGroupBaseAlignment;

	// raygen + misses + closest hits
	uint groupCount = 1 + (uint)this->MissShaders.size() + (uint)this->ClosestHitShaders.size();

	// retrieve the shader handles from the pipeline
	vector<uint8_t> shaderHandleStorage( (size_t)handleSize * (size_t)groupCount );
	VLK_CALL( RayTracingExtension::vkGetRayTracingShaderGroupHandlesKHR( this->Parent->GetParent()->GetDevice(), this->Pipeline, 0, groupCount, (size_t)shaderHandleStorage.size(), shaderHandleStorage.data() ) );

	// calculate the ranges of the handles
	uint32_t index = 0;
	buffer->RaygenDeviceAddress = GetGroupAddress( index, 1, alignedHandleSize , alignedGroupBaseSize );
	buffer->MissDeviceAddress = GetGroupAddress( index, (uint)this->MissShaders.size(), alignedHandleSize, alignedGroupBaseSize );
	buffer->ClosestHitDeviceAddress = GetGroupAddress( index, (uint)this->ClosestHitShaders.size(), alignedHandleSize, alignedGroupBaseSize );
	buffer->CallableDeviceAddress = GetGroupAddress( index, 0, alignedHandleSize, alignedGroupBaseSize );
	VkDeviceSize bufferSize = index;

	// allocate the buffer memory
	buffer->BufferHandle = this->Parent->GetParent()->CreateVulkanBuffer(
		VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY,
		bufferSize,
		buffer->DeviceMemory
	);
	buffer->BufferSize = bufferSize;

	void *mapptr = buffer->MapMemory();
	VkDeviceAddress buffer_address = buffer->GetDeviceAddress();

	uint base_index = 0; // is updated by the methods
	CopyGroupDataAndUpdateStridedRange( buffer->RaygenDeviceAddress, buffer_address, shaderHandleStorage, mapptr, base_index, 1, handleSize );
	CopyGroupDataAndUpdateStridedRange( buffer->MissDeviceAddress, buffer_address, shaderHandleStorage, mapptr, base_index, (uint)this->MissShaders.size(), handleSize );
	CopyGroupDataAndUpdateStridedRange( buffer->ClosestHitDeviceAddress, buffer_address, shaderHandleStorage, mapptr, base_index, (uint)this->ClosestHitShaders.size(), handleSize );
	CopyGroupDataAndUpdateStridedRange( buffer->CallableDeviceAddress, buffer_address, shaderHandleStorage, mapptr, base_index, 0, handleSize );

	buffer->UnmapMemory();

	return buffer;
	}

Vlk::RayTracingPipeline::~RayTracingPipeline()
	{
	// remove objects
	this->CleanupPipeline();

	// unregister from parent
	this->Parent->RemoveRayTracingPipeline( this );
	}