
#include "Widgets.h"

// Implement needed imgui code
#include <imgui.cpp>
#include <imgui_draw.cpp>
#include <imgui_widgets.cpp>
#include <imgui_tables.cpp>
#include <backends/imgui_impl_glfw.cpp>
#include <backends/imgui_impl_vulkan.cpp>

UIWidgets::UIWidgets()
	{
	ImGui::CreateContext();
	}

UIWidgets::~UIWidgets()
	{
	}
