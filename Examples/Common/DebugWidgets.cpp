#include "Common.h"
#include "DebugWidgets.h"
#include "Camera.h"

#include <Vlk_VertexBuffer.h>
#include <Vlk_IndexBuffer.h>
#include <Vlk_Pipeline.h>
#include <Vlk_DescriptorSetLayout.h>
#include <Vlk_ShaderModule.h>
#include <Vlk_GraphicsPipeline.h>
#include <Vlk_DescriptorPool.h>
#include <Vlk_CommandPool.h>

using glm::mat4;
using glm::uvec2;
using glm::vec2;
using glm::vec3;

static const uint max_set_count = 20;
static const uint max_descriptor_count = 20;

class DebugVertex
    {
    public:
        vec3 Coords = {};
        vec3 Color = {};

        static Vlk::VertexBufferDescription GetVertexBufferDescription()
            {
            Vlk::VertexBufferDescription desc;
            desc.SetVertexInputBindingDescription( 0, sizeof( DebugVertex ), VK_VERTEX_INPUT_RATE_VERTEX );
            desc.AddVertexInputAttributeDescription( 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof( DebugVertex, Coords ) );
            desc.AddVertexInputAttributeDescription( 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof( DebugVertex, Color ) );
            return desc;
            }
    };

class MeshSource
    {
    public:
        vector<DebugVertex> Vertices;
        vector<uint16_t> Indices;

        // if remap_indices is set, indices are remapped into the larger array
        // if not set, just copies into the array, and the caller is responsible for the offsetting 
        DebugWidgets::MeshData AppendMesh( const MeshSource &src , bool remap_indices = true )
            {
            size_t vb = this->Vertices.size();
            size_t ib = this->Indices.size();

            size_t vc = src.Vertices.size();
            size_t ic = src.Indices.size();

            this->Vertices.resize( vb + vc );
            this->Indices.resize( ib + ic );

            // copy vertices into dest, at end
            memcpy( &this->Vertices[vb], src.Vertices.data(), sizeof( DebugVertex ) * vc );
             
            // copy indices into dest
            if( remap_indices )
                {
                // remap vertex indices using base index
                for( size_t i = 0; i < ic; ++i )
                    {
                    this->Indices[ib + i] = src.Indices[i] + (uint16_t)vb; 
                    }
                }
            else
                {
                // dont remap, just copy
                memcpy( &this->Indices[ib], src.Indices.data(), sizeof( uint16_t ) * ic );
                }

            DebugWidgets::MeshData ret;
            ret.VertexOffset = (uint)vb;
            ret.IndexOffset = (uint)ib;
            ret.VertexCount = (uint)vc;
            ret.IndexCount = (uint)ic;
            return ret;
            }
    };

///////////////////////////////////////////////////

static vec3 Project2Dto3D( uint projection_axis , vec2 coord )
    {
    switch( projection_axis )
        {
        case 0:
            return vec3( 0 , coord[1] , -coord[0] ); // down X axis, Y is up, Z is left
        case 1:
            return vec3( coord[0] , 0 , -coord[1] ); // down Y axis, X is right, Z is down, 
        default:
            return vec3( coord[0] , coord[1] , 0 ); // down Z axis, X is right, Y is up 
        }
    }

static MeshSource GenerateCircle( vec3 center, float radius, vec3 color, uint projection_axis, uint segments )
    {
    MeshSource ret = {};

    ret.Vertices.resize( segments );
    ret.Indices.resize( (size_t)segments * 2 );

    for( uint i = 0; i < segments; ++i )
        {
        float angle = (float)i / (float)segments * 3.14159f * 2.f;

        // generate vertex
        vec2 coord2 = {cosf( angle ) * radius , sinf( angle ) * radius};
        ret.Vertices[i].Coords = Project2Dto3D( projection_axis , coord2 ) + center;
        ret.Vertices[i].Color = color;

        // create a segment from this vertex to the next, wrap the last around to the first vertex
        ret.Indices[((size_t)i*2)+0] = (uint16_t)i;
        ret.Indices[((size_t)i*2)+1] = (uint16_t)((i+1) % segments);
        }

    return ret;
    }

