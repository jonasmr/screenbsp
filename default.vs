
#version 150

uniform vec3 Size;

uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewMatrix;

in vec3 VertexIn;
in vec3 NormalIn;
out vec3 COL;

void main(void)  
{
	vec4 pos = vec4(VertexIn, 1.0);
	pos.xyz *= Size * 1.0;
	vec3 dir = normalize(vec3(-0.8, 0.2, 0.57));
	vec3 normal = NormalIn;
	vec4 n1;
	n1.xyz = normal;
	n1.w = 0.0;

	n1 = ModelViewMatrix * n1;
	normal = n1.xyz;
	float diffuse = max(dot(dir, normal), 0.3*dot(-dir, normal));
	vec3 ambient = vec3(0.4, 0.4, 0.4);
	COL = vec3(1,1,1) * diffuse * 0.5 + ambient;
	gl_Position = ProjectionMatrix * ModelViewMatrix * pos;
}  