#include "Vlk_Common.inl"
#include "Vlk_RayTracingExtension.h"
#include "Vlk_RayTracingAccBuffer.h"
#include "Vlk_RayTracingBLASEntry.h"
#include "Vlk_RayTracingTLASEntry.h"
#include "Vlk_RayTracingPipeline.h"

#include "Vlk_RayTracingAccBuffer.h"
#include "Vlk_Buffer.h"
#include "Vlk_CommandPool.h"

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

Vlk::RayTracingAccBuffer* Vlk::RayTracingExtension::CreateAccBuffer( VkAccelerationStructureCreateInfoKHR createInfo )
	{
	RayTracingAccBuffer* buffer = new RayTracingAccBuffer( this );

	// allocate the buffer memory
	buffer->BufferPtr = std::unique_ptr<Buffer>( 
		this->Parent->CreateBuffer( 
			BufferTemplate::GenericBuffer(
				VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY,
				createInfo.size 
				)
			)
		);

	// create the acceleration struct
	createInfo.buffer = buffer->BufferPtr->GetBuffer();
	VLK_CALL( vkCreateAccelerationStructureKHR( this->Parent->GetDevice(), &createInfo, nullptr, &buffer->AccelerationStructure ) );

	return buffer;
	}

VkCommandPool Vlk::RayTracingExtension::CreateInternalCommandPool()
	{
	VkCommandPool cmdPool{};
	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = this->Parent->PhysicalDeviceQueueGraphicsFamily;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	VLK_CALL( vkCreateCommandPool( this->Parent->Device, &commandPoolCreateInfo, nullptr, &cmdPool ) );
	return cmdPool;
	}

void Vlk::RayTracingExtension::CreateInternalCommandBuffers( VkCommandPool cmdPool , uint32_t num_entries , VkCommandBuffer *cmdBuffers )
	{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = cmdPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = num_entries;
	VLK_CALL( vkAllocateCommandBuffers( this->Parent->Device, &commandBufferAllocateInfo, cmdBuffers ) );
	}

void Vlk::RayTracingExtension::SubmitAndFreeInternalCommandBuffers( VkCommandPool cmdPool, uint32_t num_entries, VkCommandBuffer* cmdBuffers )
	{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = num_entries;
	submitInfo.pCommandBuffers = cmdBuffers;
	VLK_CALL( vkQueueSubmit( this->Parent->GraphicsQueue, 1, &submitInfo, nullptr ) );
	VLK_CALL( vkQueueWaitIdle( this->Parent->GraphicsQueue ) );
	vkFreeCommandBuffers( this->Parent->Device, cmdPool, (uint32_t)num_entries, cmdBuffers );
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
	std::vector<RayTracingAccBuffer*> nonCompactedBLASes( num_entries );
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
		this->vkGetAccelerationStructureBuildSizesKHR( this->Parent->GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildGeometryInfo[i], maxPrimitiveCount.data(), &buildSizesInfo );

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
	Buffer* scratchBuffer = this->Parent->CreateBuffer(
		BufferTemplate::GenericBuffer(
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
	VLK_CALL( vkCreateQueryPool( this->Parent->GetDevice(), &queryPoolCreateInfo, nullptr, &queryPool ) );
	vkResetQueryPool( this->Parent->GetDevice(), queryPool, 0, (uint32_t)num_entries );

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
		this->Parent->Device, 
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
	vkDestroyCommandPool( this->Parent->Device, cmdPool, nullptr );
	vkDestroyQueryPool( this->Parent->GetDevice(), queryPool, nullptr );
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
	Buffer* stagingBuffer = this->Parent->CreateBuffer(
		BufferTemplate::GenericBuffer( 
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
		instance.accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR( this->Parent->GetDevice(), &deviceAddressInfo );
		}
	stagingBuffer->UnmapMemory();

	// set up a copy on the gpu to copy to
	Buffer* TLASInstancesBuffer = this->Parent->CreateBuffer(
		BufferTemplate::GenericBuffer(
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
	vkGetAccelerationStructureBuildSizesKHR( this->Parent->GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildGeometryInfo, &count, &buildSizesInfo );

	// allocate the TLAS
	VkAccelerationStructureCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	createInfo.size = buildSizesInfo.accelerationStructureSize;
	this->TLAS = this->CreateAccBuffer( createInfo );

	// allocate the scratch buffer
	Buffer* scratchBuffer = this->Parent->CreateBuffer(
		BufferTemplate::GenericBuffer(
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
	vkDestroyCommandPool( this->Parent->Device, cmdPool, nullptr );

	// delete temp memorys
	delete scratchBuffer;
	delete TLASInstancesBuffer;
	delete stagingBuffer;
	}

Vlk::RayTracingAccBuffer* Vlk::RayTracingExtension::GetTLAS()
	{
	return this->TLAS;
	}

Vlk::RayTracingPipeline* Vlk::RayTracingExtension::CreateRayTracingPipeline()
	{
	RayTracingPipeline* pipeline = new RayTracingPipeline();
	pipeline->Parent = this;
	this->RayTracingPipelines.insert( pipeline );
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