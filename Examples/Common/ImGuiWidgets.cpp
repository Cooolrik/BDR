
#include "ImGuiWidgets.h"
#include <Vlk_DescriptorPool.h>

// Implement needed imgui code
#include <imgui.cpp>
#include <imgui_draw.cpp>
#include <imgui_widgets.cpp>
#include <imgui_tables.cpp>
#include <backends/imgui_impl_glfw.cpp>
#include <backends/imgui_impl_vulkan.cpp>

#include <vector>
#include <set>
#include <memory>
#include <stdexcept>

ImGuiWidgets::ImGuiWidgets( const Vlk::Renderer* renderer, GLFWwindow* window ) : Renderer(renderer), Window(window)
	{
	// create a separate descriptor pool for the gui rendering
	this->DescriptorPool = this->Renderer->CreateDescriptorPool( Vlk::DescriptorPoolTemplate::Maximized() );

	// create the ImGui context, init core ImGui stuff
	this->Context = ImGui::CreateContext();

	// init imgui for glfw
	if( !ImGui_ImplGlfw_InitForVulkan( window, true ) )
		{
		throw std::runtime_error( "Error: ImGui_ImplGlfw_InitForVulkan failed" );
		}

	// init imgui for vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = this->Renderer->GetInstance();
	init_info.PhysicalDevice = this->Renderer->GetPhysicalDevice();
	init_info.Device = this->Renderer->GetDevice();
	init_info.Queue = this->Renderer->GetGraphicsQueue();
	init_info.DescriptorPool = this->DescriptorPool->GetDescriptorPool();
	init_info.MinImageCount = (uint32_t)this->Renderer->GetSwapChainImages().size();
	init_info.ImageCount = (uint32_t)this->Renderer->GetSwapChainImages().size();
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	if( !ImGui_ImplVulkan_Init( &init_info, this->Renderer->GetRenderPass() ) )
		{
		throw std::runtime_error( "Error: ImGui_ImplVulkan_Init failed" );
		}

	// run a blocking command that uploads the font to the GPU
	this->Renderer->RunBlockingCommandBuffer( 
		[&]( VkCommandBuffer cmd ) 
			{
			ImGui_ImplVulkan_CreateFontsTexture( cmd );
			} 
		);

	// don't need the CPU side of textures
	ImGui_ImplVulkan_DestroyFontUploadObjects();


	ImGui::StyleColorsDark();
	}

ImGuiWidgets::~ImGuiWidgets()
	{
	if( this->DescriptorPool ) { delete this->DescriptorPool; }
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	}

void ImGuiWidgets::NewFrame()
	{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	}

void ImGuiWidgets::EndFrameAndRender()
	{
	ImGui::Render();
	}

void ImGuiWidgets::Draw( VkCommandBuffer cmd )
	{
	ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), cmd );
	}
