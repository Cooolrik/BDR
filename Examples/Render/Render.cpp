
#include "Common.h"

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <cstdint> // Necessary for UINT32_MAX
#include <algorithm>
#include <fstream>
#include <array>
#include <chrono>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <Vlk_Renderer.h>
#include <Vlk_ShaderModule.h>
#include <Vlk_GraphicsPipeline.h>
#include <Vlk_ComputePipeline.h>
#include <Vlk_CommandPool.h>
#include <Vlk_VertexBuffer.h>
#include <Vlk_IndexBuffer.h>
#include <Vlk_DescriptorSetLayout.h>
#include <Vlk_DescriptorPool.h>
#include <Vlk_Buffer.h>
#include <Vlk_Image.h>

#include "RenderData.h"

#include "Texture.h"

//#include "debugging.h"

#ifdef NDEBUG
const bool useValidationLayers = false;
#else
const bool useValidationLayers = true;
#endif

// makes sure the return value is VK_SUCCESS or throws an exception
#define VLK_CALL( s ) { VkResult VLK_CALL_res = s; if( VLK_CALL_res != VK_SUCCESS ) { throw_vulkan_error( VLK_CALL_res , "Vulcan call " #s " failed (did not return VK_SUCCESS)"); } }

RenderData *renderData;

// dont warn about throw_vulkan_error local functions removed, if not used in a file
#pragma warning( push )
#pragma warning( disable : 4505 )
static void throw_vulkan_error( VkResult errorvalue, const char* errorstr )
	{
	char str[20];
	sprintf_s( str, "%d", (int)errorvalue );
	throw std::runtime_error( errorstr );
	}
#pragma warning( pop )

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
													 VkDebugUtilsMessageTypeFlagsEXT messageType,
													 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
													 void* pUserData )
	{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
	}


