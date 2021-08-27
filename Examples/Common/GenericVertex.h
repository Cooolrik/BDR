#pragma once

#pragma warning( push )
#pragma warning( disable : 4201 )

#include "Vlk_VertexBuffer.h"
#include <Tool_Vertex.h>
#include <glm/glm.hpp>

class GenericVertex : public Tools::Vertex
	{
	public:
		static Vlk::VertexBufferDescription GetVertexBufferDescription()
			{
			Vlk::VertexBufferDescription desc;
			desc.SetVertexInputBindingDescription( 0, sizeof( GenericVertex ), VK_VERTEX_INPUT_RATE_VERTEX );
			desc.AddVertexInputAttributeDescription( 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof( GenericVertex, Coords ) );
			desc.AddVertexInputAttributeDescription( 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof( GenericVertex, Normal ) );
			desc.AddVertexInputAttributeDescription( 0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof( GenericVertex, TexCoords ) );
			return desc;
			}
	};

#pragma warning( pop )