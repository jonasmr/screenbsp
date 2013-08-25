#version 150

uniform vec3 Size;


uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewMatrix;
uniform float UseVertexColor;
uniform vec4 ConstantColor;

in vec3 VertexIn;
in vec4 ColorIn;
in vec2 TC0In;
in vec3 NormalIn;

out vec4 Color;

void main(void)  
{
	vec4 pos = vec4(VertexIn, 1.0);
	pos.xyz *= Size * 1.0;
	Color = mix(ConstantColor, ColorIn, UseVertexColor);
	gl_Position = ProjectionMatrix * (ModelViewMatrix * pos);
}  