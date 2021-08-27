#include "RenderMesh.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <unordered_map>

#include "Vlk_VertexBuffer.h"
#include "Vlk_IndexBuffer.h"

#include "GenericVertex.h"

#include "../Tools/Common/Tool_Multimesh.h"

typedef unsigned int uint;
using std::vector;

bool RenderMesh::LoadMesh( Vlk::Renderer* renderer, const char* path )
	{
	std::vector<GenericVertex> vertices;
	std::vector<uint16_t> indices;

	// load in model
	if( !this->MeshData.Load( path ) )
		{
		throw std::runtime_error( std::string( "Error, cannot load mmbin file: " ) + std::string( path ) );
		return false;
		}

	// set up base indices for the zeptomesh
	size_t MeshVertexCount = this->MeshData.Vertices.size();
	size_t MeshIndexCount = this->MeshData.Indices.size();

	// copy into the arrays
	vertices.resize( MeshVertexCount );
	memcpy( vertices.data(), this->MeshData.Vertices.data(), sizeof( GenericVertex ) * MeshVertexCount );
	indices.resize( MeshIndexCount );
	memcpy( indices.data(), this->MeshData.Indices.data(), sizeof( uint16_t ) * MeshIndexCount );

	// clear the arrays in the mesh, it's not needed anymore
	this->MeshData.Vertices.clear();
	this->MeshData.CompressedVertices.clear();
	this->MeshData.Indices.clear();

	// setup gpu buffers
	this->vertexBuffer = renderer->CreateVertexBuffer(
		Vlk::VertexBufferTemplate::VertexBuffer(
			GenericVertex::GetVertexBufferDescription(),
			(uint)vertices.size(),
			vertices.data()
		)
	);
	this->indexBuffer = renderer->CreateIndexBuffer(
		Vlk::IndexBufferTemplate::IndexBuffer(
			VK_INDEX_TYPE_UINT16,
			(uint)indices.size(),
			indices.data()
		)
	);

	return true;
	}

RenderMesh::~RenderMesh()
	{
	this->Clear();
	}

void RenderMesh::Clear()
	{
	if( vertexBuffer ) { delete vertexBuffer; vertexBuffer = nullptr; }
	if( indexBuffer ) { delete indexBuffer; indexBuffer = nullptr; }
	}
