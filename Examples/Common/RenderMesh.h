#pragma once

#include <vector>
#include <Tool_Multimesh.h>

#include "Vlk_VertexBuffer.h"
#include <Tool_Vertex.h>
#include <glm/glm.hpp>

namespace Vlk
	{
	class Renderer;
	class VertexBuffer;
	class IndexBuffer;
	};

typedef Tools::Multimesh::Submesh RenderSubmesh;

class RenderMesh 
    {
	private:
		// the buffers of the render mesh
		Vlk::VertexBuffer* vertexBuffer{};
		Vlk::IndexBuffer* indexBuffer{};

		// the mesh data
		Tools::Multimesh MeshData;

	public:
		Vlk::VertexBuffer* GetVertexBuffer() const { return vertexBuffer; }
		Vlk::IndexBuffer* GetIndexBuffer() const { return indexBuffer; }

		// load in all meshes in one go from mmbin files
		bool LoadMesh( Vlk::Renderer* renderer, const char* path );

		// get a ZeptoMesh from the allocator, to retreive data on the mesh and submeshes
		// note that the vertices and indices lists are empty, as the data is uploaded to GPU buffers instead.
		const Tools::Multimesh& GetMeshData() const { return MeshData; }

		// clear all allocations
		void Clear();

		// remove allocation
		~RenderMesh();
    };

