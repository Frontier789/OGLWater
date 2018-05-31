#version 130

/*****  Camera Specification  *****/
uniform vec3 u_camD;
uniform vec3 u_camU;
uniform vec3 u_camR;

uniform float u_aspectR;
uniform float u_focalD;

/*****  Input data  *****/
in vec2 in_pos;

/*****  Output data  *****/
out vec3 va_rayV;
out vec2 va_tpt;

void main()
{
	gl_Position = vec4(in_pos,0,1);
	
	vec2 canvasP = in_pos * vec2(u_aspectR,1) / 2;
	
	va_rayV = mat3(u_camD,u_camR,u_camU) * vec3(u_focalD, canvasP.x, canvasP.y);
	va_tpt = (in_pos + vec2(1))/2;
}

