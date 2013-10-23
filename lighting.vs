
#version 150

uniform vec3 Size;

uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewMatrix;
uniform vec3 poshack;
uniform mat4 ShadowMatrix;
in vec3 VertexIn;
in vec4 ColorIn;
in vec2 TC0In;
in vec3 NormalIn;



out vec3 fNormal;
out vec3 fWorldPos;
out vec4 fShadowCoords;
//out vec4 ColorOut;

void main(void)  
{
	vec4 pos = vec4(VertexIn,1.0);
	//pos += vec4(poshack, 1.0);
	pos.xyz *= Size;
	mat3 foo;
	foo[0] = ModelViewMatrix[0].xyz;
	foo[1] = ModelViewMatrix[1].xyz;
	foo[2] = ModelViewMatrix[2].xyz;
	fNormal = foo * NormalIn;
	vec4 vWorldPos4 = ModelViewMatrix * pos;
	//vec4 vWorldPos4 = pos;
	fWorldPos = vWorldPos4.xyz / vWorldPos4.w;
	//gl_FrontColor = vec4(1,1,1,1);
	gl_Position = ProjectionMatrix * vWorldPos4;
	fShadowCoords = ShadowMatrix * vWorldPos4;
	// gl_Position = ProjectionMatrix * (ModelViewMatrix * vec4(VertexIn,1.0));
	//gl_Position = ProjectionMatrix * (ModelViewMatrix * vec4(VertexIn,1.0));
}  