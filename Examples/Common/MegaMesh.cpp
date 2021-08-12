#include "MegaMesh.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <unordered_map>

#include "Vertex.h"

#include "Vlk_VertexBuffer.h"
#include "Vlk_IndexBuffer.h"

#include "../Tools/Common/Tool_Multimesh.h"

typedef unsigned int uint;
using std::vector;

std::vector<MegaMesh> MegaMeshAllocator::LoadMeshes( Vlk::Renderer* renderer, std::vector<const char*> paths )
	{
	std::vector<MegaMesh> ret;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	this->Clear();

	ret.resize( paths.size() );

	// load in all models, append into one large memory allocation
	for( size_t i = 0; i < paths.size(); ++i )
		{
		Tools::Multimesh source_mesh;

		// load in source mesh from cache, or build it if it is not cached yet
		std::string cache_file = std::string( paths[i] );
		if( !source_mesh.load( cache_file.c_str() ) )
			{
			throw std::runtime_error( std::string("Error, cannot load mmbin file: ") + std::string( paths[i] ) );
			}

		// set up the render mega meshes
		ret[i].SubMeshes.resize( source_mesh.SubMeshes.size() );
		for( size_t q = 0; q < source_mesh.SubMeshes.size(); ++q )
			{
			// set up the render mesh offsets
			ret[i].SubMeshes[q].vertexOffset = (uint)vertices.size();
			ret[i].SubMeshes[q].vertexCount = (uint)source_mesh.SubMeshes[q].vertices.size();
			ret[i].SubMeshes[q].indexOffset = (uint)indices.size();
			ret[i].SubMeshes[q].indexCount = (uint)source_mesh.SubMeshes[q].indices.size();
			ret[i].SubMeshes[q].boundingSphereCenter = source_mesh.SubMeshes[q].boundingSphere;
			ret[i].SubMeshes[q].boundingSphereRadius = source_mesh.SubMeshes[q].boundingSphereRadius;

			ret[i].SubMeshes[q].rejectionConeCenter = source_mesh.SubMeshes[q].rejectionConeCenter;
			ret[i].SubMeshes[q].rejectionConeDirection = source_mesh.SubMeshes[q].rejectionConeDirection;
			ret[i].SubMeshes[q].rejectionConeCutoff = source_mesh.SubMeshes[q].rejectionConeCutoff;

			for( uint lod = 0 ; lod < 4; ++lod )
				{
				ret[i].SubMeshes[q].LODIndexCounts[lod] = source_mesh.SubMeshes[q].LODTriangleCounts[lod] * 3;
				ret[i].SubMeshes[q].LODQuantizeDistances[lod] = source_mesh.SubMeshes[q].LODQuantizeDistances[lod];
				}

			// squash vertex data into render vertex struct
			vector<Vertex> renderVertices( source_mesh.SubMeshes[q].vertices.size() );
			for( size_t v = 0 ; v < source_mesh.SubMeshes[q].vertices.size() ; ++v )
				{
				glm::vec3& coords = source_mesh.SubMeshes[q].vertices[v].Coords;
				glm::vec3& normals = source_mesh.SubMeshes[q].vertices[v].Normals;
				glm::vec2& texCoords = source_mesh.SubMeshes[q].vertices[v].TexCoords;
				
				renderVertices[v].X_Y_Z_U = glm::vec4( coords.x, coords.y, coords.z, texCoords.x );
				renderVertices[v].NX_NY_NZ_V = glm::vec4( normals.x, normals.y, normals.z, texCoords.y );
				}

			// append to buffers
			vertices.insert( vertices.end(), renderVertices.begin(), renderVertices.end() );
			indices.insert( indices.end(), source_mesh.SubMeshes[q].indices.begin(), source_mesh.SubMeshes[q].indices.end() );
			}
		}

	// setup gpu buffers
	vertexBuffer = renderer->CreateVertexBuffer( 
		Vlk::VertexBufferTemplate::VertexBuffer(
			Vertex::GetVertexBufferDescription(), 
			(uint)vertices.size(), 
			vertices.data() 
			)
		);
	indexBuffer = renderer->CreateIndexBuffer( 
		Vlk::IndexBufferTemplate::IndexBuffer(
			VK_INDEX_TYPE_UINT32, 
			(uint)indices.size(), 
			indices.data() 
			)
		);
		
	return ret;
	}

MegaMeshAllocator::~MegaMeshAllocator()
	{
	this->Clear();
	}

void MegaMeshAllocator::Clear()
	{
	if( vertexBuffer ) { delete vertexBuffer; vertexBuffer = nullptr; }
	if( indexBuffer ) { delete indexBuffer; indexBuffer = nullptr; }
	}
