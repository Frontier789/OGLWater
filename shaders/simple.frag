#version 130

/*****  Camera Specification  *****/
uniform vec3 u_camP;
uniform vec3 u_camU;
uniform vec3 u_camR;

/*****  Input data  *****/
in vec3 va_rayV;
in vec2 va_tpt;

/*****  Output data  *****/
out vec4 out_color;

/*****  Auxiliary data  *****/
uniform float u_time;
uniform samplerCube u_sky;
uniform samplerCube u_skyBlur;
uniform sampler2D u_wtex;
uniform int u_fast = 0;

/*****  Implementation functions  *****/
float estimate(vec3 p);
vec3  heat_color(float nval);
vec3  march(vec3 p,vec3 v,out bool hit,out int steps);
vec3  normal(vec3 p);
float shadow(vec3 p,vec3 n,float dp,float softness);
float ambient_occ(vec3 p,vec3 n);

/*****  Global constants  *****/
const float farPlane  = 1000;
const float nearPlane = 0.1;
const float epsilon  = 0.005;

const vec3 sun_dir = normalize(vec3(1,1.5,0.5));

vec3 trace_color_hit(vec3 pos,vec3 v,bool hit,bool blur)
{
	if (hit) {
		vec3 n = normal(pos);
		vec3 r = reflect(v,n);
		float d = dot(sun_dir,n);
		float s = 1;
		float ao = 1;
		if (u_fast == 0) {
			s = shadow(pos,n,d,32);
			ao = ambient_occ(pos,n);
		}
		
		//   phong  // 
		
		vec3 ambient  = vec3(.9,.6,.5)*texture(u_sky,r).rgb*.15;
		vec3 diffuse  = vec3(.9,.6,.5);
		vec3 specular = vec3(1,1,1);
		float spow    = 8;
		
		float sd = max(dot(r,sun_dir),0);
		
		return ambient * ao + diffuse * max(d,0) * s + specular * pow(sd,spow) * s;
		
		
		//    fém   // return texture(u_sky,r)*vec4(vec3(s*ao),1);
		// lépés bw // return vec4(vec3(1-float(steps)/200),1);
		//   hőkép  // return vec4(heat_color(float(steps)/200),1);
	} else if (blur) {
		return texture(u_skyBlur,v).rgb;
	} else {
		return texture(u_sky,v).rgb;
	}
}

vec3 trace_color(vec3 p,vec3 v)
{
	bool hit;
	int steps;
	vec3 pos = march(p,v,hit,steps); 
	
	return trace_color_hit(pos,v,hit,true);
}

void main()
{
	vec3 p = u_camP;
	vec3 v = normalize(va_rayV);
	
	bool hit;
	int steps;
	vec3 pos = march(p,v,hit,steps); 
	float dist = hit ? length(pos - p) : 0;
	
	vec4  samp = texture(u_wtex,va_tpt);
	vec3  w    = normalize(vec3(samp.x,1,samp.y));
	
	if (hit && dist < samp.w || samp.w < epsilon) {
		out_color = vec4(trace_color_hit(pos,v,hit,false),1);
	} else {
		vec3 r = reflect(v,w);
		
		float dv = abs(dot(v,w));
		
		vec3 refrac = -normalize(w*.1 - v*.9);
		
		p += v * samp.w;
		
		if (u_fast == 1)
			out_color = vec4(vec3(texture(u_sky,refrac)*dv + texture(u_sky,r)*(1-dv)) + vec3(0,vec2(sin(u_time),cos(u_time))*.05),1);
		else {
			float s = shadow(p,w,1,128);
			s = 1- (1-s)*.5;
			out_color = vec4(trace_color(p,refrac)*dv + trace_color(p,r)*(1-dv) + vec3(0,vec2(sin(u_time),cos(u_time))*.05),1)*vec4(s,s,s,1);
			
			
			// out_color = vec4(trace_color(p,refrac).xyz + vec3(0,vec2(sin(u_time),cos(u_time))*.05),1);
			// out_color = vec4(w,1);
			// out_color = vec4(vec3(dvw),1);
		} 
	}
}

#define OccIter 12
#define OccDelta 0.5

float ambient_occ(vec3 p,vec3 n)
{
	float sum  = 0;
	float pow2 = 1;
	for (int i=1;i<OccIter;i++) {
		sum  += (i*OccDelta - estimate(p + i*OccDelta*n)) / pow2;
		pow2 *= 1.35;
	}
	
	return 2.0 - sum*0.2;
}