static MeshSource GenerateSphere( vec3 center, float radius, uint segments , const vec3 axiscolors[3] = nullptr)
    {
    MeshSource ret;

    for( uint i = 0; i < 3; ++i )
        {
        vec3 color = glm::vec3();
        if( axiscolors )
            color = axiscolors[i];
        else
            color[i] = 1.f; // red, green, blue for axis 0, 1, 2

        ret.AppendMesh( GenerateCircle( center, radius, color, i, segments ) );
        }

    return ret;
    }

static MeshSource GenerateCone( vec3 center, float length, float radius, uint segments )
    {
    MeshSource ret;

    vec3 color = glm::vec3(1);

    ret.Vertices.resize( 5 );
    ret.Vertices[0] = {center , color};
    ret.Vertices[1] = {center+vec3(radius,0,length) , color};
    ret.Vertices[2] = {center+vec3(-radius,0,length) , color};
    ret.Vertices[3] = {center+vec3(0,radius,length) , color};
    ret.Vertices[4] = {center+vec3(0,-radius,length) , color};
    ret.Indices = {0,1,0,2,0,3,0,4};

    ret.AppendMesh( GenerateCircle( center+vec3(0,0,length), radius, color, 2, segments ) );

    return ret;
    }


static MeshSource GenerateAxies( vec3 center, float length, const vec3 axiscolors[3] = nullptr )
    {
    MeshSource ret;

    for( uint i = 0; i < 3; ++i )
        {
        MeshSource axis;

        vec3 color = glm::vec3();
        if( axiscolors )
            color = axiscolors[i];
        else
            color[i] = 1.f; // red, green, blue for axis 0, 1, 2

        vec3 direction = vec3( 0 );
        direction[i] = length;

        axis.Vertices.resize( 2 );
        axis.Vertices[0].Coords = center;
        axis.Vertices[0].Color = color;
        axis.Vertices[1].Coords = center + direction;
        axis.Vertices[1].Color = color;

        axis.Indices.resize( 2 );
        axis.Indices[0] = 0;
        axis.Indices[1] = 1;

        ret.AppendMesh( axis );
        }

    return ret;
    }

static MeshSource GenerateVector( vec3 center, vec3 direction )
    {
    MeshSource ret;

    vec3 color = glm::vec3(1);

    ret.Vertices.resize( 2 );
    ret.Vertices[0].Coords = center;
    ret.Vertices[0].Color = color;
    ret.Vertices[1].Coords = center + direction;
    ret.Vertices[1].Color = color;

    ret.Indices.resize( 2 );
    ret.Indices[0] = 0;
    ret.Indices[1] = 1;

    return ret;
    }


static MeshSource GeneratePlane( uint axis, glm::vec3 minv, glm::vec3 maxv, uint tesselation, vec3 color = vec3(1) )
    {
    uint axis_s = 1;
    uint axis_t = 2;

    if( axis == 1 )
        {
        axis_s = 0;
        axis_t = 2;
        }
    else if( axis == 2 )
        {
        axis_s = 0;
        axis_t = 1;
        }

    glm::vec3 delta_s{};
    glm::vec3 delta_t{};

    delta_s[axis_s] = maxv[axis_s] - minv[axis_s];
    delta_t[axis_t] = maxv[axis_t] - minv[axis_t];

    MeshSource ret;

    ret.Vertices.resize( (tesselation + 1) * 4 );
    ret.Indices.resize( (tesselation + 1) * 4 );

    // generate the vertices
    for( size_t a = 0; a < (tesselation+1); ++a )
        {
        float alpha = float( a ) / float( tesselation );
    
        glm::vec3 coords[4] =
            {
                { minv + (delta_s * alpha) },
                { minv + (delta_s * alpha) + (delta_t) },
                { minv + (delta_t * alpha) },
                { minv + (delta_t * alpha) + (delta_s) }
            };

        for( size_t i = 0; i < 4; ++i )
            {
            ret.Vertices[a*4+i].Coords = coords[i];
            ret.Vertices[a*4+i].Color = color;
            ret.Indices[a * 4 + i] = (uint16_t)(a * 4 + i);
            }
        }

    return ret;
    }

