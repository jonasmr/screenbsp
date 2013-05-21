
varying vec2 TexCoord;

void main(void)  
{
	gl_FrontColor = gl_Color;
	TexCoord = gl_MultiTexCoord0.st;
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
}  