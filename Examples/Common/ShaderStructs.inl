
// if in C++: 
// use a namespace and using/typedefs so that we can reuse the code in shader code
#ifdef __cplusplus
namespace ShaderStructs {
using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;
using glm::uint;
#endif

// the object struct keeps all information about an object in the scene. 
// an object is an instance of a specific mesh at a specific transformation
// each frame, if an object is deemed visible, a specific instance is 
// assigned to render the object in a specific batch. there are multiple 
// batches that can render the same object, because of LODing, or using
// different materials.
struct Object
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
	uint _b1;

	uint LODQuantizations[4];
	};

// an instance is a temporary object used only during the rendering of 
// a frame. it gets assigned an object, and other data which is instance-
// specific, such as vertex quantization level, which is part of the LODing
struct Instance
	{
	uint objectID; // object to render
	uint quantizationMask;
	uint quantizationRound;
	uint _b1;
	};

// a mesh is the source mesh, which is used by multiple objects. there can
// be multiple submeshes that are part of the same mesh.
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
struct Batch
	{
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	int vertexOffset;
	uint firstInstance;
	};

// mark end of c++ block
#ifdef __cplusplus
};
#endif
