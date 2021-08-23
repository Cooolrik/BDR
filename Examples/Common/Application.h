#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 4100 4201 4127 4189 )

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

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
#include <Vlk_Sampler.h>

#include "Common.h"

#include "Texture.h"
#include "Camera.h"
#include "ImGuiWidgets.h"

class ApplicationBase
	{
	public:
		// application settings
		std::string WindowTitle = "Example";
		bool RequireRaytracing = false;
		bool EnableValidation = false;
		bool EnableVerboseOutput = false;
		uint InitialWindowDimensions[2] = { 800, 600 };
		uint InitialWindowPosition[2] = { 50, 50 };
		bool ShowConsole = true;
		uint InitialConsoleDimensions[2] = { 1600, 400 };
		uint InitialConsolePosition[2] = { 50, 700 };
		bool UseTripleBuffering = true;
		bool WaitInLoopForCameraDirty = false; // mutually exclusive with UseWidgets, as ImGUI need to continuously update
		bool UseWidgets = true;

		// set by init, freed by deinit
		GLFWwindow* Window = nullptr;
		VkSurfaceKHR_T* Surface = nullptr;
		Vlk::Renderer* Renderer = nullptr;
		Camera Camera = {};
		HWND ConsoleWindow = nullptr;
		HANDLE ConsoleOutputHandle = nullptr;
		CONSOLE_SCREEN_BUFFER_INFO ConsoleBufferInfo = {};
		ImGuiWidgets* UIWidgets = nullptr;

		uint CurrentFrameIndex = 0;

		void Init();
		void Deinit();
		void RecreateSwapchain();
	};

template<class T> class Application : public ApplicationBase
	{
	public:
		// the application scene class
		T* Scene = nullptr;

		bool DrawFrame()
			{
			// autofail if camera has got a resize event
			if( this->Camera.framebufferResized )
				{
				return false;
				}

			// get the next frame. if fail, check if out of date
			VkResult result;
			result = this->Renderer->AcquireNextFrame( this->CurrentFrameIndex );
			if( result != VK_SUCCESS )
				{
				if( result == VK_ERROR_OUT_OF_DATE_KHR )
					{
					return false;
					}
				else
					{
					throw std::exception( "AcquireNextFrame: error, call failed" );
					}
				}

			// update view 
			this->Camera.UpdateFrame();

			// call the implementation to create a command buffer and draw the scene
			VkCommandBuffer buffer = this->Scene->DrawScene();
			std::vector<VkCommandBuffer> buffers = { buffer };
			result = this->Renderer->SubmitRenderCommandBuffersAndPresent( buffers );
			if( result == VK_ERROR_OUT_OF_DATE_KHR )
				{
				return false;
				}

			// we are done, render is not dirty anymore
			this->Camera.render_dirty = false;
			return true;
			}

		void RenderLoop()
			{
			while( !glfwWindowShouldClose( this->Window ) )
				{
				glfwPollEvents();

				// update camera & scene
				this->Camera.UpdateInput( this->Window );
				this->Scene->UpdateScene();

				// if we should wait for camera to be dirty
				if( this->WaitInLoopForCameraDirty && !this->Camera.render_dirty )
					{
					// camera not dirty, idle a bit before polling again
					Sleep( 10 );
					continue;
					}

				// draw the new frame
				if( !this->DrawFrame() )
					{
					// render failed, try to recreate swap chain
					// as it can be because of framebuffer resize that the render failed
					this->RecreateSwapchain();
					this->Scene->SetupPerFrameData();
					}
				}

			this->Renderer->WaitForDeviceIdle();
			}

		// run the application
		int Run()
			{
			try
				{
				this->Init();

				this->Scene = new T( *this );
			
				this->Scene->SetupScene();
				this->Scene->SetupPerFrameData();

				// main loop
				this->RenderLoop();

				delete this->Scene;

				this->Deinit();
				}
			catch( const std::exception& e )
				{
				std::cerr << e.what() << std::endl;
				return EXIT_FAILURE;
				}
			return EXIT_SUCCESS;
			}

	};


