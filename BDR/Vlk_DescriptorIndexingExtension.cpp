#include "Vlk_Common.inl"
#include "Vlk_DescriptorIndexingExtension.h"

#include "Vlk_RayTracingAccBuffer.h"

#include <stdexcept>
#include <algorithm>

VkResult Vlk::DescriptorIndexingExtension::AddRequiredDeviceExtensions( 
	VkPhysicalDeviceFeatures2* physicalDeviceFeatures,
	VkPhysicalDeviceProperties2* physicalDeviceProperties,
	std::vector<const char*>* extensionList
	)
	{
	UNREFERENCED_PARAMETER( physicalDeviceProperties );

	// enable extensions needed for ray tracing
	Extension::AddExtensionToList( extensionList, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME );

	// set up query structs

	// features
	VR_ADD_STRUCT_TO_VULKAN_LINKED_LIST( physicalDeviceFeatures, this->DescriptorIndexingFeaturesQuery, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES );

	return VkResult::VK_SUCCESS;
	}

bool Vlk::DescriptorIndexingExtension::SelectDevice(
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
	if( !this->DescriptorIndexingFeaturesQuery.shaderStorageBufferArrayNonUniformIndexing )
		return false;
	if( !this->DescriptorIndexingFeaturesQuery.shaderSampledImageArrayNonUniformIndexing )
		return false;
	if( !this->DescriptorIndexingFeaturesQuery.runtimeDescriptorArray )
		return false;

	return true;
	}

VkResult Vlk::DescriptorIndexingExtension::CreateDevice( VkDeviceCreateInfo* deviceCreateInfo )
	{

	VR_ADD_STRUCT_TO_VULKAN_LINKED_LIST( deviceCreateInfo, this->DescriptorIndexingFeaturesCreate, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES );

	// enable required features
	this->DescriptorIndexingFeaturesCreate.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
	this->DescriptorIndexingFeaturesCreate.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	this->DescriptorIndexingFeaturesCreate.runtimeDescriptorArray = VK_TRUE;

	return VkResult::VK_SUCCESS;
	}
