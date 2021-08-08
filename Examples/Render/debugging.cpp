// more free form file that has debugging stuff in it. not part of main code

#include "debugging.h"


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

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <stb_image.h>

#include <Vlk_Renderer.h>
#include <Vlk_ShaderModule.h>
#include <Vlk_GraphicsPipeline.h>
#include <Vlk_ComputePipeline.h>
#include <Vlk_CommandPool.h>
#include <Vlk_VertexBuffer.h>
#include <Vlk_IndexBuffer.h>
#include <Vlk_DescriptorLayout.h>
#include <Vlk_DescriptorPool.h>
#include <Vlk_Buffer.h>
#include <Vlk_Image.h>

#include "RenderData.h"

const uint imageW = 512;
const uint imageH = 512;

// makes sure the return value is VK_SUCCESS or throws an exception
#define VLK_CALL( s ) { VkResult VLK_CALL_res = s; if( VLK_CALL_res != VK_SUCCESS ) { throw_vulkan_error( VLK_CALL_res , "Vulcan call " #s " failed (did not return VK_SUCCESS)"); } }

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


class PerFrameDebugData
	{
	public:
		RenderData* renderData = nullptr;

		Vlk::DescriptorPool* debugDescriptorPool = nullptr;
		VkDescriptorSet debugDescriptorSet = nullptr;
		Vlk::Image* debugImage = nullptr;
		Vlk::Buffer* debugUBO = nullptr;

		VkDescriptorSet debugRenderDescriptorSet = nullptr;
		Vlk::Buffer* debugRenderUBO = nullptr;

		~PerFrameDebugData()
			{
			delete debugDescriptorPool;
			delete debugImage;
			delete debugUBO;
			delete debugRenderUBO;
			}
	};

class DebugData
	{
	public:
		RenderData* renderData = nullptr;
		std::vector<PerFrameDebugData> perFrameData;

		GLFWwindow* debugWindow = nullptr;

		Vlk::ShaderModule* debugShader = nullptr;
		Vlk::ShaderModule* debugVertShader = nullptr;
		Vlk::ShaderModule* debugFragShader = nullptr;

		Vlk::ComputePipeline* debugPipeline = nullptr;
		Vlk::DescriptorLayout* debugDescriptorLayout = nullptr;
		VkSampler debugSampler = nullptr;

		Vlk::GraphicsPipeline* debugRenderPipeline = nullptr;
		Vlk::DescriptorLayout* debugRenderDescriptorLayout = nullptr;

		Vlk::VertexBuffer* debugAxiesVertexBuffer{};
		Vlk::IndexBuffer* debugAxiesIndexBuffer{};

		float* stored_image = nullptr;
		float stored_image_render_scale_value = 1;

		~DebugData()
			{
			perFrameData.clear();

			delete debugShader;
			delete debugVertShader;
			delete debugFragShader;

			delete debugPipeline;
			delete debugDescriptorLayout;

			delete debugRenderPipeline;
			delete debugRenderDescriptorLayout;

			if(this->debugSampler != nullptr)
				{
				vkDestroySampler( renderData->renderer->GetDevice(), debugSampler, nullptr );
				}

			delete debugAxiesVertexBuffer;
			delete debugAxiesIndexBuffer;

			delete stored_image;
			
			glfwDestroyWindow( debugWindow );
			}

	};

struct DebugUBO
	{
	glm::mat4x4 view;
	glm::mat4x4 viewI;
	glm::vec2 imageSize;
	float mip_level;
	float P00;
	float P11;
	float znear;
	float zfar;
	float pyramidWidth;
	float pyramidHeight;

	};

