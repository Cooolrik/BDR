#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "../../Common/ShaderCommon.inl"

layout(std140, set = 0 , binding = 0) uniform SceneRenderUBO 
	{
	mat4 view;
	mat4 proj;
	mat4 viewI;
	mat4 projI;
	vec3 viewPosition; // origin of the view in world space
	float _b1;
	} Scene;

layout( push_constant ) uniform constants
	{
	vec3 CompressedVertexScale;
	uint quantizationMask;
	vec3 CompressedVertexTranslate;
	uint quantizationRound;
	vec3 Color;
	uint materialID;
	uint vertexCutoffIndex;
	} Object;

layout(location = 0) in uvec4 inCoords;
layout(location = 1) in vec2 inNormals;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 viewDir;
layout(location = 3) out vec3 worldPos;
layout(location = 4) out vec3 color;
layout(location = 5) flat out uint materialID;

void main() 
	{
	vec3 origin = vec3(Scene.viewI * vec4(0, 0, 0, 1));
	
	// quantize position 
	uvec3 quantPos = inCoords.xyz;
	if( gl_VertexIndex >= Object.vertexCutoffIndex )
		{
		quantPos &= Object.quantizationMask;
		quantPos |= Object.quantizationRound;
		}
	vec3 vertexPos = ( vec3(quantPos) * Object.CompressedVertexScale) + Object.CompressedVertexTranslate;
			
	// transfer to world coords
	worldPos = vertexPos; //vec3(Scene.transform * vec4(vertexPos, 1.0));
	fragNormal = vec3( /*Scene.transformIT **/ vec4( decodeNormals(inNormals) , 0.0));

	vec3 viewPos = (Scene.view * vec4(worldPos, 1.0)).xyz;

	viewDir = -viewPos;
	fragTexCoord = vec2(inTexCoords);

	materialID = Object.materialID;
	
	color = Object.Color;

	gl_Position = Scene.proj * Scene.view * vec4(worldPos, 1.0);
	}
