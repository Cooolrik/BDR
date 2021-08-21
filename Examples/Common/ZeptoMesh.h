#pragma once

#include <vector>
#include <Vertex.h>
#include <Tool_Multimesh.h>

namespace Vlk
	{
	class Renderer;
	class VertexBuffer;
	class IndexBuffer;
	};

typedef Tools::Multimesh ZeptoMesh;
typedef Tools::Multimesh::Submesh ZeptoSubMesh;

class ZeptoMeshAllocator
	{
	private:
		Vlk::VertexBuffer* vertexBuffer{};
		Vlk::IndexBuffer* indexBuffer{};

		// the currently loaded meshes in the allocator
		std::vector<std::unique_ptr<ZeptoMesh>> Meshes;

	public:
		Vlk::VertexBuffer* GetVertexBuffer() const { return vertexBuffer; }
		Vlk::IndexBuffer* GetIndexBuffer() const { return indexBuffer; }

		// load in all meshes in one go from mmbin files
		// TODO: make the allocator dynamic. right now only static allocation on setup
		bool LoadMeshes( Vlk::Renderer* renderer, std::vector<const char*> paths );

		// get the number of ZeptoMeshes in the current allocation
		unsigned int GetMeshCount() const { return (unsigned int)Meshes.size(); }

		// get a ZeptoMesh from the allocator, to retreive data on the mesh and submeshes
		// note that the vertices and indices lists are empty, as the data is uploaded to GPU buffers instead.
		const ZeptoMesh* GetMesh( unsigned int index ) const { return this->Meshes[index].get(); }

		// clear the allocator, release all memory
		void Clear();

		~ZeptoMeshAllocator();
	};