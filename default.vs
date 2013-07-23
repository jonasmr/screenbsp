
uniform vec3 Size;

varying vec3 COL;

void main(void)  
{
	vec4 pos = gl_Vertex;
	pos.xyz *= Size * 1.0;
	vec3 dir = normalize(vec3(-0.8, 0.2, 0.57));
	vec3 normal = gl_Normal;
	vec4 n1;
	n1.xyz = normal;
	n1.w = 0.0;

	n1 = gl_ModelViewMatrix * n1;
	normal = n1.xyz;
	float diffuse = max(dot(dir, normal), 0.3*dot(-dir, normal));
	vec3 ambient = vec3(0.4, 0.4, 0.4);

	gl_FrontColor = gl_Color;
	COL = vec3(1,1,1) * diffuse * 0.5 + ambient;
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * pos;
}  