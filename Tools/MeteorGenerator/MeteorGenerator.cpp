
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
using Tools::Compressed16Vertex;

const uint PlaneQuadTesselation = 63; // x*x*2 = number of tris per quad
const uint MeshTesselation = 4;
const uint NumberOfMeshesToGenerate = 6;
const bool LockBorderVertices = true;
const bool SaveUncompressedVertices = true;
const bool RandomizeScale = true;
const bool DeformSurface = true;

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
		vector<Compressed16Vertex> CompressedVertices = {};
		std::unordered_map<Vertex, uint> VerticesMap = {};
		vector<Triangle> Triangles = {};
		uint LockedVerticesCount = 0;

		uint LODTriangleCounts[4] = {};
		uint LODQuantizeBits[4] = {};

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
		vector<uint> QuantizeMesh( uint border_quantize_bits, uint quantize_bits , uint tris_to_consider = UINT_MAX );

		void CalculateQuantizedLODs();
	};

vector<glm::vec3> GlobalCoords{};
std::unordered_map<glm::vec3, uint> GlobalCoordsMap{};
vector<Submesh> Submeshes;

float CompressedVertexScale;
glm::vec3 CompressedVertexTranslate;

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
				verts[i].Normal = glm::vec4( 0 );
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
	vector<Compressed16Vertex> original_compressed_vertices = this->CompressedVertices;

	for( size_t v = 0 ; v < original_vertices.size() ; ++v )
		{
		this->Vertices[new_vertex_location[v]] = original_vertices[v];
		}

	for( size_t v = 0 ; v < original_compressed_vertices.size() ; ++v )
		{
		this->CompressedVertices[new_vertex_location[v]] = original_compressed_vertices[v];
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

vector<uint> Submesh::QuantizeMesh( uint border_quantize_bits, uint quantize_bits, uint tris_to_consider )
	{
	vector<glm::u16vec3> Coords( this->CompressedVertices.size() );
	vector<uint> ret;

	if( tris_to_consider == UINT_MAX )
		tris_to_consider = uint(this->Triangles.size());

	// copy locked coords verbatim
	for( uint v = 0; v < this->LockedVerticesCount; ++v )
		{
		Coords[v].x = uint16_t( this->CompressedVertices[v].Coords[0] );
		Coords[v].y = uint16_t( this->CompressedVertices[v].Coords[1] );
		Coords[v].z = uint16_t( this->CompressedVertices[v].Coords[2] );
		}

	// quantize the remaining coords
	uint quantize_mask = 0xffffffff << quantize_bits; // shift up mask as many bits as to remove
	uint quantize_mask_round = ( 0x1 << quantize_bits ) >> 1;
	for( uint v = this->LockedVerticesCount; v < uint(this->CompressedVertices.size()); ++v )
		{
		Coords[v].x = uint16_t( (this->CompressedVertices[v].Coords[0] & quantize_mask) | quantize_mask_round );
		Coords[v].y = uint16_t( (this->CompressedVertices[v].Coords[1] & quantize_mask) | quantize_mask_round );
		Coords[v].z = uint16_t( (this->CompressedVertices[v].Coords[2] & quantize_mask) | quantize_mask_round );
		}

	// now, check all the triangles that we are to consider
	ret.reserve( tris_to_consider );
	for( size_t t = 0 ; t < tris_to_consider ; ++t )
		{
		glm::u16vec3 v0 = Coords[this->Triangles[t].VertexIds[0]];
		glm::u16vec3 v1 = Coords[this->Triangles[t].VertexIds[1]];
		glm::u16vec3 v2 = Coords[this->Triangles[t].VertexIds[2]];

		if( v0 == v1 || v1 == v2 || v2 == v0 )
			{
			continue;
			}

		//// check if this triangle (with these vertices) already exist in the list 
		//// of valid tris. it this case, this tri is redundant
		//bool found_match = false;
		//for( size_t q = 0; q < ret.size(); ++q )
		//	{
		//	glm::u16vec3 qv0 = Coords[this->Triangles[ret[q]].VertexIds[0]];
		//	glm::u16vec3 qv1 = Coords[this->Triangles[ret[q]].VertexIds[1]];
		//	glm::u16vec3 qv2 = Coords[this->Triangles[ret[q]].VertexIds[2]];		
		//
		//	if( (qv0 == v0 && qv1 == v1 && qv2 == v2) ||
		//		(qv0 == v1 && qv1 == v2 && qv2 == v0) ||
		//		(qv0 == v2 && qv1 == v0 && qv2 == v1) )
		//		{
		//		found_match = true;
		//		break;
		//		}
		//	}
		//if( found_match )
		//	{
		//	continue;
		//	}

		// this triangle still has some size, keep it in the list
		ret.emplace_back( uint(t) );
		}

	return ret;
	}

void Submesh::CalculateQuantizedLODs()
	{
	vector<TriangleNeighbours> tri_neighbours = this->GetNeighbours();
	vector<bool> lock_vertices( this->Vertices.size() , false );

	// find all edge vertices and mark as locked
	if( LockBorderVertices )
		{
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
		}

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
	this->LODQuantizeBits[0] = 0; // no quantization

	// for the lod levels [1 -> 3], do quantization, to reduce tris by 50% each step (so 100%, 50%, 25%, 12.5%)
	uint tri_count = LODTriangleCounts[0];
	uint quant_shift = 0;
	uint border_quant_shift = 0;
	uint tris_to_consider = LODTriangleCounts[0];
	for( uint lod = 1 ; lod < 4 ; ++lod )
		{
		tri_count /= 2;

		printf( "LOD %d, looking for less or equal to %d tris\n",lod, tri_count );

		// do a binary search to find LOD level which is just below the threshold
		bool found_lod = false;
		vector<uint> valid_tris;
		while( quant_shift < 15 )
			{
			valid_tris = this->QuantizeMesh( border_quant_shift , quant_shift, tris_to_consider );
			++quant_shift;
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
			this->LODQuantizeBits[lod] = quant_shift;
			this->LODTriangleCounts[lod] = uint( valid_tris.size() );
			tris_to_consider = this->LODTriangleCounts[lod];

			printf( "LOD level %d, Quantization %d , Tris %d\n", lod, this->LODQuantizeBits[lod], this->LODTriangleCounts[lod] );
			}
		else
			{
			// if no lod was found, no point in trying anymore, just use previous level values for the remaining lods and break
			for( uint i = lod ; i < 4 ; ++i )
				{
				this->LODQuantizeBits[i] = this->LODQuantizeBits[lod - 1];
				this->LODTriangleCounts[i] = this->LODTriangleCounts[lod - 1];
				printf( "LOD level %d, Quantization %d , Tris %d- no fewer tris possible\n", i, this->LODQuantizeBits[i], this->LODTriangleCounts[i] );
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
			fs << "vn " << Submeshes[m].Vertices[v].Normal.x
				<< " " << Submeshes[m].Vertices[v].Normal.y
				<< " " << Submeshes[m].Vertices[v].Normal.z
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
		vector<glm::u16vec3> CompressedCoords( Submeshes[m].Vertices.size() );
		
		uint quantize_bits = Submeshes[m].LODQuantizeBits[lod];
		uint tri_count = Submeshes[m].LODTriangleCounts[lod];

		// copy locked coords verbatim
		for( uint v = 0; v < Submeshes[m].LockedVerticesCount; ++v )
			{
			CompressedCoords[v].x = uint16_t( Submeshes[m].CompressedVertices[v].Coords[0] );
			CompressedCoords[v].y = uint16_t( Submeshes[m].CompressedVertices[v].Coords[1] );
			CompressedCoords[v].z = uint16_t( Submeshes[m].CompressedVertices[v].Coords[2] );
			}

		// quantize the remaining coords
		uint quantize_mask = 0xffffffff << quantize_bits; // shift up mask as many bits as to remove
		uint quantize_mask_round = (0x1 << quantize_bits) >> 1;
		for( uint v = Submeshes[m].LockedVerticesCount; v < uint( Submeshes[m].CompressedVertices.size() ); ++v )
			{
			CompressedCoords[v].x = uint16_t( (Submeshes[m].CompressedVertices[v].Coords[0] & quantize_mask) | quantize_mask_round );
			CompressedCoords[v].y = uint16_t( (Submeshes[m].CompressedVertices[v].Coords[1] & quantize_mask) | quantize_mask_round );
			CompressedCoords[v].z = uint16_t( (Submeshes[m].CompressedVertices[v].Coords[2] & quantize_mask) | quantize_mask_round );
			}

		fs << "g mesh_" << m << std::endl;

		for( size_t v = 0; v < Submeshes[m].Vertices.size(); ++v )
			{
			glm::vec3 trcoord = ( glm::vec3( CompressedCoords[v] ) * CompressedVertexScale ) + CompressedVertexTranslate;

			fs << "v " << trcoord.x
				<< " " << trcoord.y
				<< " " << trcoord.z
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
			fs << "vn " << Submeshes[m].Vertices[v].Normal.x
				<< " " << Submeshes[m].Vertices[v].Normal.y
				<< " " << Submeshes[m].Vertices[v].Normal.z
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
		Tools::Multimesh::Submesh& outmesh = out.SubMeshes[m];

		outmesh.VertexOffset = (uint)out.Vertices.size();
		outmesh.VertexCount = (uint)Submeshes[m].Vertices.size();

		outmesh.IndexOffset = (uint)out.Indices.size();
		outmesh.IndexCount = (uint)Submeshes[m].Triangles.size() * 3;

		// append all vertices and indices to the main list in the multimesh
		out.Vertices.insert( out.Vertices.end(), Submeshes[m].Vertices.begin(), Submeshes[m].Vertices.end() );
		out.CompressedVertices.insert( out.CompressedVertices.end(), Submeshes[m].CompressedVertices.begin(), Submeshes[m].CompressedVertices.end() );

		// add the triangles into the index list
		out.Indices.resize( (size_t)outmesh.IndexOffset + (size_t)outmesh.IndexCount );
		for( size_t t = 0; t < Submeshes[m].Triangles.size(); ++t )
			{
			out.Indices[outmesh.IndexOffset + (t * 3 + 0)] = Submeshes[m].Triangles[t].VertexIds[0];
			out.Indices[outmesh.IndexOffset + (t * 3 + 1)] = Submeshes[m].Triangles[t].VertexIds[1];
			out.Indices[outmesh.IndexOffset + (t * 3 + 2)] = Submeshes[m].Triangles[t].VertexIds[2];
			}

		out.SubMeshes[m].LockedVertexCount = Submeshes[m].LockedVerticesCount;
		for( uint lod = 0; lod < 4; ++lod )
			{
			out.SubMeshes[m].LODQuantizeBits[lod] = Submeshes[m].LODQuantizeBits[lod];
			out.SubMeshes[m].LODIndexCounts[lod] = Submeshes[m].LODTriangleCounts[lod] * 3;
			}
		}

	out.CalcBoundingVolumesAndRejectionCones();

	out.CompressedVertexScale = CompressedVertexScale;
	out.CompressedVertexTranslate = CompressedVertexTranslate;

	// dont save the uncompressed vertices
	if( !SaveUncompressedVertices )
		{
		out.Vertices.clear();
		}

	out.Save( path );
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

		glm::vec3 MeshScale( 1, 1, 1 );
		if( RandomizeScale )
			MeshScale.z = frand( 0.5, 3.0 );

		// deform mesh 
		if( DeformSurface )
			{
			for( size_t c = 0 ; c < GlobalCoords.size(); ++c )
				{
				GlobalCoords[c] = glm::normalize( GlobalCoords[c] );
				GlobalCoords[c] *= MeshScale;
				GlobalCoords[c] *= 1.0 + perlin.accumulatedOctaveNoise3D_0_1( GlobalCoords[c].x, GlobalCoords[c].y, GlobalCoords[c].z, octaves );

				// update aabb
				minv = glm::vec3(
					min( minv.x, GlobalCoords[c].x ),
					min( minv.y, GlobalCoords[c].y ),
					min( minv.z, GlobalCoords[c].z )
				);
				maxv = glm::vec3(
					max( maxv.x, GlobalCoords[c].x ),
					max( maxv.y, GlobalCoords[c].y ),
					max( maxv.z, GlobalCoords[c].z )
				);
				}
			}

		glm::vec3 delta = (maxv - minv);
		float longest_dimension = max(delta.x,max(delta.y,delta.z));
		
		CompressedVertexScale = longest_dimension / float( 0xffff ); // 0->0xffff maps to longest dimension
		CompressedVertexTranslate = minv;

		// update all normals:
		// accumulate vertex normals
		vector<glm::vec3> global_normals( GlobalCoords.size() );
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

		// create compressed 3d coords and normals
		vector<Tools::Compressed16Vertex> compressed_vertices( GlobalCoords.size() );
		for( size_t v = 0 ; v < GlobalCoords.size(); ++v )
			{
			Tools::Vertex vert;

			vert.Coords = GlobalCoords[v];
			vert.Normal = global_normals[v];
			vert.TexCoords = glm::vec2( 0 ); // we will pack uvs separately

			compressed_vertices[v] = vert.Compress( CompressedVertexScale , CompressedVertexTranslate );
			}

		// update coords and normals in vertices
		for( size_t m = 0; m < Submeshes.size(); ++m )
			{
			Submeshes[m].CompressedVertices.resize( Submeshes[m].Vertices.size() );
			for( size_t t = 0; t < Submeshes[m].Triangles.size(); ++t )
				{
				for( size_t v = 0 ; v < 3; ++v )
					{
					uint cid = Submeshes[m].Triangles[t].GlobalCoordIds[v];
					uint vid = Submeshes[m].Triangles[t].VertexIds[v];
					
					Submeshes[m].Vertices[vid].Coords = GlobalCoords[cid];
					Submeshes[m].Vertices[vid].Normal = global_normals[cid];

					// copy compressed vertex from global vertex, but replace texcoords from local vertex
					Submeshes[m].CompressedVertices[vid] = compressed_vertices[cid];
					Submeshes[m].CompressedVertices[vid].TexCoords = glm::packHalf2x16( Submeshes[m].Vertices[vid].TexCoords );
					}
				}
			}

		for( size_t m = 0; m < Submeshes.size(); ++m )
			{
			Submeshes[m].CalculateQuantizedLODs();
			}

		// debug output lods
#ifdef _DEBUG
		for( uint i = 0; i < 4; ++i )
			{
			char str[30];
			sprintf_s( str, "lod%d.obj", i );
			output_obj_lod( i, str );
			}
#endif

		char str[MAX_PATH];
		sprintf_s( str, "../../Examples/Assets/meteor_%d.mmbin", q );
		output_multimesh( str );

		GlobalCoords.clear();
		GlobalCoordsMap.clear();
		Submeshes.clear();
		}

	return EXIT_SUCCESS;
	}
