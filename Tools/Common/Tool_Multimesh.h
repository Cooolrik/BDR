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

			// for the whole multimesh
			glm::vec3 boundingSphere = {};
			float boundingSphereRadius = 0.f;

			// calculate bounding spheres and rejection cones for all submeshes as well as the whole multimesh
			void calcBoundingSpheresAndRejectionCones();

			// build from .obj file
			void build( const char* path, unsigned int max_tris, unsigned int max_verts );

			// load/save binary
			bool load( const char* path );
			void save( const char* path );
		};

	class Multimesh::Submesh
		{
		public:
			std::vector<Vertex> vertices = {};
			std::vector<unsigned int> indices = {};

			glm::vec3 boundingSphere = {};
			float boundingSphereRadius = 0.f;

			glm::vec3 rejectionConeCenter = {};
			glm::vec3 rejectionConeDirection = {};
			float rejectionConeCutoff = 0.f;

			unsigned int LODTriangleCounts[4] = {};
			float LODQuantizeDistances[4] = {};

			void calcBoundingSphereAndRejectionCone();
		};

	};
