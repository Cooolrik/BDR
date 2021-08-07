#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"

namespace Vlk
    {
    class DescriptorLayout
        {
        private:
            DescriptorLayout() = default;
            DescriptorLayout( const DescriptorLayout& other );
            friend class Renderer;

            Renderer* Parent = nullptr;

            VkDescriptorSetLayout DescriptorSetLayout = nullptr;
            std::vector<VkDescriptorSetLayoutBinding> Bindings;

        public:

            // Sets the binding at index "bindingIndex" to a uniform buffer binding
            uint AddUniformBufferBinding( VkShaderStageFlags stageFlags, uint arrayCount = 1 );

            // Sets the binding at index "bindingIndex" to a storage buffer binding
            uint AddStorageBufferBinding( VkShaderStageFlags stageFlags, uint arrayCount = 1 );

            // Sets the binding at index "bindingIndex" to a sampler binding
            uint AddSamplerBinding( VkShaderStageFlags stageFlags, uint arrayCount = 1 );

            // Sets the binding at index "bindingIndex" to an acceleration structure binding
            uint AddAccelerationStructureBinding( VkShaderStageFlags stageFlags, uint arrayCount = 1 );

            // Sets the binding at index "bindingIndex" to a stored image binding
            uint AddStoredImageBinding( VkShaderStageFlags stageFlags, uint arrayCount = 1 );

            // Creates the DescriptorSetLayout object
            void BuildDescriptorSetLayout();

            ~DescriptorLayout();

            uint GetBindingCount() const;

            BDGetMacro( VkDescriptorSetLayout, DescriptorSetLayout );
            BDGetMacro( std::vector<VkDescriptorSetLayoutBinding>, Bindings );
        };
    };