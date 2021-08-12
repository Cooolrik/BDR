
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

#include "../Common/Tool_Vertex.h"
#include "../Common/Tool_Multimesh.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <PerlinNoise.hpp>

typedef unsigned int uint;
using std::vector;
using std::unordered_multimap;
using Tools::Vertex;

const uint PlaneQuadTesselation = 16; // 16*16*2 = 512 tris
const uint MeshTesselation = 8;
const uint NumberOfMeshesToGenerate = 6;

#define sanity_check( must_be_true ) if( !(must_be_true) ) { throw std::runtime_error("Sanity check failed: " # must_be_true ); }

// inclusive random, value can be either mini or maxi
inline float frand( float minv = 0.f, float maxv = 1.f )
	{
	return ( ( (float)rand() / (float)RAND_MAX ) * ( maxv - minv ) ) + minv;
	}

class Triangle
	{
	public:
		uint GlobalCoordIds[3];
		uint VertexIds[3];
	};

class TriangleNeighbours
	{
	public:
		int NeighbourIds[3]; // -1: no neighbour, -2: complex (multiple neighbours)
	};

class Submesh
	{
	public:
		vector<Vertex> Vertices = {};
		std::unordered_map<Vertex, uint> VerticesMap = {};
		vector<Triangle> Triangles = {};
		uint LockedVerticesCount = 0;

		uint LODTriangleCounts[4] = {};
		float LODQuantizeDistances[4] = {};

		uint GetVertexId( Vertex v );
		void GenerateTriangle( Vertex v0, Vertex v1, Vertex v2 );
		void GeneratePlane( uint axis, bool flipped, glm::vec3 minv, glm::vec3 maxv, glm::vec2 minuv, glm::vec2 maxuv );

		// gets vertex ids from half-edge id ( tri_id * 3 + half_edge_local_index )
		void GetVerticesOfHalfEdge( uint half_edge_id, uint vertex_ids[2] );

		// finds neighbours of triangles
		vector<TriangleNeighbours> GetNeighbours();

		// reorders vertices and remaps triangles
		void ReorderVertices( vector<uint> new_vertex_location );

		// reorders triangles, keeps vertices in place
		void ReorderTriangles( vector<uint> new_triangle_location );

		void FindLockedVertices();

		// quantize mesh and return the remaining valid triangles
		vector<uint> QuantizeMesh( float quantize_distance , uint tris_to_consider = UINT_MAX );

		void CalculateQuantizedLODs();
	};

vector<glm::vec3> GlobalCoords{};
std::unordered_map<glm::vec3, uint> GlobalCoordsMap{};
vector<Submesh> Submeshes;

uint GetGlobalCoordId( glm::vec3 c )
	{
	if( GlobalCoordsMap.count( c ) == 0 )
		{
		GlobalCoordsMap[c] = static_cast<uint>( GlobalCoords.size() );
		GlobalCoords.push_back( c );
		}
	return (uint)GlobalCoordsMap[c];
	}

uint Submesh::GetVertexId( Vertex v )
	{
	if( this->VerticesMap.count( v ) == 0 )
		{
		this->VerticesMap[v] = static_cast<uint>( this->Vertices.size() );
		this->Vertices.push_back( v );
		}
	return (uint)this->VerticesMap[v];
	}

void Submesh::GenerateTriangle( Vertex v0, Vertex v1, Vertex v2 )
	{
	Triangle t;

	t.GlobalCoordIds[0] = GetGlobalCoordId( v0.Coords );
	t.GlobalCoordIds[1] = GetGlobalCoordId( v1.Coords );
	t.GlobalCoordIds[2] = GetGlobalCoordId( v2.Coords );

	t.VertexIds[0] = GetVertexId( v0 );
	t.VertexIds[1] = GetVertexId( v1 );
	t.VertexIds[2] = GetVertexId( v2 );

	this->Triangles.push_back( t );
	}

void Submesh::GeneratePlane( uint axis, bool flipped, glm::vec3 minv, glm::vec3 maxv, glm::vec2 minuv, glm::vec2 maxuv )
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
				verts[i].Coords = coords[i];
				verts[i].Normals = glm::vec4( 0 );
				verts[i].TexCoords = uvcoords[i];
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

