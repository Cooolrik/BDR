#include "ZeptoMesh.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <unordered_map>

#include "Vlk_VertexBuffer.h"
#include "Vlk_IndexBuffer.h"

#include "../Tools/Common/Tool_Multimesh.h"

typedef unsigned int uint;
using std::vector;

bool ZeptoMeshAllocator::LoadMeshes( Vlk::Renderer* renderer, std::vector<const char*> paths )
	{
	std::vector<ZeptoVertex> vertices;
	std::vector<uint16_t> indices;

	// clear any current allocation
	this->Clear();

	// load in all models, append into one large memory allocation
	this->Meshes.resize( paths.size() );
	for( size_t i = 0; i < paths.size(); ++i )
		{
		this->Meshes[i] = std::unique_ptr<ZeptoMesh>( new ZeptoMesh );
		ZeptoMesh& mesh = *(this->Meshes[i].get());

		// load in source mesh from cache, or build it if it is not cached yet
		std::string cache_file = std::string( paths[i] );
		if( !mesh.Load( cache_file.c_str() ) )
			{
			throw std::runtime_error( std::string("Error, cannot load mmbin file: ") + std::string( paths[i] ) );
			return false;
			}

		// set up base indices for the zeptomesh
		size_t MeshVertexBase = vertices.size();
		size_t MeshVertexCount = mesh.CompressedVertices.size();
		size_t MeshIndexBase = indices.size();
		size_t MeshIndexCount = mesh.Indices.size();

		// copy into the arrays
		vertices.resize( MeshVertexBase + MeshVertexCount );
		memcpy( &( vertices.data()[MeshVertexBase] ), mesh.CompressedVertices.data(), sizeof( ZeptoVertex ) * MeshVertexCount );
		indices.resize( MeshIndexBase + MeshIndexCount );
		memcpy( &( indices.data()[MeshIndexBase] ), mesh.Indices.data(), sizeof( uint16_t ) * MeshIndexCount );

		// clear the arrays in the mesh, it's not needed anymore
		mesh.Vertices.clear();
		mesh.CompressedVertices.clear();
		mesh.Indices.clear();

		// update the vertex and index offsets in the submeshes, as the mesh now resides in a larger array
		for( size_t q = 0; q < mesh.SubMeshes.size(); ++q )
			{
			mesh.SubMeshes[q].VertexOffset += (uint)MeshVertexBase;
			mesh.SubMeshes[q].IndexOffset += (uint)MeshIndexBase;
			}
		}

	// setup gpu buffers
	vertexBuffer = renderer->CreateVertexBuffer( 
		Vlk::VertexBufferTemplate::VertexBuffer(
			ZeptoVertex::GetVertexBufferDescription(),
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
		
	return true;
	}

ZeptoMeshAllocator::~ZeptoMeshAllocator()
	{
	this->Clear();
	}

void ZeptoMeshAllocator::Clear()
	{
	if( vertexBuffer ) { delete vertexBuffer; vertexBuffer = nullptr; }
	if( indexBuffer ) { delete indexBuffer; indexBuffer = nullptr; }
	this->Meshes.clear();
	}
