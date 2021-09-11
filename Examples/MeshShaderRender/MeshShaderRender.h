#pragma once

#include "Application.h"
#include "ZeptoMesh.h"
#include "RenderMesh.h"
#include "UI.h"

class SceneRender
	{
	public:
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewI;
		glm::mat4 projI;
		glm::vec3 viewPosition; // origin of the view in world space
		float _b1;
	};

class ObjectRender
	{
	public:
		glm::vec3 CompressedVertexScale;
		glm::uint quantizationMask;
		glm::vec3 CompressedVertexTranslate;
		glm::uint quantizationRound;
		glm::vec3 Color;
		glm::uint materialID;
		glm::uint vertexCutoffIndex;
	};

struct PerFrameData
	{
	// from renderer
	VkFramebuffer Framebuffer = nullptr;
	VkImage SwapChainImage = nullptr;
	const Vlk::Image* ColorTarget = nullptr;
	const Vlk::Image* DepthTarget = nullptr;

	// my per frame data
	unique_ptr<Vlk::CommandPool> CommandPool = nullptr;
	unique_ptr<Vlk::DescriptorPool> RenderDescriptorPool = nullptr;
	unique_ptr<Vlk::Buffer> SceneUBO = nullptr;
	VkDescriptorSet RenderDescriptorSet = nullptr;
	};

class MeshShaderRender
	{
	private:
		friend class UI;

		// from application class
		ApplicationBase& app;
		Vlk::Renderer* Renderer;
		Camera& Camera;
		DebugWidgets& Widgets;

		// application data
		vector<PerFrameData> PerFrameData;

		unique_ptr<Vlk::Pipeline> RenderPipeline = nullptr;
		unique_ptr<Vlk::DescriptorSetLayout> RenderPipelineDescriptorSetLayout = nullptr;

		vector<unique_ptr<Texture>> Textures = {}; // all textures
		unique_ptr<Vlk::Sampler> LinearSampler = nullptr; // standard bilinear sampler 

		unique_ptr<ZeptoMeshAllocator> MeshAlloc = nullptr;

		unique_ptr<Vlk::ShaderModule> RenderTaskShader = nullptr;
		unique_ptr<Vlk::ShaderModule> RenderMeshShader = nullptr;
		unique_ptr<Vlk::ShaderModule> RenderFragShader = nullptr;

		UI ui;

	public:
		MeshShaderRender( ApplicationBase& _app ) :
			app( _app ),
			Renderer( _app.Renderer ),
			Camera( _app.Camera ),
			Widgets( *_app.DebugWidgets )
			{}

		void SetupScene();
		void SetupPerFrameData();
		void UpdateScene();
		VkCommandBuffer DrawScene();

		uint CalcSubmeshQuantization( const ZeptoMesh &zmesh, uint submeshIndex );
		uint GetLODOfQuantization( const ZeptoMesh &zmesh, uint submeshIndex, uint quantization );
	};
