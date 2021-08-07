#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

layout(set = 0 , binding = 0) uniform UniformBufferObject 
	{
	mat4 view;
	mat4 proj;
	mat4 viewI;
	mat4 projI;
	} ubo;

layout(location = 0) in vec4 inPositionU;
layout(location = 1) in vec4 inNormalV;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragColor;

void main() 
	{
	fragTexCoord = vec2(inPositionU.w,inNormalV.w);
	fragColor = inNormalV.xyz;
	gl_Position = ubo.proj * ubo.view * vec4(inPositionU.xyz, 1.0);
	}
