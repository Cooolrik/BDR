#include "Vlk_Common.inl"
#include "Vlk_MeshShaderExtension.h"
#include "Vlk_Buffer.h"

#include <stdexcept>
#include <algorithm>

VkResult Vlk::MeshShaderExtension::AddRequiredDeviceExtensions( 
	VkPhysicalDeviceFeatures2* physicalDeviceFeatures,
	VkPhysicalDeviceProperties2* physicalDeviceProperties,
	std::vector<const char*>* extensionList
	)
	{
	UNREFERENCED_PARAMETER( physicalDeviceProperties );

	// enable extensions needed for ray tracing
	Extension::AddExtensionToList( extensionList, VK_NV_MESH_SHADER_EXTENSION_NAME );

	// set up query structs

	// features
	VR_ADD_STRUCT_TO_VULKAN_LINKED_LIST( physicalDeviceFeatures, this->MeshShaderFeaturesQuery, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV );

	return VkResult::VK_SUCCESS;
	}

bool Vlk::MeshShaderExtension::SelectDevice(
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
	if( !this->MeshShaderFeaturesQuery.meshShader )
		return false;
	if( !this->MeshShaderFeaturesQuery.taskShader )
		return false;

	return true;
	}

VkResult Vlk::MeshShaderExtension::CreateDevice( VkDeviceCreateInfo* deviceCreateInfo )
	{
	VR_ADD_STRUCT_TO_VULKAN_LINKED_LIST( deviceCreateInfo, this->MeshShaderFeaturesCreate, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV );

	// enable required features
	this->MeshShaderFeaturesCreate.meshShader = VK_TRUE;
	this->MeshShaderFeaturesCreate.taskShader = VK_TRUE;

	return VkResult::VK_SUCCESS;
	}

VkResult Vlk::MeshShaderExtension::PostCreateInstance()
	{
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCmdDrawMeshTasksNV );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCmdDrawMeshTasksIndirectNV );
	VR_GET_EXTENSION_FUNCTION_ADDRESS( vkCmdDrawMeshTasksIndirectCountNV );

	return VkResult::VK_SUCCESS;
	}

PFN_vkCmdDrawMeshTasksNV Vlk::MeshShaderExtension::vkCmdDrawMeshTasksNV = nullptr;
PFN_vkCmdDrawMeshTasksIndirectNV Vlk::MeshShaderExtension::vkCmdDrawMeshTasksIndirectNV = nullptr;
PFN_vkCmdDrawMeshTasksIndirectCountNV Vlk::MeshShaderExtension::vkCmdDrawMeshTasksIndirectCountNV = nullptr;
