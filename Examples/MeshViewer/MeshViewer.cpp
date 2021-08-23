
#include "MeshViewer.h"

void MeshViewer::SetupScene()
	{
	// the meshes and textures to read
	std::vector<const char*> source_mesh_names =
		{
		"../Assets/meteor_0.mmbin"
		};
	std::vector<const char*> source_tex_names =
		{
		"../Assets/image1.dds",
		"../Assets/image2.dds",
		"../Assets/image3.dds",
		"../Assets/image4.dds",
		};

	// allocate, read in the meshes
	this->MeshAlloc = u_ptr( new ZeptoMeshAllocator );
	this->MeshAlloc->LoadMeshes( this->Renderer, source_mesh_names );

	// read in textures
	this->Textures.resize( source_tex_names.size() );
	for( size_t t = 0; t < source_tex_names.size(); ++t )
		{
		this->Textures[t] = u_ptr( new Texture() );
		this->Textures[t]->LoadDDS( this->Renderer , source_tex_names[t] );
		}

	// load the shaders
	vertexRenderShader = u_ptr(Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_VERTEX_BIT, "shaders/Render.vert.spv" ));
	fragmentRenderShader = u_ptr(Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_FRAGMENT_BIT, "shaders/Render.frag.spv" ));

	// setup the render pipeline
	Vlk::DescriptorSetLayoutTemplate rdlt;
	rdlt.AddUniformBufferBinding( VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT );	// 0 - buffer object
	rdlt.AddSamplerBinding( VK_SHADER_STAGE_FRAGMENT_BIT , 16 );								// 1 - color texture, 16 textures
	this->RenderPipelineDescriptorSetLayout = u_ptr(this->Renderer->CreateDescriptorSetLayout( rdlt ));

	this->RenderPipeline = u_ptr(this->Renderer->CreateGraphicsPipeline());
	this->RenderPipeline->SetVertexDataTemplateFromVertexBuffer( this->MeshAlloc->GetVertexBuffer() );
	this->RenderPipeline->AddShaderModule( vertexRenderShader.get() );
	this->RenderPipeline->AddShaderModule( fragmentRenderShader.get() );
	this->RenderPipeline->SetDescriptorSetLayout( this->RenderPipelineDescriptorSetLayout.get() );
	this->RenderPipeline->SetSinglePushConstantRange( sizeof( ObjectRender ), VK_SHADER_STAGE_VERTEX_BIT );
	this->RenderPipeline->BuildPipeline();

	// create a standard linear shader for texture mapping
	this->LinearSampler = u_ptr(this->Renderer->CreateSampler( Vlk::SamplerTemplate::Linear() ));

	// add a pointer to the ui
	ui.mv = this;
	}

void MeshViewer::SetupPerFrameData()
	{
	this->PerFrameData.clear();

	vector<VkFramebuffer> framebuffers = this->Renderer->GetFramebuffers();
	vector<VkImage> swapChainImages = this->Renderer->GetSwapChainImages();

	uint num_frames = (uint)framebuffers.size();

	const uint max_set_count = 20;
	const uint max_descriptor_count = 20;

	this->PerFrameData.resize( num_frames );

	for( uint f = 0; f < num_frames; ++f )
		{
		::PerFrameData& frame = this->PerFrameData[f];
		frame.Framebuffer = framebuffers[f];
		frame.SwapChainImage = swapChainImages[f];
		frame.ColorTarget = this->Renderer->GetColorTargetImage( f );
		frame.DepthTarget = this->Renderer->GetColorTargetImage( f );
			
		frame.CommandPool = u_ptr(this->Renderer->CreateCommandPool( 1 ));
		frame.RenderDescriptorPool = u_ptr(this->Renderer->CreateDescriptorPool( Vlk::DescriptorPoolTemplate::General( max_set_count, max_descriptor_count ) ));
		frame.SceneUBO = u_ptr(this->Renderer->CreateBuffer( Vlk::BufferTemplate::UniformBuffer( sizeof( SceneRender ) ) ));
		}

	}

void MeshViewer::UpdateScene()
	{
	if( this->app.UIWidgets )
		{
		this->app.UIWidgets->NewFrame();
		this->ui.Update();
		this->app.UIWidgets->EndFrameAndRender();
		}
	}

