#include "Vlk_Common.inl"
#include "Vlk_RayTracingExtension.h"
#include "Vlk_RayTracingAccelerationStructure.h"
#include "Vlk_RayTracingBLASEntry.h"
#include "Vlk_RayTracingTLASEntry.h"
#include "Vlk_RayTracingPipeline.h"
#include "Vlk_RayTracingShaderBindingTable.h"

#include "Vlk_RayTracingAccelerationStructure.h"
#include "Vlk_Buffer.h"
#include "Vlk_CommandPool.h"
#include "Vlk_ShaderModule.h"

#include <stdexcept>
#include <algorithm>

void Vlk::RayTracingExtension::RemoveRayTracingPipeline( RayTracingPipeline* pipeline )
	{
	auto it = this->RayTracingPipelines.find( pipeline );
	if( it == this->RayTracingPipelines.end() )
		{
		throw runtime_error( "Error: RemoveRayTracingPipeline() pipeline is not registered with the renderer, have you already removed it?" );
		}
	this->RayTracingPipelines.erase( it );
	}

Vlk::RayTracingAccelerationStructure* Vlk::RayTracingExtension::CreateAccBuffer( VkAccelerationStructureCreateInfoKHR createInfo )
	{
	RayTracingAccelerationStructure* buffer = new RayTracingAccelerationStructure( this );

	// allocate the buffer memory
	buffer->ASBuffer = std::unique_ptr<Buffer>( 
		this->Module->CreateBuffer( 
			BufferTemplate::ManualBuffer(
				VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY,
				createInfo.size 
				)
			)
		);

	// create the acceleration struct
	createInfo.buffer = buffer->ASBuffer->GetBuffer();
	VLK_CALL( vkCreateAccelerationStructureKHR( this->Module->GetDevice(), &createInfo, nullptr, &buffer->AccelerationStructure ) );

	return buffer;
	}

VkCommandPool Vlk::RayTracingExtension::CreateInternalCommandPool()
	{
	VkCommandPool cmdPool{};
	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = this->Module->PhysicalDeviceQueueGraphicsFamily;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	VLK_CALL( vkCreateCommandPool( this->Module->Device, &commandPoolCreateInfo, nullptr, &cmdPool ) );
	return cmdPool;
	}

void Vlk::RayTracingExtension::CreateInternalCommandBuffers( VkCommandPool cmdPool , uint32_t num_entries , VkCommandBuffer *cmdBuffers )
	{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = cmdPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = num_entries;
	VLK_CALL( vkAllocateCommandBuffers( this->Module->Device, &commandBufferAllocateInfo, cmdBuffers ) );
	}

void Vlk::RayTracingExtension::SubmitAndFreeInternalCommandBuffers( VkCommandPool cmdPool, uint32_t num_entries, VkCommandBuffer* cmdBuffers )
	{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = num_entries;
	submitInfo.pCommandBuffers = cmdBuffers;
	VLK_CALL( vkQueueSubmit( this->Module->GraphicsQueue, 1, &submitInfo, nullptr ) );
	VLK_CALL( vkQueueWaitIdle( this->Module->GraphicsQueue ) );
	vkFreeCommandBuffers( this->Module->Device, cmdPool, (uint32_t)num_entries, cmdBuffers );
	}

VkResult Vlk::RayTracingExtension::BeginInternalCommandBuffer( VkCommandBuffer cmdBuffer )
	{
	// begin the command buffer
	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;
	VLK_CALL( vkBeginCommandBuffer( cmdBuffer, &commandBufferBeginInfo ) );
	return VkResult::VK_SUCCESS;
	}

