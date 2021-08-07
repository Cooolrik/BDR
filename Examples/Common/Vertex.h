#pragma once

#pragma warning( push )
#pragma warning( disable : 4201 )

#include "Vlk_VertexBuffer.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

class Vertex
	{
	public:
		glm::vec4 X_Y_Z_U = glm::vec4(0);
		glm::vec4 NX_NY_NZ_V = glm::vec4( 0 );

		static Vlk::VertexBufferDescription GetVertexBufferDescription()
			{
			Vlk::VertexBufferDescription desc;
			desc.SetVertexInputBindingDescription( 0, sizeof( Vertex ), VK_VERTEX_INPUT_RATE_VERTEX );
			desc.AddVertexInputAttributeDescription( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof( Vertex, X_Y_Z_U ) );
			desc.AddVertexInputAttributeDescription( 0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof( Vertex, NX_NY_NZ_V ) );
			return desc;
			}

		bool operator==( const Vertex& other ) const 
			{
			return ( X_Y_Z_U == other.X_Y_Z_U ) && ( NX_NY_NZ_V == other.NX_NY_NZ_V ) /*&& (texCoord == other.texCoord)*/;
			}
	};

namespace std 
	{
	template<> struct hash<Vertex> 
		{
		size_t operator()( Vertex const& vertex ) const 
			{
			return ( ( hash<glm::vec4>()( vertex.X_Y_Z_U ) ^
				( hash<glm::vec4>()( vertex.NX_NY_NZ_V ) << 1 ) ) >> 1 )
				/*^( hash<glm::vec2>()( vertex.texCoord ) << 1 )*/;
			}
		};
	}

#pragma warning( pop )