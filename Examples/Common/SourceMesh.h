#pragma once

#pragma warning( push )
#pragma warning( disable : 26812 4100 4201 4127 4189 )

#include <vector>

typedef unsigned int uint;

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

class SourceVertex
	{
	public:
		glm::vec3 coord;
		glm::vec3 normal;
		glm::vec2 texCoord;

		bool operator==( const SourceVertex& other ) const
			{
			return (coord == other.coord) && (normal == other.normal) && (texCoord == other.texCoord);
			}
	};

namespace std
	{
	template<> struct hash<SourceVertex>
		{
		size_t operator()( SourceVertex const& vertex ) const
			{
			return ( (hash<glm::vec3>()(vertex.coord) ^ 
					 (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				     (hash<glm::vec2>()(vertex.texCoord) << 1 );
			}
		};
	}

class SourceMesh
	{
	public:
		std::vector<SourceVertex> Vertices;
		std::vector<uint> Indices;

		void loadModel( const char* path );
	};

#pragma warning( pop )