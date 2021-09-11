#pragma once

#include "Vlk_Renderer.h"
#include "Vlk_Extension.h"

namespace Vlk
    {
    class MeshShaderExtension : public Extension
        {
        BDSubmoduleMacro( MeshShaderExtension, Extension, Renderer );

        private:

            VkPhysicalDeviceMeshShaderFeaturesNV MeshShaderFeaturesQuery{};
            VkPhysicalDeviceMeshShaderFeaturesNV MeshShaderFeaturesCreate{};

            VkPhysicalDeviceMeshShaderPropertiesNV MeshShaderProperties{};

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

            // called after device is created, good place to set up dynamic methods and call stuff post create device 
            virtual VkResult PostCreateInstance();

            // Extension dynamic methods

            // VK_KHR_acceleration_structure
            static PFN_vkCmdDrawMeshTasksNV vkCmdDrawMeshTasksNV;
            static PFN_vkCmdDrawMeshTasksIndirectNV vkCmdDrawMeshTasksIndirectNV;
            static PFN_vkCmdDrawMeshTasksIndirectCountNV vkCmdDrawMeshTasksIndirectCountNV;

        };
    };













