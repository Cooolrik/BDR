#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

//#extension GL_AMD_gpu_shader_half_float: require

#include "common.glsl"

layout(set = 0, binding = 2) readonly buffer SceneObjects { SceneObject i[]; } sceneObjects;

layout(location = 0) in uvec4 inCoords;
layout(location = 1) in vec2 inNormals;
layout(location = 2) in vec2 inTexCoords;

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

// per-mesh information
layout(set = 0, binding = 4) readonly buffer MeshBuffer
	{   
	Mesh meshes[];
	} meshBuffer;

// oct encoded normals
vec3 decodeNormals( vec2 inpacked )
	{
    vec3 n = vec3( inpacked.x, inpacked.y, 1.f - abs( inpacked.x ) - abs( inpacked.y ) );
    float t = max( -n.z , 0.f );
	n.x += (n.x>0.f)?-t:t;
    n.y += (n.y>0.f)?-t:t;
    return normalize( n );
	}

void main() 
	{
	vec3 origin = vec3(ubo.viewI * vec4(0, 0, 0, 1));

	uint objectID = instanceBuffer.instances[gl_InstanceIndex].objectID;
	uint meshID = sceneObjects.i[objectID].meshID;

	// quantize position 
	uvec3 quantPos = inCoords.xyz;
	if( gl_VertexIndex >= sceneObjects.i[objectID].vertexCutoffIndex )
		{
		quantPos &= instanceBuffer.instances[gl_InstanceIndex].quantizationMask;
		quantPos |= instanceBuffer.instances[gl_InstanceIndex].quantizationRound;
		}
	vec3 vertexPos = ( vec3(quantPos) * meshBuffer.meshes[meshID].CompressedVertexScale) + meshBuffer.meshes[meshID].CompressedVertexTranslate;
			
	// transfer to world coords
	worldPos = vec3(sceneObjects.i[objectID].transform * vec4(vertexPos, 1.0));
	fragNormal = vec3(sceneObjects.i[objectID].transformIT * vec4( decodeNormals(inNormals) , 0.0));

	viewDir = vec3(worldPos - origin);
	fragTexCoord = vec2(inTexCoords);

	materialID = sceneObjects.i[objectID].materialID;
	
	gl_Position = ubo.proj * ubo.view * vec4(worldPos, 1.0);
	}
