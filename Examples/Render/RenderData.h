#pragma once

#include <Common.h>

#include "Vlk_Renderer.h"
#include "Vlk_ShaderModule.h"
#include "Vlk_GraphicsPipeline.h"
#include "Vlk_ComputePipeline.h"
#include "Vlk_CommandPool.h"
#include "Vlk_VertexBuffer.h"
#include "Vlk_IndexBuffer.h"
#include "Vlk_DescriptorSetLayout.h"
#include "Vlk_DescriptorPool.h"
#include "Vlk_Buffer.h"
#include "Vlk_Sampler.h"


#include <ImGuiWidgets.h>

#include <Camera.h>
#include <ZeptoMesh.h>
#include <Texture.h>

// object - an actual object in the scene, the ID is global as long as the object exists
// instance - in the current frame, one instance of a mesh, which renders a specific object. the instance Id changes from frame to frame based on culling.
// batch - all instances of a certain mesh and material combo
// mesh - the source mesh. many objects can use the same mesh

struct ObjectData
	{
	glm::mat4 transform{};
	glm::mat4 transformIT{}; // inverse transpose model matrix
	glm::vec4 boundingSphere{}; // world-coords sphere bounds of object
	glm::vec4 rejectionConeDirectionAndCutoff{};
	glm::vec3 rejectionConeCenter{};
	glm::uint meshID{};
	glm::uint materialID{};
	glm::uint batchID{};
	glm::uint vertexCutoffIndex = 0; // only quantize vertices after this index, the ones before are locked, to avoid gaps.
	glm::uint LODQuantizations[4];
	float bSphereRadiusCompressedScale; // bsphere radius in compressed vertex scale ( = radius / zmesh.CompressedVertexScale ) 
	};

struct BatchData
	{
	VkDrawIndexedIndirectCommand drawCmd; 
	};

struct InstanceData
	{
	uint objectID; // object to render
	uint quantizationMask;
	uint quantizationRound;
	};

struct MeshData
	{
	glm::vec3 CompressedVertexScale = {}; // object space min value for vertices
	glm::uint _b1; 
	glm::vec3 CompressedVertexTranslate = {}; // value to multiply vertices to get object space
	glm::uint _b2;
	};

struct CullingSettingsUBO
	{
	glm::mat4 viewTransform; // the view transform
	glm::vec3 viewPosition; // the position of the view
	float frustumXx; // frustum culling data X.x
	float frustumXz; // frustum culling data X.z
	float frustumYy; // frustum culling data Y.y
	float frustumYz; // frustum culling data Y.z
	float nearZ; // near depth
	float farZ; // far depth

	// quantization and zbuffer culling info
	float screenHeightOverTanFovY;
	bool pyramidCull; // is set, use depth pyramid to cull

	uint32_t objectCount; // number of objects in scene to consider

	// debug values, only for debugging purposes
	float debug_float_value1; 
	float debug_float_value2; 
	float debug_float_value3; 
	};

class UniformBufferObject
	{
	public:
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewI;
		glm::mat4 projI;
		glm::vec3 viewPosition;
	};

using glm::vec3;
using glm::vec4;


struct DebugData
	{	
	vec3 center;
	float rejectiondot;
	float object_rejection_cutoff;
	uint quantization_level;
	int isvisible;

	float temp[10];
	};

struct DepthReducePushConstants
	{
	glm::vec2 destDimensions;
	};

struct CompactingPushConstants
	{
	uint commandsCount;
	};

inline glm::vec4 normalizePlane(glm::vec4 p)
	{
	return p / glm::length(glm::vec3(p));
	}

class RenderData;

// allocated on a per-frame basis
class PerFrameData
	{
	public:
		Vlk::Renderer* renderer = nullptr; // copy of renderer to be able to access during cleanup
		uint indexInSwapChain;

		VkFramebuffer framebuffer = nullptr;
		VkImage swapChainImage = nullptr;

		VkImage colorImage = nullptr;
		VkImageView colorImageView = nullptr;
		VkImage depthImage = nullptr;
		VkImageView depthImageView = nullptr;

		Vlk::Image* depthPyramidImage = nullptr;
		std::vector<VkImageView> depthPyramidImageMipViews;

		Vlk::CommandPool* commandPool = nullptr;
		Vlk::DescriptorPool* descriptorPool = nullptr;

		Vlk::Buffer* uniformBuffer = nullptr; // render uniform buffer 
		Vlk::Buffer* cullingUBO = nullptr; // culling uniform buffer 
		
		VkDescriptorSet renderDescriptorSet = nullptr; // render descriptor set
		VkDescriptorSet cullingDescriptorSet = nullptr; // culling shader descriptor set
		VkDescriptorSet compactingDescriptorSet = nullptr; // culling shader descriptor set
		std::vector<VkDescriptorSet> depthReduceDescriptorSets; // depth reduce shader for depth pyramid descriptor set 

		Vlk::Buffer* initialDrawBuffer = nullptr; // buffer with all initial but empty batches 
		Vlk::Buffer* renderObjectsBuffer = nullptr; // list of all objects to consider for culling, along with their batches
		Vlk::Buffer* filteredDrawBuffer = nullptr; // buffer with all batches filled in with non-culled instances 
		Vlk::Buffer* compactedDrawBuffer = nullptr; // buffer with all batches filled in with non-culled instances 
		Vlk::Buffer* instanceToObjectBuffer = nullptr; // mapping from instance to objectID, created in the culling

		Vlk::Buffer *debugOutputBuffer = nullptr;

		~PerFrameData()
			{
			delete commandPool;
			delete uniformBuffer;
			delete cullingUBO;
			delete descriptorPool;
			delete debugOutputBuffer;

			for(auto p : depthPyramidImageMipViews)
				{
				vkDestroyImageView( renderer->GetDevice(), p, nullptr );
				}
			delete depthPyramidImage;
			
			delete renderObjectsBuffer;
			delete instanceToObjectBuffer;
			delete initialDrawBuffer;
			delete filteredDrawBuffer;
			delete compactedDrawBuffer;
			}
	};

