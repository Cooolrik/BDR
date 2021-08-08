#include "Vlk_Common.inl"
#include "Vlk_BufferDeviceAddressExtension.h"
#include "Vlk_Buffer.h"

#include <stdexcept>
#include <algorithm>

VkResult Vlk::BufferDeviceAddressExtension::AddRequiredDeviceExtensions( 
	VkPhysicalDeviceFeatures2* physicalDeviceFeatures,
	VkPhysicalDeviceProperties2* physicalDeviceProperties,
	std::vector<const char*>* extensionList
	)
	{
	UNREFERENCED_PARAMETER( physicalDeviceProperties );

	// enable extensions needed for ray tracing
	Extension::AddExtensionToList( extensionList, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME );

	// set up query structs

	// features
	VR_ADD_STRUCT_TO_VULKAN_LINKED_LIST( physicalDeviceFeatures, this->BufferDeviceAddressFeaturesQuery, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR );

	return VkResult::VK_SUCCESS;
	}

bool Vlk::BufferDeviceAddressExtension::SelectDevice(
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
	if( !this->BufferDeviceAddressFeaturesQuery.bufferDeviceAddress )
		return false;

	return true;
	}

VkResult Vlk::BufferDeviceAddressExtension::CreateDevice( VkDeviceCreateInfo* deviceCreateInfo )
	{

	VR_ADD_STRUCT_TO_VULKAN_LINKED_LIST( deviceCreateInfo, this->BufferDeviceAddressFeaturesCreate, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR );

	// enable required features
	this->BufferDeviceAddressFeaturesCreate.bufferDeviceAddress = VK_TRUE;

	return VkResult::VK_SUCCESS;
	}