void Vlk::RayTracingExtension::BuildBLAS( const std::vector<RayTracingBLASEntry*>& BLASEntries )
	{
	if( !this->BLASes.empty() )
		{
		throw runtime_error( "RayTracingExtension::BuildBLAS() Error: Already set up" );
		}
	
	size_t num_entries = BLASEntries.size();

	// temporary structures where the data is stored before compacting. also resize the array with the final compacted structures
	std::vector<RayTracingAccelerationStructure*> nonCompactedBLASes( num_entries );
	this->BLASes.resize( num_entries );

	// set up build structs for blas processings. one structure per blas entry
	// set destination structure to null for now, we will poll in the next step the needed allocation and allocate the struct
	std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildGeometryInfo( num_entries );
	for( size_t i = 0; i < num_entries; ++i )
		{
		RayTracingBLASEntry* entry = BLASEntries[i];

		buildGeometryInfo[i].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildGeometryInfo[i].flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
		buildGeometryInfo[i].geometryCount = (uint32_t)entry->GetGeometryCount();
		buildGeometryInfo[i].pGeometries = entry->GetAccelerationStructureGeometries().data();
		buildGeometryInfo[i].mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildGeometryInfo[i].type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildGeometryInfo[i].srcAccelerationStructure = nullptr;
		buildGeometryInfo[i].dstAccelerationStructure = nullptr;
		}

	// poll the largest needed scratch space. we will reuse the scratch space for each processing
	// so we need to know how big it needs to be.
	VkDeviceSize maxScratchSpace = 0;
	std::vector<VkDeviceSize> nonCompactedSizes( num_entries );
	std::vector<VkDeviceSize> compactedSizes( num_entries );
	for( size_t i = 0; i < num_entries; ++i )
		{
		RayTracingBLASEntry* entry = BLASEntries[i];

		// collect the number of primitives for each geometry in the current blas entry
		size_t geom_count = entry->GetGeometryCount();
		std::vector<uint32_t> maxPrimitiveCount( geom_count );
		for( size_t q = 0; q < geom_count; ++q )
			{
			maxPrimitiveCount[q] = entry->GetAccelerationStructureBuildRangeInfos()[q].primitiveCount;
			}
			
		// poll the size info for the structure and needed scratch space
		VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
		buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		this->vkGetAccelerationStructureBuildSizesKHR( this->Module->GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildGeometryInfo[i], maxPrimitiveCount.data(), &buildSizesInfo );

		// Create acceleration structure object to receive the acceleration data
		// set the worst case memory size to allocate. this will be compacted later
		VkAccelerationStructureCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		createInfo.size = buildSizesInfo.accelerationStructureSize; 
		nonCompactedBLASes[i] = this->CreateAccBuffer( createInfo );

		// set the created buffer as destination structure in the buildGeometryInfo list
		buildGeometryInfo[i].dstAccelerationStructure = nonCompactedBLASes[i]->AccelerationStructure;

		// update the size stats
		maxScratchSpace = std::max( maxScratchSpace, buildSizesInfo.buildScratchSize );
		nonCompactedSizes[i] = buildSizesInfo.accelerationStructureSize;
		}

	// Allocate the scrach space. It is sized to be able to handle any of the entries
	Buffer* scratchBuffer = this->Module->CreateBuffer(
		BufferTemplate::ManualBuffer(
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY,
			maxScratchSpace
			)
		);
	VkDeviceAddress scratchAddress = scratchBuffer->GetDeviceAddress();

	// Allocate a query pool for querying size needed for post-compaction
	VkQueryPoolCreateInfo queryPoolCreateInfo{};
	queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	queryPoolCreateInfo.queryCount = (uint32_t)num_entries;
	queryPoolCreateInfo.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
	VkQueryPool queryPool;
	VLK_CALL( vkCreateQueryPool( this->Module->GetDevice(), &queryPoolCreateInfo, nullptr, &queryPool ) );
	vkResetQueryPool( this->Module->GetDevice(), queryPool, 0, (uint32_t)num_entries );

	// Create a command pool to generate the structures. Allocate one command buffer per entry, as to not time out the driver.
	VkCommandPool cmdPool = this->CreateInternalCommandPool();
	std::vector<VkCommandBuffer> cmdBuffers( num_entries );
	this->CreateInternalCommandBuffers( cmdPool, (uint32_t)num_entries, cmdBuffers.data() );

	// do each entry in a separate command buffer	
	for( size_t i = 0; i < num_entries; ++i )
		{
		RayTracingBLASEntry* entry = BLASEntries[i];
		size_t geom_count = entry->GetGeometryCount();

		VLK_CALL( this->BeginInternalCommandBuffer( cmdBuffers[i] ) );

		// set the scratch address for the build (reused by each run)
		buildGeometryInfo[i].scratchData.deviceAddress = scratchAddress;

		// setup a vector of pointers to the build range info structs, this (struct**) is how Vulkan wants it presented.
		std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> buildRangeInfos;
		buildRangeInfos.resize( geom_count );
		for( size_t q = 0; q < geom_count; ++q )
			{
			buildRangeInfos[q] = &entry->GetAccelerationStructureBuildRangeInfos()[q];
			}
		
		// create the blas 
		vkCmdBuildAccelerationStructuresKHR( cmdBuffers[i], 1, &buildGeometryInfo[i], buildRangeInfos.data() );
		
		// add a memory barrier for the scratch memory, to make sure the command buffers run sequentially
		// and that one is done writing before the next starts reading
		VkMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
		vkCmdPipelineBarrier(
			cmdBuffers[i], 
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 
			0, 1, &barrier, 0, nullptr, 0, nullptr 
			);

		// store the calculated compacted size to the query pool
		vkCmdWriteAccelerationStructuresPropertiesKHR( 
			cmdBuffers[i], 
			1, &nonCompactedBLASes[i]->AccelerationStructure,
			VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, 
			queryPool, (uint32_t)i 
			);

		// end the command buffer
		VLK_CALL( vkEndCommandBuffer( cmdBuffers[i] ) );
		}

	// submit the command buffers to the graphics queue, wait synchronously for it to finish, and remove the buffers
	this->SubmitAndFreeInternalCommandBuffers( cmdPool, (uint32_t)num_entries, cmdBuffers.data() );

	// get the compacted sizes
	vkGetQueryPoolResults( 
		this->Module->Device, 
		queryPool, 0, 
		(uint32_t)compactedSizes.size(), compactedSizes.size() * sizeof( VkDeviceSize ), compactedSizes.data(), sizeof( VkDeviceSize ), 
		VK_QUERY_RESULT_WAIT_BIT 
		);

	// create a new command buffer that copies and compacts 
	VkCommandBuffer compactingCmdBuffer{};
	this->CreateInternalCommandBuffers( cmdPool, 1, &compactingCmdBuffer );
	VLK_CALL( this->BeginInternalCommandBuffer( compactingCmdBuffer ) );

	// add new smaller copies and add copy commands to the command buffer
	for( size_t i = 0; i < num_entries; ++i )
		{
		// Create acceleration structure object to receive the acceleration data
		// set the worst case memory size to allocate. this will be compacted later
		VkAccelerationStructureCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		createInfo.size = compactedSizes[i];
		this->BLASes[i] = this->CreateAccBuffer( createInfo );

		// Copy while compacting the blas structure
		VkCopyAccelerationStructureInfoKHR copyInfo{ VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR };
		copyInfo.src = nonCompactedBLASes[i]->AccelerationStructure;
		copyInfo.dst = this->BLASes[i]->AccelerationStructure;
		copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
		vkCmdCopyAccelerationStructureKHR( compactingCmdBuffer, &copyInfo );
		}

	// submit the copy commands
	VLK_CALL( vkEndCommandBuffer( compactingCmdBuffer ) );
	this->SubmitAndFreeInternalCommandBuffers( cmdPool, 1, &compactingCmdBuffer );

	// remove the temporary allocated blases
	for( size_t i = 0; i < num_entries; ++i )
		{
		delete nonCompactedBLASes[i];
		}

	// done with the pools and scratch buffer
	vkDestroyCommandPool( this->Module->Device, cmdPool, nullptr );
	vkDestroyQueryPool( this->Module->GetDevice(), queryPool, nullptr );
	delete scratchBuffer;
	}

