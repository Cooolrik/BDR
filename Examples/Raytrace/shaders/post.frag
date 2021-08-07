#version 450

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

const float xsize = 1024.0;
const float ysize = 512.0;

void main()
	{
    vec4 col = texture(texSampler, fragTexCoord );
		
	if( col.w < 1 )
		{
		outColor = vec4(0,0,0,1);
		return;
        }

	col /= (col.w);
	outColor = col;
	}
