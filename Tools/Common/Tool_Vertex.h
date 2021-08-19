#pragma once

#pragma warning( push )
#pragma warning( disable : 4201 )

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Tools
	{
	struct alignas( 16 ) Compressed16Vertex
		{
		glm::uint16_t Coords[3] = {}; //  16-bit encoded coords (0->1, scale to bounding box)
		glm::uint16_t _unnused = 0;
		glm::uint Normals = 0; // oct encoded normals
		glm::uint TexCoords = 0; // half2 encoded uvs
		};

	class Vertex
		{
		public:
			glm::vec3 Coords;
			glm::vec3 Normals;
			glm::vec2 TexCoords;

			bool operator==( const Vertex& other ) const
				{
				return ( Coords == other.Coords ) && ( Normals == other.Normals ) && ( TexCoords == other.TexCoords );
				}

			// create a compressed vertex. the inverted scale and translate transforms will be applied to the 3d coordinate before storing
			Compressed16Vertex Compress( glm::vec3& scale, glm::vec3& translate );
		};

	};

namespace std
	{
	template<> struct hash<Tools::Vertex>
		{
		size_t operator()( Tools::Vertex const& vertex ) const
			{
			return ( ( hash<glm::vec3>()( vertex.Coords ) ^ ( hash<glm::vec3>()( vertex.Normals ) << 1 ) ) >> 1 ) ^ ( hash<glm::vec2>()( vertex.TexCoords ) << 1 );
			}
		};
	}

#pragma warning( pop )