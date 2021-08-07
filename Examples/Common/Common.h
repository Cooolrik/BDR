#pragma once

// disable warning for "enum class" since we can't modify Vulcan SDK
#pragma warning( disable : 26812 4100 4201 4127 4189 )

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <vector>

typedef unsigned int uint;
using std::vector;

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GetConstMacro( type , name ) const type Get##name() const { return this->name; }

// inclusive random, value can be either mini or maxi
inline int irand( int mini, int maxi )
	{
	// get a large random value and cap it
	uint delta = ( maxi - mini + 1 );
	uint rv = rand() + rand();
	rv *= rand();
	rv *= rand();
	rv %= delta;

	// add it to mini and return
	return mini + rv;
	}

// inclusive random, value can be either mini or maxi
inline float frand( float minv = 0.f, float maxv = 1.f )
	{
	return ( ( (float)rand() / (float)RAND_MAX ) * ( maxv - minv ) ) + minv;
	}

inline float frand_radian()
	{
	return frand( 0, glm::pi<float>() * 2.f );
	}

inline glm::vec3 vrand( float minv = -1.f, float maxv = 1.f )
	{
	return glm::vec3( frand( minv, maxv ), frand( minv, maxv ), frand( minv, maxv ) );
	}

inline glm::vec3 vrand( glm::vec3 minv, glm::vec3 maxv )
	{
	return glm::vec3( frand( minv.x, maxv.x ), frand( minv.y, maxv.y ), frand( minv.z, maxv.z ) );
	}

inline glm::vec3 vrand_unit()
	{
	glm::vec3 v = vrand();
	return glm::normalize( v );
	}
