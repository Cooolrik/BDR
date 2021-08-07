
// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 4100 4201 4127 4189 )

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <cstdint> // Necessary for UINT32_MAX
#include <algorithm>
#include <fstream>
#include <array>
#include <chrono>
#include <unordered_map>
#include <vector>


#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "PerlinNoise.hpp"

typedef unsigned int uint;
using std::vector;

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>

const uint PlaneQuadTesselation = 4; // 16*16*2 = 512 tris
const uint MeshTesselation = 8;


// inclusive random, value can be either mini or maxi
inline float frand( float minv = 0.f, float maxv = 1.f )
	{
	return ( ( (float)rand() / (float)RAND_MAX ) * ( maxv - minv ) ) + minv;
	}

class Vertex
	{
	public:
		glm::vec4 X_Y_Z_U;
		glm::vec4 NX_NY_NZ_V;

		bool operator==( const Vertex& other ) const
			{
			return ( X_Y_Z_U == other.X_Y_Z_U ) && ( NX_NY_NZ_V == other.NX_NY_NZ_V );
			}
	};

namespace std
	{
	template<> struct hash<Vertex>
		{
		size_t operator()( Vertex const& vertex ) const
			{
			return ( ( hash<glm::vec4>()( vertex.X_Y_Z_U ) ^
				( hash<glm::vec4>()( vertex.NX_NY_NZ_V ) << 1 ) ) >> 1 );
			}
		};
	}

class Triangle
	{
	public:
		uint GlobalCoordIds[3];
		uint VertexIds[3];
	};

class Submesh
	{
	public:
		vector<Vertex> Vertices{};
		std::unordered_map<Vertex, uint32_t> VerticesMap{};
		vector<Triangle> Triangles{};

		uint GetVertexId( Vertex v );
		void GenerateTriangle( Vertex v0, Vertex v1, Vertex v2 );
		void GeneratePlane( uint axis, bool flipped, glm::vec3 minv, glm::vec3 maxv, glm::vec2 minuv, glm::vec2 maxuv );
	};

vector<glm::vec3> GlobalCoords{};
std::unordered_map<glm::vec3, uint32_t> GlobalCoordsMap{};
vector<Submesh> Submeshes;

uint GetGlobalCoordId( glm::vec3 c )
	{
	if( GlobalCoordsMap.count( c ) == 0 )
		{
		GlobalCoordsMap[c] = static_cast<uint32_t>( GlobalCoords.size() );
		GlobalCoords.push_back( c );
		}
	return (uint)GlobalCoordsMap[c];
	}

uint Submesh::GetVertexId( Vertex v )
	{
	if( this->VerticesMap.count( v ) == 0 )
		{
		this->VerticesMap[v] = static_cast<uint32_t>( this->Vertices.size() );
		this->Vertices.push_back( v );
		}
	return (uint)this->VerticesMap[v];
	}

void Submesh::GenerateTriangle( Vertex v0, Vertex v1, Vertex v2 )
	{
	Triangle t;

	t.GlobalCoordIds[0] = GetGlobalCoordId( glm::vec3( v0.X_Y_Z_U ) );
	t.GlobalCoordIds[1] = GetGlobalCoordId( glm::vec3( v1.X_Y_Z_U ) );
	t.GlobalCoordIds[2] = GetGlobalCoordId( glm::vec3( v2.X_Y_Z_U ) );

	t.VertexIds[0] = GetVertexId( v0 );
	t.VertexIds[1] = GetVertexId( v1 );
	t.VertexIds[2] = GetVertexId( v2 );

	this->Triangles.push_back( t );
	}