const std::vector<Vertex> axies_vertices = {
	// X plane
	{ {0,0,0,1}, {1,0,0,0} },			// origin
	{ {0,1,0,1}, {1,0,0,1} },			// +y
	{ {0,1,1,0}, {1,0,0,1} },			// +y +z
	{ {0,0,1,0}, {1,0,0,0} },			//    +z

	// Y plane
	{ {0,0,0,0}, {0,1,0,1} },			// origin
	{ {0,0,1,0}, {0,1,0,0} },			// +z
	{ {1,0,1,1}, {0,1,0,0} },			// +z +x
	{ {1,0,0,1}, {0,1,0,1} },			//    +x

	// Z plane
	{ {0,0,0,0}, {0,0,1,0} },			// origin
	{ {1,0,0,1}, {0,0,1,0} },			// +x
	{ {1,1,0,1}, {0,0,1,1} },			// +x, +y
	{ {0,1,0,0}, {0,0,1,1} },			//     +y
	};

const std::vector<uint> axies_indices = {
	// X plane
	0,1,2,0,2,3,// 2 tris
	2,1,0,3,2,0,// 2 tris - backface

	// Y plane
	4,5,6,4,6,7, // 2 tris
	6,5,4,7,6,4, // 2 tris - backface

	// Y plane
	8,9,10,8,10,11, // 2 tris
	10,9,8,11,10,8, // 2 tris - backface
	};

static RenderData* renderData = nullptr;
static DebugData* debugData = nullptr;
static PerFrameData* currentFrame;
static PerFrameData* previousFrame;
static PerFrameDebugData* debugCurrentFrame;
static PerFrameDebugData* debugPreviousFrame;

void debugSetup( RenderData* _renderData )
	{
	renderData = _renderData;

	debugData = new DebugData();
	debugData->renderData = renderData;

	debugData->debugWindow = glfwCreateWindow( renderData->camera.ScreenW, renderData->camera.ScreenH, "Debugwindow", nullptr, nullptr );

	// place the window to the right
	GLFWmonitor* primary = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode( primary );
	glfwSetWindowPos( debugData->debugWindow, mode->width - renderData->camera.ScreenW - 50, 50 );

	debugData->debugShader = Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_COMPUTE_BIT, "shaders/debugShader.comp.spv" );
	debugData->debugVertShader = Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_VERTEX_BIT, "shaders/debugVertShader.vert.spv" );
	debugData->debugFragShader = Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_FRAGMENT_BIT, "shaders/debugFragShader.frag.spv" );

	VkSamplerCreateInfo samplerCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.minLod = (float)0;
	samplerCreateInfo.maxLod = (float)16;
	VLK_CALL( vkCreateSampler( renderData->renderer->GetDevice(), &samplerCreateInfo, 0, &debugData->debugSampler ) );

	debugData->debugAxiesVertexBuffer = renderData->renderer->CreateVertexBuffer(
		Vlk::VertexBufferTemplate::VertexBuffer(
			Vertex::GetVertexBufferDescription(), 
			(uint)axies_vertices.size(), 
			axies_vertices.data() 
			)
		);
	debugData->debugAxiesIndexBuffer = renderData->renderer->CreateIndexBuffer( 
		Vlk::IndexBufferTemplate::IndexBuffer(
			VK_INDEX_TYPE_UINT32, 
			(uint)axies_indices.size(), 
			axies_indices.data() 
			)
		);

	debugData->stored_image = new float[imageW*imageH*4];

	debugRecreateSwapChain();
	}