void Vlk::RayTracingExtension::BuildTLAS( const std::vector<RayTracingTLASEntry*>& TLASEntries )
	{
	if( this->TLAS != nullptr )
		{
		throw runtime_error( "RayTracingExtension::BuildBLAS() Error: Already set up" );
		}

	size_t num_entries = TLASEntries.size();

	// Create a command pool and buffer to generate the structures. 
	VkCommandPool cmdPool = this->CreateInternalCommandPool();
	VkCommandBuffer cmdBuffer;
	this->CreateInternalCommandBuffers( cmdPool, 1, &cmdBuffer );
	VLK_CALL( this->BeginInternalCommandBuffer( cmdBuffer ) );

	// set up a staging buffer with the TLAS instances
	VkDeviceSize tlas_buffer_size = num_entries * sizeof( VkAccelerationStructureInstanceKHR );
	Buffer* stagingBuffer = this->Module->CreateBuffer(
		BufferTemplate::ManualBuffer( 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY,
			tlas_buffer_size
			)
		);
	
	// set up TLAS instance structures
	VkAccelerationStructureInstanceKHR* TLASInstances = (VkAccelerationStructureInstanceKHR*)stagingBuffer->MapMemory();
	for( size_t i = 0; i < num_entries; ++i )
		{
		RayTracingTLASEntry* entry = TLASEntries[i];
		VkAccelerationStructureInstanceKHR& instance = TLASInstances[i];

		// copy values
		memcpy( instance.transform.matrix, entry->Transformation, sizeof( float ) * 12 );
		instance.instanceCustomIndex = entry->InstanceCustomIndex;
		instance.mask = entry->Mask;
		instance.instanceShaderBindingTableRecordOffset = entry->HitGroupId;
		instance.flags = entry->Flags;

		// retrieve the address of the blas
		VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo{};
		deviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		deviceAddressInfo.accelerationStructure = BLASes[entry->BlasId]->GetAccelerationStructure();
		instance.accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR( this->Module->GetDevice(), &deviceAddressInfo );
		}
	stagingBuffer->UnmapMemory();

	// set up a copy on the gpu to copy to
	Buffer* TLASInstancesBuffer = this->Module->CreateBuffer(
		BufferTemplate::ManualBuffer(
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
			VMA_MEMORY_USAGE_GPU_ONLY,
			tlas_buffer_size
		)
	);

	// set up copy from staging buffer to on device buffer
	VkBufferCopy copyRegion{};
	copyRegion.size = tlas_buffer_size;
	vkCmdCopyBuffer( cmdBuffer, stagingBuffer->GetBuffer(), TLASInstancesBuffer->GetBuffer(), 1, &copyRegion );

	// Memory barrier, to make sure the copy from staging is done before continuing
	VkMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
	vkCmdPipelineBarrier( cmdBuffer, 
		VK_PIPELINE_STAGE_TRANSFER_BIT, 
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		0, 
		1, &barrier, 
		0, nullptr, 
		0, nullptr 
		);

	// set up the instances acceleration structure, point at the on-gpu instance data
	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	accelerationStructureGeometry.geometry.instances.data.deviceAddress = TLASInstancesBuffer->GetDeviceAddress();

	// Set up build information for the acceleration structure
	VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
	buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	buildGeometryInfo.geometryCount = 1;
	buildGeometryInfo.pGeometries = &accelerationStructureGeometry;
	buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	buildGeometryInfo.srcAccelerationStructure = nullptr;

	// Get needed acceleration structure and scratch space sizes
	uint32_t count = (uint32_t)num_entries;
	VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
	buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR( this->Module->GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildGeometryInfo, &count, &buildSizesInfo );

	// allocate the TLAS
	VkAccelerationStructureCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	createInfo.size = buildSizesInfo.accelerationStructureSize;
	this->TLAS = this->CreateAccBuffer( createInfo );

	// allocate the scratch buffer
	Buffer* scratchBuffer = this->Module->CreateBuffer(
		BufferTemplate::ManualBuffer(
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
			VMA_MEMORY_USAGE_GPU_ONLY,
			buildSizesInfo.buildScratchSize
		)
	);
	VkDeviceAddress scratchAddress = scratchBuffer->GetDeviceAddress();

	// add the buffers into the build info
	buildGeometryInfo.dstAccelerationStructure = this->TLAS->AccelerationStructure;
	buildGeometryInfo.scratchData.deviceAddress = scratchAddress;

	// Setup build range for creation
	VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
	buildRangeInfo.primitiveCount = (uint32_t)num_entries;
	const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildRangeInfo;

	// Build the TLAS
	vkCmdBuildAccelerationStructuresKHR( cmdBuffer, 1, &buildGeometryInfo, &pBuildOffsetInfo );

	// submit the copy commands
	VLK_CALL( vkEndCommandBuffer( cmdBuffer ) );
	this->SubmitAndFreeInternalCommandBuffers( cmdPool, 1, &cmdBuffer );
	vkDestroyCommandPool( this->Module->Device, cmdPool, nullptr );

	// delete temp memorys
	delete scratchBuffer;
	delete TLASInstancesBuffer;
	delete stagingBuffer;
	}

