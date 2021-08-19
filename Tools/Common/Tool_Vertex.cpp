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

inline glm::uint octEncodeNormal( const glm::vec3& normal )
	{
	const float invL1Norm = ( 1.f ) / ( fabs( normal[0] ) + fabs( normal[1] ) + fabs( normal[2] ) );
	uint32_t ret;
	int16_t* encoded = (int16_t*)( &ret );

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

	// TODO: Add additional code to test possible encodings

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

	ret.Normals = octEncodeNormal( this->Normals );

	ret.TexCoords = glm::packHalf2x16( this->TexCoords );

	return ret;
	}
