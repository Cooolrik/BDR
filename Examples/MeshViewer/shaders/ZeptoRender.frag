#version 460
precision highp float;
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 viewDir;
layout(location = 3) in vec3 worldPos;
layout(location = 4) in vec3 color;
layout(location = 5) in flat uint materialID;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D texSamplers[256];

mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv)
	{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );
 
    // solve the linear system
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
    // construct a scale-invariant frame 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( T * invmax, B * invmax, N );
	}

void main() 
	{
	vec3 N = normalize(fragNormal);
	vec3 V = normalize(viewDir);

	mat3 TBN = cotangent_frame( N , viewDir, fragTexCoord );

	//outColor = vec4( vec3( dFdx(fragTexCoord) , 0 ) , 1 );//vec4( TBN[0] , 1 ); 

	vec3 normal =  vec3(0,0,1); //normalize(texture(texSamplers[materialID],fragTexCoord).rgb);
	//
	normal = normalize(TBN * normal);
	//
	float light = dot(normal,normalize(vec3(0.7f,0.7f,0.7f))) / 2 + 0.5; 
	//vec3 base_color = texture(texSamplers[materialID],fragTexCoord).rgb;
	//vec3 base_color = vec3(fragTexCoord.x,0,fragTexCoord.y);
	//
	//uint r = (gl_PrimitiveID >> 6) & 0xff;
	//uint g = (materialID ) & 0xff;
	//uint b = (gl_PrimitiveID << 2) & 0xff;
	//
	//outColor = vec4( float(r/256.0), float(g/256.0), float(b/256.0), 1.0 );

	//outColor = vec4((TBN[2]/2.f)+0.5f,1); 
	outColor = vec4(light*color,1); 
	//outColor = vec4(base_color,1); 


	//outColor = vec4( dFdxCoarse( viewDir ) , 1 ); 

	}