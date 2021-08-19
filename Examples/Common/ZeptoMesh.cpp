#include "ZeptoMesh.h"

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

std::vector<ZeptoMesh> ZeptoMeshAllocator::LoadMeshes( Vlk::Renderer* renderer, std::vector<const char*> paths )
	{
	std::vector<ZeptoMesh> ret;
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

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

		ret[i].AABB[0] = source_mesh.AABB[0];
		ret[i].AABB[1] = source_mesh.AABB[1];

		ret[i].CompressedVertexScale = source_mesh.CompressedVertexScale;
		ret[i].CompressedVertexTranslate = source_mesh.CompressedVertexTranslate;

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
				ret[i].SubMeshes[q].LODQuantizeBits[lod] = source_mesh.SubMeshes[q].LODQuantizeBits[lod];
				}

			// squash vertex data into render vertex struct
			vector<Vertex> renderVertices( source_mesh.SubMeshes[q].compressed_vertices.size() );
			for( size_t v = 0 ; v < source_mesh.SubMeshes[q].compressed_vertices.size() ; ++v )
				{
				renderVertices[v].Coords[0] = source_mesh.SubMeshes[q].compressed_vertices[v].Coords[0];
				renderVertices[v].Coords[1] = source_mesh.SubMeshes[q].compressed_vertices[v].Coords[1];
				renderVertices[v].Coords[2] = source_mesh.SubMeshes[q].compressed_vertices[v].Coords[2];
				renderVertices[v]._buffer = 0;
				renderVertices[v].Normals = source_mesh.SubMeshes[q].compressed_vertices[v].Normals;
				renderVertices[v].TexCoords = source_mesh.SubMeshes[q].compressed_vertices[v].TexCoords;
				}
			
			vector<uint16_t> renderIndices( source_mesh.SubMeshes[q].indices.size() );
			for( size_t t = 0 ; t < source_mesh.SubMeshes[q].indices.size() ; ++t )
				{
				renderIndices[t] = (uint16_t)source_mesh.SubMeshes[q].indices[t];
				}

			// append to buffers
			vertices.insert( vertices.end(), renderVertices.begin(), renderVertices.end() );
			indices.insert( indices.end(), renderIndices.begin(), renderIndices.end() );
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
			VK_INDEX_TYPE_UINT16, 
			(uint)indices.size(), 
			indices.data() 
			)
		);
		
	return ret;
	}

ZeptoMeshAllocator::~ZeptoMeshAllocator()
	{
	this->Clear();
	}

void ZeptoMeshAllocator::Clear()
	{
	if( vertexBuffer ) { delete vertexBuffer; vertexBuffer = nullptr; }
	if( indexBuffer ) { delete indexBuffer; indexBuffer = nullptr; }
	}
