#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"

namespace Vlk
    {
    class Extension
        {
        protected:
            Extension() = default;
            Extension( const RayTracingExtension& other );
            friend class Renderer;

            Renderer* Parent = nullptr;
            
            static void AddExtensionToList( std::vector<const char*>* extensionList, const char* extensionName );
 
        public:

            // called before instance is created
            virtual VkResult CreateInstance( VkInstanceCreateInfo* instanceCreateInfo, std::vector<const char*>* extensionList );

            // called after instance is created, good place to set up dynamic methods and call stuff post create instance 
            virtual VkResult PostCreateInstance();

            // called to add required device extensions
            virtual VkResult AddRequiredDeviceExtensions( 
                VkPhysicalDeviceFeatures2* physicalDeviceFeatures, 
                VkPhysicalDeviceProperties2* physicalDeviceProperties,
                std::vector<const char*>* extensionList
                );

            // called to select pysical device. return true if the device is acceptable
            virtual bool SelectDevice(
                const VkSurfaceCapabilitiesKHR& surfaceCapabilities,
                const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats,
                const std::vector<VkPresentModeKHR>& availablePresentModes,
                const VkPhysicalDeviceFeatures2& physicalDeviceFeatures,
                const VkPhysicalDeviceProperties2& physicalDeviceProperties
            );

            // called before device is created
            virtual VkResult CreateDevice( VkDeviceCreateInfo* deviceCreateInfo );

            // called after device is created, good place to set up dynamic methods and call stuff post create device 
            virtual VkResult PostCreateDevice();

            // called before any extension is deleted. makes it possible to remove data that is dependent on some other extension
            virtual VkResult Cleanup();

            // dtor, removes any allocated data
            virtual ~Extension();

            // get the parent renderer
            BDGetMacro( Renderer*, Parent );
        };
    };
