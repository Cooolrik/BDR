#include "MegaMesh.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <unordered_map>

#include "Vertex.h"

#include "Vlk_VertexBuffer.h"
#include "Vlk_IndexBuffer.h"

#include <tiny_obj_loader.h>

typedef unsigned int uint;
using std::vector;

inline void update_bb_axis( float coord, float& minv, float& maxv )
	{
	if( coord < minv )
		minv = coord;
	if( coord > maxv )
		maxv = coord;
	}

class MegaMeshAllocator::SourceSubMesh
	{
	public:
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;

		glm::vec3 boundingSphere;
		float boundingSphereRadius;

		glm::vec3 rejectionConeCenter;
		glm::vec3 rejectionConeDirection;
		float rejectionConeCutoff;

		void updateBoundingSphere()
			{
			glm::vec3 minv = glm::vec3( FLT_MAX );
			glm::vec3 maxv = glm::vec3( -FLT_MAX );

			for( size_t i = 0; i < vertices.size(); ++i )
				{
				update_bb_axis( vertices[i].X_Y_Z_U.x, minv.x, maxv.x );
				update_bb_axis( vertices[i].X_Y_Z_U.y, minv.y, maxv.y );
				update_bb_axis( vertices[i].X_Y_Z_U.z, minv.z, maxv.z );
				}

			// place sphere in center of bb (TODO:do better) and find max radius
			this->boundingSphere = ( minv + maxv ) / 2.f;
			this->boundingSphereRadius = 0;
			for( size_t i = 0; i < vertices.size(); ++i )
				{
				float l = glm::length( boundingSphere - glm::vec3(vertices[i].X_Y_Z_U) );
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
					coords[c] = glm::vec3(vertices[indices[t * 3 + c]].X_Y_Z_U);
					}
				glm::vec3 tri_normal = glm::normalize(glm::cross( coords[1] - coords[0], coords[2] - coords[0]));
				dir -= tri_normal;
				}

			dir = glm::normalize( dir );

			this->rejectionConeCenter = center;
			this->rejectionConeDirection = dir;
			this->rejectionConeCutoff = 0.0f;
			}

	};

