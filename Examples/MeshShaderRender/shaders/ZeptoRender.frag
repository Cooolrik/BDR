#version 450
 
layout(location = 0) out vec4 FragColor;
 
layout(location = 0) in vec4 color;
layout(location = 1) in vec4 pos; 

void main()
{
  FragColor = -color * pos;
}