static MeshSource GenerateCube( vec3 minv, vec3 maxv, uint tesselation , const vec3 axiscolors[3] = nullptr )
    {
    MeshSource ret;

    glm::vec3 colors[3] = {};

    // either copy colors, or set values per axis
    for( uint i = 0; i < 3; ++i )
        {
        if( axiscolors )
            colors[i] = axiscolors[i];
        else
            colors[i][i] = 1.f;
        }

    ret.AppendMesh( GeneratePlane( 0, glm::vec3( minv.x, minv.y, minv.z ), glm::vec3( minv.x, maxv.y, maxv.z ) , tesselation , colors[0] ) );
    ret.AppendMesh( GeneratePlane( 0, glm::vec3( maxv.x, minv.y, minv.z ), glm::vec3( maxv.x, maxv.y, maxv.z ) , tesselation , colors[0] ) );
    ret.AppendMesh( GeneratePlane( 1, glm::vec3( minv.x, minv.y, minv.z ), glm::vec3( maxv.x, minv.y, maxv.z ) , tesselation , colors[1] ) );
    ret.AppendMesh( GeneratePlane( 1, glm::vec3( minv.x, maxv.y, minv.z ), glm::vec3( maxv.x, maxv.y, maxv.z ) , tesselation , colors[1] ) );
    ret.AppendMesh( GeneratePlane( 2, glm::vec3( minv.x, minv.y, minv.z ), glm::vec3( maxv.x, maxv.y, minv.z ) , tesselation , colors[2] ) );
    ret.AppendMesh( GeneratePlane( 2, glm::vec3( minv.x, minv.y, maxv.z ), glm::vec3( maxv.x, maxv.y, maxv.z ) , tesselation , colors[2] ) );

    return ret;
    }



void DebugWidgets::SetupMeshes()
    {
    MeshSource meshSources;

    GenerateSphere( vec3( 0 ), 1.f, 40 );

    // generate meshes
    this->Meshes.emplace_back( meshSources.AppendMesh( GenerateSphere( vec3(0) , 1.f , 40 ) , false ) ); // UnitSphere
    this->Meshes.emplace_back( meshSources.AppendMesh( GenerateAxies( vec3(0), 100.f ) , false ) ); // CoordinateAxies
    this->Meshes.emplace_back( meshSources.AppendMesh( GenerateCone( vec3(0), 1.f, 1.f, 40 ), false ) ); // UnitCone
    this->Meshes.emplace_back( meshSources.AppendMesh( GenerateVector( vec3(0), vec3(0,0,1) ), false ) ); // UnitVectorZ
    this->Meshes.emplace_back( meshSources.AppendMesh( GenerateCube( vec3(0), vec3(1), 4 ), false ) ); // UnitVectorZ

    this->VertexBuffer = u_ptr(this->Renderer->CreateVertexBuffer(
        Vlk::VertexBufferTemplate::VertexBuffer(
            DebugVertex::GetVertexBufferDescription(), 
            (uint)meshSources.Vertices.size(), 
            meshSources.Vertices.data() 
        )
    ));
    this->IndexBuffer = u_ptr(this->Renderer->CreateIndexBuffer( 
        Vlk::IndexBufferTemplate::IndexBuffer(
            VK_INDEX_TYPE_UINT16, 
            (uint)meshSources.Indices.size(), 
            meshSources.Indices.data() 
        )
    ));
    }

