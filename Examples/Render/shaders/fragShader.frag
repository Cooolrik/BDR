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

void main() 
	{
	vec3 N = normalize(fragNormal);

	float light = (dot(N,vec3(0.7f,0.7f,0.7f)) + 2.f)/3.f;

	vec3 base_color = texture(texSamplers[materialID],fragTexCoord).rgb;
	//vec3 base_color = vec3(fragTexCoord.x,0,fragTexCoord.y);
	//
	//uint r = (gl_PrimitiveID >> 6) & 0xff;
	//uint g = (materialID ) & 0xff;
	//uint b = (gl_PrimitiveID << 2) & 0xff;
	//
	//outColor = vec4( float(r/256.0), float(g/256.0), float(b/256.0), 1.0 );

	outColor = vec4(light*base_color,1); 
	//outColor = vec4(base_color,1); 
	}