class RenderData
	{
	public:
		GLFWwindow* window = nullptr;
		VkSurfaceKHR surface = nullptr;
		ImGuiWidgets* guiwidgets = nullptr;

		Vlk::Renderer* renderer{};
		
		Vlk::Pipeline* renderPipeline = nullptr;
		Vlk::DescriptorSetLayout* renderDescriptorLayout = nullptr;

		Vlk::Pipeline* cullingPipeline = nullptr;
		Vlk::DescriptorSetLayout* cullingDescriptorLayout = nullptr;

		Vlk::Pipeline* compactingPipeline = nullptr;
		Vlk::DescriptorSetLayout* compactingDescriptorLayout = nullptr;

		Vlk::Pipeline* depthReducePipeline = nullptr;
		Vlk::DescriptorSetLayout* depthReduceDescriptorLayout = nullptr;

		Vlk::Sampler* depthSampler = nullptr;

		Vlk::Buffer* objectsBuffer = nullptr; // all objects information
		Vlk::Buffer* meshesBuffer = nullptr; // all meshes information

		std::vector<PerFrameData> renderFrames{};

		PerFrameData* currentFrame = nullptr;
		PerFrameData* previousFrame = nullptr;

		// top most image w&h and number of mips
		uint DepthPyramidImageW;
		uint DepthPyramidImageH;
		uint DepthPyramidMipMapLevels;

		ZeptoMeshAllocator* MeshAlloc{};
		std::vector<std::unique_ptr<Texture>> Textures;
		Vlk::Sampler *TexturesSampler = nullptr;

		Vlk::ShaderModule* vertexRenderShader = nullptr;
		Vlk::ShaderModule* fragmentRenderShader = nullptr;
		Vlk::ShaderModule* cullingShader = nullptr;
		Vlk::ShaderModule* depthReduceShader = nullptr;
		Vlk::ShaderModule* compactingShader = nullptr;
		
		// scene data
		std::vector<ObjectData> objects;
		std::vector<BatchData> batches;
		std::vector<uint32_t> renderObjects;
		std::vector<MeshData> meshes;

		Camera camera;

		uint64_t frame_tris = 0;
		uint64_t frame_verts = 0;
		uint64_t frame_insts = 0;
		uint64_t frame_dcs = 0;

		uint64_t total_tris = 0;
		uint64_t total_verts = 0;
		uint64_t total_insts = 0;
		uint64_t total_dcs = 0;
		uint64_t total_frames = 0;

		uint64_t scene_tris = 0;
		uint64_t scene_verts = 0;
		uint	 scene_objects = 0;
		uint	 scene_batches = 0;

		void clearPerFrameData()
			{
			this->renderFrames.clear();

			previousFrame = nullptr;
			currentFrame = nullptr;
			}

		void cleanup()
			{
			if (this->depthSampler != nullptr)
				{
				delete this->depthSampler;
				}
			if(this->TexturesSampler)
				{
				delete this->TexturesSampler;
				}

			this->Textures.clear();
			
			delete renderDescriptorLayout;
			delete cullingDescriptorLayout;
			delete depthReduceDescriptorLayout;
			delete compactingDescriptorLayout;

			delete MeshAlloc;

			delete objectsBuffer;
			delete meshesBuffer;

			clearPerFrameData();

			delete renderPipeline;
			delete cullingPipeline;
			delete compactingPipeline;
			delete depthReducePipeline;

			if( guiwidgets )
				{
				delete guiwidgets;
				}

			delete compactingShader;
			delete vertexRenderShader;
			delete fragmentRenderShader;
			delete renderer;

			glfwDestroyWindow( window );
			glfwTerminate();
			}

		~RenderData()
			{
			this->cleanup();
			}
	};

