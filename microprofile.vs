
varying vec3 TC;
void main(void)  
{
	gl_FrontColor = gl_Color;
	TC.xy = gl_TexCoord.xy;
	TC.z = gl_TexCoord.y > 0.99 : 0.0 : 1.0;
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
}  