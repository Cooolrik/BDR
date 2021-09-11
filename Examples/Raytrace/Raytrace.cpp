// disable warnings in external libs and headers
#pragma warning( disable : 26812 4100 4201 4127 4189 4996)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <vector>
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
#include <filesystem>
		
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

typedef unsigned int uint;
using std::vector;

#include <Vlk_Renderer.h>
#include <Vlk_ShaderModule.h>
#include <Vlk_GraphicsPipeline.h>
#include <Vlk_CommandPool.h>
#include <Vlk_VertexBuffer.h>
#include <Vlk_IndexBuffer.h>
#include <Vlk_DescriptorSetLayout.h>
#include <Vlk_DescriptorPool.h>
#include <Vlk_Image.h>
#include <Vlk_Sampler.h>
#include <Vlk_RayTracingExtension.h>
#include <Vlk_RayTracingPipeline.h>
#include <Vlk_RayTracingShaderBindingTable.h>
#include <Vlk_RayTracingBLASEntry.h>
#include <Vlk_RayTracingTLASEntry.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define FAST_OBJ_IMPLEMENTATION
#include <fast_obj.h>


#include <SourceMesh.h>

// number of frames to render after input is received
// basically to keep the renderer hot
#define HOT_FRAMES 100000

#ifdef NDEBUG
const bool useValidationLayers = false;
#else
const bool useValidationLayers = true;
#endif

class UniformBufferObject
	{
	public:
		glm::mat4 view;
		glm::mat4 proj; 
		glm::mat4 viewI;
		glm::mat4 projI;
		glm::vec3 lightPosition;
		uint frameCount; // resets every time the camera is moved
		float CameraAperture;
		float CameraFocusDistance;
		bool InteractiveRender;
	};

class PushConstants
	{
	public:
		glm::mat4 model;
		glm::mat4 modelIT; // inverse transpose model matrix
	};

class Vertex
	{
	public:
		float coord[3];
		float buffer1;
		float normal[3];
		float buffer2;
		float texCoord[2];
		float buffer3[2];

		static Vlk::VertexBufferDescription GetVertexBufferDescription()
			{
			Vlk::VertexBufferDescription desc;
			desc.SetVertexInputBindingDescription( 0, sizeof( Vertex ), VK_VERTEX_INPUT_RATE_VERTEX );
			desc.AddVertexInputAttributeDescription( 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof( Vertex, coord ) );
			desc.AddVertexInputAttributeDescription( 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof( Vertex, normal ) );
			desc.AddVertexInputAttributeDescription( 0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof( Vertex, texCoord ) );
			return desc;
			}
	};

class RenderMesh
	{
	public:
		Vlk::VertexBuffer* vertexBuffer{};
		Vlk::IndexBuffer* indexBuffer{};
		uint GeometryIndex{};

		std::vector<Vertex> vertices;
		std::vector<uint> indices;

		static RenderMesh *LoadMesh( Vlk::Renderer* renderer, const char* _path )
			{
			RenderMesh* m = new RenderMesh();
			std::filesystem::path path(_path);

			std::string cache_file = path.filename().string() + ".cache";
			if(!m->loadCache( cache_file.c_str() ))
				{
				SourceMesh mesh;
				mesh.loadModel( path.string().c_str() );

				m->vertices.resize( mesh.Vertices.size() );
				for(size_t v = 0; v < m->vertices.size(); ++v)
					{
					m->vertices[v].coord[0] = mesh.Vertices[v].coord.x;
					m->vertices[v].coord[1] = mesh.Vertices[v].coord.y;
					m->vertices[v].coord[2] = mesh.Vertices[v].coord.z;
					m->vertices[v].normal[0] = mesh.Vertices[v].normal.x;
					m->vertices[v].normal[1] = mesh.Vertices[v].normal.y;
					m->vertices[v].normal[2] = mesh.Vertices[v].normal.z;
					m->vertices[v].texCoord[0] = mesh.Vertices[v].texCoord.x;
					m->vertices[v].texCoord[1] = mesh.Vertices[v].texCoord.y;
					}
				m->indices.resize( mesh.Indices.size() );
				for(size_t i = 0; i < m->indices.size(); ++i)
					{
					m->indices[i] = mesh.Indices[i];
					}
				m->saveCache( cache_file.c_str() );
				}

			m->vertexBuffer = renderer->CreateVertexBuffer(
				Vlk::VertexBufferTemplate::VertexBuffer(
					Vertex::GetVertexBufferDescription(),
					(uint)m->vertices.size(),
					m->vertices.data(),
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
					)
				);
			m->indexBuffer = renderer->CreateIndexBuffer( 
				Vlk::IndexBufferTemplate::IndexBuffer(
					VK_INDEX_TYPE_UINT32, 
					(uint)m->indices.size(), 
					m->indices.data(),
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
					)
				);

			return m;
			}

		void saveCache( const char* path )
			{
			std::ofstream fs;

			fs.open( path, std::ofstream::binary );

			uint vertex_count = (uint)vertices.size();
			uint index_count = (uint)indices.size();

			fs.write( (char*)&vertex_count, sizeof( uint ) );
			fs.write( (char*)&index_count, sizeof( uint ) );

			fs.write( (char*)vertices.data(), sizeof( Vertex ) * vertex_count );
			fs.write( (char*)indices.data(), sizeof( uint ) * index_count );

			fs.close();
			}

		bool loadCache( const char* path )
			{
			std::ifstream fs;

			fs.open( path, std::ifstream::binary );
			if(!fs.is_open())
				return false;

			uint vertex_count;
			uint index_count;

			fs.read( (char*)&vertex_count, sizeof( uint ) );
			fs.read( (char*)&index_count, sizeof( uint ) );

			vertices.resize( vertex_count );
			indices.resize( index_count );

			fs.read( (char*)vertices.data(), sizeof( Vertex ) * vertex_count );
			fs.read( (char*)indices.data(), sizeof( uint ) * index_count );

			fs.close();

			return true;
			}

		~RenderMesh()
			{
			delete vertexBuffer;
			delete indexBuffer;
			}
	};