void Submesh::GeneratePlane( uint axis, bool flipped, glm::vec3 minv, glm::vec3 maxv, glm::vec2 minuv, glm::vec2 maxuv )
	{
	uint axis_s=1;
	uint axis_t=2;

	if( axis == 1 )
		{
		axis_s = 0;
		axis_t = 2;
		}
	else if( axis == 2 )
		{
		axis_s = 0;
		axis_t = 1;
		}

	glm::vec3 delta_s{};
	glm::vec3 delta_t{};
	glm::vec2 deltauv_s{};
	glm::vec2 deltauv_t{};

	delta_s[axis_s] = maxv[axis_s] - minv[axis_s];
	delta_t[axis_t] = maxv[axis_t] - minv[axis_t];
	deltauv_s[0] = maxuv[0] - minuv[0];
	deltauv_t[1] = maxuv[1] - minuv[1];

	for( uint t = 0; t < PlaneQuadTesselation; ++t )
		{
		float alpha_t[2] = {
			float( t ) / float( PlaneQuadTesselation ),
			float( t + 1 ) / float( PlaneQuadTesselation )
			};

		for( uint s = 0; s < PlaneQuadTesselation; ++s )
			{
			float alpha_s[2] = {
				float( s ) / float( PlaneQuadTesselation ),
				float( s + 1 ) / float( PlaneQuadTesselation )
				};

			glm::vec3 coords[4] = {
				minv + ( delta_s * alpha_s[0] ) + ( delta_t * alpha_t[0] ),
				minv + ( delta_s * alpha_s[1] ) + ( delta_t * alpha_t[0] ),
				minv + ( delta_s * alpha_s[0] ) + ( delta_t * alpha_t[1] ),
				minv + ( delta_s * alpha_s[1] ) + ( delta_t * alpha_t[1] )
				};

			glm::vec2 uvcoords[4] = {
				minuv + ( deltauv_s * alpha_s[0] ) + ( deltauv_t * alpha_t[0] ),
				minuv + ( deltauv_s * alpha_s[1] ) + ( deltauv_t * alpha_t[0] ),
				minuv + ( deltauv_s * alpha_s[0] ) + ( deltauv_t * alpha_t[1] ),
				minuv + ( deltauv_s * alpha_s[1] ) + ( deltauv_t * alpha_t[1] )
				};

			Vertex verts[4];
			for( uint i = 0; i < 4; ++i )
				{
				verts[i].X_Y_Z_U = glm::vec4( coords[i], uvcoords[i].x );
				verts[i].NX_NY_NZ_V = glm::vec4( 0.f , 0.f , 0.f , uvcoords[i].y );
				}

			if( flipped )
				{
				this->GenerateTriangle( verts[0], verts[2], verts[1] );
				this->GenerateTriangle( verts[1], verts[2], verts[3] );
				}
			else
				{
				this->GenerateTriangle( verts[0], verts[1], verts[2] );
				this->GenerateTriangle( verts[1], verts[3], verts[2] );
				}
			}
		}

	}

void GeneratePlanes( uint axis, bool flipped, glm::vec3 minv, glm::vec3 maxv, glm::vec2 minuv, glm::vec2 maxuv )
	{
	uint axis_s = 1;
	uint axis_t = 2;

	if( axis == 1 )
		{
		axis_s = 0;
		axis_t = 2;
		}
	else if( axis == 2 )
		{
		axis_s = 0;
		axis_t = 1;
		}

	glm::vec3 delta_s{};
	glm::vec3 delta_t{};
	glm::vec2 deltauv_s{};
	glm::vec2 deltauv_t{};

	delta_s[axis_s] = maxv[axis_s] - minv[axis_s];
	delta_t[axis_t] = maxv[axis_t] - minv[axis_t];
	deltauv_s[0] = maxuv[0] - minuv[0];
	deltauv_t[1] = maxuv[1] - minuv[1];

	for( uint t = 0; t < MeshTesselation; ++t )
		{
		float alpha_t[2] = {
			float( t ) / float( MeshTesselation ),
			float( t + 1 ) / float( MeshTesselation )
			};

		for( uint s = 0; s < MeshTesselation; ++s )
			{
			float alpha_s[2] = {
				float( s ) / float( MeshTesselation ),
				float( s + 1 ) / float( MeshTesselation )
				};

			glm::vec3 coords[2] = {
				minv + ( delta_s * alpha_s[0] ) + ( delta_t * alpha_t[0] ),
				minv + ( delta_s * alpha_s[1] ) + ( delta_t * alpha_t[1] )
				};

			glm::vec2 uvcoords[2] = {
				minuv + ( deltauv_s * alpha_s[0] ) + ( deltauv_t * alpha_t[0] ),
				minuv + ( deltauv_s * alpha_s[1] ) + ( deltauv_t * alpha_t[1] )
				};

			Submeshes.push_back( Submesh() );
			Submeshes.back().GeneratePlane( axis , flipped , coords[0] , coords[1], uvcoords[0], uvcoords[1] );
			}
		}

	}