void debugRecreateSwapChain()
	{
	debugData->perFrameData.clear();

	debugData->perFrameData.resize( renderData->renderFrames.size() );
	for( size_t i=0 ; i < renderData->renderFrames.size() ; ++i ) 
		{
		debugData->perFrameData[i].renderData = renderData;

		debugData->perFrameData[i].debugImage = renderData->renderer->CreateImage(
			Vlk::ImageTemplate::General2D(
				VK_FORMAT_R32G32B32A32_SFLOAT,
				imageW,
				imageH,
				1
				)
			);

		debugData->perFrameData[i].debugDescriptorPool = renderData->renderer->CreateDescriptorPool( 10 , 10 , 10 );
		debugData->perFrameData[i].debugUBO = renderData->renderer->CreateBuffer( Vlk::BufferTemplate::UniformBuffer( sizeof( DebugUBO ) ) );
		debugData->perFrameData[i].debugRenderUBO = renderData->renderer->CreateBuffer( Vlk::BufferTemplate::UniformBuffer( sizeof( UniformBufferObject ) ) );
		}

	// debug compute pipeline
	debugData->debugDescriptorLayout = renderData->renderer->CreateDescriptorLayout();
	debugData->debugDescriptorLayout->AddStoredImageBinding( VK_SHADER_STAGE_COMPUTE_BIT );		// 0 - destination image
	debugData->debugDescriptorLayout->AddUniformBufferBinding( VK_SHADER_STAGE_COMPUTE_BIT );	// 1 - ubo
	debugData->debugDescriptorLayout->AddSamplerBinding( VK_SHADER_STAGE_COMPUTE_BIT );			// 2 - sampled image
	debugData->debugDescriptorLayout->BuildDescriptorSetLayout();
	
	debugData->debugPipeline = renderData->renderer->CreateComputePipeline();
	debugData->debugPipeline->SetShaderModule( debugData->debugShader );
	debugData->debugPipeline->SetDescriptorLayout( debugData->debugDescriptorLayout );
	debugData->debugPipeline->BuildPipeline();

	// debug render pipeline
	debugData->debugRenderDescriptorLayout = renderData->renderer->CreateDescriptorLayout();
	debugData->debugRenderDescriptorLayout->AddUniformBufferBinding( VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT ); // 0 - buffer object
	//debugData->debugRenderDescriptorLayout->AddSamplerBinding( VK_SHADER_STAGE_FRAGMENT_BIT ); // 1 - texture
	//debugData->debugRenderDescriptorLayout->AddSamplerBinding( VK_SHADER_STAGE_COMPUTE_BIT );			// 2 - sampled image
	debugData->debugRenderDescriptorLayout->BuildDescriptorSetLayout();

	debugData->debugRenderPipeline = renderData->renderer->CreateGraphicsPipeline();
	debugData->debugRenderPipeline->SetVertexDataTemplateFromVertexBuffer( debugData->debugAxiesVertexBuffer );
	debugData->debugRenderPipeline->AddShaderModule( debugData->debugVertShader );
	debugData->debugRenderPipeline->AddShaderModule( debugData->debugFragShader );
	debugData->debugRenderPipeline->SetDescriptorLayout( debugData->debugRenderDescriptorLayout );
	debugData->debugRenderPipeline->BuildPipeline();

	}

