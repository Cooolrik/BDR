#pragma once

#include <vector>
#include "Tool_Vertex.h"

namespace Tools
	{
	class Multimesh
		{
		public:
			class Submesh;

			std::vector<Submesh> SubMeshes;

			// bounding volumes for the full mesh
			glm::vec3 boundingSphere = {};
			float boundingSphereRadius = 0.f;
			glm::vec3 AABB[2] = {};
			
			// apply this transform to all compressed vertices
			glm::vec3 CompressedVertexScale;
			glm::vec3 CompressedVertexTranslate;

			// calculate bounding volumes and rejection cones for all 
			// submeshes as well as the full multimesh
			void calcBoundingVolumesAndRejectionCones();

			// build from .obj file
			void build( const char* path, unsigned int max_tris, unsigned int max_verts );

			// load/save binary
			bool load( const char* path );
			bool save( const char* path );
			void serialize( std::fstream& fs , bool reading = true );
		};

	class Multimesh::Submesh
		{
		public:
			std::vector<Vertex> vertices = {};
			std::vector<Compressed16Vertex> compressed_vertices = {}; 
			std::vector<uint16_t> indices = {};

			glm::vec3 boundingSphere = {};
			float boundingSphereRadius = 0.f;
			glm::vec3 AABB[2] = {};

			glm::vec3 rejectionConeCenter = {};
			glm::vec3 rejectionConeDirection = {};
			float rejectionConeCutoff = 0.f;

			unsigned int LODTriangleCounts[4] = {}; // number of triangles to use for the lod
			unsigned int LODQuantizeBits[4] = {}; // number of lower bits to remove for the lod (0: full res)

			void calcBoundingVolumesAndRejectionCone();
		};

	};