void output_obj( const char *path )
	{
	std::ofstream fs;

	fs.open( path, std::ofstream::out );

	size_t vertex_base = 1;

	for( size_t m = 0; m < Submeshes.size(); ++m )
		{
		fs << "g mesh_" << m << std::endl;

		for( size_t v = 0; v < Submeshes[m].Vertices.size(); ++v )
			{
			fs << "v " << Submeshes[m].Vertices[v].X_Y_Z_U.x 
				<< " " << Submeshes[m].Vertices[v].X_Y_Z_U.y 
				<< " " << Submeshes[m].Vertices[v].X_Y_Z_U.z 
				<< std::endl;
			}

		for( size_t v = 0; v < Submeshes[m].Vertices.size(); ++v )
			{
			fs << "vt " << Submeshes[m].Vertices[v].X_Y_Z_U.w
				<< " " << Submeshes[m].Vertices[v].NX_NY_NZ_V.w 
				<< std::endl;
			}

		for( size_t v = 0; v < Submeshes[m].Vertices.size(); ++v )
			{
			fs << "vn " << Submeshes[m].Vertices[v].NX_NY_NZ_V.x
				<< " " << Submeshes[m].Vertices[v].NX_NY_NZ_V.y
				<< " " << Submeshes[m].Vertices[v].NX_NY_NZ_V.z
				<< std::endl;
			}

		for( size_t t = 0; t < Submeshes[m].Triangles.size(); ++t )
			{
			fs << "f " << Submeshes[m].Triangles[t].VertexIds[0] + vertex_base
				<< "/" << Submeshes[m].Triangles[t].VertexIds[0] + vertex_base
				<< "/" << Submeshes[m].Triangles[t].VertexIds[0] + vertex_base 
				<< " " << Submeshes[m].Triangles[t].VertexIds[1] + vertex_base
				<< "/" << Submeshes[m].Triangles[t].VertexIds[1] + vertex_base
				<< "/" << Submeshes[m].Triangles[t].VertexIds[1] + vertex_base
				<< " " << Submeshes[m].Triangles[t].VertexIds[2] + vertex_base
				<< "/" << Submeshes[m].Triangles[t].VertexIds[2] + vertex_base
				<< "/" << Submeshes[m].Triangles[t].VertexIds[2] + vertex_base
				<< std::endl;
			}

		vertex_base += Submeshes[m].Vertices.size();
		}

	fs.close();
	}

