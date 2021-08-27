#pragma once

#include <vector>
#include <set>
#include <memory>
#include <functional>

// include Vulkan.h and VMA memory allocator
#pragma warning( push )
#pragma warning( disable : 26812 ) // Disable warning for "enum class" since we can't modify Vulcan SDK
#include <vk_mem_alloc.h> // includes vulkan.h

#define BDSubmoduleMacro( _classname , _parentclass , _mainmodule )\
	public:\
		~_classname();\
	protected:\
		_classname( const _mainmodule* _module ) : _parentclass( _module ) {};\
		_classname( const _classname& other );\
		friend class _mainmodule;\
	private:

#define BDSubmoduleBaseMacro( _classname , _mainmodule )\
	class _mainmodule;\
	class _classname\
		{\
		protected:\
			const _mainmodule* Module = nullptr;\
		public:\
			~_classname() {};\
			const _mainmodule* GetModule() const { return this->Module; };\
		protected:\
			_classname( const _mainmodule* _module ) { this->Module = _module; };\
			_classname( const _classname& other );\
			friend class _mainmodule;\
		};

// used if a submodule is part of an extension but derives from one of the main classes
#define BDExtensionSubmoduleMacro( _classname , _parentclass , _exmodule )\
	protected:\
		const _exmodule* ExModule = nullptr;\
	public:\
		~_classname();\
		const _exmodule* GetExModule() const { return this->ExModule; };\
	protected:\
		_classname( const _exmodule* _exmodule ) : _parentclass( _exmodule->GetModule() ) {};\
		_classname( const _classname& other );\
		friend class _exmodule;\
	private:



#define BDGetMacro( type , name ) const type Get##name() const { return this->name; }
#define BDGetCustomNameMacro( type , publicname , privatename ) const type Get##publicname() const { return this->privatename; }
#define BDGetConstRefMacro( type , name ) const type& Get##name() const { return this->name; }

