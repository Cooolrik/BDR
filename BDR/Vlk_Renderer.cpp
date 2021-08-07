
#include "Vlk_Common.inl"

#include "Vlk_GraphicsPipeline.h"
#include "Vlk_ComputePipeline.h"
#include "Vlk_CommandPool.h"
#include "Vlk_Buffer.h"
#include "Vlk_VertexBuffer.h"
#include "Vlk_IndexBuffer.h"
#include "Vlk_DescriptorLayout.h"
#include "Vlk_DescriptorPool.h"
#include "Vlk_UniformBuffer.h"
#include "Vlk_Image.h"
#include "Vlk_Sampler.h"
#include "Vlk_RayTracingExtension.h"
#include "Vlk_BufferDeviceAddressExtension.h"
#include "Vlk_DescriptorIndexingExtension.h"
#include "Vlk_Helpers.h"

#include <map>

// the number of frames which are allowed to be running concurrently in different stages 
const int MaximumConcurrentRenderFrames = 2;

// the selected validation layers
static const vector<const char*> ValidationLayers =
	{
	"VK_LAYER_KHRONOS_validation"
	};

// possible depth formats
static const vector<VkFormat> depthFormats =
	{
	VK_FORMAT_D32_SFLOAT,
	VK_FORMAT_D32_SFLOAT_S8_UINT,
	VK_FORMAT_D24_UNORM_S8_UINT
	};


static bool haveAllValidationLayers()
	{
	// get the list of global layers
	uint layerCount;
	VLK_CALL( vkEnumerateInstanceLayerProperties( &layerCount, nullptr ) );
	vector<VkLayerProperties> vulkanLayers( layerCount );
	VLK_CALL( vkEnumerateInstanceLayerProperties( &layerCount, vulkanLayers.data() ) );

	// make sure all selected validation layers exists
	for( const char* layerName : ValidationLayers )
		{
		bool layerFound = false;
		for( const VkLayerProperties& layerProperties : vulkanLayers )
			{
			if( _stricmp( layerProperties.layerName, layerName ) == 0 )
				{
				layerFound = true;
				break;
				}
			}
		if( !layerFound )
			{
			// failed to find this one, so fail
			return false;
			}
		}

	// all validation layers found. success
	return true;
	}

inline VkResult _vkCreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger )
	{
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );
	if( func == nullptr )
		{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	return func( instance, pCreateInfo, pAllocator, pDebugMessenger );
	}

inline void _vkDestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator )
	{
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );
	if( func != nullptr )
		{
		func( instance, debugMessenger, pAllocator );
		}
	}

VkCommandBuffer Vlk::Renderer::BeginInternalCommandBuffer()
	{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandPool = this->InternalCommandPool;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	VLK_CALL(vkAllocateCommandBuffers( this->Device, &commandBufferAllocateInfo, &commandBuffer ));

	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VLK_CALL( vkBeginCommandBuffer( commandBuffer, &commandBufferBeginInfo ));

	return commandBuffer;
	}

void Vlk::Renderer::EndAndSubmitInternalCommandBuffer( VkCommandBuffer buffer )
	{
	vkEndCommandBuffer( buffer );

	// submit the command buffer to the graphics queue
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &buffer;
	vkQueueSubmit( this->GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );

	// synchronously wait for it to finish
	vkQueueWaitIdle( this->GraphicsQueue );

	// done with the buffer
	vkFreeCommandBuffers( this->Device, this->InternalCommandPool, 1, &buffer );
	}

VkBuffer Vlk::Renderer::CreateVulkanBuffer( VkBufferUsageFlags bufferUsageFlags, VmaMemoryUsage memoryPropertyFlags, VkDeviceSize deviceSize, VmaAllocation& deviceMemory ) const
	{
	VkBuffer buffer;

	// create the buffer handle
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = deviceSize;
	bufferCreateInfo.usage = bufferUsageFlags;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memoryPropertyFlags;
	VLK_CALL( vmaCreateBuffer( this->MemoryAllocator, &bufferCreateInfo, &allocInfo, &buffer, &deviceMemory, nullptr ) );

	return buffer;
	}

Vlk::Renderer* Vlk::Renderer::Create( const CreateParameters& createParameters )
	{
	Renderer* pThis = new Renderer();

	pThis->EnableVulkanValidation = createParameters.EnableVulkanValidation;

	// make sure all validation layers exist
	if( pThis->EnableVulkanValidation && !haveAllValidationLayers() )
		{
		throw std::runtime_error( "Vulkan validation is not available." );
		}

	// application information, version 0.1
	VkApplicationInfo applicationInfo{};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = "VulkanRenderer";
	applicationInfo.applicationVersion = VK_MAKE_VERSION( 0, 1, 0 );
	applicationInfo.pEngineName = "VulkanRenderer";
	applicationInfo.engineVersion = VK_MAKE_VERSION( 0, 1, 0 );
	applicationInfo.apiVersion = VK_API_VERSION_1_2;

	// instance creation info
	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;

	// setup list of needed extensions from calling app
	for( uint i = 0; i < createParameters.NeededExtensionsCount; ++i )
		{
		pThis->ExtensionList.push_back( createParameters.NeededExtensions[i] );
		}

	// enable additional extensions
	pThis->BufferDeviceAddressEXT = new BufferDeviceAddressExtension();
	pThis->BufferDeviceAddressEXT->Parent = pThis;
	pThis->EnabledExtensions.push_back( pThis->BufferDeviceAddressEXT );

	pThis->DescriptorIndexingEXT = new DescriptorIndexingExtension();
	pThis->DescriptorIndexingEXT->Parent = pThis;
	pThis->EnabledExtensions.push_back( pThis->DescriptorIndexingEXT );

	if( createParameters.EnableRayTracingExtension )
		{
		pThis->RayTracingEXT = new RayTracingExtension();
		pThis->RayTracingEXT->Parent = pThis;
		pThis->EnabledExtensions.push_back( pThis->RayTracingEXT );
		}

	// call all enabled extensions pre-create instance
	for( auto ext : pThis->EnabledExtensions )
		{
		VLK_CALL( ext->CreateInstance(  &instanceCreateInfo, &pThis->ExtensionList ) );
		}

	// setup vulkan validation extension if wanted
	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{};
	if( pThis->EnableVulkanValidation )
		{
		// add to extensions to enable
		pThis->ExtensionList.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

		// add the validation layers
		instanceCreateInfo.enabledLayerCount = static_cast<uint>( ValidationLayers.size() );
		instanceCreateInfo.ppEnabledLayerNames = ValidationLayers.data();

		// insert create info into create list
		debugUtilsMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugUtilsMessengerCreateInfo.messageSeverity = createParameters.DebugMessageSeverityMask;
		debugUtilsMessengerCreateInfo.messageType = createParameters.DebugMessageTypeMask;
		debugUtilsMessengerCreateInfo.pfnUserCallback = createParameters.DebugMessageCallback;
		debugUtilsMessengerCreateInfo.pNext = instanceCreateInfo.pNext;
		instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfo;
		}

	// create instance
	instanceCreateInfo.enabledExtensionCount = static_cast<uint>( pThis->ExtensionList.size() );
	instanceCreateInfo.ppEnabledExtensionNames = pThis->ExtensionList.data();
	VLK_CALL( vkCreateInstance( &instanceCreateInfo, nullptr, &pThis->Instance ) );

	// create debug messager
	if( pThis->EnableVulkanValidation )
		{
		VLK_CALL( _vkCreateDebugUtilsMessengerEXT( pThis->Instance, &debugUtilsMessengerCreateInfo, nullptr, &pThis->DebugUtilsMessenger ) );
		}

	// call enabled extensions post-create
	for( auto ext : pThis->EnabledExtensions )
		{
		VLK_CALL( ext->PostCreateInstance() );
		}

	return pThis;
	}


