#version 130

/*****  Camera Specification  *****/
uniform float u_time;
uniform vec3  u_userWaves[300];
uniform int   u_userWaveN = 0;

in float va_dist;
in vec2 va_uv;

out vec4 out_color;
out vec4 out_uv;

float rand(vec2 co) {
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float wave_fun_d(float x,float len,float u,float D,float A,float O,float H)
{
	return A*(x - u)*cos(len*A + O) * H / len;
}

float pow2(float x) {return x*x;}

#define PI 3.14159265358979
vec2 wave_d(vec2 ori,float amp,float freq,float speed,float offset,vec2 uv)
{
	freq *= 2*PI;
	offset -= u_time*speed;
	
	float LENx = pow2(uv.x - ori.x);
	float LENy = pow2(uv.y - ori.y);
	float len = sqrt(LENx+LENy);
	float dx  = wave_fun_d(uv.x,len,ori.x,LENx,freq,offset,amp);
	float dy  = wave_fun_d(uv.y,len,ori.y,LENy,freq,offset,amp);
	
	return vec2(dx,dy);
}

float user_wave_fun_d(float len,float A,float B,float C,float D,float x)
{
	float coef = A*len+D;
	return (-2*sin(coef)*coef + cos(coef))*A*(x-B)*exp(-coef*coef)/len;
}

vec2 user_wave_d(vec2 ori,float amp,float freq,float offset,vec2 uv)
{
	float LENx = pow2(uv.x - ori.x);
	float LENy = pow2(uv.y - ori.y);
	float len = sqrt(LENx+LENy);
	
	float dx  = -user_wave_fun_d(len,freq,ori.x,LENy,offset,uv.x)*amp;
	float dy  = -user_wave_fun_d(len,freq,ori.y,LENx,offset,uv.y)*amp;
	
	return vec2(dx,dy);
}

#define N 50

vec2 uvToWorld(vec2 uv) 
{
	return (uv - vec2(.5))*70;
}

vec2 parsurf(vec2 uv)
{
	uv = uvToWorld(uv);
	
	vec2 dxdy = vec2(0,0);
	
	for (int i=0;i<N;++i) {
		vec2 ori = vec2(rand(vec2(i,i+0.5)),rand(vec2(-i,-i+1)))*70-vec2(35);
		float r = i/float(N-1);
		
		dxdy += wave_d(ori,pow(r,8)*.3,(1-r)*2,(pow(1-r,2)*20+1)*1.5,i,uv);
	}
	
	for (int i=0;i<u_userWaveN;++i) {
		dxdy += user_wave_d(uvToWorld(u_userWaves[i].xy),5/(8+pow2(u_userWaves[i].z)),.3,-u_userWaves[i].z*4,uv);
	}
	
	dxdy *= .8;
	
	return dxdy;
}

void main()
{
	vec2 dxdy = parsurf(va_uv);
	
	out_color = vec4(-dxdy.x,-dxdy.y,1,va_dist);
	out_uv    = vec4(va_uv,0,1);
}

