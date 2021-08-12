layout(set = 0 , binding = 0) uniform UniformBufferObject 
	{
	mat4 view;
	mat4 proj;
	mat4 viewI;
	mat4 projI;
	vec3 viewPosition; // origin of the view in world space
	} ubo;

struct SceneObject
	{
    mat4 transform;
    mat4 transformIT;
	vec4 boundingSphere;
	vec4 rejectionConeDirectionAndCutoff;
	vec3 rejectionConeCenter;
    uint geometryID;
    uint materialID;
	uint batchID;
	uint vertexCutoffIndex;
	float LODQuantizations[4];
	};

struct Instance
	{
	uint objectID; // object to render
	float quantization; // quantization distance 
	};