inline uint NextHalfEdgeInTriangle( uint he_id )
	{
	return ( he_id ) - ( he_id % 3 ) + (( he_id + 1 ) % 3);
	}

void Submesh::GetVerticesOfHalfEdge( uint half_edge_id, uint vertex_ids[2] )
	{
	uint tri_id = half_edge_id / 3;
	uint half_edge_local_index = half_edge_id % 3;

	vertex_ids[0] = this->Triangles[tri_id].VertexIds[half_edge_local_index];
	vertex_ids[1] = this->Triangles[tri_id].VertexIds[(half_edge_local_index+1)%3];
	}

vector<TriangleNeighbours> Submesh::GetNeighbours()
	{
	vector<TriangleNeighbours> ret;

	// set up all half-edges (3 per triangle) that start at each vertex
	unordered_multimap<uint, uint> VertexHalfEdgeMap;
	for( size_t t = 0; t < Triangles.size(); ++t )
		{
		for( size_t e = 0; e < 3; ++e )
			{
			uint vertex_id = Triangles[t].VertexIds[e];
			uint half_edge_id = uint(( t * 3 ) + e);

			// add to start of half edge
			VertexHalfEdgeMap.insert( { vertex_id , half_edge_id } );
			}
		}

	// for each triangle, for each half-edge, find all neighbour half edges and triangles 
	ret.resize( Triangles.size() );
	for( size_t t = 0; t < Triangles.size(); ++t )
		{
		for( size_t e = 0; e < 3; ++e )
			{
			// initially reset to "no neighbour", and we'll update if we find a match
			ret[t].NeighbourIds[e] = -1;

			// get the vertices of this half edge
			uint half_edge_id = uint( ( t * 3 ) + e );
			uint vertex_ids[2];
			this->GetVerticesOfHalfEdge( half_edge_id, vertex_ids );

			// check all half edges that start around each of the vertices
			for( size_t v = 0; v < 2; ++v )
				{
				// find all half edges that start at this vertex
				auto range = VertexHalfEdgeMap.equal_range( vertex_ids[v] );
				for( auto it = range.first ; it != range.second ; ++it )
					{
					uint cmp_half_edge_id = it->second; // get the half edge to compare
					if( cmp_half_edge_id == half_edge_id ) // skip it if it is our current half edge
						continue;

					// get the vertices of the other half-edge
					uint cmp_vertex_ids[2];
					this->GetVerticesOfHalfEdge( cmp_half_edge_id, cmp_vertex_ids );

					// if we have a match, we have a match (duh)
					if( ( vertex_ids[0] == cmp_vertex_ids[0] ) && ( vertex_ids[1] == cmp_vertex_ids[1] ) ||
						( vertex_ids[0] == cmp_vertex_ids[1] ) && ( vertex_ids[1] == cmp_vertex_ids[0] ) )
						{
						uint neighbour_triangle_id = cmp_half_edge_id / 3;

						// the half-edges shares both vertex ids, we have a match
						if( ret[t].NeighbourIds[e] == -1 )
							ret[t].NeighbourIds[e] = neighbour_triangle_id; // we currently have only one match 
						else
							ret[t].NeighbourIds[e] = -2; // we already have a match, so this edge is complex, mark as such
						}
					}
				}
			}
		}

	return ret;
	}

void Submesh::ReorderVertices( vector<uint> new_vertex_location )
	{
	vector<Vertex> original_vertices = this->Vertices;

	for( size_t v = 0 ; v < original_vertices.size() ; ++v )
		{
		this->Vertices[new_vertex_location[v]] = original_vertices[v];
		}

	for( size_t t = 0; t < Triangles.size(); ++t )
		{
		for( size_t e = 0; e < 3; ++e )
			{
			this->Triangles[t].VertexIds[e] = new_vertex_location[this->Triangles[t].VertexIds[e]];
			}
		}
	}

void Submesh::ReorderTriangles( vector<uint> new_triangle_location )
	{
	vector<Triangle> original_triangles = this->Triangles;

	for( size_t t = 0 ; t < original_triangles.size() ; ++t )
		{
		this->Triangles[new_triangle_location[t]] = original_triangles[t];
		}
	}

