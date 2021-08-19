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
    uint meshID;
    uint materialID;
	uint batchID;
	uint vertexCutoffIndex;
	uint LODQuantizations[4];
	};

struct Instance
	{
	uint objectID; // object to render
	uint quantizationMask; 
	uint quantizationRound; 
	};

struct Mesh
	{
	vec3 CompressedVertexScale; 
	uint _b1; 
	vec3 CompressedVertexTranslate; 
	uint _b2;
	};
	
// the draw command layout for the indirect rendering call
// most of the command is already filled out, we only add to the
// instance count if an object is deemed visible
struct DrawCommand
	{
	uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;
    uint firstInstance;
	};
	