Vlk::RayTracingAccelerationStructure* Vlk::RayTracingExtension::GetTLAS()
	{
	return this->TLAS;
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

Vlk::RayTracingPipeline* Vlk::RayTracingExtension::CreateRayTracingPipeline( const RayTracingPipelineTemplate& rtt )
	{
	RayTracingPipeline* pipeline = new RayTracingPipeline( this );
	pipeline->PipelineBindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
	pipeline->MissShadersCount = (uint)rtt.MissShaders.size();
	pipeline->ClosestHitShadersCount = (uint)rtt.ClosestHitShaders.size();

	_RayTracingPipelineBuilder pipelineBuilder;
	pipelineBuilder.Device = this->Module->GetDevice();

	// add the raygen shader
	if( rtt.RaygenShader == nullptr || rtt.RaygenShader->GetStage() != VK_SHADER_STAGE_RAYGEN_BIT_KHR )
		{
		throw runtime_error( "RayTracingExtension::CreateRayTracingPipeline() Error: RaygenShader must be a raygen shader with raygen shader stage set" );
		}
	pipelineBuilder.AddGeneralShader( rtt.RaygenShader );

	// add the miss shaders
	for( size_t i = 0; i < rtt.MissShaders.size(); ++i )
		{
		if( rtt.MissShaders[i]->GetStage() != VK_SHADER_STAGE_MISS_BIT_KHR )
			{
			throw runtime_error( "RayTracingExtension::CreateRayTracingPipeline() Error: MissShader must be a miss shader with miss shader stage set" );
			}
		pipelineBuilder.AddGeneralShader( rtt.MissShaders[i] );
		}

	// add the closest hits shaders
	for( size_t i = 0; i < rtt.ClosestHitShaders.size(); ++i )
		{
		if( rtt.ClosestHitShaders[i]->GetStage() != VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR )
			{
			throw runtime_error( "RayTracingExtension::CreateRayTracingPipeline() Error: ClosestHitShaders must all be set as closest hit shader stage bit" );
			}
		pipelineBuilder.AddClosestHitShader( rtt.ClosestHitShaders[i] );
		}


	// create the layout first
	VLK_CALL( vkCreatePipelineLayout( this->Module->GetDevice(), &rtt.PipelineLayoutCreateInfo, nullptr, &pipeline->PipelineLayout ) );

	// then create the pipeline
	VkRayTracingPipelineCreateInfoKHR pipelineInfo = rtt.RayTracingPipelineCreateInfo;
	pipelineInfo.stageCount = (uint32_t)pipelineBuilder.stageCreateInfos.size();
	pipelineInfo.pStages = pipelineBuilder.stageCreateInfos.data();
	pipelineInfo.groupCount = (uint32_t)pipelineBuilder.shaderGroupCreateInfos.size();
	pipelineInfo.pGroups = pipelineBuilder.shaderGroupCreateInfos.data();
	pipelineInfo.maxPipelineRayRecursionDepth = 2;
	pipelineInfo.layout = pipeline->PipelineLayout;
	VLK_CALL( RayTracingExtension::vkCreateRayTracingPipelinesKHR( this->Module->GetDevice(), nullptr, nullptr, 1, &pipelineInfo, nullptr, &pipeline->PipelineHandle ) );


	return pipeline;
	}

VkResult Vlk::RayTracingExtension::PostCreateInstance()
	{
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCreateAccelerationStructureKHR );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkDestroyAccelerationStructureKHR );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCmdBuildAccelerationStructuresKHR );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCmdBuildAccelerationStructuresIndirectKHR );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkBuildAccelerationStructuresKHR );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCopyAccelerationStructureKHR );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCopyAccelerationStructureToMemoryKHR );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCopyMemoryToAccelerationStructureKHR );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkWriteAccelerationStructuresPropertiesKHR );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCmdCopyAccelerationStructureKHR );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCmdCopyAccelerationStructureToMemoryKHR );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCmdCopyMemoryToAccelerationStructureKHR );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkGetAccelerationStructureDeviceAddressKHR );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCmdWriteAccelerationStructuresPropertiesKHR );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkGetDeviceAccelerationStructureCompatibilityKHR );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkGetAccelerationStructureBuildSizesKHR );

	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCmdSetRayTracingPipelineStackSizeKHR );             
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCmdTraceRaysIndirectKHR );                          
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCmdTraceRaysKHR );									
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCreateRayTracingPipelinesKHR );                     
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkGetRayTracingCaptureReplayShaderGroupHandlesKHR );  
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkGetRayTracingShaderGroupHandlesKHR );               
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkGetRayTracingShaderGroupStackSizeKHR );             

	return VkResult::VK_SUCCESS;
	}

