
#include "MeshViewer.h"
#include "UI.h"

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

#include "Zeptomesh.h"

void UI::Update()
    {
	static float window_width = { 400.f };

	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos( ImVec2( main_viewport->WorkPos.x + main_viewport->WorkSize.x - window_width - 5, main_viewport->WorkPos.y + 5 ), ImGuiCond_Always );
	ImGui::SetNextWindowSize( ImVec2( window_width, main_viewport->WorkSize.y - 10 ), ImGuiCond_Always );

	ImGui::Begin( "Mesh Viewer", nullptr, ImGuiWindowFlags_NoMove );

	ImGui::Checkbox( "Camera Controls", &this->show_camera_controls );
	if( show_camera_controls )
		{
		ImGui::Indent( 10 );
		ImGui::SliderFloat3( "Target", &this->mv->Camera.cameraTarget.x, -100.f, 100.f, "%.1f" );
		ImGui::SliderAngle( "Orbit H", &this->mv->Camera.cameraRot.x, -360.f, 360.f );
		ImGui::SliderAngle( "Orbit V", &this->mv->Camera.cameraRot.y, -89.f, 89.f );
		ImGui::SliderFloat( "Dist", &this->mv->Camera.cameraDist, 0.f, 1000.f, "%.1f", ImGuiSliderFlags_Logarithmic);
		ImGui::Indent( -10 );
		}


	ImGui::Checkbox( "Select Submesh", &this->select_a_zeptomesh );
	if( select_a_zeptomesh )
		{
		ImGui::Indent( 10 );

		int mesh_count = (int)this->mv->MeshAlloc->GetMeshCount();
		if( mesh_count > 0 )
			{
			if( this->selected_zeptomesh_index < 0 )
				this->selected_zeptomesh_index = 0;
			if( this->selected_zeptomesh_index >= mesh_count )
				this->selected_zeptomesh_index = mesh_count - 1;
		
			if( mesh_count > 1 )
				{
				ImGui::SliderInt( "Zeptomesh Id", &this->selected_zeptomesh_index, 0, mesh_count - 1 );
				}
		
			const ZeptoMesh* zm = this->mv->MeshAlloc->GetMesh( selected_zeptomesh_index );
			int submesh_count = (int)zm->SubMeshes.size();
			if( submesh_count > 0 )
				{
				if( this->selected_submesh_index < 0 )
					this->selected_submesh_index = 0;
				if( this->selected_submesh_index >= submesh_count )
					this->selected_submesh_index = submesh_count - 1;
		
				if( submesh_count > 1 )
					{
					ImGui::SliderInt( "Submesh Id", &this->selected_submesh_index, 0, submesh_count - 1 );
					}
				}
			}

		ImGui::SliderFloat("viewDot", &this->viewDot, -1, 1);
		ImGui::Checkbox( "visible", &this->visible );

		ImGui::Indent( -10 );
		}

	ImVec2 window_pos = ImGui::GetWindowPos();
	window_width = main_viewport->WorkPos.x + main_viewport->WorkSize.x - 5 - window_pos.x;

	ImGui::End();
    }
