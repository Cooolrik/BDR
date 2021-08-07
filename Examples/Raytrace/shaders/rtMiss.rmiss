#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "rtcommon.glsl"

layout(location = 0) rayPayloadInEXT hitPayload payload;

void main()
    {
    vec3 unit_direction = normalize(gl_WorldRayDirectionEXT);
    float t = 0.5 * (unit_direction.y + 1.0);

    // sky
    payload.hitValue.xyz = mix( vec3(0.7, 1.0, 0.5) , vec3(0.5, 0.7, 1.0) , t );
    
    // dusk
    //payload.hitValue.xyz = mix( vec3(0.0, 0.0, 0.0) , vec3(0.1, 0.1, 0.2) , t );
    
    // black
    //payload.hitValue.xyz = vec3(0.01, 0.01, 0.01);
    }