VkResult Vlk::RayTracingExtension::AddRequiredDeviceExtensions( 
	VkPhysicalDeviceFeatures2* physicalDeviceFeatures,
	VkPhysicalDeviceProperties2* physicalDeviceProperties,
	std::vector<const char*>* extensionList
	)
	{
	// enable extensions needed for ray tracing
	Extension::AddExtensionToList( extensionList, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME );
	Extension::AddExtensionToList( extensionList, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME );
	Extension::AddExtensionToList( extensionList, VK_KHR_MAINTENANCE3_EXTENSION_NAME );
	Extension::AddExtensionToList( extensionList, VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME );
	Extension::AddExtensionToList( extensionList, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME );
	
	// set up query structs

	// features
	VR_ADD_STRUCT_TO_VULKAN_LINKED_LIST( physicalDeviceFeatures, this->AccelerationStructureFeaturesQuery, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR );
	VR_ADD_STRUCT_TO_VULKAN_LINKED_LIST( physicalDeviceFeatures, this->RayTracingPipelineFeaturesQuery, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR );
	VR_ADD_STRUCT_TO_VULKAN_LINKED_LIST( physicalDeviceFeatures, this->HostQueryResetFeaturesQuery, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES );
	
	// properties
	VR_ADD_STRUCT_TO_VULKAN_LINKED_LIST( physicalDeviceProperties, this->AccelerationStructureProperties, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR );
	VR_ADD_STRUCT_TO_VULKAN_LINKED_LIST( physicalDeviceProperties, this->RayTracingPipelineProperties, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR );

	return VkResult::VK_SUCCESS;
	}

