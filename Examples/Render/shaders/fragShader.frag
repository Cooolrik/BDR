#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 viewDir;
layout(location = 3) in vec3 worldPos;
layout(location = 4) in flat uint materialID;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D texSamplers[256];

// Converts a color from sRGB gamma to linear light gamma
vec4 toLinear(vec4 sRGB)
	{
    bvec4 cutoff = lessThan(sRGB, vec4(0.04045));
    vec4 higher = pow((sRGB + vec4(0.055))/vec4(1.055), vec4(2.4));
    vec4 lower = sRGB/vec4(12.92);
    return mix(higher, lower, cutoff);
	}

mat3 compute_tangent_frame()
	{
	// get edge vectors
    vec3 dp1 = dFdx( worldPos );
    vec3 dp2 = dFdy( worldPos );
    vec2 duv1 = dFdx( fragTexCoord );
    vec2 duv2 = dFdy( fragTexCoord );

	// solve the linear system
    vec3 dp2perp = cross( dp2, fragNormal );
    vec3 dp1perp = cross( fragNormal, dp1 );
    vec3 tangent = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 binormal = dp2perp * duv1.y + dp1perp * duv2.y;

	// construct a scale-invariant frame 
    float invmax = inversesqrt( max( dot(tangent,tangent), dot(binormal,binormal) ) );
    return mat3( tangent * invmax, binormal * invmax, fragNormal );
	}

void main() 
	{
	vec3 N = normalize(fragNormal);
	vec3 V = normalize(viewDir);

	mat3 TBN = compute_tangent_frame();

	vec3 normal =  toLinear(texture(texSamplers[0],fragTexCoord*20.f)).rgb;

	normal = normalize(TBN * normal);

	float light = dot(normal,normalize(vec3(0.7f,0.7f,0.7f))) / 2 + 0.5; 
	vec3 base_color = toLinear(texture(texSamplers[materialID],fragTexCoord)).rgb;
	
	outColor = vec4(light * base_color , 1 );

	}