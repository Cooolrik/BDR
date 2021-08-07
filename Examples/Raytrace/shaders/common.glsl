layout(set = 0 , binding = 0) uniform UniformBufferObject 
	{
	mat4 view;
	mat4 proj;
	mat4 viewI;
	mat4 projI;
	vec3 lightPosition;
	} ubo;

layout(set = 0, binding = 1) uniform sampler2D texSampler;
