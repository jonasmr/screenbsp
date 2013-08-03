uniform vec3 Size;

void main(void)  
{
	vec4 pos = gl_Vertex;
	pos.xyz *= Size * 1.0;
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * pos;
}  