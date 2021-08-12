#include "Tool_Multimesh.h"

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

void Tools::Multimesh::Submesh::calcBoundingSphereAndRejectionCone()
	{
	glm::vec3 minv = glm::vec3( FLT_MAX );
	glm::vec3 maxv = glm::vec3( -FLT_MAX );

	for( size_t i = 0; i < vertices.size(); ++i )
		{
		update_bb_axis( vertices[i].Coords.x, minv.x, maxv.x );
		update_bb_axis( vertices[i].Coords.y, minv.y, maxv.y );
		update_bb_axis( vertices[i].Coords.z, minv.z, maxv.z );
		}

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

void Tools::Multimesh::calcBoundingSpheresAndRejectionCones()
	{
	uint submesh_count = (uint)SubMeshes.size();
	for( uint i = 0; i < submesh_count; ++i )
		{
		SubMeshes[i].calcBoundingSphereAndRejectionCone();
		}

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

bool Tools::Multimesh::load( const char* path )
	{
	std::ifstream fs;

	fs.open( path, std::ifstream::binary );
	if( !fs.is_open() )
		return false;

	uint submesh_count;
	fs.read( (char*)&submesh_count, sizeof( uint ) );
	this->SubMeshes.resize( submesh_count );
	for( uint i = 0; i < submesh_count; ++i )
		{
		uint vertex_count;
		uint index_count;

		fs.read( (char*)&vertex_count, sizeof( uint ) );
		fs.read( (char*)&index_count, sizeof( uint ) );

		this->SubMeshes[i].vertices.resize( vertex_count );
		this->SubMeshes[i].indices.resize( index_count );

		fs.read( (char*)this->SubMeshes[i].vertices.data(), sizeof( Vertex ) * vertex_count );
		fs.read( (char*)this->SubMeshes[i].indices.data(), sizeof( uint ) * index_count );
		fs.read( (char*)&this->SubMeshes[i].boundingSphere.x, sizeof( float ) );
		fs.read( (char*)&this->SubMeshes[i].boundingSphere.y, sizeof( float ) );
		fs.read( (char*)&this->SubMeshes[i].boundingSphere.z, sizeof( float ) );
		fs.read( (char*)&this->SubMeshes[i].boundingSphereRadius, sizeof( float ) );

		fs.read( (char*)&this->SubMeshes[i].rejectionConeCenter.x, sizeof( float ) );
		fs.read( (char*)&this->SubMeshes[i].rejectionConeCenter.y, sizeof( float ) );
		fs.read( (char*)&this->SubMeshes[i].rejectionConeCenter.z, sizeof( float ) );
		fs.read( (char*)&this->SubMeshes[i].rejectionConeDirection.x, sizeof( float ) );
		fs.read( (char*)&this->SubMeshes[i].rejectionConeDirection.y, sizeof( float ) );
		fs.read( (char*)&this->SubMeshes[i].rejectionConeDirection.z, sizeof( float ) );
		fs.read( (char*)&this->SubMeshes[i].rejectionConeCutoff, sizeof( float ) );

		fs.read( (char*)this->SubMeshes[i].LODTriangleCounts, sizeof( uint ) * 4 );
		fs.read( (char*)this->SubMeshes[i].LODQuantizeDistances, sizeof( float ) * 4 );
		}

	fs.read( (char*)&this->boundingSphere.x, sizeof( float ) );
	fs.read( (char*)&this->boundingSphere.y, sizeof( float ) );
	fs.read( (char*)&this->boundingSphere.z, sizeof( float ) );
	fs.read( (char*)&this->boundingSphereRadius, sizeof( float ) );

	fs.close();

	return true;
	}

void Tools::Multimesh::save( const char* path )
		{
		std::ofstream fs;

		fs.open( path, std::ofstream::binary );

		uint submesh_count = (uint)SubMeshes.size();

		fs.write( (char*)&submesh_count, sizeof( uint ) );

		for( uint i = 0; i < submesh_count; ++i )
			{
			uint vertex_count = (uint)SubMeshes[i].vertices.size();
			uint index_count = (uint)SubMeshes[i].indices.size();

			fs.write( (char*)&vertex_count, sizeof( uint ) );
			fs.write( (char*)&index_count, sizeof( uint ) );

			fs.write( (char*)this->SubMeshes[i].vertices.data(), sizeof( Vertex ) * vertex_count );
			fs.write( (char*)this->SubMeshes[i].indices.data(), sizeof( uint ) * index_count );
			fs.write( (char*)&this->SubMeshes[i].boundingSphere.x, sizeof( float ) );
			fs.write( (char*)&this->SubMeshes[i].boundingSphere.y, sizeof( float ) );
			fs.write( (char*)&this->SubMeshes[i].boundingSphere.z, sizeof( float ) );
			fs.write( (char*)&this->SubMeshes[i].boundingSphereRadius, sizeof( float ) );

			fs.write( (char*)&this->SubMeshes[i].rejectionConeCenter.x, sizeof( float ) );
			fs.write( (char*)&this->SubMeshes[i].rejectionConeCenter.y, sizeof( float ) );
			fs.write( (char*)&this->SubMeshes[i].rejectionConeCenter.z, sizeof( float ) );
			fs.write( (char*)&this->SubMeshes[i].rejectionConeDirection.x, sizeof( float ) );
			fs.write( (char*)&this->SubMeshes[i].rejectionConeDirection.y, sizeof( float ) );
			fs.write( (char*)&this->SubMeshes[i].rejectionConeDirection.z, sizeof( float ) );
			fs.write( (char*)&this->SubMeshes[i].rejectionConeCutoff, sizeof( float ) );

			fs.write( (char*)this->SubMeshes[i].LODTriangleCounts, sizeof( uint ) * 4 );
			fs.write( (char*)this->SubMeshes[i].LODQuantizeDistances, sizeof( float ) * 4 );
			}

		fs.write( (char*)&this->boundingSphere.x, sizeof( float ) );
		fs.write( (char*)&this->boundingSphere.y, sizeof( float ) );
		fs.write( (char*)&this->boundingSphere.z, sizeof( float ) );
		fs.write( (char*)&this->boundingSphereRadius, sizeof( float ) );

		fs.close();
		}