class MegaMeshAllocator::SourceMesh
	{
	public:
		std::vector<SourceSubMesh> SubMeshes;

		// for the full megamesh
		glm::vec3 boundingSphere;
		float boundingSphereRadius;

		void buildModel( const char* path , uint max_tris , uint max_verts )
			{
			tinyobj::attrib_t attrib;

			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materiallist;

			std::string warn, err;

			std::vector<Vertex> vertices;
			std::unordered_map<Vertex, uint32_t> uniqueVertices{};
			std::vector<unsigned int> indices;

			if( !tinyobj::LoadObj( &attrib, &shapes, &materiallist, &warn, &err, path ) )
				{
				throw std::runtime_error( warn + err );
				}

			// load all tris of all shapes
			for( const auto& shape : shapes )
				{
				uint tri_count = (uint)shape.mesh.material_ids.size();

				for( uint t_id = 0; t_id < tri_count; ++t_id )
					{
					for( uint c_id = 0 ; c_id < 3 ; ++c_id )
						{
						const tinyobj::index_t& index = shape.mesh.indices[t_id * 3 + c_id];

						Vertex vertex{};

						vertex.X_Y_Z_U[0] = attrib.vertices[3 * index.vertex_index + 0];
						vertex.X_Y_Z_U[1] = attrib.vertices[3 * index.vertex_index + 1];
						vertex.X_Y_Z_U[2] = attrib.vertices[3 * index.vertex_index + 2];
						vertex.X_Y_Z_U[3] = 0.f;

						vertex.NX_NY_NZ_V[0] = attrib.normals[3 * index.normal_index + 0];
						vertex.NX_NY_NZ_V[1] = attrib.normals[3 * index.normal_index + 1];
						vertex.NX_NY_NZ_V[2] = attrib.normals[3 * index.normal_index + 2];
						vertex.NX_NY_NZ_V[3] = 0.f;

						if( index.texcoord_index >= 0 )
							{
							vertex.X_Y_Z_U[3] = attrib.texcoords[2 * index.texcoord_index + 0];
							vertex.NX_NY_NZ_V[3] = 1.f - attrib.texcoords[2 * index.texcoord_index + 1];
							}

						if( uniqueVertices.count( vertex ) == 0 ) 
							{
							uniqueVertices[vertex] = static_cast<uint32_t>( vertices.size() );
							vertices.push_back( vertex );
							}
						indices.push_back( uniqueVertices[vertex] );
						}

					// check if we are at capacity
					if( indices.size() >= (size_t)max_tris * 3 ||
						uniqueVertices.size() >= (size_t)(max_verts-2)
						)
						{
						// at capacity, need to start a new mesh
						SubMeshes.push_back( SourceSubMesh() );
						SubMeshes.back().vertices = vertices;
						SubMeshes.back().indices = indices;
						SubMeshes.back().updateBoundingSphere();
						uniqueVertices.clear();
						vertices.clear();
						indices.clear();
						}

					}
				}

			// save off the last mesh
			if( indices.size() > 0 )
				{
				SubMeshes.push_back( SourceSubMesh() );
				SubMeshes.back().vertices = vertices;
				SubMeshes.back().indices = indices;
				SubMeshes.back().updateBoundingSphere();
				}

			// calc full bsphere (TODO: improve this)
			// using weighted average of spheres as center
			this->boundingSphere = glm::vec3( 0 );
			this->boundingSphereRadius = 0;
			uint submesh_count = (uint)SubMeshes.size();
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

		void saveCache( const char* path )
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
				}

			fs.write( (char*)&this->boundingSphere.x, sizeof( float ) );
			fs.write( (char*)&this->boundingSphere.y, sizeof( float ) );
			fs.write( (char*)&this->boundingSphere.z, sizeof( float ) );
			fs.write( (char*)&this->boundingSphereRadius, sizeof( float ) );

			fs.close();
			}

		bool loadCache( const char* path )
			{
			std::ifstream fs;

			fs.open( path, std::ifstream::binary );
			if( !fs.is_open() )
				return false;

			uint submesh_count;
			fs.read( (char*)&submesh_count, sizeof( uint ) );
			this->SubMeshes.resize(submesh_count);
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
				}

			fs.read( (char*)&this->boundingSphere.x, sizeof( float ) );
			fs.read( (char*)&this->boundingSphere.y, sizeof( float ) );
			fs.read( (char*)&this->boundingSphere.z, sizeof( float ) );
			fs.read( (char*)&this->boundingSphereRadius, sizeof( float ) );

			fs.close();

			return true;
			}
	};

std::vector<MegaMesh> MegaMeshAllocator::LoadMeshes( Vlk::Renderer* renderer, std::vector<const char*> paths, uint max_tris, uint max_verts )
	{
	std::vector<MegaMesh> ret;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	this->Clear();

	ret.resize( paths.size() );

	// load in all models, append into one large memory allocation
	for( size_t i = 0; i < paths.size(); ++i )
		{
		SourceMesh source_mesh;

		// load in source mesh from cache, or build it if it is not cached yet
		std::string cache_file = std::string( paths[i] ) + ".cache";
		if( !source_mesh.loadCache( cache_file.c_str() ) )
			{
			source_mesh.buildModel( paths[i], max_tris, max_verts );
			source_mesh.saveCache( cache_file.c_str() );
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

			// append to buffers
			vertices.insert( vertices.end(), source_mesh.SubMeshes[q].vertices.begin(), source_mesh.SubMeshes[q].vertices.end() );
			indices.insert( indices.end(), source_mesh.SubMeshes[q].indices.begin(), source_mesh.SubMeshes[q].indices.end() );
			}
		}

	// setup gpu buffers
	vertexBuffer = renderer->CreateVertexBuffer( Vertex::GetVertexBufferDescription(), (uint)vertices.size(), vertices.data() );
	indexBuffer = renderer->CreateIndexBuffer( VK_INDEX_TYPE_UINT32, (uint)indices.size(), indices.data() );

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