vector<uint> Submesh::QuantizeMesh( float quantize_distance , uint tris_to_consider )
	{
	vector<glm::vec3> Coords( this->Vertices.size() );
	vector<uint> ret;

	if( tris_to_consider == UINT_MAX )
		tris_to_consider = uint(this->Triangles.size());

	// copy locked coords verbatim
	for( uint v = 0; v < this->LockedVerticesCount; ++v )
		{
		Coords[v] = this->Vertices[v].Coords;
		}

	// quantize the remaining coords
	for( uint v = this->LockedVerticesCount; v < uint(this->Vertices.size()); ++v )
		{
		Coords[v] = glm::trunc( this->Vertices[v].Coords / quantize_distance ) * quantize_distance;
		}

	// now, check all the triangles that we are to consider
	ret.reserve( tris_to_consider );
	for( size_t t = 0 ; t < tris_to_consider ; ++t )
		{
		glm::vec3 v0 = Coords[this->Triangles[t].VertexIds[0]];
		glm::vec3 v1 = Coords[this->Triangles[t].VertexIds[1]];
		glm::vec3 v2 = Coords[this->Triangles[t].VertexIds[2]];

		if( v0 == v1 || v1 == v2 || v2 == v0 )
			{
			continue;
			}

		ret.emplace_back( uint(t) );
		}

	return ret;
	}

