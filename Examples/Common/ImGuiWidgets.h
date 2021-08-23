#pragma once

#include <Vlk_Renderer.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

class ImGuiWidgets
	{
	private:
		const Vlk::Renderer* Renderer;
		GLFWwindow* Window;
		Vlk::DescriptorPool* DescriptorPool = nullptr;
		ImGuiContext* Context;

	public:
		ImGuiWidgets( const Vlk::Renderer* renderer , GLFWwindow* window );
		~ImGuiWidgets();

		// updates the gui from the input
		void NewFrame();
		void EndFrameAndRender();

		// renders the draw commands into vulkan
		void Draw( VkCommandBuffer cmd );
	};