VkCommandBuffer createTransientCommandBuffer( uint frame )
	{
	PerFrameData* currentFrame = renderData->currentFrame;
	PerFrameData* previousFrame = renderData->previousFrame;

	Vlk::CommandPool* pool = currentFrame->commandPool;
	pool->ResetCommandPool();
	VkCommandBuffer buffer = pool->BeginCommandBuffer();

	// pre filter the objects to render
	if (renderData->camera.update_render_list)
		{
		// set up batch buffer
		VkBufferCopy batchDataTransfer;
		batchDataTransfer.dstOffset = 0;
		batchDataTransfer.size = renderData->scene_batches * sizeof( BatchData );
		batchDataTransfer.srcOffset = 0;
		vkCmdCopyBuffer( buffer, currentFrame->initialDrawBuffer->GetBuffer(), currentFrame->filteredDrawBuffer->GetBuffer(), 1, &batchDataTransfer );

		// make sure the buffer is done before using in culling shader
		pool->QueueUpBufferMemoryBarrier( currentFrame->filteredDrawBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT );
		pool->PipelineBarrier( VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT );

		pool->BindComputePipeline( renderData->cullingPipeline );
		pool->BindDescriptorSet( renderData->cullingPipeline, currentFrame->cullingDescriptorSet );
		pool->DispatchCompute( (renderData->scene_objects / 256) + 1 );

		// make sure the buffer is update by culling shader before using in vertex shader
		pool->QueueUpBufferMemoryBarrier( currentFrame->filteredDrawBuffer, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT );
		pool->PipelineBarrier( VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT );
		pool->QueueUpBufferMemoryBarrier( currentFrame->instanceToObjectBuffer, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT );
		pool->PipelineBarrier( VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT );
		}

	// render
	pool->BeginRenderPass( currentFrame->framebuffer );
	pool->BindVertexBuffer( renderData->MeshAlloc->GetVertexBuffer() );
	pool->BindIndexBuffer( renderData->MeshAlloc->GetIndexBuffer() );
	pool->BindGraphicsPipeline( renderData->renderPipeline );
	pool->BindDescriptorSet( renderData->renderPipeline, currentFrame->renderDescriptorSet );
	pool->DrawIndexedIndirect( currentFrame->filteredDrawBuffer, 0, renderData->scene_batches, sizeof( BatchData ) );
	//debugDrawGraphicsPipeline( pool );
	pool->EndRenderPass();

	// prepare depth target for reading from compute shader
	pool->QueueUpImageMemoryBarrier( currentFrame->depthImage , VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	pool->PipelineBarrier( VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT );

	pool->BindComputePipeline( renderData->depthReducePipeline );
	for(size_t i = 0; i < currentFrame->depthReduceDescriptorSets.size(); ++i)
		{
		DepthReducePushConstants pc;
		uint w = renderData->DepthPyramidImageW >> i;
		uint h = renderData->DepthPyramidImageH >> i;
		pc.destDimensions = glm::vec2( w, h );

		pool->BindDescriptorSet( renderData->depthReducePipeline, currentFrame->depthReduceDescriptorSets[i] );
		pool->PushConstants( renderData->depthReducePipeline, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof( pc ), &pc );

		uint computew = (w - 1) / 32 + 1;
		uint computeh = (h - 1) / 32 + 1;
		pool->DispatchCompute( computew, computeh );
		}

	// prepare depth target for reading from compute shader
	pool->QueueUpImageMemoryBarrier( currentFrame->depthImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_ACCESS_SHADER_READ_BIT,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,VK_IMAGE_ASPECT_DEPTH_BIT );
	pool->PipelineBarrier( VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT , VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT );

	// prepare the color target and swap chain images for transfer
	pool->QueueUpImageMemoryBarrier( currentFrame->colorImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT );
	pool->QueueUpImageMemoryBarrier( currentFrame->swapChainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_NONE_KHR, VK_ACCESS_TRANSFER_WRITE_BIT );
	pool->PipelineBarrier( VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT );

	// update stats
	renderData->frame_tris += renderData->scene_tris;
	renderData->frame_verts += renderData->scene_verts;
	renderData->frame_insts += renderData->scene_objects;
	renderData->frame_dcs++;

	// copy render target to swap image
	VkImageCopy copyRegion = {};
	copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.srcSubresource.layerCount = 1;
	copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.dstSubresource.layerCount = 1;
	copyRegion.extent = { renderData->camera.ScreenW, renderData->camera.ScreenH, 1 };
	vkCmdCopyImage( buffer, currentFrame->colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, currentFrame->swapChainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion );

	//debugCommandBuffer( pool, buffer );

	// restore swap chain image to present type
	pool->QueueUpImageMemoryBarrier( currentFrame->swapChainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE_KHR );
	pool->PipelineBarrier( VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );

	pool->EndCommandBuffer();

	return buffer;
	}


static bool DrawFrame()
	{
	if( renderData->camera.framebufferResized )
		{
		return false;
		}

	VkResult result;
	uint image_index;
	result = renderData->renderer->AcquireNextFrame( image_index );
	if( result == VK_ERROR_OUT_OF_DATE_KHR )
		{
		return false;
		}

	renderData->previousFrame = renderData->currentFrame;
	renderData->currentFrame = &renderData->renderFrames[image_index];

	// reset stats for the frame
	renderData->frame_tris = 0;
	renderData->frame_verts = 0;
	renderData->frame_insts = 0;
	renderData->frame_dcs = 0;

	// update view 
	renderData->camera.UpdateFrame();

	//debugDrawPreFrame( renderData->currentFrame, renderData->previousFrame );

	PerFrameData* currentFrame = renderData->currentFrame;
	PerFrameData* previousFrame = renderData->previousFrame;

	currentFrame->descriptorPool->ResetDescriptorPool();

	currentFrame->renderDescriptorSet = currentFrame->descriptorPool->BeginDescriptorSet( renderData->renderDescriptorLayout );
	currentFrame->descriptorPool->SetBuffer( 0, currentFrame->uniformBuffer );
	for(uint i = 0; i < 256; ++i)
		{
		Texture *ptex = renderData->Textures[i % renderData->Textures.size()].get();
		currentFrame->descriptorPool->SetImageInArray( 1, i, ptex->GetImage()->GetImageView(), renderData->TexturesSampler->GetSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		}
	currentFrame->descriptorPool->SetBuffer( 2, renderData->objectsBuffer );
	currentFrame->descriptorPool->SetBuffer( 3, currentFrame->instanceToObjectBuffer );
	currentFrame->descriptorPool->SetBuffer( 4, renderData->meshesBuffer );
	currentFrame->descriptorPool->EndDescriptorSet();

	currentFrame->cullingDescriptorSet = currentFrame->descriptorPool->BeginDescriptorSet( renderData->cullingDescriptorLayout );
	currentFrame->descriptorPool->SetBuffer( 0, currentFrame->cullingUBO );
	currentFrame->descriptorPool->SetBuffer( 1, currentFrame->renderObjectsBuffer );
	currentFrame->descriptorPool->SetBuffer( 2, renderData->objectsBuffer );
	currentFrame->descriptorPool->SetBuffer( 3, currentFrame->filteredDrawBuffer );
	currentFrame->descriptorPool->SetBuffer( 4, currentFrame->instanceToObjectBuffer );
	if( previousFrame )
		currentFrame->descriptorPool->SetImage( 5, previousFrame->depthPyramidImage->GetImageView(), renderData->depthSampler->GetSampler(), VK_IMAGE_LAYOUT_GENERAL );
	else
		currentFrame->descriptorPool->SetImage( 5, currentFrame->depthPyramidImage->GetImageView(), renderData->depthSampler->GetSampler(), VK_IMAGE_LAYOUT_GENERAL ); // so we have something, but it wont cull
	currentFrame->descriptorPool->EndDescriptorSet();

	currentFrame->depthReduceDescriptorSets.resize( renderData->DepthPyramidMipMapLevels );
	for(uint i = 0; i < renderData->DepthPyramidMipMapLevels; ++i)
		{
		currentFrame->depthReduceDescriptorSets[i] = currentFrame->descriptorPool->BeginDescriptorSet( renderData->depthReduceDescriptorLayout );
		if(i == 0)
			{
			currentFrame->descriptorPool->SetImage( 0, currentFrame->depthImageView, renderData->depthSampler->GetSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			currentFrame->descriptorPool->SetImage( 1, currentFrame->depthPyramidImageMipViews[0], VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL );
			}
		else
			{
			currentFrame->descriptorPool->SetImage( 0, currentFrame->depthPyramidImageMipViews[i-1], renderData->depthSampler->GetSampler(), VK_IMAGE_LAYOUT_GENERAL );
			currentFrame->descriptorPool->SetImage( 1, currentFrame->depthPyramidImageMipViews[i], VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL );
			}
		currentFrame->descriptorPool->EndDescriptorSet();
		}

	// update rendering ubo
	UniformBufferObject *ubo = (UniformBufferObject*)renderData->currentFrame->uniformBuffer->MapMemory();
	ubo->view = renderData->camera.view;
	ubo->proj = renderData->camera.proj;
	ubo->viewI = renderData->camera.viewI;
	ubo->projI = renderData->camera.projI;
	ubo->viewPosition = renderData->camera.cameraPosition;
	renderData->currentFrame->uniformBuffer->UnmapMemory();

	// update culling ubo
	CullingSettingsUBO *cubo = (CullingSettingsUBO*)renderData->currentFrame->cullingUBO->MapMemory();
	cubo->viewTransform = renderData->camera.view;
	cubo->viewPosition = renderData->camera.cameraPosition;
	glm::mat4 projectionT = glm::transpose( renderData->camera.proj );
	glm::vec4 frustumX = normalizePlane( projectionT[3] + projectionT[0] ); // x + w < 0
	glm::vec4 frustumY = normalizePlane( projectionT[3] + projectionT[1] ); // y + w < 0
	cubo->frustumXx = frustumX.x;
	cubo->frustumXz = frustumX.z;
	cubo->frustumYy = -frustumY.y;
	cubo->frustumYz = frustumY.z;
	cubo->nearZ = renderData->camera.nearZ;
	cubo->farZ = renderData->camera.farZ;
	cubo->pyramidCull = renderData->camera.pyramid_cull;
	cubo->Proj00 = renderData->camera.proj[0][0]; // projection[0][0]
	cubo->Proj11 = renderData->camera.proj[1][1]; // projection[1][1]
	cubo->pyramidWidth = (float)renderData->DepthPyramidImageW; // width of the largest mip in the depth pyramid
	cubo->pyramidHeight = (float)renderData->DepthPyramidImageH; // height of the largest mip in the depth pyramid
	cubo->objectCount = renderData->scene_objects;
	cubo->debug_float_value1 = renderData->camera.debug_float_value1;
	cubo->debug_float_value2 = renderData->camera.debug_float_value2;
	cubo->debug_float_value3 = renderData->camera.debug_float_value3;
	renderData->currentFrame->cullingUBO->UnmapMemory();

	// create the command buffer and send to present 
	std::vector<VkCommandBuffer> buffers = { createTransientCommandBuffer( image_index ) };
	result = renderData->renderer->SubmitRenderCommandBuffersAndPresent( buffers );
	if( result == VK_ERROR_OUT_OF_DATE_KHR )
		{
		return false;
		}

	//debugDrawPostFrame();

	// update totals
	renderData->total_tris += renderData->frame_tris;
	renderData->total_verts += renderData->frame_verts;
	renderData->total_insts += renderData->frame_insts;
	renderData->total_dcs += renderData->frame_dcs;
	renderData->total_frames++;

	// we are done, render is not dirty anymore
	renderData->camera.render_dirty = false;
	return true;
	}

uint nearest2powerBelowValue( uint value )
	{
	for(uint i = 0; i < 31; ++i)
		{
		if( (uint)(1 << (i+1)) >= value )
			return i;
		}
	return 31; // capped at 31
	}

void createPerFrameData()
	{
	renderData->clearPerFrameData();

	Vlk::Renderer* renderer = renderData->renderer;

	// calc depth pyramid size
	uint pw = nearest2powerBelowValue( renderData->camera.ScreenW );
	uint ph = nearest2powerBelowValue( renderData->camera.ScreenH );
	renderData->DepthPyramidMipMapLevels = (pw > ph) ? pw : ph;
	renderData->DepthPyramidImageW = 1 << pw;
	renderData->DepthPyramidImageH = 1 << ph;

	// get the framebuffers and depth images
	std::vector<VkFramebuffer> framebuffers = renderer->GetFramebuffers();
	std::vector<VkImage> swapChainImages = renderer->GetSwapChainImages();

	// create the per frame data objects
	renderData->renderFrames.resize( framebuffers.size() );

	// create one command pool and descriptor set for each frame
	uint frame_count = (uint)renderData->renderFrames.size();
	
	const uint max_set_count = 20;
	const uint max_descriptor_count = 20;
		
	for( uint i = 0; i < frame_count; ++i )
		{
		PerFrameData &frame = renderData->renderFrames[i];

		frame.renderer = renderData->renderer;
		frame.indexInSwapChain = i;

		frame.framebuffer = framebuffers[i];
		frame.swapChainImage = swapChainImages[i];

		frame.colorImage = renderer->GetColorTargetImage( i )->GetImage();
		frame.colorImageView = renderer->GetColorTargetImage( i )->GetImageView();
		frame.depthImage = renderer->GetDepthTargetImage( i )->GetImage();
		frame.depthImageView = renderer->GetDepthTargetImage( i )->GetImageView();

		frame.commandPool = renderer->CreateCommandPool( 1 );
		frame.descriptorPool = renderer->CreateDescriptorPool( Vlk::DescriptorPoolTemplate::General( max_set_count , max_descriptor_count ) );

		frame.uniformBuffer = renderer->CreateBuffer( Vlk::BufferTemplate::UniformBuffer( sizeof( UniformBufferObject ) ) );
		frame.cullingUBO = renderer->CreateBuffer( Vlk::BufferTemplate::UniformBuffer( sizeof( CullingSettingsUBO ) ) );

		// create the original batch indirect render array, upload data
		frame.initialDrawBuffer = renderer->CreateBuffer(
			Vlk::BufferTemplate::ManualBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU,
				VkDeviceSize( renderData->scene_batches * sizeof( BatchData ) ),
				renderData->batches.data()
				)
			);

		// create the filtered array, this will be initialized from the orignal array each frame
		frame.filteredDrawBuffer = renderer->CreateBuffer(
			Vlk::BufferTemplate::ManualBuffer(
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY,
				VkDeviceSize( renderData->scene_batches * sizeof( BatchData ) )
				)
			);

		// create the instance buffer
		frame.instanceToObjectBuffer = renderer->CreateBuffer(
			Vlk::BufferTemplate::ManualBuffer(
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY,
				VkDeviceSize( (VkDeviceSize)renderData->scene_objects * 4 /*num_lods*/ * sizeof( InstanceData ) )
				)
			);

		// create the render object array
		frame.renderObjectsBuffer = renderer->CreateBuffer(
			Vlk::BufferTemplate::ManualBuffer(
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY,
				VkDeviceSize( renderData->scene_objects * sizeof( uint32_t ) ),
				renderData->renderObjects.data()
				)
			);

		// set up the depth image mip pyramid
		frame.depthPyramidImage = renderData->renderer->CreateImage( 
			Vlk::ImageTemplate::General2D( 
				VK_FORMAT_R32_SFLOAT, 
				renderData->DepthPyramidImageW, 
				renderData->DepthPyramidImageH, 
				renderData->DepthPyramidMipMapLevels 
				) 
			);
		
		// create specific image views for each mip map level in the depth pyramid
		frame.depthPyramidImageMipViews.resize( renderData->DepthPyramidMipMapLevels );
		for(uint m = 0; m < renderData->DepthPyramidMipMapLevels; ++m)
			{
			// setup the image view for the specific mip map level
			VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			imageViewCreateInfo.image = frame.depthPyramidImage->GetImage();
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = VK_FORMAT_R32_SFLOAT;
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCreateInfo.subresourceRange.baseMipLevel = m;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			VLK_CALL( vkCreateImageView( renderData->renderer->GetDevice(), &imageViewCreateInfo, 0, &frame.depthPyramidImageMipViews[m] ) );
			}
	
		}

	}

void recreateSwapChain()
	{
	// recreate framebuffer, wait until we have a real extent again
	int width = 0, height = 0;
	glfwGetFramebufferSize( renderData->window, &width, &height );
	if( width == 0 || height == 0 )
		{
		printf( "Waiting for window...\n" );
		}
	while( width == 0 || height == 0 )
		{
		glfwGetFramebufferSize( renderData->window, &width, &height );
		glfwWaitEvents();
		}
	renderData->renderer->WaitForDeviceIdle();

	printf( "Recreating swap chain. Framebuffer size = (%d,%d)...\n", width, height );

	Vlk::Renderer::CreateSwapChainParameters createSwapChainParameters;
	createSwapChainParameters.SurfaceFormat = renderData->renderer->GetSurfaceFormat();
	createSwapChainParameters.PresentMode = renderData->renderer->GetPresentMode();
	createSwapChainParameters.RenderExtent = { static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) };
	renderData->renderer->RecreateSwapChain( createSwapChainParameters );

	createPerFrameData();

	//debugRecreateSwapChain();

	renderData->camera.framebufferResized = false;
	}

void LoadTexture( const char* path )
	{
	renderData->Textures.emplace_back( std::make_unique<Texture>() );
	renderData->Textures.back()->LoadDDS( renderData->renderer, path );
	}

void SetupScene()
	{
	////////////////////

	// the meshes of the scene
	std::vector<const char*> source_mesh_names =
		{
		"../Assets/meteor_0.mmbin",
		"../Assets/meteor_1.mmbin",
		"../Assets/meteor_2.mmbin",
		"../Assets/meteor_3.mmbin",
		"../Assets/meteor_4.mmbin",
		"../Assets/meteor_5.mmbin",
		};

	std::vector<const char*> source_tex_names =
		{
		"../Assets/image1.dds",
		"../Assets/image2.dds",
		"../Assets/image3.dds",
		"../Assets/image4.dds",
		"../Assets/image5.dds",
		"../Assets/image6.dds",
		};
				
#ifdef _DEBUG
	const uint unique_meshes_count = 1;
	const uint megamesh_max_objects_cnt = 1; // number of megamesh objects in scene (not same as scene_objects)
#else
	const uint unique_meshes_count = 160;
	const uint megamesh_max_objects_cnt = 20; // number of megamesh objects in scene (not same as scene_objects)
#endif

	// for now, the objects are: megameshes * batches_per_mm * objects_per_mm

	// set up a random list of meshes, to emulate more different meshes
	// to not cheat, we will load in the mesh data for every mesh
	std::vector<const char*> meshnames;
	std::vector<uint> objects_per_mesh;
	for( uint m = 0; m < unique_meshes_count; ++m )
		{
		meshnames.push_back( source_mesh_names[irand( 0, (uint)source_mesh_names.size() - 1 )] );
		LoadTexture( source_tex_names[irand( 0, (uint)source_tex_names.size() - 1 )] );
		objects_per_mesh.push_back( 1 );
		}

	renderData->MeshAlloc = new ZeptoMeshAllocator();
	renderData->MeshAlloc->LoadMeshes( renderData->renderer, meshnames );

	// reset stats
	renderData->scene_tris = 0;
	renderData->scene_verts = 0;
	renderData->scene_objects = 0;
	renderData->scene_batches = 0;

	uint megamesh_instance_count = 0;

	const uint num_lods = 4;

	// scene data

	// batches, one item per batch (draw call) in the renderer. 
	// size() = unique_meshes_count * mm_submesh_count * num_lods 
	vector<BatchData>& batches = renderData->batches; 

	// objects, one item per object to draw on-screen. there is one object for each instance per batch.
	// each item has info on the object, such as world position etc.
	// size() = unique_meshes_count * mm_submesh_count * mm_object_count 
	vector<ObjectData>& objects = renderData->objects;

	// renderObjects has a list of the objects in "objects" which should be considered for rendering.
	// the size of this is for now equal to the whole list of objects
	// size() = objects.size()
	vector<uint>& renderObjects = renderData->renderObjects;

	// meshes is the list of unique megamesh meshes
	vector<MeshData>& meshes = renderData->meshes;

	// set up objects and render-batches for each unique megamesh 
	meshes.resize( unique_meshes_count );
	for( uint mm = 0; mm < unique_meshes_count; ++mm )
		{
		const ZeptoMesh& mesh = *(renderData->MeshAlloc->GetMesh( mm ));

		// get a number of objects for this mesh
		uint mm_object_count = megamesh_max_objects_cnt;

		// allocate batches and objects for the megamesh
		uint mm_submesh_count = (uint)mesh.SubMeshes.size();
		batches.resize( batches.size() + size_t(mm_submesh_count) * num_lods );

		// allocate one instance for each batch and object
		uint mm_object_batch_instance_count = mm_submesh_count * mm_object_count;
		objects.resize( objects.size() + mm_object_batch_instance_count );
		renderObjects.resize( renderObjects.size() + mm_object_batch_instance_count );

		uint materialID = mm;

		// set up the mesh data
		meshes[mm].CompressedVertexScale = mesh.CompressedVertexScale;
		meshes[mm].CompressedVertexTranslate = mesh.CompressedVertexTranslate;
		
		// do the objects, and each instance in each batch
		for( uint o = 0; o < mm_object_count; ++o )
			{
			glm::mat4 transform( 1 );
			float scale = 1;
#ifndef _DEBUG
			scale = ( ( frand() * 2.7f ) + 1.3f ) * 3.f; // 0.3 -> 3.0
			transform = glm::translate( transform, vrand( glm::vec3(-1,-1,-1), glm::vec3( 1,1, 1 )) * 1000.f ); // random pos
			transform = glm::rotate( transform, frand_radian(), vrand_unit() ); // random rotation
			transform = glm::scale( transform, glm::vec3( scale, scale, scale ) );
#endif
			for( uint sm = 0; sm < mm_submesh_count; ++sm )
				{
				const ZeptoSubMesh& submesh = mesh.SubMeshes[sm];

				glm::vec3 bspherecenter = transform * glm::vec4( submesh.BoundingSphere, 1.f );
				float bsphereradius = submesh.BoundingSphereRadius * scale;

				uint object_index = renderData->scene_objects + ( sm * mm_object_count ) + o;

				objects[object_index].transform = transform;
				objects[object_index].transformIT = glm::transpose( glm::inverse( transform ) );

				objects[object_index].boundingSphere[0] = bspherecenter[0];
				objects[object_index].boundingSphere[1] = bspherecenter[1];
				objects[object_index].boundingSphere[2] = bspherecenter[2];
				objects[object_index].boundingSphere[3] = bsphereradius;

				glm::vec3 rejectionconecenter = transform * glm::vec4( submesh.RejectionConeCenter, 1.f );
				glm::vec3 rejectionconedirection = glm::normalize(renderData->objects[object_index].transformIT * glm::vec4( submesh.RejectionConeDirection, 0.f ));
				objects[object_index].rejectionConeCenter = rejectionconecenter;
				objects[object_index].rejectionConeDirectionAndCutoff = glm::vec4( rejectionconedirection, submesh.RejectionConeCutoff );

				uint batch_id = renderData->scene_batches + (sm * num_lods); // num_lods batches per submesh

				objects[object_index].meshID = mm;
				objects[object_index].materialID = materialID;
				objects[object_index].batchID = batch_id;
				objects[object_index].vertexCutoffIndex = submesh.VertexOffset + submesh.LockedVertexCount;

				objects[object_index].LODQuantizations[0] = submesh.LODQuantizeBits[0];
				objects[object_index].LODQuantizations[1] = submesh.LODQuantizeBits[1];
				objects[object_index].LODQuantizations[2] = submesh.LODQuantizeBits[2];
				objects[object_index].LODQuantizations[3] = submesh.LODQuantizeBits[3];

				renderObjects[object_index] = object_index;
							
				// update stats
				renderData->scene_tris += (uint64_t)( submesh.IndexCount / 3 );
				renderData->scene_verts += (uint64_t)( submesh.VertexCount );
				}
			}

		// setup batches, one per submesh of the megamesh 
		// do 4 lods per submesh
		for( uint sm = 0; sm < mm_submesh_count; ++sm )
			{
			const ZeptoSubMesh& submesh = mesh.SubMeshes[sm];

			for( uint lod = 0; lod < num_lods; ++lod )
				{
				uint first_instance_index = (renderData->scene_objects * num_lods) + ( sm * mm_object_count * num_lods ) + (lod * mm_object_count);

				uint batch_id = renderData->scene_batches + (sm * num_lods) + lod;

				// set up batchdata for the batch
				batches[batch_id].drawCmd.indexCount = submesh.LODIndexCounts[lod];
				batches[batch_id].drawCmd.instanceCount = 0; // set the original array to 0 instances, this will be filled in by the culling compute shader
				batches[batch_id].drawCmd.firstIndex = submesh.IndexOffset;
				batches[batch_id].drawCmd.vertexOffset = submesh.VertexOffset;
				batches[batch_id].drawCmd.firstInstance = first_instance_index;
				}
			}

		megamesh_instance_count += mm_object_count;

		renderData->scene_objects += (mm_object_count * mm_submesh_count);
		renderData->scene_batches += mm_submesh_count*num_lods;
		}

	// create objects array
	renderData->objectsBuffer = renderData->renderer->CreateBuffer(
		Vlk::BufferTemplate::ManualBuffer(
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY,
			VkDeviceSize( renderData->scene_objects * sizeof( ObjectData ) ),
			renderData->objects.data()
			)
		);

	// create meshes array
	renderData->meshesBuffer = renderData->renderer->CreateBuffer(
		Vlk::BufferTemplate::ManualBuffer(
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY,
			VkDeviceSize( (uint)renderData->meshes.size() * sizeof( MeshData ) ),
			renderData->meshes.data()
			)
		);
	
	////////////////////
	// Calculate the per-triangle cost of the zepto meshes

	// set up objects and render-batches for each unique megamesh 
	uint64_t total_size_all = 0;
	uint64_t total_tris_all = 0;
	for( uint mm = 0; mm < unique_meshes_count; ++mm )
		{
		uint64_t num_tris = 0;
		uint64_t num_verts = 0;
		uint64_t num_batches = renderData->MeshAlloc->GetMesh(mm)->SubMeshes.size();
		uint64_t num_mesh_lods = 4;

		// add up all triangles and vertices in the megamesh
		for( uint sm = 0 ; sm < renderData->MeshAlloc->GetMesh( mm )->SubMeshes.size() ; ++sm )
			{
			const ZeptoSubMesh& mesh = renderData->MeshAlloc->GetMesh( mm )->SubMeshes[sm];
			num_tris += mesh.IndexCount / 3;
			num_verts += mesh.VertexCount;
			}

		uint64_t mesh_size_bytes = 
			( num_tris * 3 * sizeof( uint16_t ) ) +
			( num_verts * sizeof( Vertex ) ) +
			( num_batches * num_mesh_lods * sizeof( BatchData ) );

		total_size_all += mesh_size_bytes;
		total_tris_all += num_tris;
		}

	double bytes_per_triangle = double( total_size_all ) / double( total_tris_all );

	////////////////////

	// print info
	printf( "////////////////////////////\n" );
	printf( "Scene Info:\n" );
	printf( "\tUnique Mega Meshes Count: %d\n", renderData->MeshAlloc->GetMeshCount() );
	printf( "\tUnique Batches Count: %d\n", (int)renderData->scene_batches);
	printf( "\tMega Meshes Instances Count: %d\n", megamesh_instance_count );
	printf( "\tAverage objects per batch: %.2f\n", (double)renderData->scene_objects / (double)renderData->scene_batches );
	printf( "\tTotal Objects: %d\n", renderData->scene_objects );
	printf( "\tTotal Tris: %lld\n", renderData->scene_tris );
	printf( "\tTotal Verts: %lld\n", renderData->scene_verts );
	printf( "\tMesh allocation: %lld bytes\n", renderData->MeshAlloc->GetIndexBuffer()->GetBufferSize() + renderData->MeshAlloc->GetVertexBuffer()->GetBufferSize() );
	printf( "\tObject data allocation: %lld bytes\n", VkDeviceSize( renderData->scene_objects * sizeof( ObjectData ) ) );
	printf( "\tMemory cost per triangle: %.2f\n", bytes_per_triangle );
	printf( "\tAverage Megamesh triangle count: %lld\n", renderData->scene_tris / (uint64_t)megamesh_instance_count );
	printf( "\tAverage Tris per Subobject: %lld\n", renderData->scene_tris /(uint64_t)renderData->scene_objects );
	printf( "\tAverage Verts per Subobject: %lld\n", renderData->scene_verts / (uint64_t)renderData->scene_objects );
	printf( "\tAverage Tris per Vertex: %.2f\n", (double)renderData->scene_tris / (double)renderData->scene_verts );
	printf( "\n" );

	//////////////////

	// culling compute pipeline
	Vlk::DescriptorSetLayoutTemplate cdlt;
	cdlt.AddUniformBufferBinding( VK_SHADER_STAGE_COMPUTE_BIT ); // 0 - Settings UBO
	cdlt.AddStorageBufferBinding( VK_SHADER_STAGE_COMPUTE_BIT ); // 1 - RenderObjectsBuffer
	cdlt.AddStorageBufferBinding( VK_SHADER_STAGE_COMPUTE_BIT ); // 2 - SceneObjectsBuffer
	cdlt.AddStorageBufferBinding( VK_SHADER_STAGE_COMPUTE_BIT ); // 3 - FilteredDrawBuffer
	cdlt.AddStorageBufferBinding( VK_SHADER_STAGE_COMPUTE_BIT ); // 4 - InstanceToObjectBuffer
	cdlt.AddSamplerBinding( VK_SHADER_STAGE_COMPUTE_BIT );		 // 5 - depthPyramid texture
	renderData->cullingDescriptorLayout = renderData->renderer->CreateDescriptorSetLayout( cdlt );

	renderData->cullingPipeline = renderData->renderer->CreateComputePipeline();
	renderData->cullingPipeline->SetShaderModule( renderData->cullingShader );
	renderData->cullingPipeline->SetDescriptorSetLayout( renderData->cullingDescriptorLayout );
	renderData->cullingPipeline->BuildPipeline();

	// render pipeline
	Vlk::DescriptorSetLayoutTemplate rdlt;
	rdlt.AddUniformBufferBinding( VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT );	// 0 - buffer object
	rdlt.AddSamplerBinding( VK_SHADER_STAGE_FRAGMENT_BIT , 256 );								// 1 - texture
	rdlt.AddStorageBufferBinding( VK_SHADER_STAGE_VERTEX_BIT );									// 2 - SceneObjectsBuffer
	rdlt.AddStorageBufferBinding( VK_SHADER_STAGE_VERTEX_BIT );									// 3 - InstanceToObjectBuffer
	rdlt.AddStorageBufferBinding( VK_SHADER_STAGE_VERTEX_BIT );									// 4 - MeshBuffer
	renderData->renderDescriptorLayout = renderData->renderer->CreateDescriptorSetLayout( rdlt );
	
	renderData->renderPipeline = renderData->renderer->CreateGraphicsPipeline();
	renderData->renderPipeline->SetVertexDataTemplateFromVertexBuffer( renderData->MeshAlloc->GetVertexBuffer() );
	renderData->renderPipeline->AddShaderModule( renderData->vertexRenderShader );
	renderData->renderPipeline->AddShaderModule( renderData->fragmentRenderShader );
	renderData->renderPipeline->SetDescriptorSetLayout( renderData->renderDescriptorLayout );
	renderData->renderPipeline->BuildPipeline();

	// depth reduce compute pipeline
	Vlk::DescriptorSetLayoutTemplate drdlt;
	drdlt.AddSamplerBinding( VK_SHADER_STAGE_COMPUTE_BIT );		// 0 - source image
	drdlt.AddStoredImageBinding( VK_SHADER_STAGE_COMPUTE_BIT );	// 1 - destination image
	renderData->depthReduceDescriptorLayout = renderData->renderer->CreateDescriptorSetLayout( drdlt );

	renderData->depthReducePipeline = renderData->renderer->CreateComputePipeline();
	renderData->depthReducePipeline->SetShaderModule( renderData->depthReduceShader );
	renderData->depthReducePipeline->SetDescriptorSetLayout( renderData->depthReduceDescriptorLayout );
	renderData->depthReducePipeline->SetSinglePushConstantRange( sizeof( DepthReducePushConstants ), VK_SHADER_STAGE_COMPUTE_BIT );
	renderData->depthReducePipeline->BuildPipeline();
	}

void run()
	{
	renderData = new RenderData();

	glfwInit();
	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	renderData->window = glfwCreateWindow( renderData->camera.ScreenW, renderData->camera.ScreenH, "VulkanRenderTest", nullptr, nullptr );
	renderData->camera.Setup( renderData->window );

	// place the window to the right
	GLFWmonitor* primary = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode( primary );
	glfwSetWindowPos( renderData->window , 50, 50 );

	// place the console below it
	HWND hWnd = GetConsoleWindow();
	MoveWindow( hWnd, 50, 70 + renderData->camera.ScreenH, mode->width - 100, mode->height - 150 - renderData->camera.ScreenH, TRUE );

	// list the needed extensions for glfw
	uint glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

	// create the renderer, list needed extensions
	Vlk::Renderer::CreateParameters createParameters{};
	createParameters.EnableVulkanValidation = useValidationLayers;
	createParameters.EnableRayTracingExtension = true;
	createParameters.NeededExtensionsCount = glfwExtensionCount;
	createParameters.NeededExtensions = glfwExtensions;
	createParameters.DebugMessageCallback = &debugCallback;
	createParameters.DebugMessageSeverityMask =
		//VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createParameters.DebugMessageTypeMask =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	renderData->renderer = Vlk::Renderer::Create( createParameters );

	// create the window surface using glfw
	if( glfwCreateWindowSurface( renderData->renderer->GetInstance(), renderData->window, nullptr, &renderData->surface ) != VK_SUCCESS )
		{
		throw std::runtime_error( "failed to create window surface!" );
		}

	// set up the device and swap chain, hand over surface handle
	renderData->renderer->CreateDevice( renderData->surface );

	// set up swap chain 
	int width, height;
	glfwGetFramebufferSize( renderData->window, &width, &height );
	Vlk::Renderer::CreateSwapChainParameters createSwapChainParameters;
	createSwapChainParameters.SurfaceFormat = renderData->renderer->GetSurfaceFormat();
	createSwapChainParameters.PresentMode = renderData->renderer->GetPresentMode();
	createSwapChainParameters.RenderExtent = { static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) };
	renderData->renderer->CreateSwapChainAndFramebuffers( createSwapChainParameters );

	// set up graphics pipeline
	renderData->vertexRenderShader = Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_VERTEX_BIT, "shaders/vertShader.vert.spv" );
	renderData->fragmentRenderShader = Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_FRAGMENT_BIT, "shaders/fragShader.frag.spv" );
	renderData->cullingShader = Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_COMPUTE_BIT, "shaders/instanceCull.comp.spv" );
	renderData->depthReduceShader = Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_COMPUTE_BIT, "shaders/depthReduce.comp.spv" );

	//SetupDepthSampler();

	renderData->depthSampler = renderData->renderer->CreateSampler( Vlk::SamplerTemplate::DepthReduce() );
	renderData->TexturesSampler = renderData->renderer->CreateSampler( Vlk::SamplerTemplate::Linear() );

	SetupScene();

	createPerFrameData();

	//debugSetup( renderData );

	glfwFocusWindow( renderData->window );

	LARGE_INTEGER start_time;
	LARGE_INTEGER frequency;
	QueryPerformanceCounter( &start_time );
	QueryPerformanceFrequency( &frequency );

	// before entering update loop
	HANDLE h = GetStdHandle( STD_OUTPUT_HANDLE );
	CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
	GetConsoleScreenBufferInfo( h, &bufferInfo );

	// main loop
	while( !glfwWindowShouldClose( renderData->window ) )
		{
		glfwPollEvents();

		renderData->camera.UpdateInput( renderData->window );

		//debugPerFrameLoop();

		//if(renderData->camera.slow_down_frames)
		//	{
		//	Sleep( 1000 );
		//	}

#ifdef _DEBUG
		if (!renderData->camera.render_dirty)
			{
			Sleep(10);
			continue;
			}
#endif

		// draw the new frame
		if( !DrawFrame() )
			{
			// render failed, try to recreate swap chain
			// as it can be because of framebuffer resize
			recreateSwapChain();
			continue;
			}

#ifdef NDEBUG
		LARGE_INTEGER curr_time;
		QueryPerformanceCounter( &curr_time );

		// every seconds
		if( ( curr_time.QuadPart - start_time.QuadPart ) > frequency.QuadPart )
			{
			double time_delta = (double)( curr_time.QuadPart - start_time.QuadPart ) / (double)frequency.QuadPart;

			double frames_per_sec = (double)renderData->total_frames / time_delta;
			double tris_per_sec = (double)renderData->total_tris / time_delta;
			double verts_per_sec = (double)renderData->total_verts / time_delta;
			double tris_per_frame = (double)renderData->total_tris / (double)renderData->total_frames;
			double dcs_per_sec = (double)renderData->total_dcs / time_delta;
			double insts_per_sec = (double)renderData->total_insts / time_delta;

			// reset the cursor position to where it was each time
			SetConsoleCursorPosition( h, bufferInfo.dwCursorPosition );

			printf( "%12.0f FPS (%.1f ms/frame)\n", frames_per_sec, (1/frames_per_sec)*1000 );
			printf( "%12.1f Mtris/sec (%.1f Mtris/f @60fps)\n", tris_per_sec / 1000000, tris_per_sec / 60000000 );
			printf( "%12.1f Mverts/sec (%.1f Mverts/f @60fps)\n", verts_per_sec / 1000000, verts_per_sec / 60000000 );
			printf( "%12.0f DCs/sec (%.0f DCs/f @60fps)\n", dcs_per_sec, dcs_per_sec/60 );
			printf( "%12.0f insts/sec (%.0f insts/f @60fps)\n", insts_per_sec, insts_per_sec / 60 );

			// reset start time
			renderData->total_tris = 0;
			renderData->total_verts = 0;
			renderData->total_dcs = 0;
			renderData->total_insts = 0;
			renderData->total_frames = 0;
			QueryPerformanceCounter( &start_time );
			}
#endif 

		}

	renderData->renderer->WaitForDeviceIdle();

	//debugCleanup();

	delete renderData;
	}


int main(int argc, char argv[])
	{
	try {
		run();
		}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
		}

	return EXIT_SUCCESS;
	}
