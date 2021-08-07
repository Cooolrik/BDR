#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "rtcommon.glsl"

layout(location = 0) rayPayloadInEXT hitPayload payload;
hitAttributeEXT vec3 attribs;
layout(location = 1) rayPayloadEXT bool isShadowed;

struct Vertex
{
  vec3 pos;
  vec3 nrm;
  vec2 texCoord;
};

struct Instance
{
    mat4 transform;
    mat4 transformIT;
    uint geometryID;
    uint materialID;
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 2, set = 0) uniform UniformBufferObject 
	{
	mat4 view;
	mat4 proj;
	mat4 viewI;
	mat4 projI;
	vec3 lightPosition;
    uint frameCount;
	} ubo;
layout(binding = 3, set = 0) buffer Vertices { Vertex v[]; } vertices[];
layout(binding = 4, set = 0) buffer Indices { uint i[]; } indices[];
layout(binding = 5, set = 0) uniform sampler2D textureSampler[];
layout(binding = 6, set = 0) buffer Instances { Instance i[]; } instances;

float Schlick(const float cosine, const float refractionIndex)
    {
	float r0 = (1 - refractionIndex) / (1 + refractionIndex);
	r0 *= r0;
	return r0 + (1 - r0) * pow(1 - cosine, 5);
    }

// Dielectric scatter
vec3 ScatterDieletric(float RefractionIndex, const vec3 direction, const vec3 normal, inout uint seed)
    {
	const float dot = dot(direction, normal);
	const vec3 outwardNormal = dot > 0 ? -normal : normal;
	const float niOverNt = dot > 0 ? RefractionIndex : 1 / RefractionIndex;
	const float cosine = dot > 0 ? RefractionIndex * dot : -dot;

	const vec3 refracted = refract(direction, outwardNormal, niOverNt);
	const float reflectProb = refracted != vec3(0) ? Schlick(cosine, RefractionIndex) : 1;

	const vec4 texColor = vec4(1);
	
	if( Rnd(seed) < reflectProb )
        {
        return reflect(direction, normal);
        }
    else
        {
        return refracted;
        }
    }

void main()
    {
    uint geometryID = instances.i[gl_InstanceCustomIndexEXT].geometryID;
    uint materialID = instances.i[gl_InstanceCustomIndexEXT].materialID;
   
    ivec3 idx = ivec3(  indices[nonuniformEXT(geometryID)].i[3 * gl_PrimitiveID + 0] , 
                        indices[nonuniformEXT(geometryID)].i[3 * gl_PrimitiveID + 1] , 
                        indices[nonuniformEXT(geometryID)].i[3 * gl_PrimitiveID + 2] );
    
    Vertex v0 = vertices[nonuniformEXT(geometryID)].v[idx.x];
    Vertex v1 = vertices[nonuniformEXT(geometryID)].v[idx.y];
    Vertex v2 = vertices[nonuniformEXT(geometryID)].v[idx.z];
    
    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    
    vec3 worldPos = v0.pos * barycentrics.x + v1.pos * barycentrics.y + v2.pos * barycentrics.z;
    worldPos = vec3( instances.i[gl_InstanceCustomIndexEXT].transform * vec4(worldPos, 1.0) );
    
    vec2 texCoord = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;
    vec3 N = normalize(v0.nrm * barycentrics.x + v1.nrm * barycentrics.y + v2.nrm * barycentrics.z);
 
    // Initialize the random number
    uint seed = Seed(gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x, ubo.frameCount);
    
    vec3 geom_normal = normalize(cross(v1.pos - v0.pos, v2.pos - v0.pos));
        
    payload.rayOrigin = worldPos;

    if( materialID == 0 )
        {   
        payload.hitValue = vec4( texture(textureSampler[0], texCoord).xyz , 1.0 );
        payload.rayDir = normalize(N + RandomPointOnUnitSphere(seed)); // lambertian
        }
    else if( materialID == 1 )
        { 
        payload.hitValue = vec4(0.5,0.5,0.5,1.0);
        payload.rayDir = reflect(gl_WorldRayDirectionEXT, N); // shiny metal
        } 
    else if( materialID == 2 )
        { 
        payload.hitValue = vec4(0.6,0.3,0.2,1.0);
        payload.rayDir = normalize(normalize(reflect(gl_WorldRayDirectionEXT, N)) + RandomPointOnUnitSphere(seed)*1.0); // fuzzy metal
        }
    else if( materialID == 3 )
        {
        // glass
        payload.hitValue = vec4(0.95,0.95,0.95,1.0);
        const float ir = 1.5;
        float refraction_ratio = 1.0/ir;
        if( gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT )
            {
            refraction_ratio = ir;
            }
        vec3 unit_direction = normalize(gl_WorldRayDirectionEXT);
        payload.rayDir = normalize(ScatterDieletric( 1.5 , unit_direction , N , seed ));
        }
    else if( materialID == 4 )
        {
        // emissive red
        payload.hitValue = vec4(1,0,0,0);
        }
    else if( materialID == 5 )
        {
        // emissive green
        payload.hitValue = vec4(0,1,0,0);
        }
    else if( materialID == 6 )
        {
        // emissive blue
        payload.hitValue = vec4(0,0,1,0);
        }

}