int main( int argc, char argv[] )
	{
	std::uint32_t seed = 128767863;
	std::uint32_t octaves = 18;
	const siv::PerlinNoise perlin( seed );

	for( uint q = 0; q < 6; ++q )
		{
		glm::vec3 minv = glm::vec3( -1 );
		glm::vec3 maxv = glm::vec3( 1 );

		GeneratePlanes( 0, true, glm::vec3( minv.x, minv.y, minv.z ), glm::vec3( minv.x, maxv.y, maxv.z ), glm::vec2( 0, 0 ), glm::vec2( 1, 1 ) );
		GeneratePlanes( 0, false, glm::vec3( maxv.x, minv.y, minv.z ), glm::vec3( maxv.x, maxv.y, maxv.z ), glm::vec2( 0, 0 ), glm::vec2( 1, 1 ) );
		GeneratePlanes( 1, false, glm::vec3( minv.x, minv.y, minv.z ), glm::vec3( maxv.x, minv.y, maxv.z ), glm::vec2( 0, 0 ), glm::vec2( 1, 1 ) );
		GeneratePlanes( 1, true, glm::vec3( minv.x, maxv.y, minv.z ), glm::vec3( maxv.x, maxv.y, maxv.z ), glm::vec2( 0, 0 ), glm::vec2( 1, 1 ) );
		GeneratePlanes( 2, true, glm::vec3( minv.x, minv.y, minv.z ), glm::vec3( maxv.x, maxv.y, minv.z ), glm::vec2( 0, 0 ), glm::vec2( 1, 1 ) );
		GeneratePlanes( 2, false, glm::vec3( minv.x, minv.y, maxv.z ), glm::vec3( maxv.x, maxv.y, maxv.z ), glm::vec2( 0, 0 ), glm::vec2( 1, 1 ) );

		glm::vec3 MeshScale( 1, 1, frand( 0.5, 3.0 ) );

		// deform mesh 
		for( size_t c = 0 ; c < GlobalCoords.size(); ++c )
			{
			GlobalCoords[c] = glm::normalize( GlobalCoords[c] );
			GlobalCoords[c] *= MeshScale;
			GlobalCoords[c] *= 1.0 + perlin.accumulatedOctaveNoise3D_0_1( GlobalCoords[c].x, GlobalCoords[c].y, GlobalCoords[c].z, octaves );
			}

		// update all normals
		vector<glm::vec3> global_normals;
		global_normals.resize( GlobalCoords.size() );

		// accumulate vertex normals
		for( size_t m = 0; m < Submeshes.size(); ++m )
			{
			for( size_t t = 0; t < Submeshes[m].Triangles.size(); ++t )
				{
				glm::vec3 coords[3];

				for( size_t v = 0 ; v < 3; ++v )
					{
					uint cid = Submeshes[m].Triangles[t].GlobalCoordIds[v];
					coords[v] = GlobalCoords[cid];
					}

				// area-weighted normal
				glm::vec3 tri_normal = glm::cross( coords[2] - coords[0], coords[1] - coords[0] );

				for( size_t v = 0 ; v < 3; ++v )
					{
					uint cid = Submeshes[m].Triangles[t].GlobalCoordIds[v];
					global_normals[cid] += tri_normal;
					}
				}
			}

		// normalize normals
		for( size_t v = 0 ; v < global_normals.size(); ++v )
			{
			global_normals[v] = glm::normalize( global_normals[v] );
			}

		// update coords and normals in vertices
		for( size_t m = 0; m < Submeshes.size(); ++m )
			{
			for( size_t t = 0; t < Submeshes[m].Triangles.size(); ++t )
				{
				for( size_t v = 0 ; v < 3; ++v )
					{
					uint cid = Submeshes[m].Triangles[t].GlobalCoordIds[v];
					uint vid = Submeshes[m].Triangles[t].VertexIds[v];
					
					Submeshes[m].Vertices[vid].X_Y_Z_U.x = GlobalCoords[cid].x;
					Submeshes[m].Vertices[vid].X_Y_Z_U.y = GlobalCoords[cid].y;
					Submeshes[m].Vertices[vid].X_Y_Z_U.z = GlobalCoords[cid].z;

					Submeshes[m].Vertices[vid].NX_NY_NZ_V.x = global_normals[cid].x;
					Submeshes[m].Vertices[vid].NX_NY_NZ_V.y = global_normals[cid].y;
					Submeshes[m].Vertices[vid].NX_NY_NZ_V.z = global_normals[cid].z;
					}
				}
			}

		char str[30];
		sprintf_s( str, "meteor_%d.obj", q );
		output_obj( str );

		GlobalCoords.clear();
		GlobalCoordsMap.clear();
		Submeshes.clear();
		}

	return EXIT_SUCCESS;
	}
