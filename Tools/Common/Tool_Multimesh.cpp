#include "Tool_Multimesh.h"
#include "Tool_Serializer.h"

#include <vector>
#include <iostream>
#include <fstream>

typedef unsigned int uint;
using std::vector;

inline void update_bb_axis( float coord, float& minv, float& maxv )
	{
	if( coord < minv )
		minv = coord;
	if( coord > maxv )
		maxv = coord;
	}

void Tools::Multimesh::Submesh::calcBoundingVolumesAndRejectionCone()
	{
	glm::vec3 minv = glm::vec3( FLT_MAX );
	glm::vec3 maxv = glm::vec3( -FLT_MAX );

	for( size_t i = 0; i < vertices.size(); ++i )
		{
		update_bb_axis( vertices[i].Coords.x, minv.x, maxv.x );
		update_bb_axis( vertices[i].Coords.y, minv.y, maxv.y );
		update_bb_axis( vertices[i].Coords.z, minv.z, maxv.z );
		}

	this->AABB[0] = minv;
	this->AABB[1] = maxv;

	// place sphere in center of bb (TODO:do better) and find max radius
	this->boundingSphere = ( minv + maxv ) / 2.f;
	this->boundingSphereRadius = 0;
	for( size_t i = 0; i < vertices.size(); ++i )
		{
		float l = glm::length( boundingSphere - vertices[i].Coords );
		if( l > this->boundingSphereRadius )
			{
			this->boundingSphereRadius = l;
			}
		}

	// calc test rejection cone
	glm::vec3 dir( 0 );
	glm::vec3 center( this->boundingSphere );

	// get the average negative normal of all triangles
	for( size_t t = 0; t < indices.size()/3; ++t )
		{
		glm::vec3 coords[3];
		for( size_t c = 0; c < 3; ++c )
			{
			coords[c] = vertices[indices[t * 3 + c]].Coords;
			}
		glm::vec3 tri_normal = glm::normalize(glm::cross( coords[1] - coords[0], coords[2] - coords[0]));
		dir -= tri_normal;
		}

	dir = glm::normalize( dir );

	this->rejectionConeCenter = center;
	this->rejectionConeDirection = dir;
	this->rejectionConeCutoff = 0.0f;
	}

void Tools::Multimesh::calcBoundingVolumesAndRejectionCones()
	{
	uint submesh_count = (uint)SubMeshes.size();

	glm::vec3 minv = glm::vec3( FLT_MAX );
	glm::vec3 maxv = glm::vec3( -FLT_MAX );

	for( uint i = 0; i < submesh_count; ++i )
		{
		SubMeshes[i].calcBoundingVolumesAndRejectionCone();

		if( minv.x > SubMeshes[i].AABB[0].x )
			minv.x = SubMeshes[i].AABB[0].x;
		if( minv.y > SubMeshes[i].AABB[0].y )
			minv.y = SubMeshes[i].AABB[0].y;
		if( minv.z > SubMeshes[i].AABB[0].z )
			minv.z = SubMeshes[i].AABB[0].z;
		if( maxv.x < SubMeshes[i].AABB[1].x )
			maxv.x = SubMeshes[i].AABB[1].x;
		if( maxv.y < SubMeshes[i].AABB[1].y )
			maxv.y = SubMeshes[i].AABB[1].y;
		if( maxv.z < SubMeshes[i].AABB[1].z )
			maxv.z = SubMeshes[i].AABB[1].z;
		}

	this->AABB[0] = minv;
	this->AABB[1] = maxv;

	// calc full bsphere 
	// using weighted average of spheres as center
	this->boundingSphere = glm::vec3( 0 );
	this->boundingSphereRadius = 0;
	for( uint i = 0; i < submesh_count; ++i )
		{
		this->boundingSphere += SubMeshes[i].boundingSphere;
		}
	boundingSphere /= (float)submesh_count;
	for( uint i = 0; i < submesh_count; ++i )
		{
		float r = glm::length( SubMeshes[i].boundingSphere - this->boundingSphere ) + SubMeshes[i].boundingSphereRadius;
		if( r > this->boundingSphereRadius )
			{
			this->boundingSphereRadius = r;
			}
		}
	}

void Tools::Multimesh::serialize( std::fstream& fs, bool reading )
	{
	Serializer s( fs, reading );

	s.Item( this->boundingSphere.x );
	s.Item( this->boundingSphere.y );
	s.Item( this->boundingSphere.z );
	s.Item( this->boundingSphereRadius );

	s.Item( this->AABB[0] );
	s.Item( this->AABB[1] );

	s.Item( this->CompressedVertexScale );
	s.Item( this->CompressedVertexTranslate );

	s.VectorSize( this->SubMeshes );
	for( size_t i = 0; i < this->SubMeshes.size(); ++i )
		{
		Submesh& smesh = this->SubMeshes[i];

		s.Vector( smesh.vertices );
		s.Vector( smesh.indices );
		s.Vector( smesh.compressed_vertices );

		s.Item( smesh.boundingSphere );
		s.Item( smesh.boundingSphereRadius );
		  
		s.Item( smesh.AABB[0] );
		s.Item( smesh.AABB[1] );
		  
		s.Item( smesh.rejectionConeCenter );
		s.Item( smesh.rejectionConeDirection );
		s.Item( smesh.rejectionConeCutoff );

		for( uint lod = 0; lod < 4; ++lod )
			{
			s.Item( smesh.LODTriangleCounts[lod] );
			s.Item( smesh.LODQuantizeBits[lod] );
			}
		}
	}

bool Tools::Multimesh::load( const char* path )
	{
	std::fstream fs;

	fs.open( path, std::fstream::in | std::fstream::binary );
	if( !fs.is_open() )
		return false;

	this->serialize( fs, true );

	fs.close();
	return true;
	}

bool Tools::Multimesh::save( const char* path )
	{
	std::fstream fs;
	
	fs.open( path, std::fstream::out | std::ofstream::binary );
	if( !fs.is_open() )
		return false;
	
	this->serialize( fs, false );

	fs.close();
	return true;
	}
