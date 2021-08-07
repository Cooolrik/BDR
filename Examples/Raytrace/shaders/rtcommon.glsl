#define EPS 0.05
const float M_PI = 3.141592653589;

struct hitPayload
    {
    vec4 hitValue; // w is non-zero as long as we should continue scatter
    vec3 rayOrigin;
    vec3 rayDir;
    };

uint Seed(uint val0, uint val1)
    {
    uint v0 = val0;
    uint v1 = val1;
    uint s0 = 0;

    for(uint n = 0; n < 16; n++)
        {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
        }

    return v0;
    }

uint LCG(inout uint prev)
    {
    uint LCG_A = 1664525u;
    uint LCG_C = 1013904223u;
    prev = (LCG_A * prev + LCG_C);
    return prev & 0x00FFFFFF;
    }

float Rnd(inout uint seed)
    {
    return (float(LCG(seed)) / float(0x01000000));
    }

void ComputeDefaultBasis(const vec3 normal, out vec3 x, out vec3 y)
    {
    // ZAP's default coordinate system for compatibility
    vec3 z  = normal;
    const float yz = -z.y * z.z;
    y = normalize(((abs(z.z) > 0.99999f) ? vec3(-z.x * z.y, 1.0f - z.y * z.y, yz) : vec3(-z.x * z.z, yz, 1.0f - z.z * z.z)));
    x = cross(y, z);
    }

vec2 RandomPointInUnitDisc( inout uint seed )
    {
    vec2 point;
    for(;;)
        {
        point = vec2( Rnd(seed), Rnd(seed) ) * 2.0 - 1.0;
        if( dot(point,point) <= 1 )
            return point;
        }
    }

vec3 RandomPointOnUnitSphere( inout uint seed )
    {
    vec3 point;
    for(;;)
        { 
        point = vec3( Rnd(seed), Rnd(seed), Rnd(seed) ) * 2.0 - 1.0;
        if( dot(point,point) <= 1 )
            return normalize(point);
        }
    }