#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

layout(std140, set = 0 , binding = 0) uniform SceneUniforms 
	{
	mat4 View;
	mat4 Proj;
	mat4 ViewI;
	mat4 ProjI;
	vec3 ViewPosition; // origin of the view in world space
	} Scene;

layout( push_constant ) uniform PushConstants
	{
	mat4 Transform;
	vec3 Color;
	} Object;

layout(location = 0) in vec3 Coords;
layout(location = 1) in vec3 Color;

layout(location = 0) out vec3 fragColor;

void main() 
	{
	vec3 worldPos = (Object.Transform * vec4(Coords, 1)).xyz;
	gl_Position = Scene.Proj * Scene.View * vec4(worldPos, 1.0);
	fragColor = Color * Object.Color;
	}
