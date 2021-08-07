#include "SourceMesh.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <unordered_map>

#include <fast_obj.h>

typedef unsigned int uint;
using std::vector;

void SourceMesh::loadModel( const char* path )
	{
	fastObjMesh* m = fast_obj_read( path );
	if(m == nullptr)
		{
		throw std::runtime_error( "Error: Could not read .obj file" );
		}

	// load all tris of all shapes
	for(unsigned int ii = 0; ii < m->group_count; ii++)
		{
		std::unordered_map<SourceVertex, uint32_t> uniqueVertices{};
		const fastObjGroup& grp = m->groups[ii];
		uint tri_count = grp.face_count;

		int idx = 0;
		for(uint t_id = 0; t_id < tri_count; ++t_id)
			{
			unsigned int fv = m->face_vertices[grp.face_offset + t_id];
			if(fv != 3)
				{
				throw std::runtime_error( "Error: Can only read triangulated wavefront files" );
				}

			for(uint c_id = 0; c_id < 3; ++c_id)
				{
				fastObjIndex mi = m->indices[grp.index_offset + idx];

				SourceVertex vertex{};

				vertex.coord[0] = m->positions[3 * mi.p + 0];
				vertex.coord[1] = m->positions[3 * mi.p + 1];
				vertex.coord[2] = m->positions[3 * mi.p + 2];

				if(mi.n)
					{
					vertex.normal[0] = m->normals[3 * mi.n + 0];
					vertex.normal[1] = m->normals[3 * mi.n + 1];
					vertex.normal[2] = m->normals[3 * mi.n + 2];
					}

				if(mi.t)
					{
					vertex.texCoord[0] = m->texcoords[2 * mi.t + 0];
					vertex.texCoord[1] = 1.f - m->texcoords[2 * mi.t + 1];
					}

				if(uniqueVertices.count( vertex ) == 0)
					{
					uniqueVertices[vertex] = static_cast<uint32_t>(this->Vertices.size());
					this->Vertices.push_back( vertex );
					}
				this->Indices.push_back( uniqueVertices[vertex] );

				++idx;
				}
			}
		}

	fast_obj_destroy( m );

	// make sure we have valid normals 
	std::vector<glm::vec3> vertex_normals( this->Vertices.size() );

	// get the average negative normal of all triangles
	for(size_t t = 0; t < this->Indices.size() / 3; ++t)
		{
		glm::vec3 tri[3];
		for(size_t c = 0; c < 3; ++c)
			{
			tri[c] = this->Vertices[this->Indices[t * 3 + c]].coord;
			}
		glm::vec3 tri_area_normal = glm::cross( tri[2] - tri[0] , tri[1] - tri[0] );
		for(size_t c = 0; c < 3; ++c)
			{
			vertex_normals[this->Indices[t * 3 + c]] += tri_area_normal;
			}
		}

	// for each vertex, if the normal is close to 0, replace it
	for(size_t v = 0; v < this->Vertices.size(); ++v )
		{
		if(glm::length( this->Vertices[v].normal ) < 0.1f)
			{
			this->Vertices[v].normal = glm::normalize( vertex_normals[v] );
			}
		}

	}