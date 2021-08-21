
#define LAYOUT_READONLY_STORAGE_ARRAY_BUFFER( _set , _binding , _type , _name ) layout(set = _set, binding = _binding ) readonly buffer _name##BUFFER { _type _name##Array[]; } _name##Buffer;


// oct encoded normals
vec3 decodeNormals( vec2 inpacked )
    {
    vec3 n = vec3( inpacked.x, inpacked.y, 1.f - abs( inpacked.x ) - abs( inpacked.y ) );
    float t = max( -n.z, 0.f );
    n.x += ( n.x > 0.f ) ? -t : t;
    n.y += ( n.y > 0.f ) ? -t : t;
    return normalize( n );
    }
