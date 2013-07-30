
uniform vec3 Size;
varying vec3 fNormal;
varying vec3 fWorldPos;

void main(void)  
{
	vec4 pos = gl_Vertex;
	pos.xyz *= Size * 1.0;
	mat3 foo;
	foo[0] = gl_ModelViewMatrix[0].xyz;
	foo[1] = gl_ModelViewMatrix[1].xyz;
	foo[2] = gl_ModelViewMatrix[2].xyz;
	fNormal = foo * gl_Normal;
	vec4 vWorldPos4 = gl_ModelViewMatrix * pos;
	fWorldPos = vWorldPos4.xyz / vWorldPos4.w;
	gl_FrontColor = vec4(1,1,1,1);
	gl_Position = gl_ProjectionMatrix * vWorldPos4;
}  