void debugDrawPreFrame( PerFrameData* _currentFrame, PerFrameData* _previousFrame )
	{
	currentFrame = _currentFrame;
	previousFrame = _previousFrame;

	debugCurrentFrame = &debugData->perFrameData[currentFrame->indexInSwapChain];
	if(previousFrame != nullptr)
		{
		debugPreviousFrame = &debugData->perFrameData[previousFrame->indexInSwapChain];
		}
	else
		{
		debugPreviousFrame = nullptr;
		}

	DebugUBO debugubo;

	if( renderData->camera.debug_float_value1 < 0.f )
		renderData->camera.debug_float_value1 = 0.f;
	if(renderData->camera.debug_float_value1 > 16.f )
		renderData->camera.debug_float_value1 = 16.f;

	debugubo.view = renderData->camera.view;
	debugubo.viewI = renderData->camera.viewI;
	debugubo.imageSize = glm::vec2( imageW, imageH );
	debugubo.mip_level = renderData->camera.debug_float_value1;
	debugubo.P00 = renderData->camera.proj[0][0];
	debugubo.P11 = renderData->camera.proj[1][1];
	debugubo.znear = renderData->camera.nearZ;
	debugubo.zfar = renderData->camera.farZ;
	debugubo.pyramidWidth = (float)renderData->DepthPyramidImageW;
	debugubo.pyramidHeight = (float)renderData->DepthPyramidImageH;

	void *mem = debugCurrentFrame->debugUBO->MapMemory();
	memcpy( mem , &debugubo , sizeof( debugubo ) );
	debugCurrentFrame->debugUBO->UnmapMemory();
	
	UniformBufferObject ubo{};
	ubo.view = renderData->camera.view;
	ubo.proj = renderData->camera.proj;
	ubo.viewI = renderData->camera.viewI;
	ubo.projI = renderData->camera.projI;
	mem = debugCurrentFrame->debugRenderUBO->MapMemory();
	memcpy( mem, &ubo, sizeof( ubo ) );
	debugCurrentFrame->debugRenderUBO->UnmapMemory();

	debugCurrentFrame->debugDescriptorPool->ResetDescriptorPool();
	
	debugCurrentFrame->debugDescriptorSet = debugCurrentFrame->debugDescriptorPool->BeginDescriptorSet( debugData->debugDescriptorLayout );
	debugCurrentFrame->debugDescriptorPool->SetImage( 0 , debugCurrentFrame->debugImage->GetImageView() , nullptr , VK_IMAGE_LAYOUT_GENERAL );
	debugCurrentFrame->debugDescriptorPool->SetBuffer( 1 , debugCurrentFrame->debugUBO );
	debugCurrentFrame->debugDescriptorPool->SetImage( 2, currentFrame->depthPyramidImage->GetImageView(), debugData->debugSampler, VK_IMAGE_LAYOUT_GENERAL);
	debugCurrentFrame->debugDescriptorPool->EndDescriptorSet();

	debugCurrentFrame->debugRenderDescriptorSet = debugCurrentFrame->debugDescriptorPool->BeginDescriptorSet( debugData->debugRenderDescriptorLayout );
	debugCurrentFrame->debugDescriptorPool->SetBuffer( 0, debugCurrentFrame->debugRenderUBO );
	debugCurrentFrame->debugDescriptorPool->EndDescriptorSet();

	}

void debugDrawPostFrame()
	{
	}

void debugDrawGraphicsPipeline( Vlk::CommandPool* pool )
	{
	//pool->BindGraphicsPipeline( debugData->debugRenderPipeline );
	//pool->BindDescriptorSet( debugData->debugRenderPipeline , debugCurrentFrame->debugRenderDescriptorSet );
	//pool->BindVertexBuffer( debugData->debugAxiesVertexBuffer );
	//pool->BindIndexBuffer( debugData->debugAxiesIndexBuffer );
	//pool->DrawIndexed( 0 , (uint)axies_indices.size() );
	}


void debugCommandBuffer( Vlk::CommandPool* pool , VkCommandBuffer buffer )
	{
	if( !renderData->camera.debug_render_layer )
		return;

	// run the debug shader, generate the output image
	pool->BindComputePipeline( debugData->debugPipeline );
	pool->BindDescriptorSet( debugData->debugPipeline , debugCurrentFrame->debugDescriptorSet );
	pool->DispatchCompute( imageW/32, imageH/32 );

	pool->QueueUpImageMemoryBarrier( debugCurrentFrame->debugImage->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT );
	pool->PipelineBarrier( VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT );

	VkImageBlit blitRegion = {};
	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.mipLevel = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.srcOffsets[0] = { 0, 0, 0 };
	blitRegion.srcOffsets[1] = { imageW, imageH, 1 };
	blitRegion.dstOffsets[0] = { 0, 0, 0 };
	blitRegion.dstOffsets[1] = { (int)renderData->camera.ScreenW, (int)renderData->camera.ScreenH, 1 };
	vkCmdBlitImage( buffer, debugCurrentFrame->debugImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, currentFrame->swapChainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_NEAREST );
	
	pool->QueueUpImageMemoryBarrier( debugCurrentFrame->debugImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL , VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_TRANSFER_READ_BIT , VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT );
	pool->PipelineBarrier( VK_PIPELINE_STAGE_TRANSFER_BIT , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT );
	}

void debugCleanup()
	{
	delete debugData;
	}

