
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

	ImGui::Checkbox( "AABB", &this->RenderAABB );
	ImGui::Checkbox( "Bound Sphere", &this->RenderBoundingSphere );
	ImGui::Checkbox( "Reject Cone", &this->RenderRejectionCone );

	ImGui::Checkbox( "Camera Controls", &this->ShowCameraControls );
	if( this->ShowCameraControls )
		{
		ImGui::Indent( 10 );
		ImGui::SliderFloat3( "Target", &this->mv->Camera.cameraTarget.x, -100.f, 100.f, "%.1f" );
		ImGui::SliderAngle( "Orbit H", &this->mv->Camera.cameraRot.x, -360.f, 360.f );
		ImGui::SliderAngle( "Orbit V", &this->mv->Camera.cameraRot.y, -89.f, 89.f );
		ImGui::SliderFloat( "Dist", &this->mv->Camera.cameraDist, 0.f, 1000.f, "%.1f", ImGuiSliderFlags_Logarithmic);
		ImGui::Indent( -10 );
		}

	ImGui::Checkbox( "Select Submesh", &this->SelectSubmesh );
	if( this->SelectSubmesh )
		{
		ImGui::Indent( 10 );

		const ZeptoMesh* zm = this->mv->MeshAlloc->GetMesh( 0 );
		int submesh_count = (int)zm->SubMeshes.size();
		if( submesh_count > 0 )
			{
			if( this->SelectedSubmeshIndex < 0 )
				this->SelectedSubmeshIndex = 0;
			if( this->SelectedSubmeshIndex >= submesh_count )
				this->SelectedSubmeshIndex = submesh_count - 1;
		
			if( submesh_count > 1 )
				{
				ImGui::SliderInt( "Submesh Id", &this->SelectedSubmeshIndex, 0, submesh_count - 1 );

				uint quantization = this->mv->CalcSubmeshQuantization( *zm, this->SelectedSubmeshIndex );
				uint lod = this->mv->GetLODOfQuantization( *zm, this->SelectedSubmeshIndex, quantization );

				ImGui::TextDisabled( "Quantization: %d", quantization );
				ImGui::TextDisabled( "LOD: %d", lod );
				ImGui::TextDisabled( "Tris Count: %d", zm->SubMeshes[this->SelectedSubmeshIndex].LODIndexCounts[lod]/3 );
				}
			}

		ImGui::Indent( -10 );
		}

	ImVec2 window_pos = ImGui::GetWindowPos();
	window_width = main_viewport->WorkPos.x + main_viewport->WorkSize.x - 5 - window_pos.x;

	ImGui::End();
    }