VkCommandBuffer MeshViewer::DrawScene()
	{
	::PerFrameData& currentFrame = this->PerFrameData[this->app.CurrentFrameIndex];
	Vlk::DescriptorPool* renderDescriptorPool = currentFrame.RenderDescriptorPool.get();

	// update the descriptor
	renderDescriptorPool->ResetDescriptorPool();
	currentFrame.RenderDescriptorSet = renderDescriptorPool->BeginDescriptorSet( this->RenderPipelineDescriptorSetLayout.get() );
	renderDescriptorPool->SetBuffer( 0, currentFrame.SceneUBO.get() );
	for( uint i=0; i<16; ++i )
		{
		Texture* ptex = this->Textures[i % this->Textures.size()].get();
		renderDescriptorPool->SetImageInArray( 1, i, ptex->GetImage()->GetImageView(), this->LinearSampler->GetSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		}
	renderDescriptorPool->EndDescriptorSet();

	// set scene data
	SceneRender scene = {};
	scene.view = this->Camera.view;
	scene.proj = this->Camera.proj;
	scene.viewI = this->Camera.viewI;
	scene.projI = this->Camera.projI;
	scene.viewPosition = this->Camera.cameraPosition;

	// begin command buffer
	Vlk::CommandPool* pool = currentFrame.CommandPool.get();
	pool->ResetCommandPool();
	VkCommandBuffer buffer = pool->BeginCommandBuffer();

	// update the ubo
	pool->UpdateBuffer( currentFrame.SceneUBO.get(), 0, sizeof( SceneRender ), &scene );

	// render the objects
	pool->BeginRenderPass( currentFrame.Framebuffer );
	pool->BindVertexBuffer( this->MeshAlloc->GetVertexBuffer() );
	pool->BindIndexBuffer( this->MeshAlloc->GetIndexBuffer() );
	pool->BindGraphicsPipeline( this->RenderPipeline.get() );
	pool->BindDescriptorSet( this->RenderPipeline.get(), currentFrame.RenderDescriptorSet );

	// update and render each submesh
	for( uint m = 0; m < this->MeshAlloc->GetMeshCount(); ++m )
		{
		if( this->MeshAlloc->GetMesh( m ) == nullptr )
			continue;

		const ZeptoMesh& zmesh = *(this->MeshAlloc->GetMesh(m));

		ObjectRender pc;

		pc.CompressedVertexTranslate = zmesh.CompressedVertexTranslate;
		pc.CompressedVertexScale = zmesh.CompressedVertexScale;

		for( size_t sm = 0; sm < zmesh.SubMeshes.size(); ++sm )
			{
			const ZeptoSubMesh& submesh = zmesh.SubMeshes[sm];

			// calc quantization of the submesh
			uint quantization = uint( this->Camera.debug_float_value1 );
			if( (int)quantization < 0 )
				quantization = 0;
			if( quantization > 16 )
				quantization = 16;

			// calc the lod of the submesh
			uint lod = 0;
			while( lod < 3 )
				{
				if( submesh.LODQuantizeBits[lod + 1] > quantization )
					break; // found it
				++lod;
				}

			// set the relevant info
			uint r = (uint)( sm / 256 );
			uint g = (uint)( sm / 16 ) % 16;
			uint b = (uint)sm % 16;

			pc.Color = glm::vec3( 1, 1, 1 ); // glm::vec3( float( r ) / 256.f, float( g ) / 256.f, float( b ) / 256.f );
			pc.materialID = 0;
			pc.vertexCutoffIndex = submesh.VertexOffset + submesh.LockedVertexCount;
			pc.quantizationMask = 0xffffffff << quantization;
			pc.quantizationRound = ( 0x1 << quantization ) >> 1;

			pool->PushConstants( this->RenderPipeline.get(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( ObjectRender ), &pc );
			pool->DrawIndexed( submesh.LODIndexCounts[lod], 1, submesh.IndexOffset, submesh.VertexOffset, 0 );
			}
		}

	// draw gui
	if( this->app.UIWidgets )
		{
		this->app.UIWidgets->Draw( buffer );
		}

	// done with the rendering
	pool->EndRenderPass();

	// prepare the color target and swap chain image for transfer
	pool->QueueUpImageMemoryBarrier( currentFrame.ColorTarget, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT );
	pool->QueueUpImageMemoryBarrier( currentFrame.SwapChainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_NONE_KHR, VK_ACCESS_TRANSFER_WRITE_BIT );
	pool->PipelineBarrier( VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT );

	// copy color target to swap image
	VkImageCopy copyRegion = {};
	copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.srcSubresource.layerCount = 1;
	copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.dstSubresource.layerCount = 1;
	copyRegion.extent = { this->Camera.ScreenW, this->Camera.ScreenH, 1 };
	vkCmdCopyImage( buffer, currentFrame.ColorTarget->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, currentFrame.SwapChainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion );

	// restore swap chain image to "present" type
	pool->QueueUpImageMemoryBarrier( currentFrame.SwapChainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE_KHR );
	pool->PipelineBarrier( VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );

	// done with the buffer, return it
	currentFrame.CommandPool->EndCommandBuffer();
	return buffer;
	}
