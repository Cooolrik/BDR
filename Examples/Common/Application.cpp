
#include "Application.h"


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData )
	{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
	}

void ApplicationBase::Init()
	{
	if( this->UseWidgets && this->WaitInLoopForCameraDirty )
		{
		throw std::runtime_error( "Error: UseWidgets and WaitInLoopForCameraDirty are mutually exclusive" );
		}

	glfwInit();
	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	this->Window = glfwCreateWindow( this->InitialWindowDimensions[0], this->InitialWindowDimensions[1], this->WindowTitle.c_str(), nullptr, nullptr );
	this->Camera.Setup( this->Window );
	this->Camera.ScreenW = this->InitialWindowDimensions[0];
	this->Camera.ScreenH = this->InitialWindowDimensions[1];
	this->Camera.aspectRatio = float( this->Camera.ScreenW ) / float( this->Camera.ScreenH );

	// place the window to the right
	GLFWmonitor* primary = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode( primary );
	glfwSetWindowPos( this->Window, this->InitialWindowPosition[0], this->InitialWindowPosition[1] );

	// place the console below it
	this->ConsoleWindow = GetConsoleWindow();
	ShowWindow( this->ConsoleWindow, ( this->ShowConsole ) ? SW_SHOW : SW_HIDE );
	MoveWindow( this->ConsoleWindow, this->InitialConsolePosition[0], this->InitialConsolePosition[1], this->InitialConsoleDimensions[0], this->InitialConsoleDimensions[1], TRUE );

	// list the needed extensions for glfw
	uint glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

	// create the renderer, list needed extensions
	Vlk::Renderer::CreateParameters createParameters{};
	createParameters.EnableVulkanValidation = this->EnableValidation;
	createParameters.EnableRayTracingExtension = this->RequireRaytracing;
	createParameters.NeededExtensionsCount = glfwExtensionCount;
	createParameters.NeededExtensions = glfwExtensions;
	createParameters.DebugMessageCallback = &debugCallback;
	createParameters.DebugMessageSeverityMask =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	if( this->EnableVerboseOutput ) createParameters.DebugMessageSeverityMask |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
	createParameters.DebugMessageTypeMask =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	this->Renderer = Vlk::Renderer::Create( createParameters );

	// create the window surface using glfw
	if( glfwCreateWindowSurface( this->Renderer->GetInstance(), this->Window, nullptr, &this->Surface ) != VK_SUCCESS )
		{
		throw std::runtime_error( "failed to create window surface!" );
		}

	// set up the device and swap chain, hand over surface handle
	this->Renderer->CreateDevice( this->Surface );

	// set up swap chain 
	int width, height;
	glfwGetFramebufferSize( this->Window, &width, &height );
	Vlk::Renderer::CreateSwapChainParameters createSwapChainParameters;
	createSwapChainParameters.SurfaceFormat = this->Renderer->GetSurfaceFormat();
	createSwapChainParameters.PresentMode = ( this->UseTripleBuffering ) ? VK_PRESENT_MODE_FIFO_KHR : this->Renderer->GetPresentMode(); // VK_PRESENT_MODE_FIFO_KHR is always available, so we can safely choose it
	createSwapChainParameters.RenderExtent = { static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) };
	this->Renderer->CreateSwapChainAndFramebuffers( createSwapChainParameters );

	this->ConsoleOutputHandle = GetStdHandle( STD_OUTPUT_HANDLE );
	GetConsoleScreenBufferInfo( this->ConsoleOutputHandle, &this->ConsoleBufferInfo );

	if( this->UseWidgets )
		{
		this->UIWidgets = new ::ImGuiWidgets( this->Renderer , this->Window );
		this->DebugWidgets = new ::DebugWidgets( this->Renderer );
		}
	}

void ApplicationBase::Deinit()
	{
	this->Renderer->WaitForDeviceIdle();

	if( this->UIWidgets )
		{
		delete this->UIWidgets;
		}
	if( this->DebugWidgets )
		{
		delete this->DebugWidgets;
		}

	delete this->Renderer;

	glfwDestroyWindow( this->Window );
	glfwTerminate();
	}

void ApplicationBase::RecreateSwapchain()
	{
	// recreate framebuffer, wait until we have a real extent again
	int width = 0, height = 0;
	glfwGetFramebufferSize( this->Window, &width, &height );
	if( width == 0 || height == 0 )
		{
		printf( "Waiting for window...\n" );
		}
	while( width == 0 || height == 0 )
		{
		glfwGetFramebufferSize( this->Window, &width, &height );
		glfwWaitEvents();
		}
	this->Renderer->WaitForDeviceIdle();

	printf( "Recreating swap chain. Framebuffer size = (%d,%d)...\n", width, height );

	Vlk::Renderer::CreateSwapChainParameters createSwapChainParameters;
	createSwapChainParameters.SurfaceFormat = this->Renderer->GetSurfaceFormat();
	createSwapChainParameters.PresentMode = this->Renderer->GetPresentMode();
	createSwapChainParameters.RenderExtent = { static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) };
	this->Renderer->RecreateSwapChain( createSwapChainParameters );

	this->Camera.framebufferResized = false;
	}