DebugWidgets::DebugWidgets( const Vlk::Renderer *renderer ) : Renderer(renderer)
	{
    this->SetupMeshes();

    // load the shaders
    this->VertShader = u_ptr(Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_VERTEX_BIT, "shaders/DebugRender.vert.spv" ));
    this->FragShader = u_ptr(Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_FRAGMENT_BIT, "shaders/DebugRender.frag.spv" ));

    // setup descriptor set layout
    Vlk::DescriptorSetLayoutTemplate rdlt;
    rdlt.AddUniformBufferBinding( VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT );	// 0 - buffer object
    this->RenderDescriptorSetLayout = u_ptr(this->Renderer->CreateDescriptorSetLayout( rdlt ));

    // setup the render pipeline, we will render lines
    unique_ptr<Vlk::GraphicsPipelineTemplate> gpt = u_ptr(new Vlk::GraphicsPipelineTemplate());
    gpt->SetVertexDataTemplateFromVertexBuffer( this->VertexBuffer.get() );
    gpt->AddShaderModule( this->VertShader.get() );
    gpt->AddShaderModule( this->FragShader.get() );
    gpt->AddDescriptorSetLayout( this->RenderDescriptorSetLayout.get() );
    gpt->AddPushConstantRange( VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( DebugWidgets::PushConstants ) );
    gpt->SetInputAssemblyToListOfLines();
    this->RenderPipeline = u_ptr( this->Renderer->CreateGraphicsPipeline( *gpt ) );

    // setup per frame data
    vector<VkFramebuffer> framebuffers = this->Renderer->GetFramebuffers();
    uint num_frames = (uint)framebuffers.size();

    this->PerFrameData.clear();
    this->PerFrameData.resize( num_frames );
    for( uint f = 0; f < num_frames; ++f )
        {
        struct DebugWidgets::PerFrameData& frame = this->PerFrameData[f];

        frame.DescriptorPool = u_ptr(this->Renderer->CreateDescriptorPool( Vlk::DescriptorPoolTemplate::General( max_set_count, max_descriptor_count ) ));
        frame.SceneUBO = u_ptr(this->Renderer->CreateBuffer( Vlk::BufferTemplate::UniformBuffer( sizeof( DebugWidgets::SceneUniforms ) ) ));
        
        frame.DescriptorSet = frame.DescriptorPool->BeginDescriptorSet( this->RenderDescriptorSetLayout.get() );
        frame.DescriptorPool->SetBuffer( 0, frame.SceneUBO.get() );
        frame.DescriptorPool->EndDescriptorSet();
        }
	}

DebugWidgets::~DebugWidgets()
	{
	}

void DebugWidgets::UpdateSceneData( const Camera& cam )
    {
    this->SceneData.View = cam.view;
    this->SceneData.Proj = cam.proj;
    this->SceneData.ViewI = cam.viewI;
    this->SceneData.ProjI = cam.projI;
    this->SceneData.ViewPosition = cam.cameraPosition;
    }

void DebugWidgets::UpdateSceneData( SceneUniforms _sceneData )
    {
    this->SceneData = _sceneData;
    }

void DebugWidgets::SetViewport( float x, float y, float width, float height, float minDepth, float maxDepth )
    {
    this->Viewport.x = x;
    this->Viewport.y = y;
    this->Viewport.width = width;
    this->Viewport.height = height;
    this->Viewport.minDepth = minDepth;
    this->Viewport.maxDepth = maxDepth;
    }

void DebugWidgets::SetScissorRectangle( int32_t x, int32_t y, uint32_t width, uint32_t height )
    {
    this->ScissorRectangle.offset.x = x;
    this->ScissorRectangle.offset.y = y;
    this->ScissorRectangle.extent.width = width;
    this->ScissorRectangle.extent.height = height;
    }

void DebugWidgets::BeginFrame( unsigned int frame_id )
    {
    this->CurrentFrame = frame_id;
    struct DebugWidgets::PerFrameData& frame = this->PerFrameData[this->CurrentFrame];

    void *pmem = frame.SceneUBO->MapMemory();
    memcpy( pmem, &this->SceneData, sizeof( SceneUniforms ) );
    frame.SceneUBO->UnmapMemory();
    }