bool Vlk::RayTracingExtension::SelectDevice(
	const VkSurfaceCapabilitiesKHR& surfaceCapabilities,
	const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats,
	const std::vector<VkPresentModeKHR>& availablePresentModes,
	const VkPhysicalDeviceFeatures2& physicalDeviceFeatures,
	const VkPhysicalDeviceProperties2& physicalDeviceProperties
	)
	{
	UNREFERENCED_PARAMETER( surfaceCapabilities );
	UNREFERENCED_PARAMETER( availableSurfaceFormats );
	UNREFERENCED_PARAMETER( availablePresentModes );
	UNREFERENCED_PARAMETER( physicalDeviceFeatures );
	UNREFERENCED_PARAMETER( physicalDeviceProperties );

	// check for needed features
	if( !this->AccelerationStructureFeaturesQuery.accelerationStructure )
		return false;
	if( !this->RayTracingPipelineFeaturesQuery.rayTracingPipeline )
		return false;
	if( !this->HostQueryResetFeaturesQuery.hostQueryReset )
		return false;

	return true;
	}

VkResult Vlk::RayTracingExtension::CreateDevice( VkDeviceCreateInfo* deviceCreateInfo )
	{
	VR_ADD_STRUCT_TO_VULKAN_LINKED_LIST( deviceCreateInfo, this->AccelerationStructureFeaturesCreate, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR );
	VR_ADD_STRUCT_TO_VULKAN_LINKED_LIST( deviceCreateInfo, this->RayTracingPipelineFeaturesCreate, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR );
	VR_ADD_STRUCT_TO_VULKAN_LINKED_LIST( deviceCreateInfo, this->HostQueryResetFeaturesCreate, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES );

	// enable required features
	this->AccelerationStructureFeaturesCreate.accelerationStructure = VK_TRUE;
	this->RayTracingPipelineFeaturesCreate.rayTracingPipeline = VK_TRUE;
	this->HostQueryResetFeaturesCreate.hostQueryReset = VK_TRUE;

	return VkResult::VK_SUCCESS;
	}