class Instance
	{
	public:
		glm::mat4 transform{};
		glm::mat4 transformIT{}; // inverse transpose model matrix
		glm::uint geometryID{};
		glm::uint materialID{};
		glm::uint buffer[2];
	};

std::vector<Vertex> quad = {
		{{-1.0f,-1.0f,0},0,{0,0,0},0,{0,0},{0,0}},
		{{ 1.0f, 1.0f,0},0,{0,0,0},0,{1,1},{1,1}},
		{{ 1.0f,-1.0f,0},0,{0,0,0},0,{1,0},{1,0}},
		{{-1.0f,-1.0f,0},0,{0,0,0},0,{0,0},{0,0}},
		{{-1.0f, 1.0f,0},0,{0,0,0},0,{0,1},{0,1}},
		{{ 1.0f, 1.0f,0},0,{0,0,0},0,{1,1},{1,1}},
	};

float frand( float minv = 0.f , float maxv = 1.f )
	{
	return ( ( (float)rand() / (float)RAND_MAX ) * ( maxv - minv ) ) + minv;
	}

class VulkanRenderTest
	{
	private:
		GLFWwindow* window = nullptr;
		VkSurfaceKHR surface = nullptr;
		bool framebufferResized = false;
		float aspectRatio = 800.f / 600.f;
		int ScreenW = 800;
		int ScreenH = 600;

		Vlk::Renderer* renderer{};
		Vlk::DescriptorPool* descriptorPool = nullptr;

		std::vector<VkFramebuffer> framebuffers{};
		std::vector<VkImage> colorTargets;
		std::vector<VkImage> swapChainImages;
		std::vector<Vlk::CommandPool*> commandPools{};
		std::vector<Vlk::Buffer*> uniformBuffers{};
		std::vector<VkDescriptorSet> descriptorSets{};

		Vlk::DescriptorPool* RTDescriptorPool = nullptr;
		std::vector<VkDescriptorSet> RTDescriptorSets{};

		Vlk::DescriptorPool* QRDescriptorPool = nullptr;
		std::vector<VkDescriptorSet> QRDescriptorSets{};

		Vlk::Pipeline* graphics_pipeline{};
		Vlk::Pipeline* quadrender_pipeline{};

		Vlk::VertexBuffer* quadBuffer{};

		Vlk::Buffer* instanceBuffer{};

		//Vlk::VertexBuffer* testBuffer{};

		std::vector<RenderMesh*> RenderMeshes;
		std::vector<Vlk::Image*> Textures;

		Vlk::DescriptorSetLayout* descriptorLayout{};
		Vlk::DescriptorSetLayout* RTDescriptorLayout{};
		Vlk::DescriptorSetLayout* quadrender_descriptorLayout{};

		Vlk::RayTracingExtension* rayTracing;
		Vlk::RayTracingPipeline* raytracing_pipeline{};
		std::vector<Vlk::Image*> render_target_textures;
		Vlk::RayTracingShaderBindingTable* sbt = nullptr;

		Vlk::Sampler *linearSampler = nullptr;

		Vlk::ShaderModule* quad_vertex_shader = nullptr;
		Vlk::ShaderModule* quad_fragment_shader = nullptr;
		Vlk::ShaderModule* raygen_shader = nullptr;
		Vlk::ShaderModule* miss_shader = nullptr;
		Vlk::ShaderModule* shadowmiss_shader = nullptr;
		Vlk::ShaderModule* chit_shader = nullptr;

		Vlk::ShaderModule* vertex_shader = nullptr;
		Vlk::ShaderModule* fragment_shader = nullptr;


		float cameraFocusDist = 10.f;
		float cameraAperture = 0.1f;

		float cameraDist = 4000.f;
		glm::vec2 cameraRot{};

		glm::vec3 modelPos{};
		glm::vec3 modelRot{};

		glm::vec3 lightPos{};

		double lastxpos = 0;
		double lastypos = 0;

		unsigned int frame_count;

		bool raytrace = true;
		bool autoFocus = true;

		bool render_dirty = true;

	public:

		static void key_callback( GLFWwindow* window, int key, int scancode, int action, int mods )
			{
			auto app = reinterpret_cast<VulkanRenderTest*>( glfwGetWindowUserPointer( window ) );
			if( action == GLFW_RELEASE ) return; //only handle press events
			if( key == GLFW_KEY_R ) 
				{
				app->raytrace = !app->raytrace; 
				app->render_dirty = true;
				}
			else if( key == GLFW_KEY_F )
				{
				app->autoFocus = !app->autoFocus;
				app->render_dirty = true;
				}
			}

		void UpdateInput()
			{
			double xpos;
			double ypos;
			glfwGetCursorPos( window, &xpos, &ypos );

			double relativexpos = xpos - (double)ScreenW/2;
			double relativeypos = ypos - (double)ScreenH/2;

			if( glfwGetKey( window, GLFW_KEY_LEFT_ALT ) )
				{
				if( glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_1 ) )
					{
					cameraRot.x += ( (float)( xpos - lastxpos ) ) / 100.f;
					cameraRot.y += ( (float)( ypos - lastypos ) ) / 100.f;
					if( cameraRot.y < -1.5f )
						cameraRot.y = -1.5f;
					if( cameraRot.y > 1.5f )
						cameraRot.y = 1.5f;
					render_dirty = true;
					}
				if( glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_2 ) )
					{
					cameraDist *= 1.f + (( (float)( ypos - lastypos ) ) / 1000.f);
					render_dirty = true;
					}
				}
			else if( glfwGetKey( window, GLFW_KEY_A ) )
				{
				if( glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_1 ) )
					{
					cameraAperture *= 1.f + ( ( (float)( ypos - lastypos ) ) / 10000.f );
					render_dirty = true;
					}
				}
			else if( glfwGetKey( window, GLFW_KEY_D ) )
				{
				if( glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_1 ) )
					{
					cameraFocusDist *= 1.f + ( ( (float)( ypos - lastypos ) ) / 1000.f );
					autoFocus = false;
					render_dirty = true;
					}
				}

			lastxpos = xpos;
			lastypos = ypos;

			if( render_dirty )
				{
				if( lastxpos < 5 ) { lastxpos += ( (double)ScreenW - 10.0 ); }
				else if( lastxpos > ( (double)ScreenW - 5.0 ) ) { lastxpos -= ( (double)ScreenW - 10.0 );	}
				
				if( lastypos < 5 ) { lastypos += ( (double)ScreenH - 10.0 ); }
				else if( lastypos > ( (double)ScreenH - 5.0 ) ) { lastypos -= ( (double)ScreenH - 10.0 ); }

				if( lastxpos != xpos || lastypos != ypos ) { glfwSetCursorPos( window, lastxpos, lastypos ); }
				}
			}

		bool DrawFrame()
			{
			if( framebufferResized )
				{
				return false;
				}

			VkResult result;
			uint image_index;
			result = renderer->AcquireNextFrame( image_index );
			if( result == VK_ERROR_OUT_OF_DATE_KHR )
				{
				return false;
				}
					 
			if( autoFocus ) 
				{
				cameraFocusDist = cameraDist;
				}

			float camR = cosf( cameraRot.y ) * cameraDist;
			float camY = sinf( cameraRot.y ) * cameraDist;
			float camX = cosf( cameraRot.x ) * camR;
			float camZ = sinf( cameraRot.x ) * camR;

			UniformBufferObject ubo{};
			ubo.view = glm::lookAt( glm::vec3( camX, camY, camZ ), glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
			ubo.proj = glm::perspective( glm::radians( 45.0f ), aspectRatio, 0.1f, 10.0f );
			ubo.proj[1][1] *= -1;
			ubo.viewI = glm::inverse( ubo.view );
			ubo.projI = glm::inverse( ubo.proj );
			ubo.lightPosition = glm::vec3( lightPos.x, 0.f, lightPos.z );
			ubo.frameCount = this->frame_count;
			ubo.CameraAperture = cameraAperture;
			ubo.CameraFocusDistance = cameraFocusDist; 

			if( render_dirty )
				{
				ubo.InteractiveRender = true;
				render_dirty = false;
				}
			else
				{
				ubo.InteractiveRender = false;
				}


			void *mem = uniformBuffers[image_index]->MapMemory();
			memcpy(mem, &ubo, sizeof(ubo) );
			uniformBuffers[image_index]->UnmapMemory();

			PushConstants pc{};
			pc.model = glm::rotate( glm::mat4( 1.0f ), modelRot.y, glm::vec3( 0.0f, 1.0f, 0.0f ) );
			pc.modelIT = glm::transpose( glm::inverse( pc.model ) );

			std::vector<VkCommandBuffer> buffers = { createTransientCommandBuffer( image_index , pc ) };
			result = renderer->SubmitRenderCommandBuffersAndPresent( buffers );
			if( result == VK_ERROR_OUT_OF_DATE_KHR )
				{
				return false;
				}

			return true;
			}

		void clearFramePools()
			{
			// clear up any existing pools
			for( auto pool : commandPools )
				{
				delete pool;
				}
			commandPools.clear();

			for( auto tex : render_target_textures )
				{
				delete tex;
				}
			render_target_textures.clear();

			descriptorSets.clear();
			delete descriptorPool;
			RTDescriptorSets.clear();
			delete RTDescriptorPool;
			QRDescriptorSets.clear();
			delete QRDescriptorPool;

			for( auto buffer : uniformBuffers )
				{
				delete buffer;
				}
			uniformBuffers.clear();
			}

		void createPerFrameData()
			{
			clearFramePools();

			const uint max_set_count = 20;
			const uint max_descriptor_count = 20;

			descriptorPool = renderer->CreateDescriptorPool( Vlk::DescriptorPoolTemplate::General( max_set_count, max_descriptor_count ) );
			RTDescriptorPool = renderer->CreateDescriptorPool( Vlk::DescriptorPoolTemplate::General( max_set_count, max_descriptor_count ) );
			QRDescriptorPool = renderer->CreateDescriptorPool( Vlk::DescriptorPoolTemplate::General( max_set_count, max_descriptor_count ) );

			// create one command pool and descriptor set for each frame
			commandPools.resize( framebuffers.size() );
			descriptorSets.resize( framebuffers.size() );
			RTDescriptorSets.resize( framebuffers.size() );
			QRDescriptorSets.resize( framebuffers.size() );
			uniformBuffers.resize( framebuffers.size() );
			render_target_textures.resize( framebuffers.size() );
			for( size_t i = 0; i < framebuffers.size(); ++i )
				{
				render_target_textures[i] = renderer->CreateImage( Vlk::ImageTemplate::General2D( VK_FORMAT_R32G32B32A32_SFLOAT, 2048, 1024, 1 ) );

				commandPools[i] = renderer->CreateCommandPool( 1 );
				uniformBuffers[i] = renderer->CreateBuffer( Vlk::BufferTemplate::UniformBuffer( sizeof( UniformBufferObject ) ) );

				descriptorSets[i] = descriptorPool->BeginDescriptorSet( descriptorLayout );
				descriptorPool->SetBuffer( 0, uniformBuffers[i] );
				descriptorPool->SetImage( 1, this->Textures[0]->GetImageView(), linearSampler->GetSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
				descriptorPool->EndDescriptorSet();
				
				RTDescriptorSets[i] = RTDescriptorPool->BeginDescriptorSet( RTDescriptorLayout );
				RTDescriptorPool->SetAccelerationStructure( 0, rayTracing->GetTLAS() );
				RTDescriptorPool->SetImage( 1, render_target_textures[0]->GetImageView() , nullptr , VK_IMAGE_LAYOUT_GENERAL );
				RTDescriptorPool->SetBuffer( 2, uniformBuffers[i] );
				for( size_t q = 0; q < this->RenderMeshes.size(); ++q )
					{
					RTDescriptorPool->SetBufferInArray( 3, (uint)q, this->RenderMeshes[q]->vertexBuffer );
					RTDescriptorPool->SetBufferInArray( 4, (uint)q, this->RenderMeshes[q]->indexBuffer );
					}
				for( size_t q = 0; q < this->Textures.size(); ++q )
					{
					RTDescriptorPool->SetImageInArray( 5, (uint)q, this->Textures[q]->GetImageView(), linearSampler->GetSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
					}
				RTDescriptorPool->SetBuffer( 6, instanceBuffer );
				RTDescriptorPool->EndDescriptorSet();

				QRDescriptorSets[i] = QRDescriptorPool->BeginDescriptorSet( quadrender_descriptorLayout );
				QRDescriptorPool->SetImage( 0, render_target_textures[0]->GetImageView() , linearSampler->GetSampler() , VK_IMAGE_LAYOUT_GENERAL );
				QRDescriptorPool->EndDescriptorSet();
				}
			}

		VkCommandBuffer createTransientCommandBuffer( uint frame , PushConstants pc )
			{
			Vlk::CommandPool* pool = commandPools[frame];
			pool->ResetCommandPool();
			VkCommandBuffer buffer = pool->BeginCommandBuffer();

			if( raytrace )
				{
				pool->BindPipeline( raytracing_pipeline );
				pool->BindDescriptorSet( raytracing_pipeline, RTDescriptorSets[frame] );
				pool->TraceRays( sbt, 2048, 1024 );

				pool->BeginRenderPass( framebuffers[frame] );
				pool->BindPipeline( quadrender_pipeline );
				pool->BindVertexBuffer( quadBuffer );
				pool->BindDescriptorSet( quadrender_pipeline, QRDescriptorSets[frame] );
				pool->Draw( (uint)quad.size() );

				pool->EndRenderPass();
				}
			else
				{
				//pool->BeginRenderPass( framebuffers[frame] );
				//
				//pool->BindGraphicsPipeline( graphics_pipeline );
				//
				//// render each instance
				//for( size_t i=0; i< )
				//
				//
				//pool->BindVertexBuffer( vertexBuffer );
				//pool->BindIndexBuffer( indexBuffer );
				//pool->BindDescriptorSet( graphics_pipeline, descriptorSets[frame] );
				//
				//pool->PushConstants( graphics_pipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( pc ), &pc );
				//pool->DrawIndexed( 0, (uint)indices.size() );
				//
				//pool->EndRenderPass();
				}

			// prepare the color target and swap chain images for transfer
			pool->QueueUpImageMemoryBarrier( colorTargets[frame], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT );
			pool->QueueUpImageMemoryBarrier( swapChainImages[frame], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_NONE_KHR, VK_ACCESS_TRANSFER_WRITE_BIT );
			pool->PipelineBarrier( VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT );

			// copy render target to swap image
			VkImageCopy copyRegion = {};
			copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.srcSubresource.layerCount = 1;
			copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.dstSubresource.layerCount = 1;
			copyRegion.extent = { (uint)ScreenW, (uint)ScreenH, 1 };
			vkCmdCopyImage( buffer, colorTargets[frame], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapChainImages[frame], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion );

			// restore swap chain image to present type
			pool->QueueUpImageMemoryBarrier( swapChainImages[frame], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE_KHR );
			pool->PipelineBarrier( VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );

			pool->EndCommandBuffer();

			return buffer;
			}

		void recreateSwapChain()
			{
			// recreate framebuffer, wait until we have a real extent again
			int width = 0, height = 0;
			glfwGetFramebufferSize( window, &width, &height );
			if( width == 0 || height == 0 )
				{
				printf( "Waiting for window...\n" );
				}
			while( width == 0 || height == 0 )
				{
				glfwGetFramebufferSize( window, &width, &height );
				glfwWaitEvents();
				}
			renderer->WaitForDeviceIdle();

			printf( "Recreating swap chain. Framebuffer size = (%d,%d)...\n", width, height );

			aspectRatio = (float)width / (float)height;

			Vlk::Renderer::CreateSwapChainParameters createSwapChainParameters;
			createSwapChainParameters.SurfaceFormat = renderer->GetSurfaceFormat();
			createSwapChainParameters.PresentMode = renderer->GetPresentMode();
			createSwapChainParameters.RenderExtent = { static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) };
			renderer->RecreateSwapChain( createSwapChainParameters );
			framebuffers = renderer->GetFramebuffers();
			colorTargets.resize( framebuffers.size() );
			for( size_t i = 0; i < framebuffers.size(); ++i )
				{
				colorTargets[i] = renderer->GetColorTargetImage( (uint)i )->GetImage();
				}
			swapChainImages = renderer->GetSwapChainImages();

			delete graphics_pipeline;
			std::unique_ptr<Vlk::GraphicsPipelineTemplate> gpt = std::unique_ptr<Vlk::GraphicsPipelineTemplate>( new Vlk::GraphicsPipelineTemplate() );
			gpt->SetVertexDataTemplateFromVertexBufferDescription( Vertex::GetVertexBufferDescription() );
			gpt->AddShaderModule( vertex_shader );
			gpt->AddShaderModule( fragment_shader );
			gpt->AddDescriptorSetLayout( descriptorLayout );
			gpt->AddPushConstantRange( VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( PushConstants ) );
			gpt->SetStaticViewport( 0, 0, (float)this->ScreenW, (float)this->ScreenH );
			gpt->SetStaticScissorRectangle( 0, 0, this->ScreenW, this->ScreenH );
			graphics_pipeline = renderer->CreateGraphicsPipeline( *gpt );

			delete quadrender_pipeline;
			std::unique_ptr<Vlk::GraphicsPipelineTemplate> qpt = std::unique_ptr<Vlk::GraphicsPipelineTemplate>( new Vlk::GraphicsPipelineTemplate() );
			qpt->SetVertexDataTemplateFromVertexBuffer( quadBuffer );
			qpt->AddShaderModule( quad_vertex_shader );
			qpt->AddShaderModule( quad_fragment_shader );
			qpt->AddDescriptorSetLayout( quadrender_descriptorLayout );
			qpt->SetStaticViewport( 0, 0, (float)this->ScreenW, (float)this->ScreenH );
			qpt->SetStaticScissorRectangle( 0, 0, this->ScreenW, this->ScreenH );
			quadrender_pipeline = renderer->CreateGraphicsPipeline( *qpt );

			createPerFrameData();

			this->framebufferResized = false;
			}

		void SetupScene()
			{

			////////////////////

			// the meshes of the scene
			this->RenderMeshes.push_back( RenderMesh::LoadMesh( renderer, "../../Dependencies/common-3d-test-models/data/lucy.obj" ) );

			std::vector<Vlk::RayTracingBLASEntry> geometry_blas_entries;
			std::vector<Vlk::RayTracingBLASEntry*> blas_entries;
			geometry_blas_entries.resize( this->RenderMeshes.size() );
			blas_entries.resize( this->RenderMeshes.size() );
			for( size_t i = 0; i < this->RenderMeshes.size(); ++i )
				{
				geometry_blas_entries[i].AddGeometry( this->RenderMeshes[i]->indexBuffer, this->RenderMeshes[i]->vertexBuffer );
				blas_entries[i] = &geometry_blas_entries[i];
				this->RenderMeshes[i]->GeometryIndex = (uint)i;
				}
			rayTracing->BuildBLAS( blas_entries );

			////////////////////

			std::vector<Instance> instances;
			instances.resize(3);

			instances[0].geometryID = 0;
			instances[0].materialID = 3;
			instances[0].transform = glm::mat4( 1 );
			instances[0].transform = glm::rotate( instances[0].transform, glm::radians( -90.0f ), glm::vec3( 1, 0, 0 ) );
			instances[0].transform = glm::translate( instances[0].transform, glm::vec3( -500, 0, 0 ) );
			instances[0].transform = glm::scale( instances[0].transform , glm::vec3( 1.f ) );
				
			instances[1].geometryID = 0;
			instances[1].materialID = 4;
			instances[1].transform = glm::mat4( 1 );
			instances[1].transform = glm::rotate( instances[1].transform, glm::radians( -90.0f ), glm::vec3( 1, 0, 0 ) );
			instances[1].transform = glm::translate( instances[1].transform, glm::vec3( -1000, 0, 0 ) );
			instances[1].transform = glm::scale( instances[1].transform, glm::vec3( 1.f ) );

			instances[2].geometryID = 0;
			instances[2].materialID = 2;
			instances[2].transform = glm::mat4( 1 );
			instances[2].transform = glm::rotate( instances[2].transform, glm::radians( -90.0f ), glm::vec3( 1, 0, 0 ) );
			instances[2].transform = glm::translate( instances[2].transform, glm::vec3( -1500, 0, 0 ) );
			instances[2].transform = glm::scale( instances[2].transform, glm::vec3( 1.f ) );

			//instances[1].geometryID = 0;
			//instances[1].materialID = 1;
			//instances[1].transform = glm::scale( glm::translate( glm::mat4( 1 ), glm::vec3( 3, 1.2, 0 ) ), glm::vec3( 4, 4, 4 ) );
			//
			//instances[2].geometryID = 0;
			//instances[2].materialID = 0;
			//instances[2].transform = glm::translate( glm::mat4( 1 ), glm::vec3( 0, -1, 0 ) );
			//
			//instances[3].geometryID = 0;
			//instances[3].materialID = 2;
			//instances[3].transform = glm::scale( glm::translate( glm::mat4( 1 ), glm::vec3( -3, 1.2, 0 ) ), glm::vec3( 4, 4, 4 ) );

			//instances[4].geometryID = 0;
			//instances[4].materialID = 4;
			//instances[4].transform = glm::translate( glm::mat4( 1 ), glm::vec3( -3, 1, -2 ) );
			//
			//instances[5].geometryID = 0;
			//instances[5].materialID = 5;
			//instances[5].transform = glm::translate( glm::mat4( 1 ), glm::vec3( 0, 1, -2 ) );
			//
			//instances[6].geometryID = 0;
			//instances[6].materialID = 6;
			//instances[6].transform = glm::translate( glm::mat4( 1 ), glm::vec3( 3, 1, -2 ) );


			std::vector< Vlk::RayTracingTLASEntry> geometry_tlas_entries;
			std::vector<Vlk::RayTracingTLASEntry*> tlas_entries;
			geometry_tlas_entries.resize( instances.size() );
			tlas_entries.resize( instances.size() );

			for( size_t i = 0; i < instances.size(); ++i )
				{
				geometry_tlas_entries[i].BlasId = instances[i].geometryID;
				geometry_tlas_entries[i].InstanceCustomIndex = (uint)i;
				tlas_entries[i] = &geometry_tlas_entries[i];

				for( int x = 0; x < 3; ++x )
					{
					for( int y = 0; y < 4; ++y )
						{
						tlas_entries[i]->Transformation[x][y] = instances[i].transform[y][x];
						}
					}
				
				instances[i].transformIT = glm::transpose( glm::inverse( instances[i].transform ) );
				}

			rayTracing->BuildTLAS( tlas_entries );

			instanceBuffer = renderer->CreateBuffer(
				Vlk::BufferTemplate::ManualBuffer(
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
					VMA_MEMORY_USAGE_GPU_ONLY,
					VkDeviceSize( instances.size() * sizeof( Instance ) ),
					instances.data()
					)
				);

			//////////////////

			Vlk::DescriptorSetLayoutTemplate dslt;
			dslt.AddUniformBufferBinding( VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT ); // 0 - buffer object
			dslt.AddSamplerBinding( VK_SHADER_STAGE_FRAGMENT_BIT ); // 1 - texture
			descriptorLayout = renderer->CreateDescriptorSetLayout( dslt );

			std::unique_ptr<Vlk::GraphicsPipelineTemplate> gpt = std::unique_ptr<Vlk::GraphicsPipelineTemplate>( new Vlk::GraphicsPipelineTemplate() );
			gpt->SetVertexDataTemplateFromVertexBufferDescription( Vertex::GetVertexBufferDescription() );
			gpt->AddShaderModule( vertex_shader );
			gpt->AddShaderModule( fragment_shader );
			gpt->AddDescriptorSetLayout( descriptorLayout );
			gpt->AddPushConstantRange( VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( PushConstants ) );
			gpt->SetStaticViewport( 0, 0, (float)this->ScreenW, (float)this->ScreenH );
			gpt->SetStaticScissorRectangle( 0, 0, this->ScreenW, this->ScreenH );
			graphics_pipeline = renderer->CreateGraphicsPipeline( *gpt );

			Vlk::DescriptorSetLayoutTemplate rtdslt;
			rtdslt.AddAccelerationStructureBinding( VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR ); // 0 - TLAS
			rtdslt.AddStoredImageBinding( VK_SHADER_STAGE_RAYGEN_BIT_KHR ); // 1 - Destination image
			rtdslt.AddUniformBufferBinding( VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR ); // 2 - uniform buffer object
			rtdslt.AddStorageBufferBinding( VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR , (uint)this->RenderMeshes.size() ); // 3 - vertices
			rtdslt.AddStorageBufferBinding( VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, (uint)this->RenderMeshes.size() ); // 4 - indices
			rtdslt.AddSamplerBinding( VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, (uint)this->Textures.size() ); // 5 - texture sampler
			rtdslt.AddStorageBufferBinding( VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR ); // 6 - instances
			RTDescriptorLayout = renderer->CreateDescriptorSetLayout( rtdslt );

			std::unique_ptr<Vlk::RayTracingPipelineTemplate> rtt = std::unique_ptr<Vlk::RayTracingPipelineTemplate>( new Vlk::RayTracingPipelineTemplate() );
			rtt->SetRaygenShaderModule( raygen_shader );
			rtt->AddMissShaderModule( miss_shader );
			rtt->AddMissShaderModule( shadowmiss_shader );
			rtt->AddClosestHitShaderModule( chit_shader );
			rtt->AddDescriptorSetLayout( RTDescriptorLayout );
			raytracing_pipeline = rayTracing->CreateRayTracingPipeline( *rtt );
			sbt = rayTracing->CreateShaderBindingTable( raytracing_pipeline );

			Vlk::DescriptorSetLayoutTemplate qrdslt;
			qrdslt.AddSamplerBinding( VK_SHADER_STAGE_FRAGMENT_BIT ); // 0 - texture sampler
			quadrender_descriptorLayout = renderer->CreateDescriptorSetLayout( qrdslt );

			std::unique_ptr<Vlk::GraphicsPipelineTemplate> qpt = std::unique_ptr<Vlk::GraphicsPipelineTemplate>( new Vlk::GraphicsPipelineTemplate() );
			qpt->SetVertexDataTemplateFromVertexBuffer( quadBuffer );
			qpt->AddShaderModule( quad_vertex_shader );
			qpt->AddShaderModule( quad_fragment_shader );
			qpt->AddDescriptorSetLayout( quadrender_descriptorLayout );
			qpt->SetStaticViewport( 0, 0, (float)this->ScreenW, (float)this->ScreenH );
			qpt->SetStaticScissorRectangle( 0, 0, this->ScreenW, this->ScreenH );
			quadrender_pipeline = renderer->CreateGraphicsPipeline( *qpt );
			}

		void LoadTexture( const char* path )
			{
			// load image, force 4 channel RGBA
			int texWidth, texHeight, texChannels;
			stbi_uc* pixels = stbi_load( path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha );
			if( !pixels )
				{
				throw std::runtime_error( "failed to load texture image!" );
				}
			this->Textures.push_back( renderer->CreateImage( Vlk::ImageTemplate::Texture2D( VK_FORMAT_R8G8B8A8_SRGB , texWidth, texHeight, 1 , pixels , texWidth*texHeight*4 ) ) );
			stbi_image_free( pixels );
			}

		void run()
			{
			glfwInit();
			glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
			window = glfwCreateWindow( this->ScreenW, this->ScreenH, "VulkanRenderTest", nullptr, nullptr );
			glfwSetWindowUserPointer( window, this );
			glfwSetFramebufferSizeCallback( window, framebufferResizeCallback );
			glfwSetKeyCallback( window, key_callback );

			// list the needed extensions for glfw
			uint glfwExtensionCount = 0;
			const char** glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

			// create the renderer, list needed extensions
			Vlk::Renderer::CreateParameters createParameters{};
			createParameters.EnableVulkanValidation = useValidationLayers;
			createParameters.RequireRayTracingExtension = true;
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
			renderer = Vlk::Renderer::Create( createParameters );

			// create the window surface using glfw
			if( glfwCreateWindowSurface( renderer->GetInstance(), window, nullptr, &surface ) != VK_SUCCESS )
				{
				throw std::runtime_error( "failed to create window surface!" );
				}

			// set up the device and swap chain, hand over surface handle
			renderer->CreateDevice( surface );

			// set up swap chain 
			int width, height;
			glfwGetFramebufferSize( window, &width, &height );
			Vlk::Renderer::CreateSwapChainParameters createSwapChainParameters;
			createSwapChainParameters.SurfaceFormat = renderer->GetSurfaceFormat();
			createSwapChainParameters.PresentMode = renderer->GetPresentMode();
			createSwapChainParameters.RenderExtent = { static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) };
			renderer->CreateSwapChainAndFramebuffers( createSwapChainParameters );
			framebuffers = renderer->GetFramebuffers();
			colorTargets.resize( framebuffers.size() );
			for( size_t i=0; i<framebuffers.size(); ++i )
				{
				colorTargets[i] = renderer->GetColorTargetImage((uint)i)->GetImage();
				}
			swapChainImages = renderer->GetSwapChainImages();

			rayTracing = renderer->GetRayTracingExtension();

			this->LoadTexture("../../Dependencies/common-3d-test-models/data/beetle.png");
			this->LoadTexture("../../Dependencies/common-3d-test-models/data/lucy.png");

			quadBuffer = renderer->CreateVertexBuffer( 
				Vlk::VertexBufferTemplate::VertexBuffer(
					Vertex::GetVertexBufferDescription(), 
					(uint)quad.size(), 
					quad.data()  
					)
				);
				
			linearSampler = renderer->CreateSampler( Vlk::SamplerTemplate::Linear() );
			
			// set up graphics pipeline
			this->quad_vertex_shader = Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_VERTEX_BIT, "shaders/post.vert.spv" );
			this->quad_fragment_shader = Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_FRAGMENT_BIT, "shaders/post.frag.spv" );
			this->raygen_shader = Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_RAYGEN_BIT_KHR, "shaders/rtGen.rgen.spv" );
			this->miss_shader = Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_MISS_BIT_KHR, "shaders/rtMiss.rmiss.spv" );
			this->shadowmiss_shader = Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_MISS_BIT_KHR, "shaders/rtShadowMiss.rmiss.spv" );
			this->chit_shader = Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "shaders/rtHit.rchit.spv" );

			this->vertex_shader = Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_VERTEX_BIT, "shaders/vertShader.vert.spv" );
			this->fragment_shader = Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_FRAGMENT_BIT, "shaders/fragShader.frag.spv" );

			this->SetupScene();

			this->createPerFrameData();

			static auto startTime = std::chrono::high_resolution_clock::now();

			// main loop
			frame_count = 0;
			while( !glfwWindowShouldClose( window ) )
				{
				glfwPollEvents();

				UpdateInput();

				// check if we should update. If not, yeald some time and continue
				//if( frame_count > HOT_FRAMES )
				//	{
				//	Sleep( 10 );
				//	continue;
				//	}

				// draw the new frame
				if( !this->DrawFrame() )
					{
					recreateSwapChain();
					frame_count = 0;
					continue;
					}

				frame_count++;
				}

			renderer->WaitForDeviceIdle();

			// done with the renderer, remove stuff
			delete sbt;
			for( auto i : this->Textures )
				{
				delete i;
				}
			
			delete quadrender_descriptorLayout;
			delete RTDescriptorLayout;
			delete descriptorLayout;

			for( auto i : this->RenderMeshes )
				{
				delete i;
				}

			delete quadrender_pipeline;
			delete linearSampler;
			delete instanceBuffer;
			delete quadBuffer;
			clearFramePools();
			delete raytracing_pipeline;
			delete graphics_pipeline;
			delete raygen_shader;
			delete miss_shader;
			delete chit_shader;
			delete vertex_shader;
			delete fragment_shader;
			delete renderer;

			glfwDestroyWindow(window);
			glfwTerminate();
			}

		static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
			{
			auto app = reinterpret_cast<VulkanRenderTest*>(glfwGetWindowUserPointer(window));
			app->framebufferResized = true;
			app->frame_count = 0;
			app->ScreenW = width;
			app->ScreenH = height;
			}

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)
			{
			std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
			return VK_FALSE;
			}
	};

int main(int argc, char argv[])
	{
	VulkanRenderTest app{};
	try {
		app.run();
		}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
		}
	return EXIT_SUCCESS;
	}