float shadow(vec3 p,vec3 n,float dp,float softness)
{
	if (dp <= 0) return 0.0;
	
	p += n * epsilon * 1.1;
	
	float d = 0;
	float e = estimate(p);
	int steps = 0;
	float r = 1;
	for (steps=0;/*steps<maxSteps &&*/ d<farPlane && e > epsilon;steps++) {
		p += sun_dir*e;
		d += e;
		r = min(r,softness * e / d);
		e = estimate(p);
	}
	
	return r * (e <= epsilon ? 0.0 : 1.0);
}

vec3 march(vec3 p,vec3 v,out bool hit,out int steps)
{
	float d = 0;
	float e = estimate(p);
	for (steps=0;/*steps<maxSteps &&*/ d<farPlane && e > epsilon;steps++) {
		
		p += v*e;
		d += e;
		e = estimate(p);
	}
	
	hit = e <= epsilon;
	
	return p;
}

vec3 normal(vec3 p)
{
	return normalize(
		   vec3((estimate(p + vec3(epsilon,0,0)) - estimate(p - vec3(epsilon,0,0))),
				(estimate(p + vec3(0,epsilon,0)) - estimate(p - vec3(0,epsilon,0))),
				(estimate(p + vec3(0,0,epsilon)) - estimate(p - vec3(0,0,epsilon)))));
}

float sphere(vec3 p,vec3 O,float R)
{
	return length(p-O) - R;
}

#define Power 8
#define Iterations 7
#define Bailout 1.5

// http://blog.hvidtfeldts.net/index.php/2011/09/distance-estimated-3d-fractals-v-the-mandelbulb-different-de-approximations/
float DE(vec3 pos) {
	vec3 z = pos;
	float dr = 1.0;
	float r = 0.0;
	for (int i = 0; i < Iterations ; i++) {
		r = length(z);
		if (r>Bailout) break;
		
		// convert to polar coordinates
		float theta = acos(z.z/r);
		float phi = atan(z.y,z.x);
		dr =  pow( r, Power-1.0)*Power*dr + 1.0;
		
		// scale and rotate the point
		float zr = pow( r,Power);
		theta = theta*Power;
		phi = phi*Power;
		
		// convert back to cartesian coordinates
		z = zr*vec3(sin(theta)*cos(phi), sin(phi)*sin(theta), cos(theta));
		// z+=vec3(.9,cos(u_time*1.1+.5),sin(u_time));
		z+=pos;
	}
	return 0.5*log(r)*r/dr;
}

float sdBox(vec3 p, vec3 b)
{
  vec3 d = abs(p) - b;
  return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

float estimate(vec3 p)
{
	/*
	float dist = 1/epsilon;
	
	int N = 10;
	for (int i=0;i<N;i++) {
		float r = float(i)/(N-1)*3.14159265358979*2 + u_time - sin(float(i)/(N-1)*3.14159265358979*2 + u_time)*.5;
		float s = sin(r);
		float c = cos(r);
		mat3 m = mat3(1,0, 0,
					  0,c,-s,
					  0,s, c);
		dist = min(dist,sdBox(m*(p + vec3(0,c,s)*10),vec3(35,1,8)/5));
	}
	
	return dist;
	*/
	if (u_fast == 1)
		return sphere(p,vec3(0,0,0),4);
	else 
		return DE(p*.2);
	// return min(sphere(p,vec3(0,0,0),1),sphere(p,vec3(1,1,1),1));
}

vec3 heat_color(float nval)
{
	float b = 1.0/15;
	
	nval = clamp(nval,0,1);
	
	if (nval < b*3) {
		return vec3(0,0,smoothstep(b*0,b*3,nval));
	} else if (nval < b*6) {
		return vec3(0,smoothstep(b*3,b*6,nval),1);
	} else if (nval < b*9) {
		return vec3(smoothstep(b*6,b*9,nval),1,1-smoothstep(b*6,b*9,nval));
	} else if (nval < b*12) {
		return vec3(1,1-smoothstep(b*9,b*12,nval),0);
	} else {
		return vec3(1,smoothstep(b*12,b*15,nval),smoothstep(b*12,b*15,nval));
	}
}