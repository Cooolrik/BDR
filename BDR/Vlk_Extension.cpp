#define VR_LESS_STRICT_WARNINGS
#include "Vlk_Common.inl"
#include "Vlk_Extension.h"

void Vlk::Extension::AddExtensionToList( std::vector<const char*>* extensionList, const char* extensionName )
	{
	for( auto it = extensionList->begin(); it != extensionList->end(); it++ )
		{
		if( strcmp( ( *it ), extensionName ) == 0 )
			{
			return;
			}
		}

	// not found, add it
	extensionList->push_back( extensionName );
	}


VkResult Vlk::Extension::CreateInstance( VkInstanceCreateInfo* instanceCreateInfo, std::vector<const char*>* extensionList )
	{
	return VkResult::VK_SUCCESS;
	}

VkResult Vlk::Extension::PostCreateInstance()
	{
	return VkResult::VK_SUCCESS;
	}

VkResult Vlk::Extension::AddRequiredDeviceExtensions( 
	VkPhysicalDeviceFeatures2* PhysicalDeviceFeatures,
	VkPhysicalDeviceProperties2* PhysicalDeviceProperties,
	std::vector<const char*>* extensionList )
	{
	return VkResult::VK_SUCCESS;
	}

bool Vlk::Extension::SelectDevice(
	const VkSurfaceCapabilitiesKHR& surfaceCapabilities,
	const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats,
	const std::vector<VkPresentModeKHR>& availablePresentModes,
	const VkPhysicalDeviceFeatures2& physicalDeviceFeatures,
	const VkPhysicalDeviceProperties2& physicalDeviceProperties 
	)
	{ 
	return true; 
	}

VkResult Vlk::Extension::CreateDevice( VkDeviceCreateInfo* deviceCreateInfo )
	{
	return VkResult::VK_SUCCESS;
	}

VkResult Vlk::Extension::PostCreateDevice()
	{
	return VkResult::VK_SUCCESS;
	}

VkResult Vlk::Extension::Cleanup()
	{
	return VkResult::VK_SUCCESS;
	}

Vlk::Extension::~Extension()
	{
	}
