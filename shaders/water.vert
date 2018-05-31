#version 130

/*****  Camera Specification  *****/
uniform vec3 u_camP;
uniform mat4 u_viewM;
uniform mat4 u_projM;
uniform int  u_N;

uniform float u_time;
uniform vec3  u_userWaves[300];
uniform int   u_userWaveN = 0;

out vec2 va_uv;
out float va_dist;

float rand(vec2 co) {
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float wave_fun(float len,float A,float O,float H)
{
	return sin(len * A + O) * H;
}

float pow2(float x) {return x*x;}

#define PI 3.14159265358979
float wave(vec2 ori,float amp,float freq,float speed,float offset,vec2 uv)
{
	freq *= 2*PI;
	offset -= u_time*speed;
	
	float LENx = pow2(uv.x - ori.x);
	float LENy = pow2(uv.y - ori.y);
	float len = sqrt(LENx+LENy);
	float h   = wave_fun(len,freq,offset,amp);
	
	return h;
}

float user_wave_fun(float len,float A,float B,float C,float D)
{
	return exp(-pow2(A*len+D))*sin(A*len+D);
}

float user_wave(vec2 ori,float amp,float freq,float offset,vec2 uv)
{
	float len = length(uv-ori);
	return -user_wave_fun(len,freq,ori.x,pow2(uv.y - ori.y),offset)*amp;
}

#define N 50

vec2 uvToWorld(vec2 uv) 
{
	return (uv - vec2(.5))*70;
}

vec3 parsurf(vec2 uv)
{
	uv = uvToWorld(uv);
	
	vec3 p = vec3(uv,0).xzy;
	
	for (int i=0;i<N;++i) {
		vec2 ori = vec2(rand(vec2(i,i+0.5)),rand(vec2(-i,-i+1)))*70-vec2(35);
		float r = i/float(N-1);
		
		p.y += wave(ori,pow(r,8)*.3,(1-r)*2,(pow(1-r,2)*20+1)*1.5,i,uv);
	}
	
	for (int i=0;i<u_userWaveN;++i) {
		p.y += user_wave(uvToWorld(u_userWaves[i].xy),5/(8+pow2(u_userWaves[i].z)),.3,-u_userWaves[i].z*4,uv);
	}
	
	p.y *= .8;
	
	return p*4 - vec3(0,30,0);
}

const float epsilon  = 0.0005;

void main()
{
	vec2 base = vec2( (gl_VertexID/6)%u_N,(gl_VertexID/6)/u_N );
	vec2 offsets[6] = vec2[6](vec2(0,0),vec2(1,0),vec2(0,1),vec2(1,0),vec2(0,1),vec2(1,1)); 
	
	vec2 np = (base + offsets[gl_VertexID%6]) / u_N;
	vec3 p = parsurf(np);
	
	gl_Position = u_projM * u_viewM * vec4(p,1);
	
	va_dist = gl_Position.z;
	va_uv   = np;
}

