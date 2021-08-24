#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 )

#include "Vlk_Renderer.h"

namespace Vlk
    {
    class ShaderModule
        {
        private:
            ShaderModule() = default;
            ShaderModule( const ShaderModule& other );
            friend class ComputePipeline;
            friend class Renderer;

            VkShaderStageFlagBits Stage{};
            char* Name = nullptr;
            char* Entrypoint = nullptr;
            std::vector<char> Shader{};

        public:
            static ShaderModule* CreateFromFile(
                VkShaderStageFlagBits shaderStage,
                const char* shaderFilepath,
                const char* entrypoint = "main",
                const char* shaderName = nullptr
                );

            ~ShaderModule();

            BDGetMacro( VkShaderStageFlagBits, Stage );
            BDGetConstRefMacro( std::vector<char>, Shader );
            BDGetMacro( char*, Name );
            BDGetMacro( char*, Entrypoint );
        };
    };

