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

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

//layout(set = 0, binding = 1) uniform sampler2D texSampler;

void main() 
	{
	//vec3 N = normalize(fragNormal);
	//vec3 base_color = texture(texSampler,fragTexCoord).rgb;
	//vec3 base_color = vec3(fragTexCoord.x,0,fragTexCoord.y);
	//outColor = vec4((N*0.5+0.5)*base_color,1); 
	//outColor = vec4(base_color,1); 
	outColor = vec4(fragColor,1); 
	}