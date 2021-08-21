#include "Tool_Multimesh.h"
#include "Tool_Serializer.h"

#include <vector>
#include <iostream>
#include <fstream>

typedef unsigned int uint;
using std::vector;

void Tools::Multimesh::CalcBoundingVolumesAndRejectionCones()
	{
	uint submesh_count = (uint)SubMeshes.size();

	// calc bounding volumes of submeshes and calc aabb for whole mesh
	glm::vec3 minv = glm::vec3( FLT_MAX );
	glm::vec3 maxv = glm::vec3( -FLT_MAX );
	for( uint i = 0; i < submesh_count; ++i )
		{
		this->CalcSubmeshBoundingVolumesAndRejectionCone( i );

		glm::vec3& sm_minv = this->SubMeshes[i].AABB[0];
		glm::vec3& sm_maxv = this->SubMeshes[i].AABB[1];

		minv.x = glm::min( minv.x, sm_minv.x );
		minv.y = glm::min( minv.y, sm_minv.y );
		minv.z = glm::min( minv.z, sm_minv.z );

		maxv.x = glm::max( maxv.x, sm_maxv.x );
		maxv.y = glm::max( maxv.y, sm_maxv.y );
		maxv.z = glm::max( maxv.z, sm_maxv.z );
		}
	this->AABB[0] = minv;
	this->AABB[1] = maxv;

	// calc full bsphere 
	// using weighted average of spheres as center
	this->BoundingSphere = glm::vec3( 0 );
	this->BoundingSphereRadius = 0;
	for( uint i = 0; i < submesh_count; ++i )
		{
		this->BoundingSphere += SubMeshes[i].BoundingSphere;
		}
	BoundingSphere /= (float)submesh_count;
	for( uint i = 0; i < submesh_count; ++i )
		{
		float r = glm::length( SubMeshes[i].BoundingSphere - this->BoundingSphere ) + SubMeshes[i].BoundingSphereRadius;
		if( r > this->BoundingSphereRadius )
			{
			this->BoundingSphereRadius = r;
			}
		}
	}

void Tools::Multimesh::CalcSubmeshBoundingVolumesAndRejectionCone( unsigned int index )
	{
	Submesh& mesh = this->SubMeshes[index];

	size_t BeginVertex = mesh.VertexOffset;
	size_t EndVertex = (size_t)mesh.VertexOffset + mesh.VertexCount;
	size_t BeginIndex = mesh.IndexOffset;
	size_t EndIndex = (size_t)mesh.IndexOffset + mesh.IndexCount;

	// get the aabb of all vertices
	glm::vec3 minv = glm::vec3( FLT_MAX );
	glm::vec3 maxv = glm::vec3( -FLT_MAX );
	for( size_t i = BeginVertex; i < EndVertex; ++i )
		{
		minv.x = glm::min( minv.x, Vertices[i].Coords.x );
		minv.y = glm::min( minv.y, Vertices[i].Coords.y );
		minv.z = glm::min( minv.z, Vertices[i].Coords.z );

		maxv.x = glm::max( maxv.x, Vertices[i].Coords.x );
		maxv.y = glm::max( maxv.y, Vertices[i].Coords.y );
		maxv.z = glm::max( maxv.z, Vertices[i].Coords.z );
		}
	mesh.AABB[0] = minv;
	mesh.AABB[1] = maxv;

	// place sphere in center of aabb (TODO:do better) and find max radius
	mesh.BoundingSphere = ( minv + maxv ) / 2.f;
	mesh.BoundingSphereRadius = 0;
	for( size_t i = BeginVertex; i < EndVertex; ++i )
		{
		float l = glm::length( BoundingSphere - Vertices[i].Coords );
		if( l > mesh.BoundingSphereRadius )
			{
			mesh.BoundingSphereRadius = l;
			}
		}

	// calc test rejection cone
	glm::vec3 dir( 0 );
	glm::vec3 center( mesh.BoundingSphere );

	// get the average negative normal of all triangles
	for( size_t t = BeginIndex; t < EndIndex; t += 3 )
		{
		glm::vec3 coords[3];
		for( size_t c = 0; c < 3; ++c )
			{
			coords[c] = Vertices[BeginVertex + Indices[t + c]].Coords;
			}
		glm::vec3 tri_normal = glm::normalize( glm::cross( coords[1] - coords[0], coords[2] - coords[0] ) );
		dir -= tri_normal;
		}

	dir = glm::normalize( dir );

	mesh.RejectionConeCenter = center;
	mesh.RejectionConeDirection = dir;
	mesh.RejectionConeCutoff = 0.0f;
	}

void Tools::Multimesh::Serialize( std::fstream& fs, bool reading )
	{
	Serializer s( fs, reading );

	s.Vector( this->Vertices );
	s.Vector( this->Indices );
	s.Vector( this->CompressedVertices );

	s.Item( this->BoundingSphere.x );
	s.Item( this->BoundingSphere.y );
	s.Item( this->BoundingSphere.z );
	s.Item( this->BoundingSphereRadius );

	s.Item( this->AABB[0] );
	s.Item( this->AABB[1] );

	s.Item( this->CompressedVertexScale );
	s.Item( this->CompressedVertexTranslate );

	s.VectorSize( this->SubMeshes );
	for( size_t i = 0; i < this->SubMeshes.size(); ++i )
		{
		Tools::Multimesh::Submesh& smesh = this->SubMeshes[i];

		s.Item( smesh.VertexOffset );
		s.Item( smesh.VertexCount );
		s.Item( smesh.IndexOffset );
		s.Item( smesh.IndexCount );

		s.Item( smesh.BoundingSphereRadius );

		s.Item( smesh.BoundingSphere );
		s.Item( smesh.BoundingSphereRadius );
		  
		s.Item( smesh.AABB[0] );
		s.Item( smesh.AABB[1] );
		  
		s.Item( smesh.RejectionConeCenter );
		s.Item( smesh.RejectionConeDirection );
		s.Item( smesh.RejectionConeCutoff );

		s.Item( smesh.LockedVertexCount );

		s.Array<uint,4>( smesh.LODIndexCounts );
		s.Array<uint,4>( smesh.LODQuantizeBits );
		}
	}

bool Tools::Multimesh::Load( const char* path )
	{
	std::fstream fs;

	fs.open( path, std::fstream::in | std::fstream::binary );
	if( !fs.is_open() )
		return false;

	this->Serialize( fs, true );

	fs.close();
	return true;
	}

bool Tools::Multimesh::Save( const char* path )
	{
	std::fstream fs;
	
	fs.open( path, std::fstream::out | std::ofstream::binary );
	if( !fs.is_open() )
		return false;
	
	this->Serialize( fs, false );

	fs.close();
	return true;
	}
