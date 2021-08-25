#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"

namespace Vlk
    {
    class ShaderModule;
    class VertexBuffer;
    class DescriptorSetLayout;

    class Pipeline : public RendererSubmodule
        {
        BDSubmoduleMacro( Pipeline, RendererSubmodule, Renderer );

        protected:
            VkPipeline PipelineHandle = nullptr;
            VkPipelineLayout PipelineLayout = nullptr;
            VkPipelineBindPoint PipelineBindPoint = {};

        public:
            BDGetCustomNameMacro( VkPipeline, Pipeline , PipelineHandle );
            BDGetMacro( VkPipelineLayout, PipelineLayout );
            BDGetMacro( VkPipelineBindPoint, PipelineBindPoint );
        };
    };