VkResult Vlk::RayTracingExtension::PostCreateDevice()
	{
	return VkResult::VK_SUCCESS;
	}

VkResult Vlk::RayTracingExtension::Cleanup()
	{
	// remove TLAS acceleration structure
	if( this->TLAS )
		{
		delete this->TLAS;
		this->TLAS = nullptr;
		}

	// remove all BLAS acceleration structures
	for( size_t i = 0; i < this->BLASes.size(); ++i )
		{
		delete this->BLASes[i];
		}
	this->BLASes.clear();


	return VkResult::VK_SUCCESS;
	}


inline uint32_t alignUp( uint32_t value, uint32_t alignment )
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

static void CopyGroupDataAndUpdateStridedRange( VkStridedDeviceAddressRegionKHR& strided_range, VkDeviceAddress buffer_address, vector<uint8_t>& shaderHandleStorage, void* mapptr, uint& base_index, uint group_count, uint32_t handleSize )
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
		memcpy( dst_ptr, src_ptr, handleSize );

		// step the poiters
		src_ptr += handleSize;
		dst_ptr += strided_range.stride;
		}

	// update base index with the group count
	base_index += group_count;
	}

Vlk::RayTracingShaderBindingTable* Vlk::RayTracingExtension::CreateShaderBindingTable( const RayTracingPipeline *pipeline )
	{
	RayTracingShaderBindingTable* btable = new RayTracingShaderBindingTable( this );

	// get size of each shader group handle, and also calculate the needed alignment of the handles in the binding table
	uint32_t handleSize = this->GetRayTracingPipelineProperties().shaderGroupHandleSize;
	uint32_t alignedHandleSize = alignUp( handleSize, this->GetRayTracingPipelineProperties().shaderGroupHandleAlignment );
	uint32_t alignedGroupBaseSize = this->GetRayTracingPipelineProperties().shaderGroupBaseAlignment;

	// raygen + misses + closest hits
	uint groupCount = 1 + pipeline->GetMissShadersCount() + pipeline->GetClosestHitShadersCount();

	// retrieve the shader handles from the pipeline
	vector<uint8_t> shaderHandleStorage( (size_t)handleSize * (size_t)groupCount );
	VLK_CALL( RayTracingExtension::vkGetRayTracingShaderGroupHandlesKHR( this->GetModule()->GetDevice(), pipeline->GetPipeline(), 0, groupCount, (size_t)shaderHandleStorage.size(), shaderHandleStorage.data() ) );

	// calculate the ranges of the handles
	uint32_t index = 0;
	btable->RaygenDeviceAddress = GetGroupAddress( index, 1, alignedHandleSize, alignedGroupBaseSize );
	btable->MissDeviceAddress = GetGroupAddress( index, pipeline->GetMissShadersCount(), alignedHandleSize, alignedGroupBaseSize );
	btable->ClosestHitDeviceAddress = GetGroupAddress( index, pipeline->GetClosestHitShadersCount(), alignedHandleSize, alignedGroupBaseSize );
	btable->CallableDeviceAddress = GetGroupAddress( index, 0, alignedHandleSize, alignedGroupBaseSize );
	VkDeviceSize bufferSize = index;

	// allocate the btable memory
	btable->BufferPtr = std::unique_ptr<Buffer>(
		this->Module->CreateBuffer(
			BufferTemplate::ManualBuffer(
				VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VMA_MEMORY_USAGE_CPU_ONLY,
				bufferSize
			)
		)
		);

	void* mapptr = btable->BufferPtr->MapMemory();
	VkDeviceAddress buffer_address = btable->BufferPtr->GetDeviceAddress();

	uint base_index = 0; // is updated by the methods
	CopyGroupDataAndUpdateStridedRange( btable->RaygenDeviceAddress, buffer_address, shaderHandleStorage, mapptr, base_index, 1, handleSize );
	CopyGroupDataAndUpdateStridedRange( btable->MissDeviceAddress, buffer_address, shaderHandleStorage, mapptr, base_index, pipeline->GetMissShadersCount(), handleSize );
	CopyGroupDataAndUpdateStridedRange( btable->ClosestHitDeviceAddress, buffer_address, shaderHandleStorage, mapptr, base_index, pipeline->GetClosestHitShadersCount(), handleSize );
	CopyGroupDataAndUpdateStridedRange( btable->CallableDeviceAddress, buffer_address, shaderHandleStorage, mapptr, base_index, 0, handleSize );

	btable->BufferPtr->UnmapMemory();

	return btable;
	}