inline uint LinearFloatToSRGBByte( float f )
	{
	// scale it
	f *= debugData->stored_image_render_scale_value;

	// clamp
	if( f < 0 )
		f = 0;
	else if( f > 1 )
		f = 1;

	// linear to srgb
	if(f <= 0.0031308f)
		f = 12.92f * f;
	else
		f = powf( f, 1.f / 2.4f );

	// convert to byte range, clamp again
	f *= 255.5f;
	int v = (uint)f;
	if( v < 0 )
		v = 0;
	else if ( v > 255 )
		v = 255;
	return (uint)v;
	}


static void renderDebugImage()
	{
	// render pixels in the window
	HWND hWnd = glfwGetWin32Window( debugData->debugWindow );
	HDC hDC = GetDC( hWnd );

	BITMAPINFO bmi;
	uint* pixels;
	memset( &bmi, 0, sizeof( BITMAPINFO ) );
	bmi.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	bmi.bmiHeader.biWidth = imageW;
	bmi.bmiHeader.biHeight = imageH;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed = 0;
	bmi.bmiHeader.biClrImportant = 0;

	HBITMAP hDIB = CreateDIBSection( hDC, &bmi, DIB_RGB_COLORS, (void**)&pixels, 0, 0 );
	if(hDIB != 0)
		{
		for(uint y = 0; y < imageH; ++y)
			{
			for(uint x = 0; x < imageW; ++x)
				{
				uint r = LinearFloatToSRGBByte( debugData->stored_image[(x + (imageH - y - 1) * imageW) * 4 + 0] );
				uint g = LinearFloatToSRGBByte( debugData->stored_image[(x + (imageH - y - 1) * imageW) * 4 + 1] );
				uint b = LinearFloatToSRGBByte( debugData->stored_image[(x + (imageH - y - 1) * imageW) * 4 + 2] );
				uint a = LinearFloatToSRGBByte( debugData->stored_image[(x + (imageH - y - 1) * imageW) * 4 + 3] );
				uint val = (a << 24) + (r << 16) + (g << 8) + b;
				pixels[x + y * imageW] = val;
				}
			}
		HDC hDCMem = CreateCompatibleDC( hDC );
		SelectObject( hDCMem, hDIB );
		BitBlt( hDC, 0, 0, imageW, imageH, hDCMem, 0, 0, SRCCOPY );
		ReleaseDC( hWnd, hDCMem );
		}
	ReleaseDC( hWnd, hDC );
	}

double lastxpos = 0;
double lastypos = 0;

