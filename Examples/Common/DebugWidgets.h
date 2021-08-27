#pragma once

#include <Vlk_VertexBuffer.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

class Camera;

// debug widgets: immediate mode debug rendering 
class DebugWidgets
    {
    public:
        struct MeshData
            {
            unsigned int VertexOffset = 0;
            unsigned int VertexCount = 0;
            unsigned int IndexOffset = 0;
            unsigned int IndexCount = 0;
            };

        struct PushConstants
            {
            glm::mat4 Transform;
            };

        struct SceneUniforms
            {
            glm::mat4 View;
            glm::mat4 Proj;
            glm::mat4 ViewI;
            glm::mat4 ProjI;
            glm::vec3 ViewPosition; // origin of the view in world space
            };

        struct PerFrameData
            {
            std::unique_ptr<Vlk::DescriptorPool> DescriptorPool = nullptr;
            std::unique_ptr<Vlk::Buffer> SceneUBO = nullptr;
            VkDescriptorSet DescriptorSet = nullptr;
            };

        struct RenderItem
            {
            unsigned int MeshID = 0;
            glm::mat4 Transform = {};
            };

        const Vlk::Renderer* Renderer = nullptr;

        std::unique_ptr<Vlk::ShaderModule> VertShader = nullptr;
        std::unique_ptr<Vlk::ShaderModule> FragShader = nullptr;

        std::unique_ptr<Vlk::Pipeline> RenderPipeline = nullptr;
        std::unique_ptr<Vlk::DescriptorSetLayout> RenderDescriptorSetLayout = nullptr;

        std::vector<PerFrameData> PerFrameData = {};

        std::vector<MeshData> Meshes = {};
        std::vector<RenderItem> ItemQueue = {};

        std::unique_ptr<Vlk::VertexBuffer> VertexBuffer = nullptr;
        std::unique_ptr<Vlk::IndexBuffer> IndexBuffer = nullptr;

        SceneUniforms SceneData = {};
        unsigned int CurrentFrame = 0;
        VkViewport Viewport;
        VkRect2D ScissorRectangle;

        void SetupMeshes();

    public:
        enum WidgetTypes
            {
            UnitSphere = 0, // sphere, radius = 1, RGB
            CoordinateAxies = 1, // xyz axies, at origin, RGB -> white
            UnitCone = 2, // unit length cone, along Z axis, white
            UnitVectorZ = 3, // unit vector, along X axis
            UnitCube = 4, // unit side cube, from (0,0,0) to (1,1,1)
            };

        DebugWidgets( const Vlk::Renderer* renderer );
        ~DebugWidgets();

        // begin/end frame update
        void BeginFrame( unsigned int frame_id );
        void EndFrame();

        // update methods that the caller needs to set
        void UpdateSceneData( const Camera &pCam );
        void UpdateSceneData( SceneUniforms _sceneData );
        void SetViewport( float x, float y, float width, float height, float minDepth = 0.f, float maxDepth = 1.f );
        void SetScissorRectangle( int32_t x, int32_t y, uint32_t width, uint32_t height );

        // add an item
        void RenderWidget( WidgetTypes type, glm::mat4 transform );
        void RenderSphere( glm::vec3 world_pos, float radius );
        void RenderCone( glm::vec3 world_pos, glm::vec3 direction , float radiusA, float radiusB, float length );
        void RenderConeWithAngle( glm::vec3 world_pos, glm::vec3 world_target, float angle_rads );
        void RenderAABB( glm::vec3 minv, glm::vec3 maxv );


        // renders the draw commands into vulkan buffer
        void Draw( Vlk::CommandPool* pool );

    };

