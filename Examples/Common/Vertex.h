#pragma once

#pragma warning( push )
#pragma warning( disable : 4201 )

#include "Vlk_VertexBuffer.h"

#include <glm/glm.hpp>

class Vertex
	{
	public:
		glm::uint16_t Coords[3] = {}; //  short encoded coords
		glm::uint16_t _buffer;
		glm::uint Normals = 0; // oct encoded normals
		glm::uint TexCoords = 0; // half2 encoded uvs

		static Vlk::VertexBufferDescription GetVertexBufferDescription()
			{
			Vlk::VertexBufferDescription desc;
			desc.SetVertexInputBindingDescription( 0, sizeof( Vertex ), VK_VERTEX_INPUT_RATE_VERTEX );
			desc.AddVertexInputAttributeDescription( 0, 0, VK_FORMAT_R16G16B16A16_UINT, offsetof( Vertex, Coords ) );
			desc.AddVertexInputAttributeDescription( 0, 1, VK_FORMAT_R16G16_SNORM, offsetof( Vertex, Normals ) );
			desc.AddVertexInputAttributeDescription( 0, 2, VK_FORMAT_R16G16_SFLOAT, offsetof( Vertex, TexCoords ) );
			return desc;
			}
	};

#pragma warning( pop )