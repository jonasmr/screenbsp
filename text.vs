#version 150

uniform mat4 ProjectionMatrix;

in vec3 VertexIn;
in vec4 ColorIn;
in vec2 TC0In;

out vec4 Color;
out vec2 TC0;

void main(void)  
{
	Color = ColorIn;
	TC0 = TC0In;
	gl_Position = ProjectionMatrix * vec4(VertexIn, 1.0);
}  