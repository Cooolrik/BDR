#version 460
precision highp float;
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() 
	{
	outColor = vec4( fragColor , 1 );
	}