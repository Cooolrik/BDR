#include "Tool_Vertex.h"

typedef uint32_t uint;
using std::vector;

inline float clamp( float val, float low, float hi )
	{
	if( val <= low )
		return low;
	if( val >= hi )
		return hi;
	return val;
	}

inline int16_t normalizedFloatToInt16( float val )
	{
	val = clamp( val, -1.f, 1.f );
	val *= 32767.f;
	val = roundf( val );
	return (int16_t)val;
	}

inline float signLessThanZero( float val )
	{
	return ( val < 0.f ) ? -1.0f : 1.0f;
	}

inline float int16ToNormalizedFloat( int16_t val )
	{
	float fval = float( val );
	fval /= 32767.f;
	fval = clamp( fval, -1.f, 1.f );
	return fval;
	}

inline glm::vec3 octDecodeNormal( const int16_t encoded[2] )
	{
	float x = int16ToNormalizedFloat( encoded[0] );
	float y = int16ToNormalizedFloat( encoded[1] );
	glm::vec3 n = glm::vec3( x, y, 1.f - fabs( x ) - fabs( y ) );
	float t = glm::max( -n.z, 0.f );
	n.x += ( n.x > 0.f ) ? -t : t;
	n.y += ( n.y > 0.f ) ? -t : t;
	return glm::normalize( n );
	}

inline glm::uint octEncodeNormal( const glm::vec3& normal )
	{
	const float invL1Norm = ( 1.f ) / ( fabs( normal[0] ) + fabs( normal[1] ) + fabs( normal[2] ) );
	int16_t encoded[2];

	if( normal[2] < 0.f )
		{
		encoded[0] = normalizedFloatToInt16( ( 1.f - fabs( normal[1] * invL1Norm ) ) * signLessThanZero( normal[0] ) );
		encoded[1] = normalizedFloatToInt16( ( 1.f - fabs( normal[0] * invL1Norm ) ) * signLessThanZero( normal[1] ) );
		}
	else
		{
		encoded[0] = normalizedFloatToInt16( normal[0] * invL1Norm );
		encoded[1] = normalizedFloatToInt16( normal[1] * invL1Norm );
		}

	// try a few encodning variants in the vicinity of the encoded vector
	// which might give a better result than the found vector
	uint32_t ret;
	int16_t* best_encoded = (int16_t*)( &ret );
	float best_error = FLT_MAX; 
	for( int16_t i = -1; i < 2; ++i )
		{
		for( int16_t j = -1; j < 2; ++j )
			{
			int16_t test_round[2] = { encoded[0] + i , encoded[1] + j };
			glm::vec3 test_normal = octDecodeNormal( test_round );

			// check the cos(angle) between the original normal, and the (decoded) encoded normal 
			// since we are encoding direction vectors, we want to get as close to 1 as possible
			float test_error = fabs( 1.f - glm::dot( test_normal, normal ) );
			if( test_error < best_error )
				{
				best_error = test_error;
				best_encoded[0] = test_round[0];
				best_encoded[1] = test_round[1];
				}
			}
		}

	return ret;
	}

inline uint16_t floatToUint16( float& val )
	{
	int rval = int(trunc( val ));
	if( rval < 0 )
		rval = 0;
	if( rval > 0xffff )
		rval = 0xffff;
	return uint16_t(rval);
	}

Tools::Compressed16Vertex Tools::Vertex::Compress( glm::vec3& scale, glm::vec3& translate )
	{
	Compressed16Vertex ret;

	glm::vec3 coords = ( this->Coords - translate ) / scale;

	ret.Coords[0] = floatToUint16( coords.x );
	ret.Coords[1] = floatToUint16( coords.y );
	ret.Coords[2] = floatToUint16( coords.z );
	
	ret._unnused = 0;

	ret.Normal = octEncodeNormal( this->Normal );

	ret.TexCoords = glm::packHalf2x16( this->TexCoords );

	return ret;
	}
