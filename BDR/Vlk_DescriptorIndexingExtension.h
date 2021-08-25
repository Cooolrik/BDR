#pragma once

#include "Vlk_Renderer.h"
#include "Vlk_Extension.h"

namespace Vlk
    {
    class DescriptorIndexingExtension : public Extension
        {
        BDSubmoduleMacro( DescriptorIndexingExtension, Extension, Renderer );

        private:

            VkPhysicalDeviceDescriptorIndexingFeatures DescriptorIndexingFeaturesQuery{};
            VkPhysicalDeviceDescriptorIndexingFeatures DescriptorIndexingFeaturesCreate{};

            VkPhysicalDeviceDescriptorIndexingProperties DescriptorIndexingProperties{};

        public:

            // ####################################
            //
            // Extension code
            //


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

        };
    };