void DebugWidgets::EndFrame()
    {
    }

void DebugWidgets::RenderWidget( WidgetTypes type, glm::mat4 transform )
    {
    RenderItem item;

    item.MeshID = (uint)type;
    item.Transform = transform;

    this->ItemQueue.emplace_back( item );
    }

void DebugWidgets::RenderSphere( glm::vec3 world_pos, float radius )
    {
    mat4 trans = scale(translate( mat4( 1 ), world_pos ), vec3(radius) );
    this->RenderWidget( DebugWidgets::UnitSphere , trans );
    }

// make Z axis in object space point at the target
glm::mat4 TargetTransform( vec3 origin, vec3 target, vec3 world_up )
    {
    glm::vec3 at = glm::normalize(target-origin);

    glm::vec3 right = glm::cross( world_up, at );
    if( glm::dot( right, right ) < 0.0001f )
        {
        right = glm::cross( glm::vec3( 1, 0, 0 ), at );
        }
    right = glm::normalize( right );
    glm::vec3 up = glm::normalize(glm::cross( at, right ));

    glm::mat4 transform( 1 );

    transform[0][0] = right.x;
    transform[0][1] = right.y;
    transform[0][2] = right.z;

    transform[1][0] = up.x;
    transform[1][1] = up.y;
    transform[1][2] = up.z;

    transform[2][0] = at.x;
    transform[2][1] = at.y;
    transform[2][2] = at.z;

    transform[3][0] = origin.x; 
    transform[3][1] = origin.y; 
    transform[3][2] = origin.z; 

    return transform;
    }


void DebugWidgets::RenderCone( glm::vec3 world_pos, glm::vec3 direction , float radiusA, float radiusB, float length )
    {
    mat4 trans = TargetTransform(world_pos, world_pos+direction, glm::vec3(0.0f, 1.0f, 0.0f));

    trans[0] *= radiusA;
    trans[1] *= radiusB;
    trans[2] *= length;

    this->RenderWidget( DebugWidgets::UnitCone , trans );
    }

void DebugWidgets::RenderConeWithAngle( glm::vec3 world_pos, glm::vec3 world_target, float angle_rads )
    {
    glm::vec3 direction = world_target - world_pos;
    float length = glm::length( direction );
    direction = glm::normalize( direction );
    float radius = tanf( angle_rads )*length;
    this->RenderCone( world_pos, direction, radius, radius, length );
    }

void DebugWidgets::RenderAABB( glm::vec3 minv, glm::vec3 maxv )
    {
    vec3 delta = maxv - minv;
    mat4 trans = scale(translate( mat4( 1 ), minv ), delta );
    this->RenderWidget( DebugWidgets::UnitCube , trans );
    }


// renders the draw commands into vulkan
void DebugWidgets::Draw( Vlk::CommandPool* pool )
    {
    pool->SetViewport( this->Viewport );
    pool->SetScissorRectangle( this->ScissorRectangle );
    pool->BindPipeline( this->RenderPipeline.get() );
    pool->BindDescriptorSet( this->RenderPipeline.get(), this->PerFrameData[this->CurrentFrame].DescriptorSet );
    pool->BindVertexBuffer( this->VertexBuffer.get() );
    pool->BindIndexBuffer( this->IndexBuffer.get() );

    // now draw each of the items in the queue
    PushConstants pc;
    for( size_t i = 0; i < this->ItemQueue.size(); ++i )
        {
        RenderItem &item = this->ItemQueue[i];
        MeshData &mesh = this->Meshes[item.MeshID];

        pc.Transform = item.Transform;
        pool->PushConstants( this->RenderPipeline.get(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( pc ), &pc );
        pool->DrawIndexed(
            mesh.IndexCount,
            1,
            mesh.IndexOffset,
            mesh.VertexOffset,
            0
        );
        }

    this->ItemQueue.clear();
    }
