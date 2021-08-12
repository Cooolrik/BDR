#pragma once

#include <vector>
#include "Vertex.h"

namespace Vlk
	{
	class Renderer;
	class VertexBuffer;
	class IndexBuffer;
	};

class SubMesh
	{
	private:
		friend class MegaMeshAllocator;

		unsigned int vertexOffset; // offset into vertex buffer where mesh starts
		unsigned int vertexCount; // number of vertices

		unsigned int indexOffset; // offset into index buffer where mesh starts
		unsigned int indexCount; // number of indices (3 per triangle)

		glm::vec3 boundingSphereCenter; 
		float boundingSphereRadius;

		glm::vec3 rejectionConeCenter;
		glm::vec3 rejectionConeDirection;
		float rejectionConeCutoff;

		unsigned int LODIndexCounts[4];
		float LODQuantizeDistances[4];

	public:
		unsigned int GetVertexOffset() const { return vertexOffset; }
		unsigned int GetVertexCount() const { return vertexCount; }

		unsigned int GetIndexOffset() const { return indexOffset; }
		unsigned int GetIndexCount() const { return indexCount; }

		glm::vec3 GetBoundingSphereCenter() const { return boundingSphereCenter; }
		float GetBoundingSphereRadius() const { return boundingSphereRadius; }

		glm::vec3 GetRejectionConeCenter() const { return rejectionConeCenter; }
		glm::vec3 GetRejectionConeDirection() const { return rejectionConeDirection; }
		float GetRejectionConeCutoff() const { return rejectionConeCutoff; }

		const unsigned int* GetLODIndexCounts() const { return LODIndexCounts; }
		const float* GetLODQuantizeDistances() const { return LODQuantizeDistances; }
	};

class MegaMesh
	{
	private:
		friend class MegaMeshAllocator;

		std::vector<SubMesh> SubMeshes;

	public:
		unsigned int GetSubMeshCount() const { return (unsigned int)this->SubMeshes.size(); }
		const SubMesh& GetSubMesh( unsigned int index ) const {	return this->SubMeshes[index]; }
	};

class MegaMeshAllocator
	{
	private:
		Vlk::VertexBuffer* vertexBuffer{};
		Vlk::IndexBuffer* indexBuffer{};

	public:
		Vlk::VertexBuffer* GetVertexBuffer() const { return vertexBuffer; }
		Vlk::IndexBuffer* GetIndexBuffer() const { return indexBuffer; }

		// load in all meshes in one go from mmbin files
		// TODO: make the allocator dynamic. right now only static allocation on setup
		std::vector<MegaMesh> LoadMeshes( Vlk::Renderer* renderer, std::vector<const char*> paths );

		void Clear();

		~MegaMeshAllocator();
	};