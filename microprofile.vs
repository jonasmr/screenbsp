
#version 150

uniform mat4 ProjectionMatrix;

in vec3 VertexIn;
in vec4 ColorIn;
in vec2 TC0In;

out vec2 TC0;
out vec4 Color;

void main(void)  
{
	Color = ColorIn;
	TC0 = TC0In;
	gl_Position = ProjectionMatrix * ModelViewMatrix * vec4(VertexIn, 1.0);
}  