Vlk::RayTracingExtension::~RayTracingExtension()
	{
	}

// VK_KHR_acceleration_structure
PFN_vkCreateAccelerationStructureKHR Vlk::RayTracingExtension::vkCreateAccelerationStructureKHR = nullptr;
PFN_vkDestroyAccelerationStructureKHR Vlk::RayTracingExtension::vkDestroyAccelerationStructureKHR = nullptr;
PFN_vkCmdBuildAccelerationStructuresKHR Vlk::RayTracingExtension::vkCmdBuildAccelerationStructuresKHR = nullptr;
PFN_vkCmdBuildAccelerationStructuresIndirectKHR Vlk::RayTracingExtension::vkCmdBuildAccelerationStructuresIndirectKHR = nullptr;
PFN_vkBuildAccelerationStructuresKHR Vlk::RayTracingExtension::vkBuildAccelerationStructuresKHR = nullptr;
PFN_vkCopyAccelerationStructureKHR Vlk::RayTracingExtension::vkCopyAccelerationStructureKHR = nullptr;
PFN_vkCopyAccelerationStructureToMemoryKHR Vlk::RayTracingExtension::vkCopyAccelerationStructureToMemoryKHR = nullptr;
PFN_vkCopyMemoryToAccelerationStructureKHR Vlk::RayTracingExtension::vkCopyMemoryToAccelerationStructureKHR = nullptr;
PFN_vkWriteAccelerationStructuresPropertiesKHR Vlk::RayTracingExtension::vkWriteAccelerationStructuresPropertiesKHR = nullptr;
PFN_vkCmdCopyAccelerationStructureKHR Vlk::RayTracingExtension::vkCmdCopyAccelerationStructureKHR = nullptr;
PFN_vkCmdCopyAccelerationStructureToMemoryKHR Vlk::RayTracingExtension::vkCmdCopyAccelerationStructureToMemoryKHR = nullptr;
PFN_vkCmdCopyMemoryToAccelerationStructureKHR Vlk::RayTracingExtension::vkCmdCopyMemoryToAccelerationStructureKHR = nullptr;
PFN_vkGetAccelerationStructureDeviceAddressKHR Vlk::RayTracingExtension::vkGetAccelerationStructureDeviceAddressKHR = nullptr;
PFN_vkCmdWriteAccelerationStructuresPropertiesKHR Vlk::RayTracingExtension::vkCmdWriteAccelerationStructuresPropertiesKHR = nullptr;
PFN_vkGetDeviceAccelerationStructureCompatibilityKHR Vlk::RayTracingExtension::vkGetDeviceAccelerationStructureCompatibilityKHR = nullptr;
PFN_vkGetAccelerationStructureBuildSizesKHR Vlk::RayTracingExtension::vkGetAccelerationStructureBuildSizesKHR = nullptr;

// VK_KHR_ray_tracing_pipeline
PFN_vkCmdSetRayTracingPipelineStackSizeKHR Vlk::RayTracingExtension::vkCmdSetRayTracingPipelineStackSizeKHR = nullptr;
PFN_vkCmdTraceRaysIndirectKHR Vlk::RayTracingExtension::vkCmdTraceRaysIndirectKHR = nullptr;
PFN_vkCmdTraceRaysKHR Vlk::RayTracingExtension::vkCmdTraceRaysKHR = nullptr;
PFN_vkCreateRayTracingPipelinesKHR Vlk::RayTracingExtension::vkCreateRayTracingPipelinesKHR = nullptr;
PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR Vlk::RayTracingExtension::vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = nullptr;
PFN_vkGetRayTracingShaderGroupHandlesKHR Vlk::RayTracingExtension::vkGetRayTracingShaderGroupHandlesKHR = nullptr;
PFN_vkGetRayTracingShaderGroupStackSizeKHR Vlk::RayTracingExtension::vkGetRayTracingShaderGroupStackSizeKHR = nullptr;