void Submesh::CalculateQuantizedLODs()
	{
	vector<TriangleNeighbours> tri_neighbours = this->GetNeighbours();
	vector<bool> lock_vertices( this->Vertices.size() , false );

	// find all edge vertices and mark as locked
	for( size_t t = 0; t < Triangles.size(); ++t )
		{
		for( size_t e = 0; e < 3; ++e )
			{
			if( tri_neighbours[t].NeighbourIds[e] < 0 )
				{
				// border or complex edge, mark as locked
				uint half_edge_id = uint( ( t * 3 ) + e );
				uint vertex_ids[2];
				this->GetVerticesOfHalfEdge( half_edge_id, vertex_ids );
				for( size_t v = 0; v < 2; ++v )
					{
					lock_vertices[vertex_ids[v]] = true;
					}
				}
			}
		}
	tri_neighbours.clear();

	// move all locked vertices to the beginning of the vertex list
	vector<uint> vertex_remap( this->Vertices.size() );
	size_t vertex_count = 0;
	for( size_t v = 0; v < this->Vertices.size(); ++v )
		{
		if( lock_vertices[v] )
			{
			vertex_remap[v] = uint(vertex_count++); // store where the vertex should be moved to
			}
		}
	this->LockedVerticesCount = uint( vertex_count );
	for( size_t v = 0; v < this->Vertices.size(); ++v )
		{
		if( !lock_vertices[v] )
			{
			vertex_remap[v] = uint( vertex_count++ ); // store where the vertex should be moved to
			}
		}
	lock_vertices.clear();

	sanity_check( vertex_count == this->Vertices.size() );

	this->ReorderVertices( vertex_remap );
	vertex_remap.clear();

	// lod 0 always have all tris, and no quantization
	this->LODTriangleCounts[0] = uint( this->Triangles.size() );
	this->LODQuantizeDistances[0] = 0; // no quantization

	// for the lod levels [1 -> 3], do quantization, to reduce tris by 50% each step (so 100%, 50%, 25%, 12.5%)
	uint tri_count = LODTriangleCounts[0];
	uint quant_shift = 0;
	uint tris_to_consider = LODTriangleCounts[0];
	for( uint lod = 1 ; lod < 4 ; ++lod )
		{
		tri_count /= 2;

		printf( "LOD %d, looking for less or equal to %d tris\n",lod, tri_count );

		// do a binary search to find LOD level which is just below the threshold
		bool found_lod = false;
		float quant_level = 0;
		vector<uint> valid_tris;
		while( quant_shift < 31 )
			{
			quant_level = float( 1 << quant_shift ) / float( 1 << 16 );
			++quant_shift;

			valid_tris = this->QuantizeMesh( quant_level , tris_to_consider );
			if( uint( valid_tris.size() ) <= tri_count )
				{
				found_lod = true;
				break;
				}

			//printf( "LOD level %d, Quantization %f , Valid tris = %lld\n", lod, quant_level, valid_tris.size() );
			}

		if( found_lod )
			{
			// pack the valid triangles into the beginning of the remaining triangle list
			vector<uint> reorder_triangles( this->Triangles.size(), UINT_MAX );
			uint insert_index = 0;
			for( size_t t = 0; t < valid_tris.size(); ++t )
				{
				reorder_triangles[valid_tris[t]] = insert_index;
				++insert_index;
				}

			// make sure we have inserted all the triangles in the beginning
			sanity_check( insert_index == uint( valid_tris.size() ) );

			for( size_t t = 0; t < this->Triangles.size(); ++t )
				{
				if( reorder_triangles[t] == UINT_MAX )
					{
					reorder_triangles[t] = insert_index;
					++insert_index;
					}
				}

			// make sure we have inserted all the triangles
			sanity_check( insert_index == uint( this->Triangles.size() ) );

			// reorder the triangles
			this->ReorderTriangles( reorder_triangles );

			// fill in the lod info
			this->LODQuantizeDistances[lod] = quant_level;
			this->LODTriangleCounts[lod] = uint( valid_tris.size() );
			tris_to_consider = this->LODTriangleCounts[lod];

			printf( "LOD level %d, Quantization %f , Tris %d\n", lod, this->LODQuantizeDistances[lod], this->LODTriangleCounts[lod] );
			}
		else
			{
			// if no lod was found, no point in trying anymore, just use previous level values for the remaining lods and break
			for( uint i = lod ; i < 4 ; ++i )
				{
				this->LODQuantizeDistances[i] = this->LODQuantizeDistances[lod - 1];
				this->LODTriangleCounts[i] = this->LODTriangleCounts[lod - 1];
				printf( "LOD level %d, Quantization %f , Tris %d- no fewer tris possible\n", i, this->LODQuantizeDistances[i], this->LODTriangleCounts[i] );
				}
			}

		}

	//for( size_t v = this->LockedVerticesCount; v < this->Vertices.size(); ++v )
	//	{
	//	this->Vertices[v].Coords /= quant_value;
	//	this->Vertices[v].Coords.x = roundf( this->Vertices[v].Coords.x );
	//	this->Vertices[v].Coords.y = roundf( this->Vertices[v].Coords.y );
	//	this->Vertices[v].Coords.z = roundf( this->Vertices[v].Coords.z );
	//	this->Vertices[v].Coords *= quant_value;
	//	}

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
			fs << "v " << Submeshes[m].Vertices[v].Coords.x 
				<< " " << Submeshes[m].Vertices[v].Coords.y
				<< " " << Submeshes[m].Vertices[v].Coords.z
				<< std::endl;
			}

		for( size_t v = 0; v < Submeshes[m].Vertices.size(); ++v )
			{
			fs << "vt " << Submeshes[m].Vertices[v].TexCoords.x
				<< " " << Submeshes[m].Vertices[v].TexCoords.y
				<< std::endl;
			}

		for( size_t v = 0; v < Submeshes[m].Vertices.size(); ++v )
			{
			fs << "vn " << Submeshes[m].Vertices[v].Normals.x
				<< " " << Submeshes[m].Vertices[v].Normals.y
				<< " " << Submeshes[m].Vertices[v].Normals.z
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


void output_obj_lod( uint lod , const char* path )
	{
	std::ofstream fs;

	fs.open( path, std::ofstream::out );

	size_t vertex_base = 1;

	for( size_t m = 0; m < Submeshes.size(); ++m )
		{
		vector<glm::vec3> Coords( Submeshes[m].Vertices.size() );

		float quantize_distance = Submeshes[m].LODQuantizeDistances[lod];
		uint tri_count = Submeshes[m].LODTriangleCounts[lod];

		if( quantize_distance > 0.f )
			{
			// copy locked coords verbatim
			for( uint v = 0; v < Submeshes[m].LockedVerticesCount; ++v )
				{
				Coords[v] = Submeshes[m].Vertices[v].Coords;
				}

			// quantize the remaining coords
			for( uint v = Submeshes[m].LockedVerticesCount; v < uint( Submeshes[m].Vertices.size() ); ++v )
				{
				Coords[v] = glm::trunc( Submeshes[m].Vertices[v].Coords / quantize_distance ) * quantize_distance;
				}
			}
		else
			{
			for( uint v = 0; v < uint( Submeshes[m].Vertices.size() ); ++v )
				{
				Coords[v] = Submeshes[m].Vertices[v].Coords;
				}
			}

		fs << "g mesh_" << m << std::endl;

		for( size_t v = 0; v < Submeshes[m].Vertices.size(); ++v )
			{
			fs << "v " << Coords[v].x
				<< " " << Coords[v].y
				<< " " << Coords[v].z
				<< std::endl;
			}

		for( size_t v = 0; v < Submeshes[m].Vertices.size(); ++v )
			{
			fs << "vt " << Submeshes[m].Vertices[v].TexCoords.x
				<< " " << Submeshes[m].Vertices[v].TexCoords.y
				<< std::endl;
			}

		for( size_t v = 0; v < Submeshes[m].Vertices.size(); ++v )
			{
			fs << "vn " << Submeshes[m].Vertices[v].Normals.x
				<< " " << Submeshes[m].Vertices[v].Normals.y
				<< " " << Submeshes[m].Vertices[v].Normals.z
				<< std::endl;
			}

		for( size_t t = 0; t < tri_count; ++t )
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


void output_multimesh( const char* path )
	{
	Tools::Multimesh out;

	out.SubMeshes.resize( Submeshes.size() );
	for( size_t m = 0; m < Submeshes.size(); ++m )
		{
		out.SubMeshes[m].vertices = Submeshes[m].Vertices;
		out.SubMeshes[m].indices.resize( Submeshes[m].Triangles.size() * 3 );
		for( size_t t = 0; t < Submeshes[m].Triangles.size(); ++t )
			{
			out.SubMeshes[m].indices[t * 3 + 0] = Submeshes[m].Triangles[t].VertexIds[0];
			out.SubMeshes[m].indices[t * 3 + 1] = Submeshes[m].Triangles[t].VertexIds[1];
			out.SubMeshes[m].indices[t * 3 + 2] = Submeshes[m].Triangles[t].VertexIds[2];
			}
		memcpy( out.SubMeshes[m].LODQuantizeDistances, Submeshes[m].LODQuantizeDistances, sizeof( float ) * 4 );
		memcpy( out.SubMeshes[m].LODTriangleCounts, Submeshes[m].LODTriangleCounts, sizeof( uint ) * 4 );
		}

	out.calcBoundingSpheresAndRejectionCones();
	out.save( path );
	}

int main( int argc, char argv[] )
	{
	uint seed = 128767863;
	uint octaves = 18;
	const siv::PerlinNoise perlin( seed );
	srand( seed );

	for( uint q = 0; q < NumberOfMeshesToGenerate; ++q )
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

		// update all normals:
		// accumulate vertex normals
		vector<glm::vec3> global_normals;
		global_normals.resize( GlobalCoords.size() );
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
					
					Submeshes[m].Vertices[vid].Coords = GlobalCoords[cid];
					Submeshes[m].Vertices[vid].Normals = global_normals[cid];
					}
				}
			}

		for( size_t m = 0; m < Submeshes.size(); ++m )
			{
			Submeshes[m].CalculateQuantizedLODs();
			}

		//output_obj_lod( 0, "lod0.obj" );
		//output_obj_lod( 1, "lod1.obj" );
		//output_obj_lod( 2, "lod2.obj" );
		//output_obj_lod( 3, "lod3.obj" );

		char str[30];
		sprintf_s( str, "meteor_%d.mmbin", q );
		output_multimesh( str );

		GlobalCoords.clear();
		GlobalCoordsMap.clear();
		Submeshes.clear();
		}

	return EXIT_SUCCESS;
	}
