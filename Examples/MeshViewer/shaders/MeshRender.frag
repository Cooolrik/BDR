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
layout(location = 5) flat in uint materialID;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D texSamplers[16];

// Converts a color from linear light gamma to sRGB gamma
vec4 fromLinear(vec4 linearRGB)
{
    bvec4 cutoff = lessThan(linearRGB, vec4(0.0031308));
    vec4 higher = vec4(1.055)*pow(linearRGB, vec4(1.0/2.4)) - vec4(0.055);
    vec4 lower = linearRGB * vec4(12.92);

    return mix(higher, lower, cutoff);
}

// Converts a color from sRGB gamma to linear light gamma
vec4 toLinear(vec4 sRGB)
{
    bvec4 cutoff = lessThan(sRGB, vec4(0.04045));
    vec4 higher = pow((sRGB + vec4(0.055))/vec4(1.055), vec4(2.4));
    vec4 lower = sRGB/vec4(12.92);

    return mix(higher, lower, cutoff);
}

mat3 computeTBN()
	{
    vec3 dp1 = dFdx( worldPos );
    vec3 dp2 = dFdy( worldPos );
    vec2 duv1 = dFdx( fragTexCoord );
    vec2 duv2 = dFdy( fragTexCoord );
    vec3 dp2perp = cross( dp2, fragNormal );
    vec3 dp1perp = cross( fragNormal, dp1 );
    vec3 tangent = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 binormal = dp2perp * duv1.y + dp1perp * duv2.y;
    float invmax = inversesqrt( max( dot(tangent,tangent), dot(binormal,binormal) ) );
    return mat3( tangent * invmax, binormal * invmax, fragNormal );
	}

void main() 
	{
	//vec2 uv = vec2(gl_FragCoord.x/800,gl_FragCoord.y/800);

	mat3 TBN = computeTBN();

	vec3 normal = toLinear(texture(texSamplers[0],fragTexCoord * 20.f)).rgb;

	//if( (int(gl_FragCoord.y)/10)%2 == 0 && gl_FragCoord.x > 400)
	//	outColor.x = 1;

	normal = normalize(TBN * normal);
	outColor = vec4(normal , 1 );

	//vec3 N = normalize(fragNormal);
	//vec3 V = normalize(viewDir);
	//
 	//
	////outColor = vec4( TBN[1] , 1 );
	////outColor = vec4( fragTexCoord , 0 , 1 );
	//
	//////outColor = vec4( vec3( dFdx(fragTexCoord) , 0 ) , 1 );//vec4( TBN[0] , 1 ); 
	////
	//
	//vec3 normal = vec3(fragTexCoord.x);
	//
	////(pow(,vec3(2.2f)) * 2) - 0.5;
	//////
	//	
	////normal = normalize(TBN * normal);
	//
	//outColor = vec4(normal , 1 );
	//
	//
	//
	//
	//
	//
	//////
	//float light = dot(normal,normalize(vec3(0,1,0))); 
	//////vec3 base_color = texture(texSamplers[materialID],fragTexCoord).rgb;
	//////vec3 base_color = vec3(fragTexCoord.x,0,fragTexCoord.y);
	//////
	//////uint r = (gl_PrimitiveID >> 6) & 0xff;
	//////uint g = (materialID ) & 0xff;
	//////uint b = (gl_PrimitiveID << 2) & 0xff;
	//////
	//////outColor = vec4( float(r/256.0), float(g/256.0), float(b/256.0), 1.0 );
	////
	//////outColor = vec4((TBN[2]/2.f)+0.5f,1); 
	////outColor = vec4(light,light,light,1); 
	//////outColor = vec4(base_color,1); 
	////
	////
	////outColor = vec4( normalize(viewDir) , 1 ); 
	////
	////outColor = vec4(normalize(dFdy( -V )),1); 
	////outColor = vec4(-V,1); 
	////outColor = vec4(fragTexCoord, 0,1); 


	// convert from linear to srgb
	//outColor = toLinear( outColor );
	}