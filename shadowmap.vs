
#version 150

uniform vec3 Size;

uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewMatrix;

in vec3 VertexIn;
in vec4 ColorIn;
in vec2 TC0In;
in vec3 NormalIn;

void main(void)  
{
	vec4 pos = vec4(VertexIn,1.0);
	pos.xyz *= Size;
	vec4 vWorldPos4 = ModelViewMatrix * pos;
	gl_Position = ProjectionMatrix * vWorldPos4;
}  