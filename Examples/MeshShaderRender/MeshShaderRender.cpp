
#include "MeshShaderRender.h"

void MeshShaderRender::SetupScene()
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
	//this->RenderTaskShader = u_ptr(Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_TASK_BIT_NV, "shaders/ZeptoRender.task.spv" ));
	this->RenderMeshShader = u_ptr(Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_MESH_BIT_NV, "shaders/ZeptoRender.mesh.spv" ));
	this->RenderFragShader = u_ptr(Vlk::ShaderModule::CreateFromFile( VK_SHADER_STAGE_FRAGMENT_BIT, "shaders/ZeptoRender.frag.spv" ));

	// setup the render pipeline
	//Vlk::DescriptorSetLayoutTemplate rdlt;
	//rdlt.AddUniformBufferBinding( VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT );	// 0 - buffer object
	//rdlt.AddSamplerBinding( VK_SHADER_STAGE_FRAGMENT_BIT , 16 );								// 1 - color texture, 16 textures
	//this->RenderPipelineDescriptorSetLayout = u_ptr(this->Renderer->CreateDescriptorSetLayout( rdlt ));

	unique_ptr<Vlk::GraphicsPipelineTemplate> gpt = u_ptr(new Vlk::GraphicsPipelineTemplate());
	gpt->SetVertexDataTemplateFromVertexBuffer( this->MeshAlloc->GetVertexBuffer() );
	gpt->AddShaderModule( this->RenderMeshShader.get() );
	gpt->AddShaderModule( this->RenderFragShader.get() );
	//gpt->AddDescriptorSetLayout( this->RenderPipelineDescriptorSetLayout.get() );
	gpt->AddPushConstantRange( VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( ObjectRender ) );
	this->RenderPipeline = u_ptr( this->Renderer->CreateGraphicsPipeline( *gpt ) );

	// create a standard linear shader for texture mapping
	this->LinearSampler = u_ptr(this->Renderer->CreateSampler( Vlk::SamplerTemplate::Linear() ));

	// add a pointer to the ui
	ui.mv = this;
	}

void MeshShaderRender::SetupPerFrameData()
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

void MeshShaderRender::UpdateScene()
	{
	if( this->app.UIWidgets )
		{
		this->app.UIWidgets->NewFrame();
		this->ui.Update();
		this->app.UIWidgets->EndFrameAndRender();
		}
	}

uint MeshShaderRender::CalcSubmeshQuantization( const ZeptoMesh &zmesh, uint submeshIndex )
	{
	const uint bias = 0;
	
	// internal
	const float _fovy = this->Camera.fovY / 2.f * 3.14159f / 180.0f;
	const float _screenHeight = (float)this->Camera.ScreenH;

	// constants that dont change over the frame
	const ZeptoSubMesh &submesh = zmesh.SubMeshes[submeshIndex];
	float radius = submesh.BoundingSphereRadius;
	float bSphereRadiusCompressedScale = radius / zmesh.CompressedVertexScale; // could be stored in submesh

	const glm::vec3 &cameraPosition = this->Camera.cameraPosition;
	const float screenHeightOverTanFovY = _screenHeight / tanf( _fovy );

		
	float _distance = glm::distance( cameraPosition, submesh.BoundingSphere );
	float distanceSquared = _distance * _distance;
	float distanceSquared_minus_radiusSquared = distanceSquared - (radius * radius);
	if( distanceSquared_minus_radiusSquared < 0 )
		return bias;

	float projected_radius = screenHeightOverTanFovY * radius / sqrtf(distanceSquared_minus_radiusSquared);

	float zbuffer_level = truncf(log2( projected_radius ));

	float poss_quant = bSphereRadiusCompressedScale / projected_radius;

	poss_quant = truncf(log2( poss_quant ));

	if( poss_quant < 0 )
		poss_quant = 0.f;
	if( poss_quant > 16.f )
		poss_quant = 16.f;
	return (uint)poss_quant + bias;
	}

uint MeshShaderRender::GetLODOfQuantization( const ZeptoMesh &zmesh, uint submeshIndex, uint quantization )
	{
	const ZeptoSubMesh &submesh = zmesh.SubMeshes[submeshIndex];

	// calc the lod of the submesh
	uint lod = 0;
	while( lod < 3 )
		{
		if( submesh.LODQuantizeBits[lod + 1] > quantization )
			break; // found it
		++lod;
		}

	return lod;
	}

