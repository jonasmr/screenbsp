
varying vec3 COL;

void main(void)  
{
	vec3 dir = normalize(vec3(-0.8, 0.2, 0.57));
	vec3 normal = gl_Normal;
	float diffuse = dot(dir, normal);
	vec3 ambient = vec3(0.4, 0.4, 0.4);

	gl_FrontColor = gl_Color;
	COL = vec3(1,1,1)*diffuse*0.5 + ambient;
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
}  