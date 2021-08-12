#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(set = 0, binding = 2) readonly buffer SceneObjects { SceneObject i[]; } sceneObjects;

layout(location = 0) in vec4 inPositionU;
layout(location = 1) in vec4 inNormalV;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 viewDir;
layout(location = 3) out vec3 worldPos;
layout(location = 4) flat out uint materialID;

// per-instance information
layout(set = 0, binding = 3) readonly buffer InstanceBuffer
	{   
	Instance instances[];
	} instanceBuffer;

void main() 
	{
	vec3 origin = vec3(ubo.viewI * vec4(0, 0, 0, 1));

	uint objectID = instanceBuffer.instances[gl_InstanceIndex].objectID;
	float quantization = instanceBuffer.instances[gl_InstanceIndex].quantization;

	vec3 quantizedPos;
	if( gl_VertexIndex > sceneObjects.i[objectID].vertexCutoffIndex && quantization > 0 )
		quantizedPos = trunc(inPositionU.xyz / quantization) * quantization;
	else
		quantizedPos = inPositionU.xyz;
	
	worldPos = vec3(sceneObjects.i[objectID].transform * vec4(quantizedPos, 1.0));
	fragNormal = vec3(sceneObjects.i[objectID].transformIT * vec4(inNormalV.xyz, 0.0));

	viewDir = vec3(worldPos - origin);
	fragTexCoord = vec2(inPositionU.w,inNormalV.w);

	materialID = sceneObjects.i[objectID].materialID;
	
	gl_Position = ubo.proj * ubo.view * vec4(worldPos, 1.0);
	}