VkCommandBuffer MeshShaderRender::DrawScene()
	{
	::PerFrameData& currentFrame = this->PerFrameData[this->app.CurrentFrameIndex];
	Vlk::DescriptorPool* renderDescriptorPool = currentFrame.RenderDescriptorPool.get();

	const ZeptoMesh& zmesh = *(this->MeshAlloc->GetMesh(0));

	// update the descriptor
	//renderDescriptorPool->ResetDescriptorPool();
	//currentFrame.RenderDescriptorSet = renderDescriptorPool->BeginDescriptorSet( this->RenderPipelineDescriptorSetLayout.get() );
	//renderDescriptorPool->SetBuffer( 0, currentFrame.SceneUBO.get() );
	//for( uint i=0; i<16; ++i )
	//	{
	//	Texture* ptex = this->Textures[i % this->Textures.size()].get();
	//	renderDescriptorPool->SetImageInArray( 1, i, ptex->GetImage()->GetImageView(), this->LinearSampler->GetSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
	//	}
	//renderDescriptorPool->EndDescriptorSet();

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
	pool->SetViewport( 0, 0, (float)this->Camera.ScreenW, (float)this->Camera.ScreenH );
	pool->SetScissorRectangle( 0, 0, this->Camera.ScreenW, this->Camera.ScreenH );
	pool->BindPipeline( this->RenderPipeline.get() );
	//pool->BindDescriptorSet( this->RenderPipeline.get(), currentFrame.RenderDescriptorSet );

	pool->DrawMeshTasks( 1, 0 );

	// update and render each submesh
	//ObjectRender pc;
	//pc.CompressedVertexTranslate = zmesh.CompressedVertexTranslate;
	//pc.CompressedVertexScale = glm::vec3(zmesh.CompressedVertexScale);
	//for( size_t sm = 0; sm < zmesh.SubMeshes.size(); ++sm )
	//	{
	//	const ZeptoSubMesh& submesh = zmesh.SubMeshes[sm];
	//
	//	// calc quantization of the submesh
	//	//uint quantization = uint( this->Camera.debug_float_value1 );
	//	//quantization = ( quantization > 0 ) ? quantization : 0;
	//	//quantization = ( quantization < 16 ) ? quantization : 16;
	//	uint quantization = CalcSubmeshQuantization( zmesh, (uint)sm );
	//	uint lod = GetLODOfQuantization( zmesh, (uint)sm, quantization );
	//							
	//	pc.Color = glm::vec3(1); 
	//	if( this->ui.SelectSubmesh && this->ui.SelectedSubmeshIndex == sm )
	//		pc.Color = glm::vec3( 1, 0, 0 );
	//
	//	pc.materialID = 0;
	//	pc.vertexCutoffIndex = submesh.VertexOffset + submesh.LockedVertexCount;
	//	pc.quantizationMask = 0xffffffff << quantization;
	//	pc.quantizationRound = ( 0x1 << quantization ) >> 1;
	//
	//	pool->PushConstants( this->RenderPipeline.get(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( ObjectRender ), &pc );
	//	pool->DrawIndexed( submesh.LODIndexCounts[lod], 1, submesh.IndexOffset, submesh.VertexOffset, 0 );
	//	}
	
	// selection widgets
	if( this->ui.SelectSubmesh )
		{
		const ZeptoSubMesh& submesh = zmesh.SubMeshes[this->ui.SelectedSubmeshIndex];

		this->Camera.debug_selection_target = submesh.BoundingSphere;
		this->Camera.debug_selection_dist = submesh.BoundingSphereRadius * 5.f;

		glm::vec3 cone_color = glm::vec3( 1 );
		glm::vec3 dir = normalize(this->Camera.cameraPosition - submesh.RejectionConeCenter);
		float view_dot = glm::dot( dir, submesh.RejectionConeDirection );
		if( view_dot < submesh.RejectionConeCutoff )
			cone_color = glm::vec3( 0, 1, 0 ); // visible
		else
			cone_color = glm::vec3( 1, 0, 0 ); // rejected
		
		if( this->ui.RenderBoundingSphere )
			Widgets.RenderSphere( submesh.BoundingSphere, submesh.BoundingSphereRadius );
		if( this->ui.RenderRejectionCone )
			Widgets.RenderConeWithAngle( submesh.RejectionConeCenter, submesh.RejectionConeCenter + (submesh.RejectionConeDirection*10.f), acosf(submesh.RejectionConeCutoff), cone_color );
		if( this->ui.RenderAABB )
			Widgets.RenderAABB( submesh.AABB[0] , submesh.AABB[1] );

		}
	else	
		{
		this->Camera.debug_selection_target = zmesh.BoundingSphere;
		this->Camera.debug_selection_dist = zmesh.BoundingSphereRadius * 5.f;

		if( this->ui.RenderBoundingSphere )
			Widgets.RenderSphere( zmesh.BoundingSphere , zmesh.BoundingSphereRadius );
		if( this->ui.RenderAABB )
			Widgets.RenderAABB( zmesh.AABB[0] , zmesh.AABB[1] );
		}

	if( this->ui.RenderAxies )
		this->Widgets.RenderWidget( DebugWidgets::CoordinateAxies, glm::mat4( 1 ) );

	// draw debug stuff
	this->Widgets.SetViewport( 0, 0, (float)this->Camera.ScreenW, (float)this->Camera.ScreenH );
	this->Widgets.SetScissorRectangle( 0, 0, this->Camera.ScreenW, this->Camera.ScreenH );
	this->Widgets.Draw( pool );

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
