#pragma once

#include "Vlk_Renderer.h"

namespace Vlk
    {
    class Buffer;

    class Sampler : public RendererSubmodule
        {
        BDSubmoduleMacro( Sampler, RendererSubmodule, Renderer );

        private:
            VkSampler SamplerHandle = nullptr;

        public:
            BDGetCustomNameMacro( VkSampler, Sampler, SamplerHandle );
        };

    // template used to create an Sampler
    class SamplerTemplate
        {
        public:
            // sampler create information
            VkSamplerCreateInfo SamplerCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

            // create extensions
            bool UseSamplerReductionModeCreateInfo = false;
            VkSamplerReductionModeCreateInfoEXT SamplerReductionModeCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT };

            /////////////////////////////////

            // create a standard 2d bilinear sampler with mip mapping and repeating texturing
            static SamplerTemplate Linear();

            // create a depth Sampler that can be used to conservatively sample the depth map to create a depth reduce pyramid
            static SamplerTemplate DepthReduce( VkSamplerReductionMode reductionMode = VK_SAMPLER_REDUCTION_MODE_MAX );

        };
    };
