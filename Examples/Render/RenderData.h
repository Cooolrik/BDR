#pragma once

#include <Common.h>

#include "Vlk_Renderer.h"
#include "Vlk_ShaderModule.h"
#include "Vlk_GraphicsPipeline.h"
#include "Vlk_ComputePipeline.h"
#include "Vlk_CommandPool.h"
#include "Vlk_VertexBuffer.h"
#include "Vlk_IndexBuffer.h"
#include "Vlk_DescriptorLayout.h"
#include "Vlk_DescriptorPool.h"
#include "Vlk_UniformBuffer.h"
#include "Vlk_Sampler.h"

#include <Camera.h>
#include <MegaMesh.h>
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
	glm::uint _b1[2];
	};

struct BatchData
	{
	VkDrawIndexedIndirectCommand drawCmd; 
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
	bool pyramidCull; // is set, use depth pyramid to cull
	float Proj00; // projection[0][0]
	float Proj11; // projection[1][1]
	float pyramidWidth; // width of the largest mip in the depth pyramid
	float pyramidHeight; // height of the largest mip in the depth pyramid
	uint32_t objectCount; // number of objects in scene to consider
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

struct DepthReducePushConstants
	{
	glm::vec2 destDimensions;
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

		Vlk::UniformBuffer* uniformBuffer = nullptr; // render uniform buffer 
		Vlk::UniformBuffer* cullingUBO = nullptr; // culling uniform buffer 
		
		VkDescriptorSet renderDescriptorSet = nullptr; // render descriptor set
		VkDescriptorSet cullingDescriptorSet = nullptr; // culling shader descriptor set
		std::vector<VkDescriptorSet> depthReduceDescriptorSets; // depth reduce shader for depth pyramid descriptor set 

		Vlk::Buffer* initialDrawBuffer = nullptr; // buffer with all initial but empty batches 
		Vlk::Buffer* renderObjectsBuffer = nullptr; // list of all objects to consider for culling, along with their batches
		Vlk::Buffer* filteredDrawBuffer = nullptr; // buffer with all batches filled in with non-culled instances 
		Vlk::Buffer* instanceToObjectBuffer = nullptr; // mapping from instance to objectID, created in the culling

		~PerFrameData()
			{
			delete commandPool;
			delete uniformBuffer;
			delete cullingUBO;
			delete descriptorPool;

			for(auto p : depthPyramidImageMipViews)
				{
				vkDestroyImageView( renderer->GetDevice(), p, nullptr );
				}
			delete depthPyramidImage;
			
			delete renderObjectsBuffer;
			delete instanceToObjectBuffer;
			delete initialDrawBuffer;
			delete filteredDrawBuffer;
			}
	};

class RenderData
	{
	public:
		GLFWwindow* window = nullptr;
		VkSurfaceKHR surface = nullptr;

		Vlk::Renderer* renderer{};
		

		Vlk::GraphicsPipeline* renderPipeline = nullptr;
		Vlk::DescriptorLayout* renderDescriptorLayout = nullptr;

		Vlk::ComputePipeline* cullingPipeline = nullptr;
		Vlk::DescriptorLayout* cullingDescriptorLayout = nullptr;

		Vlk::ComputePipeline* depthReducePipeline = nullptr;
		Vlk::DescriptorLayout* depthReduceDescriptorLayout = nullptr;

		Vlk::Sampler* depthSampler = nullptr;

		Vlk::Buffer* objectsBuffer{}; // all objects information
		std::vector<PerFrameData> renderFrames{};

		PerFrameData* currentFrame = nullptr;
		PerFrameData* previousFrame = nullptr;

		// top most image w&h and number of mips
		uint DepthPyramidImageW;
		uint DepthPyramidImageH;
		uint DepthPyramidMipMapLevels;

		std::vector<MegaMesh> MegaMeshes;
		MegaMeshAllocator* MegaMeshAlloc{};
		std::vector<std::unique_ptr<Texture>> Textures;
		Vlk::Sampler *TexturesSampler = nullptr;

		Vlk::ShaderModule* vertexRenderShader = nullptr;
		Vlk::ShaderModule* fragmentRenderShader = nullptr;
		Vlk::ShaderModule* cullingShader = nullptr;
		Vlk::ShaderModule* depthReduceShader = nullptr;

		// scene data
		std::vector<ObjectData> objects;
		std::vector<BatchData> batches;
		std::vector<uint32_t> renderObjects;

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

			// done with the renderer, remove stuff
			//for (auto i : this->Textures)
			//	{
			//	delete i;
			//	}
			this->Textures.clear();
			
			delete renderDescriptorLayout;
			delete cullingDescriptorLayout;
			delete depthReduceDescriptorLayout;

			delete MegaMeshAlloc;

			delete objectsBuffer;

			clearPerFrameData();

			delete renderPipeline;
			delete cullingPipeline;
			delete depthReducePipeline;

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

