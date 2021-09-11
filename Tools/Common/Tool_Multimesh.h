#pragma once

#include <vector>
#include "Tool_Vertex.h"

namespace Tools
	{
	class Multimesh
		{
		public:
			class Submesh;
			class Meshlet;

			// full vertex and index list of the multimesh. 
			// note that not both Vertices and CompressedVertices
			// need to be filled in, as they are redundant
			std::vector<Vertex> Vertices = {};
			std::vector<Compressed16Vertex> CompressedVertices = {};
			std::vector<uint16_t> Indices = {};

			// submeshes of the mesh, with individual offsets in the vertices and indices lists
			std::vector<Submesh> SubMeshes;

			// bounding volumes for the full mesh
			glm::vec3 BoundingSphere = {};
			float BoundingSphereRadius = 0.f;
			glm::vec3 AABB[2] = {};
			
			// apply this transform to all compressed vertices
			float CompressedVertexScale;
			glm::vec3 CompressedVertexTranslate;

			// calculate bounding volumes and rejection cones for all 
			// submeshes as well as the full multimesh
			void CalcBoundingVolumesAndRejectionCones();
			void CalcSubmeshBoundingVolumesAndRejectionCone( unsigned int index );

			// load/save binary
			bool Load( const char* path );
			bool Save( const char* path );
			void Serialize( std::fstream& fs , bool reading = true );
		};

	class Multimesh::Submesh
		{
		public:
			unsigned int VertexOffset;
			unsigned int VertexCount;
			  
			unsigned int IndexOffset;
			unsigned int IndexCount;

			glm::vec3 BoundingSphere = {};
			float BoundingSphereRadius = 0.f;
			glm::vec3 AABB[2] = {};

			glm::vec3 RejectionConeCenter = {};
			glm::vec3 RejectionConeDirection = {};
			float RejectionConeCutoff = 0.f;

			unsigned int LockedVertexCount = 0; // number of vertices (in the beginning of each list) to not quantize 

			unsigned int LODIndexCounts[4] = {}; // number of triangles to use for the lod
			unsigned int LODQuantizeBits[4] = {}; // number of lower bits to remove for the lod (0: full res)
		};

	class Multimesh::Meshlet
		{
		public:




		};

	};