void debugPerFrameLoop()
	{
	double xpos;
	double ypos;
	glfwGetCursorPos( debugData->debugWindow, &xpos, &ypos );

	if(renderData->camera.debug_take_screenshot)
		{
		if(debugCurrentFrame != nullptr)
			{
			VkDeviceSize bufferSize = imageW * imageH * 4 * sizeof( float );

			Vlk::Buffer* debugBuffer = renderData->renderer->CreateBuffer(
				Vlk::BufferTemplate::ManualBuffer(
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VMA_MEMORY_USAGE_CPU_ONLY,
					bufferSize
					)
				);

			debugCurrentFrame->debugImage->CopyToBuffer(
				debugBuffer,
				imageW,
				imageH,
				1,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
			);

			float* mapdata = (float*)debugBuffer->MapMemory();
			memcpy( debugData->stored_image , mapdata , sizeof(float)*4*imageW*imageH );
			debugBuffer->UnmapMemory();
			delete debugBuffer;

			renderDebugImage();
			
			renderData->camera.debug_take_screenshot = false;
			}
		else
			{
			renderData->camera.render_dirty = true;
			}
		}

	bool should_wrap = false;
	if(glfwGetKey( debugData->debugWindow, GLFW_KEY_LEFT_ALT ))
		{
		if(glfwGetMouseButton( debugData->debugWindow, GLFW_MOUSE_BUTTON_2 ))
			{
			debugData->stored_image_render_scale_value *= 1.f + (((float)(ypos - lastypos)) / 1000.f);

			char str[200];
			sprintf_s( str, "scale value = %f", debugData->stored_image_render_scale_value );
			glfwSetWindowTitle( debugData->debugWindow, str );

			renderDebugImage();

			should_wrap = true;
			}
		}
	else
		{
		if(glfwGetMouseButton( debugData->debugWindow, GLFW_MOUSE_BUTTON_1 ))
			{
			int x = (int)xpos;
			int y = (int)ypos;

			if(x >= 0 && y >= 0 && x < imageW && y < imageH)
				{
				char str[200];
				sprintf_s( str, "(%d,%d) = %f %f %f %f",
						   x, y,
						   debugData->stored_image[(x + (y * imageW)) * 4 + 0],
						   debugData->stored_image[(x + (y * imageW)) * 4 + 1],
						   debugData->stored_image[(x + (y * imageW)) * 4 + 2],
						   debugData->stored_image[(x + (y * imageW)) * 4 + 3]
				);
				glfwSetWindowTitle( debugData->debugWindow, str );
				}

			}
		}

	lastxpos = xpos;
	lastypos = ypos;

	if(should_wrap)
		{
		if(lastxpos < 5) { lastxpos += ((double)imageW - 10.0); }
		else if(lastxpos > ( (double)imageW - 5.0 )) { lastxpos -= ((double)imageW - 10.0); }

		if(lastypos < 5) { lastypos += ((double)imageH - 10.0); }
		else if(lastypos > ( (double)imageH - 5.0 )) { lastypos -= ((double)imageH - 10.0); }

		if(lastxpos != xpos || lastypos != ypos) { glfwSetCursorPos( debugData->debugWindow, lastxpos, lastypos ); }
		}
	}




//
//
//if(renderData->previousFrame && renderData->camera.debug_screenshot)
//	{
//	VkDeviceSize bufferSize = 1024 * 1024 * 4;
//
//	Vlk::Buffer* debugBuffer = renderData->renderer->CreateGenericBuffer(
//		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//		VMA_MEMORY_USAGE_CPU_ONLY,
//		bufferSize
//	);
//
//	renderData->previousFrame->depthPyramidImage->CopyToBuffer(
//		debugBuffer,
//		renderData->DepthPyramidImageW,
//		renderData->DepthPyramidImageH,
//		1,
//		VK_IMAGE_LAYOUT_GENERAL,
//		VK_IMAGE_LAYOUT_GENERAL,
//		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
//		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
//	);
//
//	void* mapdata = debugBuffer->MapMemory();
//
//	float* pixels = (float*)mapdata;
//
//
//	float min_val = 1000000000;
//	float max_val = -1000000000;
//
//	std::ofstream fs;
//	fs.open( "debug.raw", std::ofstream::binary );
//
//	for(uint y = 0; y < renderData->DepthPyramidImageH; ++y)
//		{
//		for(uint x = 0; x < renderData->DepthPyramidImageW; ++x)
//			{
//			float value = pixels[x + (y * renderData->DepthPyramidImageW)];
//
//			float farval = 255.f;
//			float nearval = 0.1f;
//
//			int byte = 0;
//
//			float depth = (2.0f * nearval * farval) / (farval + nearval - value * (farval - nearval));
//
//			byte = (int)(depth);
//
//			if(byte < 0)
//				byte = 0;
//			if(byte > 255)
//				byte = 255;
//
//			unsigned char r = (unsigned char)byte;
//			unsigned char g = (unsigned char)0;
//			unsigned char b = (unsigned char)0;
//			unsigned char a = (unsigned char)0;
//
//			fs.write( (char*)&r, sizeof( unsigned char ) );
//			fs.write( (char*)&g, sizeof( unsigned char ) );
//			fs.write( (char*)&b, sizeof( unsigned char ) );
//			fs.write( (char*)&a, sizeof( unsigned char ) );
//			}
//		}
//
//	fs.close();
//
//	debugBuffer->UnmapMemory();
//	delete debugBuffer;
//
//	renderData->camera.debug_screenshot = false;
//	}
//
