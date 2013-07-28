
varying vec3 TC;
void main(void)  
{
	gl_FrontColor = gl_Color;
	TC.xy = gl_TexCoord[0].xy;
	TC.z = gl_TexCoord[0].y > 1.0 ? 0.0 : 1.0;
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
}  