bool Vlk::Renderer::LookupPhysicalDeviceQueueFamilies()
	{
	int graphicsFamilyIndex = -1;
	int presentFamilyIndex = -1;

	// retrieve the list of the families
	uint queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( this->PhysicalDevice, &queueFamilyCount, nullptr );
	vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties( this->PhysicalDevice, &queueFamilyCount, queueFamilies.data() );

	for( uint i = 0; i < queueFamilyCount; ++i )
		{
		if( queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
			{
			graphicsFamilyIndex = (int)i;
			}

		VkBool32 presentSupport = false;
		VLK_CALL( vkGetPhysicalDeviceSurfaceSupportKHR( this->PhysicalDevice, i, this->Surface, &presentSupport ) );
		if( presentSupport )
			{
			presentFamilyIndex = (int)i;
			}

		// early exit if we have found queue families
		if( graphicsFamilyIndex >= 0 &&
			presentFamilyIndex >= 0 )
			{
			break;
			}
		}

	if( graphicsFamilyIndex >= 0 &&
		presentFamilyIndex >= 0 )
		{
		this->PhysicalDeviceQueueGraphicsFamily = (uint)graphicsFamilyIndex;
		this->PhysicalDeviceQueuePresentFamily = (uint)presentFamilyIndex;
		return true;
		}
	else
		{
		return false;
		}
	}

bool Vlk::Renderer::ValidatePhysicalDeviceRequiredExtensionsSupported()
	{
	// list extensions
	uint extensionCount;
	VLK_CALL( vkEnumerateDeviceExtensionProperties( this->PhysicalDevice, nullptr, &extensionCount, nullptr ) );
	vector<VkExtensionProperties> availableExtensions( extensionCount );
	VLK_CALL( vkEnumerateDeviceExtensionProperties( this->PhysicalDevice, nullptr, &extensionCount, availableExtensions.data() ) );

	// make sure all required extensions are supported
	for( auto ext : this->DeviceExtensionList )
		{
		bool found = false;
		for( auto avail_ext : availableExtensions )
			{
			if( strcmp( avail_ext.extensionName, ext ) == 0 )
				{
				found = true;
				break;
				}
			}
		if( !found )
			{
			return false;
			}
		}

	// all extensions found
	return true;
	}

void Vlk::Renderer::CreateDevice( VkSurfaceKHR surface )
	{
	if( this->Device != nullptr )
		{
		throw runtime_error( "Renderer device already setup" );
		}
	if( surface == nullptr )
		{
		throw runtime_error( "No surface defined in parameters" );
		}
	this->Surface = surface;

	// retrieve devices to choose from
	uint deviceCount = 0;
	VLK_CALL( vkEnumeratePhysicalDevices( this->Instance, &deviceCount, nullptr ) );
	if( deviceCount == 0 )
		{
		throw runtime_error( "No device has Vulkan support" );
		}
	vector<VkPhysicalDevice> devices( deviceCount );
	VLK_CALL( vkEnumeratePhysicalDevices( this->Instance, &deviceCount, devices.data() ) );

	// fill in required extensions list, along with features and properties
	this->PhysicalDeviceFeatures = {};
	this->PhysicalDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	this->PhysicalDeviceProperties = {};
	this->PhysicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	this->DeviceExtensionList.clear();
	this->DeviceExtensionList.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
	for( auto ext : this->EnabledExtensions )
		{
		ext->AddRequiredDeviceExtensions( 
			&this->PhysicalDeviceFeatures,
			&this->PhysicalDeviceProperties,
			&this->DeviceExtensionList 
			);
		}

	// enumerate and look for a suitable device
	bool found_device = false;
	for( const auto& device : devices )
		{
		// try this device
		this->PhysicalDevice = device;

		// check if it has the queue families needed
		if( !LookupPhysicalDeviceQueueFamilies() )
			continue;

		// make sure all extensions are supported
		if( !ValidatePhysicalDeviceRequiredExtensionsSupported() )
			continue;

		// query capabilities and formats available
		uint count;
		VLK_CALL( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, this->Surface, &this->SurfaceCapabilities ) );
		VLK_CALL( vkGetPhysicalDeviceSurfaceFormatsKHR( device, this->Surface, &count, nullptr ) );
		if( count != 0 )
			{
			this->AvailableSurfaceFormats.resize( count );
			VLK_CALL( vkGetPhysicalDeviceSurfaceFormatsKHR( device, this->Surface, &count, this->AvailableSurfaceFormats.data() ) );
			}
		vkGetPhysicalDeviceSurfacePresentModesKHR( device, this->Surface, &count, nullptr );
		if( count != 0 )
			{
			this->AvailablePresentModes.resize( count );
			VLK_CALL( vkGetPhysicalDeviceSurfacePresentModesKHR( device, this->Surface, &count, this->AvailablePresentModes.data() ) );
			}

		// need at least one format and mode, so skip device if not available
		if( AvailableSurfaceFormats.empty() || AvailablePresentModes.empty() )
			continue;

		// query device features as well
		vkGetPhysicalDeviceFeatures2( device, &this->PhysicalDeviceFeatures );
		vkGetPhysicalDeviceProperties2( device, &this->PhysicalDeviceProperties );

		// need these features
		if( !this->PhysicalDeviceFeatures.features.samplerAnisotropy )
			continue;
		if( !this->PhysicalDeviceFeatures.features.multiDrawIndirect )
			continue;

		// call enabled extensions to make sure they are supported
		bool passed_all_extensions = true;
		for( auto ext : this->EnabledExtensions )
			{
			if( !ext->SelectDevice( this->SurfaceCapabilities, this->AvailableSurfaceFormats, this->AvailablePresentModes, this->PhysicalDeviceFeatures, this->PhysicalDeviceProperties ) )
				{
				passed_all_extensions = false;
				break;
				}
			}
		if( !passed_all_extensions )
			continue;

		// all checks out, we found a device
		found_device = true;
		break;
		}

	// make sure we have a device now
	if( !found_device )
		{
		this->PhysicalDevice = nullptr;
		throw std::runtime_error( "No suitable physical device found." );
		}

	// setup device queues
	vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos( 1 );
	float queuePriority = 1.0f;
	deviceQueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfos[0].queueFamilyIndex = PhysicalDeviceQueueGraphicsFamily;
	deviceQueueCreateInfos[0].queueCount = 1;
	deviceQueueCreateInfos[0].pQueuePriorities = &queuePriority;
	if( PhysicalDeviceQueueGraphicsFamily != PhysicalDeviceQueuePresentFamily )
		{
		// need and additional queue, as presentation and graphics are separate families
		deviceQueueCreateInfos.resize( 2 );
		deviceQueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueCreateInfos[1].queueFamilyIndex = PhysicalDeviceQueuePresentFamily;
		deviceQueueCreateInfos[1].queueCount = 1;
		deviceQueueCreateInfos[1].pQueuePriorities = &queuePriority;
		}

	// additional features
	VkPhysicalDeviceFeatures physicalDeviceFeatures{};
	physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
	physicalDeviceFeatures.multiDrawIndirect = VK_TRUE;

	// device setup and creation + queues
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>( deviceQueueCreateInfos.size() );
	deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>( this->DeviceExtensionList.size() );
	deviceCreateInfo.ppEnabledExtensionNames = this->DeviceExtensionList.data();
	
	// add creation for extensions
	for( auto ext : this->EnabledExtensions )
		{
		VLK_CALL( ext->CreateDevice( &deviceCreateInfo ) );
		}
	if( this->EnableVulkanValidation )
		{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>( ValidationLayers.size() );
		deviceCreateInfo.ppEnabledLayerNames = ValidationLayers.data();
		}
	else
		{
		deviceCreateInfo.enabledLayerCount = 0;
		}

	VLK_CALL( vkCreateDevice( this->PhysicalDevice, &deviceCreateInfo, nullptr, &Device ) );
	vkGetDeviceQueue( this->Device, PhysicalDeviceQueueGraphicsFamily, 0, &this->GraphicsQueue );
	vkGetDeviceQueue( this->Device, PhysicalDeviceQueuePresentFamily, 0, &this->PresentQueue );

	// post create call extensions
	for( auto ext : this->EnabledExtensions )
		{
		VLK_CALL( ext->PostCreateDevice() );
		}

	// set up the memory allocator
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
	allocatorInfo.physicalDevice = this->PhysicalDevice;
	allocatorInfo.device = this->Device;
	allocatorInfo.instance = this->Instance;
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	VLK_CALL( vmaCreateAllocator( &allocatorInfo, &this->MemoryAllocator ) );

	// select default surface format and presentation modes. if we find sRGB 32 bit, use that. if we can use mailbox buffering, do that.
	this->SurfaceFormat = this->AvailableSurfaceFormats[0];
	for( const auto& availableFormat : this->AvailableSurfaceFormats )
		{
		if( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
			{
			this->SurfaceFormat = availableFormat;
			break;
			}
		}
	this->PresentMode = VK_PRESENT_MODE_FIFO_KHR;
	for( const auto& availablePresentMode : this->AvailablePresentModes )
		{
		if( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
			{
			this->PresentMode = availablePresentMode;
			break;
			}
		}
	this->RenderExtent = SurfaceCapabilities.currentExtent;

	// create an internal command pool that will be used for image transfer etc
	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = PhysicalDeviceQueueGraphicsFamily;
	commandPoolCreateInfo.flags = 0;
	VLK_CALL( vkCreateCommandPool( this->Device, &commandPoolCreateInfo, nullptr, &this->InternalCommandPool ) );

	// set up sync objects for rendering
	ImageAvailableSemaphores.resize( MaximumConcurrentRenderFrames );
	RenderFinishedSemaphores.resize( MaximumConcurrentRenderFrames );
	InFlightFences.resize( MaximumConcurrentRenderFrames );

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for( uint i = 0; i < MaximumConcurrentRenderFrames; i++ )
		{
		VLK_CALL( vkCreateSemaphore( this->Device, &semaphoreCreateInfo, nullptr, &this->ImageAvailableSemaphores[i] ) );
		VLK_CALL( vkCreateSemaphore( this->Device, &semaphoreCreateInfo, nullptr, &this->RenderFinishedSemaphores[i] ) );
		VLK_CALL( vkCreateFence( this->Device, &fenceCreateInfo, nullptr, &this->InFlightFences[i] ) );
		}

	}

void Vlk::Renderer::UpdateSurfaceCapabilitiesAndFormats()
	{
	if( this->PhysicalDevice == nullptr )
		{
		throw runtime_error( "Renderer device not setup" );
		}

	// update surface capabilities
	uint count;
	VLK_CALL( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( this->PhysicalDevice, this->Surface, &SurfaceCapabilities ) );
	VLK_CALL( vkGetPhysicalDeviceSurfaceFormatsKHR( this->PhysicalDevice, this->Surface, &count, nullptr ) );
	if( count != 0 )
		{
		AvailableSurfaceFormats.resize( count );
		VLK_CALL( vkGetPhysicalDeviceSurfaceFormatsKHR( this->PhysicalDevice, this->Surface, &count, AvailableSurfaceFormats.data() ) );
		}
	vkGetPhysicalDeviceSurfacePresentModesKHR( this->PhysicalDevice, this->Surface, &count, nullptr );
	if( count != 0 )
		{
		AvailablePresentModes.resize( count );
		VLK_CALL( vkGetPhysicalDeviceSurfacePresentModesKHR( this->PhysicalDevice, this->Surface, &count, AvailablePresentModes.data() ) );
		}
	}

void Vlk::Renderer::CreateSwapChainAndFramebuffers( const CreateSwapChainParameters& parameters )
	{
	if( this->Device == nullptr )
		{
		throw runtime_error( "Renderer device not setup" );
		}
	if( this->SwapChain != nullptr )
		{
		throw runtime_error( "Swap chain already setup. Use RecreateSwapChain after first creation." );
		}

	this->UpdateSurfaceCapabilitiesAndFormats();

	this->SurfaceFormat = parameters.SurfaceFormat;
	this->PresentMode = parameters.PresentMode;
	this->RenderExtent.width = max( SurfaceCapabilities.minImageExtent.width, min( SurfaceCapabilities.maxImageExtent.width, parameters.RenderExtent.width ) );
	this->RenderExtent.height = max( SurfaceCapabilities.minImageExtent.height, min( SurfaceCapabilities.maxImageExtent.height, parameters.RenderExtent.height ) );

	// set up the wanted number of images, make sure they are within allowed values
	uint32_t imageCount = this->SurfaceCapabilities.minImageCount + 1;
	if( this->SurfaceCapabilities.maxImageCount > 0 && imageCount > this->SurfaceCapabilities.maxImageCount )
		{
		imageCount = SurfaceCapabilities.maxImageCount;
		}

	// set up swap chain.
	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = this->Surface;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = this->SurfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = this->SurfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = this->RenderExtent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	swapchainCreateInfo.preTransform = this->SurfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = this->PresentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	// select concurrent or exclusive sharing mode depending if we have one or two queues 
	uint32_t queueFamilyIndices[] = { PhysicalDeviceQueueGraphicsFamily, PhysicalDeviceQueuePresentFamily };
	if( PhysicalDeviceQueueGraphicsFamily == PhysicalDeviceQueuePresentFamily )
		{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
	else
		{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		}

	// create the swap chain
	VLK_CALL( vkCreateSwapchainKHR( this->Device, &swapchainCreateInfo, nullptr, &this->SwapChain ) );

	// retrieve the swap chain images
	VLK_CALL( vkGetSwapchainImagesKHR( this->Device, this->SwapChain, &imageCount, nullptr ) );
	this->SwapChainImages.resize( imageCount );
	VLK_CALL( vkGetSwapchainImagesKHR( this->Device, this->SwapChain, &imageCount, this->SwapChainImages.data() ) );

	// allocate render target vector
	this->TargetImages.resize( imageCount );

	// create render color targets, same format as swap chain for now
	//this->ColorImages.resize( imageCount );
	//this->ColorImageViews.resize( imageCount );
	//this->ColorImageAllocations.resize( imageCount );
	for(uint i = 0; i < imageCount; i++)
		{
		VkImage image;
		VkImageView imageView;
		VmaAllocation imageAllocation;
		
		VkImageCreateInfo colorImageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		colorImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		colorImageCreateInfo.extent.width = this->RenderExtent.width;
		colorImageCreateInfo.extent.height = this->RenderExtent.height;
		colorImageCreateInfo.extent.depth = 1;
		colorImageCreateInfo.mipLevels = 1;
		colorImageCreateInfo.arrayLayers = 1;
		colorImageCreateInfo.format = this->SurfaceFormat.format;
		colorImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		colorImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorImageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		colorImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		colorImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationCreateInfo{};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocationCreateInfo.requiredFlags = VkMemoryPropertyFlags( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		VLK_CALL( vmaCreateImage( this->MemoryAllocator, &colorImageCreateInfo, &allocationCreateInfo, &image, &imageAllocation, nullptr ) );

		// transition the color image from undefined, to device specific layout
		VkCommandBuffer commandBuffer = this->BeginInternalCommandBuffer();
		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		vkCmdPipelineBarrier( commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier );
		this->EndAndSubmitInternalCommandBuffer( commandBuffer );

		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = this->SurfaceFormat.format;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		VLK_CALL( vkCreateImageView( this->Device, &imageViewCreateInfo, nullptr, &imageView ) );

		// create Image object
		this->TargetImages[i].Color = BD_NEW_UNIQUE_PTR( Image );
		this->TargetImages[i].Color->ImageHandle = image;
		this->TargetImages[i].Color->ImageView = imageView;
		this->TargetImages[i].Color->Allocation = imageAllocation;
		}

	// select a depth format
	this->SurfaceDepthFormat = VK_FORMAT_UNDEFINED;
	for( VkFormat format : depthFormats )
		{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties( this->PhysicalDevice, format, &props );
		if( ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT ) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT )
			{
			this->SurfaceDepthFormat = format;
			break;
			}
		}
	if( this->SurfaceDepthFormat == VK_FORMAT_UNDEFINED )
		{
		throw runtime_error( "Failed to find a compatible depth surface format." );
		}
	this->DepthFormatHasStencil = ( this->SurfaceDepthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || this->SurfaceDepthFormat == VK_FORMAT_D24_UNORM_S8_UINT );
		
	// create the depth images
	//this->DepthImages.resize( imageCount );
	//this->DepthImageViews.resize( imageCount );
	//this->DepthImageAllocations.resize( imageCount );
	for (uint i = 0; i < imageCount; i++)
		{
		VkImage image;
		VkImageView imageView;
		VmaAllocation imageAllocation;

		VkImageCreateInfo depthImageCreateInfo{};
		depthImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		depthImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		depthImageCreateInfo.extent.width = this->RenderExtent.width;
		depthImageCreateInfo.extent.height = this->RenderExtent.height;
		depthImageCreateInfo.extent.depth = 1;
		depthImageCreateInfo.mipLevels = 1;
		depthImageCreateInfo.arrayLayers = 1;
		depthImageCreateInfo.format = this->SurfaceDepthFormat;
		depthImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		depthImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		depthImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		depthImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationCreateInfo{};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocationCreateInfo.requiredFlags = VkMemoryPropertyFlags( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		VLK_CALL( vmaCreateImage( this->MemoryAllocator, &depthImageCreateInfo, &allocationCreateInfo, &image, &imageAllocation, nullptr ) );

		// create the depth image view
		VkImageViewCreateInfo depthImageViewCreateInfo {};
		depthImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depthImageViewCreateInfo.image = image;
		depthImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthImageViewCreateInfo.format = this->SurfaceDepthFormat;
		depthImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		depthImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		depthImageViewCreateInfo.subresourceRange.levelCount = 1;
		depthImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		depthImageViewCreateInfo.subresourceRange.layerCount = 1;
		VLK_CALL( vkCreateImageView( this->Device, &depthImageViewCreateInfo, nullptr, &imageView ) );

		// transition the depth image from undefined, to device specific layout
		VkCommandBuffer commandBuffer = this->BeginInternalCommandBuffer();
		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (this->DepthFormatHasStencil)
			{
			imageMemoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		vkCmdPipelineBarrier( commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier );
		this->EndAndSubmitInternalCommandBuffer( commandBuffer );

		// create Image object
		this->TargetImages[i].Depth = BD_NEW_UNIQUE_PTR( Image );
		this->TargetImages[i].Depth->ImageHandle = image;
		this->TargetImages[i].Depth->ImageView = imageView;
		this->TargetImages[i].Depth->Allocation = imageAllocation;
		}

	// set up the render pass, color and depth buffers 
	VkAttachmentDescription attachmentDescriptions[2] = {};
	// color
	attachmentDescriptions[0].format = this->SurfaceFormat.format;
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// depth
	attachmentDescriptions[1].format = this->SurfaceDepthFormat;
	attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE; 
	attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// subpass with color and depth
	VkAttachmentReference colorAttachmentReference{};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference depthAttachmentReference{};
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkSubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;
	subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;

	// subpass dependency description
	VkSubpassDependency subpassDependency{};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// create the render pass
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 2;
	renderPassCreateInfo.pAttachments = attachmentDescriptions;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;
	VLK_CALL( vkCreateRenderPass( this->Device, &renderPassCreateInfo, nullptr, &this->RenderPass ) );

	// set up swap chain framebuffers
	this->Framebuffers.resize( imageCount );
	for( uint i = 0; i < imageCount; ++i )
		{
		VkImageView attachments[2] = { this->TargetImages[i].Color->GetImageView() , this->TargetImages[i].Depth->GetImageView() };

		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = this->RenderPass;
		framebufferCreateInfo.attachmentCount = 2;
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = this->RenderExtent.width;
		framebufferCreateInfo.height = this->RenderExtent.height;
		framebufferCreateInfo.layers = 1;
		VLK_CALL( vkCreateFramebuffer( this->Device, &framebufferCreateInfo, nullptr, &Framebuffers[i] ) );
		}

	// resize list of image fences
	ImagesInFlight.resize( this->SwapChainImages.size(), VK_NULL_HANDLE );
	}

void Vlk::Renderer::RecreateSwapChain( const CreateSwapChainParameters& parameters )
	{
	if( this->SwapChain == nullptr )
		{
		throw runtime_error( "Swap chain not setup." );
		}
	this->DeleteSwapChain();

	this->CreateSwapChainAndFramebuffers( parameters );

	// if we have registered pipelines, they need to be regenerated
	for( GraphicsPipeline* pipeline : this->GraphicsPipelines )
		{
		pipeline->BuildPipeline();
		}
	}

void Vlk::Renderer::DeleteSwapChain()
	{
	// tell all registered pipelines to clean up objects, they need to be recreated
	for( GraphicsPipeline* pipeline : this->GraphicsPipelines )
		{
		pipeline->CleanupPipeline();
		}

	for( size_t i = 0; i < this->Framebuffers.size(); ++i )
		{
		vkDestroyFramebuffer( this->Device, Framebuffers[i], nullptr );
		}
	Framebuffers.clear();

	if(this->RenderPass)
		{
		vkDestroyRenderPass( this->Device, this->RenderPass, nullptr );
		this->RenderPass = nullptr;
		}

	this->TargetImages.clear();

	//for (size_t i = 0; i < this->DepthImages.size(); ++i)
	//	{
	//	vkDestroyImageView( this->Device, this->DepthImageViews[i], nullptr );
	//	vmaDestroyImage( this->MemoryAllocator, this->DepthImages[i], this->DepthImageAllocations[i] );
	//	}
	//
	//for(size_t i = 0; i < this->ColorImages.size(); ++i)
	//	{
	//	vkDestroyImageView( this->Device, this->ColorImageViews[i], nullptr );
	//	vmaDestroyImage( this->MemoryAllocator, this->ColorImages[i], this->ColorImageAllocations[i] );
	//	}

	if( this->SwapChain != nullptr )
		{
		vkDestroySwapchainKHR( this->Device, this->SwapChain, nullptr );
		this->SwapChain = nullptr;
		}
	}

void Vlk::Renderer::RemoveCommandPool( CommandPool* pool )
	{
	auto it = this->CommandPools.find( pool );
	if( it == this->CommandPools.end() )
		{
		throw runtime_error( "Error: RemoveCommandPool() pool is not registered with the renderer, have you already removed it?" );
		}
	this->CommandPools.erase( it );
	}

Vlk::CommandPool* Vlk::Renderer::CreateCommandPool( uint bufferCount )
	{
	CommandPool* pool = new CommandPool();
	pool->Parent = this;
	this->CommandPools.insert( pool );

	// create the command pool vulkan object
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = PhysicalDeviceQueueGraphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	VLK_CALL( vkCreateCommandPool( this->Device, &poolInfo, nullptr, &pool->Pool ) );

	// allocate buffer objects
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = pool->Pool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = (uint32_t)bufferCount;
	pool->Buffers.resize( (size_t)bufferCount );
	VLK_CALL( vkAllocateCommandBuffers( this->Device, &commandBufferAllocateInfo, pool->Buffers.data() ) );

	return pool;
	}

void Vlk::Renderer::RemoveGraphicsPipeline( GraphicsPipeline* pipeline )
	{
	auto it = this->GraphicsPipelines.find( pipeline );
	if( it == this->GraphicsPipelines.end() )
		{
		throw runtime_error( "Error: RemoveGraphicsPipeline() pipeline is not registered with the renderer, have you already removed it?" );
		}
	this->GraphicsPipelines.erase( it );
	}

void Vlk::Renderer::RemoveComputePipeline( ComputePipeline* pipeline )
	{
	auto it = this->ComputePipelines.find( pipeline );
	if( it == this->ComputePipelines.end() )
		{
		throw runtime_error( "Error: RemoveComputePipeline() pipeline is not registered with the renderer, have you already removed it?" );
		}
	this->ComputePipelines.erase( it );
	}

Vlk::GraphicsPipeline* Vlk::Renderer::CreateGraphicsPipeline()
	{
	GraphicsPipeline* pipeline = new GraphicsPipeline();
	pipeline->Parent = this;
	this->GraphicsPipelines.insert( pipeline );
	return pipeline;
	}

Vlk::ComputePipeline* Vlk::Renderer::CreateComputePipeline()
	{
	ComputePipeline* pipeline = new ComputePipeline();
	pipeline->Parent = this;
	this->ComputePipelines.insert( pipeline );
	return pipeline;
	}

VkResult Vlk::Renderer::AcquireNextFrame( uint& index )
	{
	// wait for frame to become available (eg fence in signaled state)
	vkWaitForFences( this->Device, 1, &this->InFlightFences[this->CurrentFrame], VK_TRUE, UINT64_MAX );

	// get an available image
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR( this->Device, this->SwapChain, UINT64_MAX, this->ImageAvailableSemaphores[this->CurrentFrame], VK_NULL_HANDLE, &imageIndex );
	if( result == VK_ERROR_OUT_OF_DATE_KHR )
		{
		return VK_ERROR_OUT_OF_DATE_KHR; // need to recreate swap chain before rendering
		}

	// we will allow VK_SUBOPTIMAL_KHR here
	if( result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR )
		{
		throw std::runtime_error( "Error: AcquireNextFrame(): Failed to acquire swap chain image." );
		}

	this->CurrentImage = (uint)imageIndex;

	// If a previous frame is still using this image, wait for it as well
	if( this->ImagesInFlight[this->CurrentImage] != VK_NULL_HANDLE )
		{
		vkWaitForFences( this->Device, 1, &this->ImagesInFlight[this->CurrentImage], VK_TRUE, UINT64_MAX );
		}

	// Mark the image as now being in use by this frame
	this->ImagesInFlight[this->CurrentImage] = this->InFlightFences[this->CurrentFrame];

	index = this->CurrentImage;
	return VK_SUCCESS;
	}

VkResult Vlk::Renderer::SubmitRenderCommandBuffersAndPresent( const std::vector<VkCommandBuffer>& buffers )
	{
	// set frame as busy
	vkResetFences( this->Device, 1, &this->InFlightFences[this->CurrentFrame] );

	// submit the command buffers
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = { this->ImageAvailableSemaphores[this->CurrentFrame] };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = (uint32_t)buffers.size();
	submitInfo.pCommandBuffers = buffers.data();
	VkSemaphore signalSemaphores[] = { this->RenderFinishedSemaphores[this->CurrentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	VLK_CALL( vkQueueSubmit( this->GraphicsQueue, 1, &submitInfo, this->InFlightFences[this->CurrentFrame] ) );

	// present the image to the swap chain
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { this->SwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	uint32_t imageIndex = (uint32_t)this->CurrentImage;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	VkResult result = vkQueuePresentKHR( this->PresentQueue, &presentInfo );
	if( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR )
		{
		return VK_ERROR_OUT_OF_DATE_KHR;
		}
	if( result != VK_SUCCESS )
		{
		throw std::runtime_error( "Error: SubmitRenderCommandBuffersAndPresent(): Failed to present swap chain image." );
		}

	// done, step up current frame
	this->CurrentFrame = ( this->CurrentFrame + 1 ) % MaximumConcurrentRenderFrames;
	return VK_SUCCESS;
	}

Vlk::BufferBase* Vlk::Renderer::CreateGenericBuffer( VkBufferUsageFlags bufferUsageFlags, VmaMemoryUsage memoryPropertyFlags, VkDeviceSize deviceSize, const void* src_data )
	{
	BufferBase* buffer = new BufferBase();
	buffer->Parent = this;

	// make sure we can transfer data
	if( src_data != nullptr && memoryPropertyFlags != VMA_MEMORY_USAGE_CPU_ONLY )
		{
		bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}

	// create the vulkan buffer, and just wrap it in the BufferBase class
	buffer->BufferHandle = this->CreateVulkanBuffer(
		bufferUsageFlags,
		memoryPropertyFlags,
		deviceSize,
		buffer->DeviceMemory
		);
	buffer->BufferSize = deviceSize;

	// if src_data is set, copy it over to the buffer. use staging if the buffer is on GPU
	if( src_data != nullptr )
		{
		void* dest_ptr = nullptr;

		// map either a staging buffer or the buffer directly
		if( memoryPropertyFlags == VMA_MEMORY_USAGE_CPU_ONLY || memoryPropertyFlags == VMA_MEMORY_USAGE_CPU_TO_GPU )
			{
			dest_ptr = buffer->MapMemory();
			memcpy( dest_ptr, src_data, (size_t)deviceSize );
			buffer->UnmapMemory();
			}
		else
			{
			// GPU copy, use a staging buffer
			VkBuffer stagingBuffer;
			VmaAllocation stagingBufferMemory;

			// create a host visible staging buffer to copy data to
			stagingBuffer = this->CreateVulkanBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VMA_MEMORY_USAGE_CPU_ONLY,
				deviceSize,
				stagingBufferMemory
				);

			VLK_CALL( vmaMapMemory( this->MemoryAllocator, stagingBufferMemory, &dest_ptr ) );
			memcpy( dest_ptr, src_data, (size_t)deviceSize );
			vmaUnmapMemory( this->MemoryAllocator, stagingBufferMemory );

			// now transfer the data to the device local buffer, using a single time command
			VkCommandBuffer cmd = this->BeginInternalCommandBuffer();
			VkBufferCopy copyRegion{};
			copyRegion.size = deviceSize;
			vkCmdCopyBuffer( cmd, stagingBuffer, buffer->GetBuffer(), 1, &copyRegion );
			this->EndAndSubmitInternalCommandBuffer( cmd );

			// done with the staging buffer
			vmaDestroyBuffer( this->MemoryAllocator, stagingBuffer, stagingBufferMemory );
			}
		}

	return buffer;
	}

Vlk::VertexBuffer* Vlk::Renderer::CreateVertexBuffer( const VertexBufferDescription& description, uint vertexCount, const void* data )
	{
	VertexBuffer* vbuffer = new VertexBuffer();
	vbuffer->Parent = this;
	vbuffer->Description = description;

	VkDeviceSize bufferSize = description.GetVertexInputBindingDescription().stride;
	bufferSize *= vertexCount;

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferMemory;

	// create a host visible staging buffer to copy data to
	stagingBuffer = this->CreateVulkanBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY,
		bufferSize,
		stagingBufferMemory
		);

	// TODO: need to be able to set these additional flags from the caller
	// create an optimized device local buffer that will receive the data
	vbuffer->BufferHandle = this->CreateVulkanBuffer(
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_GPU_ONLY, 
		bufferSize,
		vbuffer->DeviceMemory
		);
	vbuffer->BufferSize = bufferSize;

	// map and copy the data to the staging buffer
	void* memoryPtr;
	VLK_CALL( vmaMapMemory( this->MemoryAllocator, stagingBufferMemory, &memoryPtr ) );
	memcpy( memoryPtr, data, (size_t)bufferSize );
	vmaUnmapMemory( this->MemoryAllocator, stagingBufferMemory );

	// now transfer the data to the device local buffer, using a single time command
	VkCommandBuffer cmd = this->BeginInternalCommandBuffer();
	VkBufferCopy copyRegion{};
	copyRegion.size = bufferSize;
	vkCmdCopyBuffer( cmd, stagingBuffer, vbuffer->GetBuffer(), 1, &copyRegion );
	this->EndAndSubmitInternalCommandBuffer( cmd );

	// done with the staging buffer and memeory
	vmaDestroyBuffer( this->MemoryAllocator, stagingBuffer, stagingBufferMemory );

	// return the created vertex buffer
	return vbuffer;
	}

Vlk::IndexBuffer* Vlk::Renderer::CreateIndexBuffer( VkIndexType indexType , uint indexCount, const void* indices )
	{
	IndexBuffer* ibuffer = new IndexBuffer();
	ibuffer->Parent = this;
	ibuffer->IndexType = indexType;

	VkDeviceSize bufferSize;
	if( indexType == VK_INDEX_TYPE_UINT32 )
		bufferSize = sizeof( uint32_t );
	else if( indexType == VK_INDEX_TYPE_UINT16 )
		bufferSize = sizeof( uint16_t );
	else
		{
		throw runtime_error( "Error: CreateIndexBuffer() indexType is invalid, only 16 or 32 bit allowed." );
		}
	bufferSize *= indexCount;

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferMemory;

	// create a host visible staging buffer to copy data to
	stagingBuffer = this->CreateVulkanBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY,
		bufferSize,
		stagingBufferMemory
		);

	// TODO: need to be able to set these additional flags from the caller
	// create an optimized device local buffer that will receive the data
	ibuffer->BufferHandle = this->CreateVulkanBuffer(
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_GPU_ONLY,
		bufferSize,
		ibuffer->DeviceMemory
		);
	ibuffer->BufferSize = bufferSize;

	// map and copy the data to the staging buffer
	void* memoryPtr;
	VLK_CALL( vmaMapMemory( this->MemoryAllocator, stagingBufferMemory, &memoryPtr ) );
	memcpy( memoryPtr, indices, (size_t)bufferSize );
	vmaUnmapMemory( this->MemoryAllocator, stagingBufferMemory );

	// now transfer the data to the device local buffer, using a single time command
	VkCommandBuffer cmd = this->BeginInternalCommandBuffer();
	VkBufferCopy copyRegion{};
	copyRegion.size = bufferSize;
	vkCmdCopyBuffer( cmd, stagingBuffer, ibuffer->GetBuffer(), 1, &copyRegion );
	this->EndAndSubmitInternalCommandBuffer( cmd );

	// done with the staging buffer and memeory
	vmaDestroyBuffer( this->MemoryAllocator, stagingBuffer, stagingBufferMemory );

	// return the created vertex buffer
	return ibuffer;
	}

void Vlk::Renderer::WaitForDeviceIdle()
	{
	if( this->Device == nullptr )
		{
		throw runtime_error( "Error: WaitForDeviceIdle() no device set up." );
		}
	vkDeviceWaitIdle( this->Device );
	}

Vlk::DescriptorLayout* Vlk::Renderer::CreateDescriptorLayout()
	{
	DescriptorLayout* layout = new DescriptorLayout();
	layout->Parent = this;

	return layout;
	}

Vlk::DescriptorPool* Vlk::Renderer::CreateDescriptorPool( uint descriptorSetCount, uint uniformBufferCount, uint samplersCount )
	{
	DescriptorPool* pool = new DescriptorPool();
	pool->Parent = this;

	VkDescriptorPoolSize descriptorPoolSizes[2];
	descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>( uniformBufferCount );
	descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>( samplersCount );

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>( 2 );
	descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;
	descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>( descriptorSetCount );

	VLK_CALL( vkCreateDescriptorPool( this->Device, &descriptorPoolCreateInfo, nullptr, &pool->Pool ) );

	return pool;
	}

Vlk::UniformBuffer* Vlk::Renderer::CreateUniformBuffer( VkDeviceSize bufferSize )
	{
	UniformBuffer* buffer = new UniformBuffer();
	buffer->Parent = this;

	// create a uniform buffer that can be mapped 
	buffer->BufferHandle = this->CreateVulkanBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY,
		bufferSize,
		buffer->DeviceMemory
		);
	buffer->BufferSize = bufferSize;

	return buffer;
	}

Vlk::Image* Vlk::Renderer::CreateImage( const ImageTemplate &it ) 
	{
	Image* image = new Image( this );
	
	VLK_CALL( vmaCreateImage( 
		this->MemoryAllocator, 
		&it.ImageCreateInfo, 
		&it.AllocationCreateInfo, 
		&image->ImageHandle, 
		&image->Allocation, 
		nullptr 
		) );

	// optionally upload pixel data to the image
	if(it.UploadSourcePtr)
		{
		// create a staging buffer to copy the pixel data to
		VmaAllocation stagingBufferMemory;
		VkBuffer stagingBuffer = this->CreateVulkanBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY,
			it.UploadSourceSize,
			stagingBufferMemory
		);

		// map the CPU buffer
		void* memoryPtr;
		VLK_CALL( vmaMapMemory( this->MemoryAllocator, stagingBufferMemory, &memoryPtr ) );
		memcpy( memoryPtr, it.UploadSourcePtr, static_cast<size_t>(it.UploadSourceSize) );
		vmaUnmapMemory( this->MemoryAllocator, stagingBufferMemory );

		// transition image to transfer optimal so we can copy to it
		VkImageMemoryBarrier imageMemoryBarrier = it.UploadLayoutTransition;
		imageMemoryBarrier.image = image->ImageHandle;
		VkCommandBuffer commandBuffer = this->BeginInternalCommandBuffer();
		vkCmdPipelineBarrier( commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier );
		this->EndAndSubmitInternalCommandBuffer( commandBuffer );

		// copy from the buffer to the image
		commandBuffer = this->BeginInternalCommandBuffer();
		vkCmdCopyBufferToImage( commandBuffer, stagingBuffer, image->ImageHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)it.UploadBufferImageCopies.size(), it.UploadBufferImageCopies.data() );
		this->EndAndSubmitInternalCommandBuffer( commandBuffer );

		// done with the buffer
		vmaDestroyBuffer( this->MemoryAllocator, stagingBuffer, stagingBufferMemory );
		}

	// transition image to final layout
	if(it.TransitionImageLayout)
		{
		VkImageMemoryBarrier imageMemoryBarrier = it.FinalLayoutTransition;
		imageMemoryBarrier.image = image->ImageHandle;

		VkPipelineStageFlags srcStageMask = it.UploadSourcePtr ? VK_PIPELINE_STAGE_TRANSFER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags destStageMask = it.FinalLayoutStageMask;

		VkCommandBuffer commandBuffer = this->BeginInternalCommandBuffer();
		vkCmdPipelineBarrier( commandBuffer, srcStageMask, destStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier );
		this->EndAndSubmitInternalCommandBuffer( commandBuffer );
		}

	// create the image view
	VkImageViewCreateInfo imageViewCreateInfo = it.ImageViewCreateInfo;
	imageViewCreateInfo.image = image->ImageHandle;
	VLK_CALL( vkCreateImageView( this->Device, &imageViewCreateInfo, nullptr, &image->ImageView ) );

	return image;
	}

Vlk::Sampler* Vlk::Renderer::CreateSampler( const SamplerTemplate& _st )
	{
	Sampler* sampler = new Sampler( this );

	// make a copy of the template which we can modify
	SamplerTemplate st = _st;

	// add extensions in linked list
	if(st.UseSamplerReductionModeCreateInfo)
		{
		st.SamplerReductionModeCreateInfo.pNext = st.SamplerCreateInfo.pNext;
		st.SamplerCreateInfo.pNext = &st.SamplerReductionModeCreateInfo;
		}

	VLK_CALL( vkCreateSampler( this->Device, &st.SamplerCreateInfo, nullptr, &sampler->SamplerHandle ) );

	return sampler;
	}

Vlk::Renderer::~Renderer()
	{
	// remove all extensions
	for( size_t i = 0; i < this->EnabledExtensions.size(); ++i )
		{
		this->EnabledExtensions[i]->Cleanup();
		}
	for( size_t i = 0; i < this->EnabledExtensions.size(); ++i )
		{
		delete this->EnabledExtensions[i];
		}
	this->EnabledExtensions.clear();

	this->DeleteSwapChain();

	for( uint i = 0; i < MaximumConcurrentRenderFrames; i++ )
		{
		vkDestroySemaphore( this->Device, this->RenderFinishedSemaphores[i], nullptr );
		vkDestroySemaphore( this->Device, this->ImageAvailableSemaphores[i], nullptr );
		vkDestroyFence( this->Device, this->InFlightFences[i], nullptr );
		}

	if( this->InternalCommandPool != nullptr )
		{
		vkDestroyCommandPool( this->Device, InternalCommandPool, nullptr );
		}

	vmaDestroyAllocator( this->MemoryAllocator );

	if( this->Device != nullptr )
		{
		vkDestroyDevice( this->Device, nullptr );
		}

	if( this->Surface != nullptr )
		{
		vkDestroySurfaceKHR( this->Instance, this->Surface, nullptr );
		}

	if( EnableVulkanValidation )
		{
		_vkDestroyDebugUtilsMessengerEXT( this->Instance, this->DebugUtilsMessenger, nullptr );
		}

	for( auto ext : this->EnabledExtensions )
		{
		delete ext;
		}

	vkDestroyInstance( this->Instance, nullptr );
	}