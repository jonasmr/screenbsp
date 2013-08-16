
#version 150

uniform vec3 Size;

uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewMatrix;

in vec3 VertexIn;
in vec4 ColorIn;
in vec2 TC0In;
in vec3 NormalIn;



out vec3 fNormal;
out vec3 fWorldPos;
//out vec4 ColorOut;

void main(void)  
{
	vec4 pos = vec4(VertexIn,1.0);
	pos.xyz *= Size * 1.0;
	mat3 foo;
	foo[0] = ModelViewMatrix[0].xyz;
	foo[1] = ModelViewMatrix[1].xyz;
	foo[2] = ModelViewMatrix[2].xyz;
	fNormal = foo * NormalIn;
	vec4 vWorldPos4 = ModelViewMatrix * pos;
	fWorldPos = vWorldPos4.xyz / vWorldPos4.w;
	//gl_FrontColor = vec4(1,1,1,1);
	gl_Position = ProjectionMatrix * vWorldPos4;
}  