namespace Vlk 
	{
	typedef unsigned int uint;

	class Buffer;
	class BufferTemplate;
	class VertexBuffer;
	class VertexBufferTemplate;
	class IndexBuffer;
	class IndexBufferTemplate;
	
	class DescriptorSetLayout;
	class DescriptorSetLayoutTemplate;

	class DescriptorPool;
	class DescriptorPoolTemplate;

	class Pipeline;
	class GraphicsPipelineTemplate;
	class ComputePipelineTemplate;

	class CommandPool;
	
	class VertexBufferDescription;
	class Image;
	class ImageTemplate;
	class Texture;
	class Extension;
	class RayTracingExtension;
	class BufferDeviceAddressExtension;
	class DescriptorIndexingExtension;
	class Sampler;
	class SamplerTemplate;

	class ShaderModule;

	BDSubmoduleBaseMacro( RendererSubmodule , Renderer );

	class Renderer
		{
		private:
			Renderer() = default;
			Renderer( const Renderer& other );
			friend class ComputePipeline;
			friend class CommandPool;
			friend class RayTracingExtension;
			friend class RayTracingPipeline;
			
			struct TargetImage
				{
				std::unique_ptr<Image> Color; // color channel RGBA
				std::unique_ptr<Image> Depth; // depth channel 
				};

			std::vector<const char*> ExtensionList;
			std::vector<const char*> DeviceExtensionList;

			VkInstance Instance = nullptr;
			VkSurfaceKHR Surface = nullptr;
			bool EnableVulkanValidation = false;
			VkDebugUtilsMessengerEXT DebugUtilsMessenger = nullptr;

			VkPhysicalDevice PhysicalDevice = nullptr;
			uint PhysicalDeviceQueueGraphicsFamily = (uint)-1;
			uint PhysicalDeviceQueuePresentFamily = (uint)-1;

			VkDevice Device = nullptr;
			VkQueue GraphicsQueue = nullptr;
			VkQueue PresentQueue = nullptr;
			VkSurfaceFormatKHR SurfaceFormat{};
			VkPresentModeKHR PresentMode{};
			VkExtent2D RenderExtent{};
			VkFormat SurfaceDepthFormat{};
			bool DepthFormatHasStencil = false;
			VkRenderPass RenderPass = nullptr;

			VmaAllocator MemoryAllocator = nullptr;

			VkSwapchainKHR SwapChain = nullptr;
			std::vector<VkImage> SwapChainImages;

			std::vector<TargetImage> TargetImages;
			std::vector<VkFramebuffer> Framebuffers;

			VkSurfaceCapabilitiesKHR SurfaceCapabilities{};
			std::vector<VkSurfaceFormatKHR> AvailableSurfaceFormats;
			std::vector<VkPresentModeKHR> AvailablePresentModes;
			
			VkPhysicalDeviceFeatures2 PhysicalDeviceFeatures{};
			VkPhysicalDeviceProperties2 PhysicalDeviceProperties{};

			VkCommandPool InternalCommandPool = nullptr; 

			std::vector<VkSemaphore> ImageAvailableSemaphores;
			std::vector<VkSemaphore> RenderFinishedSemaphores;
			std::vector<VkFence> InFlightFences;
			std::vector<VkFence> ImagesInFlight;
			uint CurrentFrame = 0;
			uint CurrentImage = 0;

			// extensions
			std::vector<Extension*> EnabledExtensions;
			DescriptorIndexingExtension* DescriptorIndexingEXT = nullptr;
			BufferDeviceAddressExtension* BufferDeviceAddressEXT = nullptr;
			RayTracingExtension* RayTracingEXT = nullptr;

			bool LookupPhysicalDeviceQueueFamilies();
			bool ValidatePhysicalDeviceRequiredExtensionsSupported();

			void DeleteSwapChain();

			// generic functions to create vulkan device buffer 
			VkBuffer CreateVulkanBuffer( VkBufferUsageFlags bufferUsageFlags, VmaMemoryUsage memoryPropertyFlags, VkDeviceSize deviceSize, VmaAllocation& deviceMemory ) const;

			/// create base vulkan buffer
			template<class B,class BT> B* NewBuffer( const BT& bt ) const;

			VkCommandBuffer BeginInternalCommandBuffer() const;
			void EndAndSubmitInternalCommandBuffer( VkCommandBuffer buffer ) const;

		public:

			// Create the VulkanRenderer object
			struct CreateParameters
				{
				// true to enable validation
				bool EnableVulkanValidation;
				
				// true to enable ray tracing extension. if not supported, the create will fail
				bool EnableRayTracingExtension;

				// list of needed extensions for eg windowing system
				uint NeededExtensionsCount;
				const char** NeededExtensions;

				// debug message callback and type/severity masks
				PFN_vkDebugUtilsMessengerCallbackEXT DebugMessageCallback;
				VkDebugUtilsMessageSeverityFlagsEXT DebugMessageSeverityMask;
				VkDebugUtilsMessageTypeFlagsEXT DebugMessageTypeMask;
				};
			static Renderer* Create( const CreateParameters& parameters );

			// setup the device with the created surface
			void CreateDevice( VkSurfaceKHR surface );

			/// create the rendering swap chain
			struct CreateSwapChainParameters
				{
				// the wanted surface format. get the default selected with GetSurfaceFormat()
				VkSurfaceFormatKHR SurfaceFormat;

				// the wanted presentation format. get the default selected with GetPresentMode()
				VkPresentModeKHR PresentMode;

				// the render extents on the surface, as reported by the framework.
				VkExtent2D RenderExtent;
				};
			void CreateSwapChainAndFramebuffers( const CreateSwapChainParameters& parameters );
			void UpdateSurfaceCapabilitiesAndFormats();
			void RecreateSwapChain( const CreateSwapChainParameters& parameters );
			
			/// get a new frame to draw to. index will receive the index of the image in the swap chain.
			/// if the return value is VK_ERROR_OUT_OF_DATE_KHR, then the swap chain needs to be recreated before rendering
			VkResult AcquireNextFrame( uint& index );

			/// submit command buffers to the current frame
			VkResult SubmitRenderCommandBuffersAndPresent( const std::vector<VkCommandBuffer>& buffers );

			/// wait for device to idle, for synching, eg shutting down
			void WaitForDeviceIdle();

			/// create a Graphics Pipeline object based on template
			Pipeline* CreateGraphicsPipeline( const GraphicsPipelineTemplate& gpt ) const;

			/// create a Compute Pipeline object based on template
			Pipeline* CreateComputePipeline( const ComputePipelineTemplate& cpt ) const;

			/// create a CommandPool object. set number of buffers to allocate
			CommandPool* CreateCommandPool( uint bufferCount ) const;

			/// create buffers based on templates
			Buffer* CreateBuffer( const BufferTemplate &bt ) const;
			VertexBuffer* CreateVertexBuffer( const VertexBufferTemplate& bt ) const;
			IndexBuffer* CreateIndexBuffer( const IndexBufferTemplate& bt ) const;
			
			/// (temporary) create descriptor layout
			DescriptorSetLayout* CreateDescriptorSetLayout( const DescriptorSetLayoutTemplate& dst ) const;

			/// Create a descriptor pool 
			DescriptorPool* CreateDescriptorPool( const DescriptorPoolTemplate& dpt ) const;

			// create image
			Image* CreateImage( const ImageTemplate& it );

			// create sampler
			Sampler* CreateSampler( const SamplerTemplate& st );

			// Create and submit synchronously a command buffer with the internal command pool.
			// The call is blocking until the command has been run, so only use during inits.
			void RunBlockingCommandBuffer( std::function<void (VkCommandBuffer cmd)> fp ) const;

			~Renderer();

			// public get methods
			BDGetMacro( VkInstance, Instance );
			BDGetMacro( VkPhysicalDevice, PhysicalDevice );
			BDGetMacro( VkDevice, Device );
			BDGetMacro( VkQueue, GraphicsQueue );
			BDGetMacro( VkQueue, PresentQueue );
			BDGetMacro( std::vector<VkSurfaceFormatKHR>, AvailableSurfaceFormats );
			BDGetMacro( std::vector<VkPresentModeKHR>, AvailablePresentModes );
			BDGetMacro( VkPhysicalDeviceFeatures2, PhysicalDeviceFeatures );
			BDGetMacro( VkSurfaceCapabilitiesKHR, SurfaceCapabilities );
			BDGetMacro( VkSurfaceFormatKHR, SurfaceFormat );
			BDGetMacro( VkPresentModeKHR, PresentMode );
			BDGetMacro( VkExtent2D, RenderExtent );
			BDGetMacro( std::vector<VkFramebuffer>, Framebuffers );
			BDGetMacro( std::vector<VkImage>, SwapChainImages );
			BDGetMacro( VkRenderPass, RenderPass );
			BDGetMacro( VmaAllocator, MemoryAllocator );

			const Image* GetColorTargetImage( uint index ) const { return this->TargetImages[index].Color.get(); }
			const Image* GetDepthTargetImage( uint index ) const { return this->TargetImages[index].Depth.get(); }

			// get extensions
			RayTracingExtension* GetRayTracingExtension() const { return this->RayTracingEXT; }

		};
	};

#pragma warning( pop )