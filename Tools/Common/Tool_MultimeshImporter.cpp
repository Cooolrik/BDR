#include "Tool_MultimeshImporter.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <unordered_map>

#include <fast_obj.h>

typedef unsigned int uint;
using std::vector;

void Tools::MultimeshImporter::buildFromWavefrontFile( const char* path , unsigned int max_tris , unsigned int max_verts, Multimesh** dest )
	{
	Multimesh* mesh = new Multimesh();

	std::vector<Vertex> vertices;
	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	std::vector<unsigned int> indices;

	fastObjMesh* m = fast_obj_read( path );
	if(m == nullptr)
		{
		throw std::runtime_error( "Error: Could not read .obj file" );
		}
			
	// load all tris of all shapes
	for(unsigned int ii = 0; ii < m->group_count; ii++)
		{
		const fastObjGroup& grp = m->groups[ii];
		uint tri_count = grp.face_count;

		int idx = 0;
		for( uint t_id = 0; t_id < tri_count; ++t_id )
			{
			unsigned int fv = m->face_vertices[grp.face_offset + t_id];
			if(fv != 3)
				{
				throw std::runtime_error( "Error: Can only read triangulated wavefront files" );
				}

			for( uint c_id = 0 ; c_id < 3 ; ++c_id )
				{
				fastObjIndex mi = m->indices[grp.index_offset + idx];

				Vertex vertex{};

				vertex.Coords[0] = m->positions[3 * mi.p + 0];
				vertex.Coords[1] = m->positions[3 * mi.p + 1];
				vertex.Coords[2] = m->positions[3 * mi.p + 2];

				if(mi.n)
					{
					vertex.Normals[0] = m->normals[3 * mi.n + 0];
					vertex.Normals[1] = m->normals[3 * mi.n + 1];
					vertex.Normals[2] = m->normals[3 * mi.n + 2];
					}

				if(mi.t)
					{
					vertex.TexCoords[0] = m->texcoords[2 * mi.t + 0];
					vertex.TexCoords[1] = 1.f - m->texcoords[2 * mi.t + 1];
					}

				if( uniqueVertices.count( vertex ) == 0 ) 
					{
					uniqueVertices[vertex] = static_cast<uint32_t>( vertices.size() );
					vertices.push_back( vertex );
					}
				indices.push_back( uniqueVertices[vertex] );

				++idx;
				}

			// check if we are at capacity
			if( indices.size() >= (size_t)max_tris * 3 ||
				uniqueVertices.size() >= (size_t)(max_verts-2)
				)
				{
				// at capacity, need to start a new mesh
				mesh->SubMeshes.emplace_back();
				mesh->SubMeshes.back().vertices = vertices;
				mesh->SubMeshes.back().indices = indices;
				mesh->SubMeshes.back().updateBoundingSphere();
				uniqueVertices.clear();
				vertices.clear();
				indices.clear();
				}

			}
		}

	fast_obj_destroy( m );

	// save off the last mesh
	if( indices.size() > 0 )
		{
		mesh->SubMeshes.emplace_back();
		mesh->SubMeshes.back().vertices = vertices;
		mesh->SubMeshes.back().indices = indices;
		mesh->SubMeshes.back().updateBoundingSphere();
		}

	// calc full bsphere (TODO: improve this)
	// using weighted average of spheres as center
	mesh->boundingSphere = glm::vec3( 0 );
	mesh->boundingSphereRadius = 0;
	uint submesh_count = uint(mesh->SubMeshes.size());
	for( uint i = 0; i < submesh_count; ++i )
		{
		mesh->boundingSphere += mesh->SubMeshes[i].boundingSphere;
		}
	mesh->boundingSphere /= (float)submesh_count;
	for( uint i = 0; i < submesh_count; ++i )
		{
		float r = glm::length( mesh->SubMeshes[i].boundingSphere - mesh->boundingSphere ) + mesh->SubMeshes[i].boundingSphereRadius;
		if( r > mesh->boundingSphereRadius )
			{
			mesh->boundingSphereRadius = r;
			}
		}

	*dest = mesh;
	}
