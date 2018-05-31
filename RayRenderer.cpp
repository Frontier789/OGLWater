#include "RayRenderer.hpp"

#include <iostream>
using std::cout;
using std::endl;

void RayRenderer::log_res(fm::Result res) {
	if (!res) {
		cout << res << endl;
	}
	
	if (res.level == Result::OPFailed) {
		exit(1);
	}
}

void RayRenderer::onResize(vec2 newSize) {
	// if (newSize == m_cam.getCanvasSize()) return;
	
	m_cam.setCanvasSize(newSize);
	m_cam.set3D(newSize,m_cam.getPosition(),vec3(),deg(90),.1,1000);
	
	m_rayShader.setUniform("u_aspectR",newSize.aspect());
	m_waterShader.getCamera() = m_cam;
	
	m_waterTex.create(newSize);
	m_waterUVTex.create(newSize);
	const Texture *texs[] = {&m_waterTex,&m_waterUVTex}; 
	m_waterFbo.create(&texs[0],2,FrameBuffer::DepthBuffer(newSize));
	
	m_repaint = true;
}

fm::Result RayRenderer::applyCam() {
	fm::Result res;
	
	res += m_rayShader.setUniform("u_camP",m_cam.getPosition());
	res += m_rayShader.setUniform("u_camD",m_cam.getViewDir());
	res += m_rayShader.setUniform("u_camU",m_cam.r().cross(m_cam.getViewDir()));
	res += m_rayShader.setUniform("u_camR",m_cam.r());

	res += m_rayShader.setUniform("u_aspectR",m_cam.getCanvasSize().aspect());
	res += m_rayShader.setUniform("u_focalD",1);
	
	m_waterShader.getCamera() = m_cam;
	
	if (res) m_repaint = true;
	
	return res;
}

int RayRenderer::getWaterVertN() const {
	return m_fastDraw ? 10 : 100;
}

fm::Result RayRenderer::loadShaders() {
	
	ShaderManager tmp;
	
	
	fm::Result res = tmp.loadFromFiles("shaders/rayinit.vert","shaders/simple.frag");
	tmp.setUniform("u_wtex",m_waterTex);
	tmp.setUniform("u_sky",m_sky);
	tmp.setUniform("u_skyBlur",m_skyBlur);
	tmp.setUniform("u_fast",m_fastDraw);
	res += tmp.validate();
	
	tmp.associate("in_pos",Assoc::Position);
	
	
	if (m_rayShader.isLoaded() && !res) {
		res.level = Result::OPChanged;
	} else {
		m_rayShader.swap(tmp);
	}
	
	
	fm::Result res2 = tmp.loadFromFiles("shaders/water.vert",
										"shaders/water.frag");
	
	tmp.setUniformNames("","u_viewM","u_projM");
	
	tmp.setUniform("u_N",getWaterVertN());
	
	if (m_waterShader.isLoaded() && !res2) {
		res2.level = Result::OPChanged;
	} else {
		m_waterShader.swap(tmp);
		
		m_maxUserWaveN = 0;
		m_waterShader.forEachUniform([&](std::string name,Shader::UniformData data){
			if (name == "u_userWaves") {
				m_maxUserWaveN = data.size;
			}
		});
	}
	
	res += res2;
	return res;
}

void RayRenderer::init(vec2 size) {
	fm::Result res;
	res += m_sky.loadFromFile("pics/cloudyExt.jpg");
	res += m_skyBlur.loadFromFile("pics/cloudyExtBlur.jpg");
	
	res += loadShaders();
	
	m_waterDD.addDraw(0,getWaterVertN() * getWaterVertN() * 6,fg::Triangles);

	m_cam.setCanvasSize(size);
	m_cam.set3D(size, vec3(0, 0, -25), vec3(), deg(90), .1, 1000);
	res += applyCam();

	m_waterTex.create(size);
	m_waterUVTex.create(size);
	const Texture *texs[] = { &m_waterTex,&m_waterUVTex };
	m_waterFbo.create(&texs[0], 2, FrameBuffer::DepthBuffer(size));
	m_waterFbo.setClearColor(vec4(0,0,0,0));
	
	m_scrQuad.positions = {vec2(-1,-1),vec2(1,-1),vec2(-1,1),vec2(1,1)};
	m_scrQuad.addDraw(fg::TriangleStrip);
	
	res += m_cursorImg.loadFromFile("pics/cur.png");
	
	log_res(res);
}

RayRenderer::RayRenderer(vec2 initSize) : 
	m_maxUserWaveN(0),
	m_fastDraw(false),
	m_repaint(true),
	m_capture(false),
	m_vidRender(false)
{
	init(initSize);
}
	
bool RayRenderer::needRedraw() {
	bool tmp = m_repaint;
	m_repaint = false;
	return tmp;
}
	
void RayRenderer::onDrag(vec2 prevp,vec2 delta,bool leftDown,bool rightDown) {
	if (leftDown && !rightDown) {
		vec3  p = m_cam.getPosition();
		float d = p.length();
		p = (p + m_cam.r() * delta.x * -0.01 * d + m_cam.u() * delta.y * 0.01 * d).sgn() * p.length();
		m_cam.setPosition(p);
		m_cam.lookAt(vec3());
		
		applyCam();
	}
	
	if (!leftDown && rightDown) {
		onClick(false,prevp);
	}
	
	m_lastCurPos = prevp + delta;
}

void RayRenderer::onScroll(float delta) {
	vec3 p = m_cam.getPosition();
	m_cam.setPosition(p * pow(2,delta));
	
	applyCam();
}

void RayRenderer::onFocus(bool gained) {
	if (gained) {
		fm::Result res;
		res += loadShaders();
		res += applyCam();
		
		log_res(res);
	}
}

template<class T>
T mix(T a,T b,float r)
{
	return a*(1-r) + b*r;
}

void RayRenderer::applyCapturedFrame() {
	if (m_capture) {
		FrameDesc newFrame{m_cam,m_clk.getTime(),m_vidClk.getTime(),{},{},m_lastCurPos};
		for (auto &cw : m_clickWaves) {
			newFrame.clickPts.push_back(cw.uv);
			newFrame.clickTimes.push_back(cw.clk.getTime());
		}
		
		m_frames.emplace_back(std::move(newFrame));
	}
	
	if (m_vidRender) {
		m_repaint = true;
		
		while (m_frames.size() > 2 && m_frames[1].timeStamp < m_vidTime) {
			m_frames.pop_front();
		}
		
		FrameDesc &frame_a = m_frames[0];
		FrameDesc &frame_b = m_frames.size() == 1 ? m_frames[0] : m_frames[1];
		
		auto elapsed = m_vidTime - frame_a.timeStamp;
		float r = (m_frames.size() == 1 ? 0 : elapsed / (frame_b.timeStamp - frame_a.timeStamp));
		
		auto frame = frame_a;
		frame.cam.setPosition(mix(frame_a.cam.getPosition(),frame_b.cam.getPosition(),r));
		frame.cam.setViewDir(mix(frame_a.cam.getViewDir(),frame_b.cam.getViewDir(),r));
		frame.clkTime = mix(frame_a.clkTime,frame_b.clkTime,r);
		frame.curp = mix(frame_a.curp,frame_b.curp,r);
		
		fm::Size i_a=0,i_b=0;
		bool hadSame = false;
		while (i_a<frame_a.clickPts.size() && i_b<frame_b.clickPts.size()) {
			if (frame_a.clickPts[i_a] != frame_b.clickPts[i_b]) {
				if (!hadSame) {
					++i_a;
				} else {
					++i_b;
				}
			} else {
				frame.clickTimes[i_a] = mix(frame_a.clickTimes[i_a],frame_b.clickTimes[i_b],r);
				hadSame = true;
				i_a++;
				i_b++;
			}
		}
		
		m_cam = frame.cam;
		m_clk.setTime(frame.clkTime);
		applyCam();
		
		fm::Size waves = frame.clickPts.size();
		m_clickWaves.resize(waves);
		for (fm::Size i=0;i<waves;++i) {
			m_clickWaves[i].uv = frame.clickPts[i];
			m_clickWaves[i].clk.pause().setTime(frame.clickTimes[i]);
		}
	}
}

void RayRenderer::update() {
	
	applyCapturedFrame();
	
	fm::Result res  = m_rayShader.setUniform("u_time",float(m_clk.getSeconds()));
	fm::Result res2 = m_waterShader.setUniform("u_time",float(m_clk.getSeconds()));
	
	m_waterShader.setUniform("u_userWaveN",int(m_clickWaves.size()));
	
	int i = 0;
	for (auto &c : m_clickWaves) {
		std::string name = "u_userWaves[" + fm::toString(i).str() + "]";
		fm::Result res3 = m_waterShader.setUniform(name,vec3(c.uv,c.clk.getSeconds()));
		++i;
	}
	
	for (fm::Size i=0;i<m_clickWaves.size();++i) {
		if (m_clickWaves[i].clk.getSeconds() > 6) {
			m_clickWaves[i] = m_clickWaves.back();
			m_clickWaves.pop_back();
			--i;
		}
	}
	
	if ((res || res2) && !m_clk.isPaused()) m_repaint = true;
}

#include <FRONTIER/OpenGL.hpp>
void RayRenderer::saveCurrentFrame()
{
	if (m_vidRender) {
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		Image img;
		img.create(m_cam.getCanvasSize());
		glReadPixels(0,0,img.getSize().w,img.getSize().h,GL_RGBA,GL_UNSIGNED_BYTE,img.getPtr());
		img.flipVertically();
		
		std::string numstr = fm::toString(m_vidFrame).str();
		while (numstr.size() < 4) numstr = "0" + numstr;
		
		float r;
		vec2 a,b;
		if (m_frames.size() == 1) {
			r = 1;
			a = m_frames[0].curp;
			b = m_frames[0].curp;
		} else {
			r = (m_vidTime - m_frames[0].timeStamp) / (m_frames[1].timeStamp - m_frames[0].timeStamp);
			a = m_frames[0].curp;
			b = m_frames[1].curp;
		}
		vec2i p = a * (1-r) + b * r;
		for (int dx=0;dx<int(m_cursorImg.getSize().w);++dx)
		for (int dy=0;dy<int(m_cursorImg.getSize().h);++dy) {
			vec2i pt = p + vec2(dx,dy);
			if (pt.x >= 0 && pt.y >= 0 && pt.x < int(img.getSize().w) && pt.y < int(img.getSize().h)) {
				Color c1 = m_cursorImg.getTexel(vec2s(dx,dy));
				Color c2 = img.getTexel(pt);
				img.setTexel(pt,vec4(c1.a/255.f * c1.rgb() + (1-c1.a/255.f)*c2.rgb(),255));
			}
		}
		
		img.saveToFile("vid/vid" + numstr + ".bmp");
		
		m_vidFrame++;
		m_vidTime += fm::seconds(1/60.0);
		
		if (m_vidTime > m_frames.back().timeStamp) {
			m_vidRender = false;
			m_clk.unPause();
			m_frames.clear();
			for (auto &cw : m_clickWaves) cw.clk.unPause();
		}
	}
}

void RayRenderer::render() {
	m_waterShader.setBlendMode(fg::BlendMode::Overwrite);
	m_waterFbo.clear();
	m_waterShader.draw(m_waterDD);
	
	FrameBuffer::bind(nullptr);
	FrameBuffer::setViewport(fm::rect2s(vec2(0,0),m_cam.getCanvasSize()));
	m_rayShader.setUniform("u_wtex",m_waterTex);
	m_rayShader.draw(m_scrQuad);
	
	saveCurrentFrame();
}

void RayRenderer::onLetter(char c)
{
	if (c == ' ') {
		if (m_clk.isPaused()) {
			m_clk.unPause();
			for (auto &c : m_clickWaves) c.clk.unPause();
		}
		else {
			m_clk.pause();
			for (auto &c : m_clickWaves) c.clk.pause();
		}
	}
	
	if (c == 'r') {
		m_clk.restart();
		m_clickWaves.clear();
	}
	
	if (c == 's') {
		m_waterUVTex.copyToImage().saveToFile("s.png");
	}
	
	if (c == 'c') {
		if (!m_capture && !m_vidRender) {
			m_capture = true;
			if (!m_fastDraw) toggleFastDraw();
			
		} else if (m_capture && !m_vidRender) {
			m_capture   = false;
			m_vidRender = true;
			m_vidTime   = m_frames.front().timeStamp;
			m_vidFrame  = 0;
			
			m_clk.pause();
			if (m_fastDraw) toggleFastDraw();
		}
	}
	
	if (c == 'f') {
		toggleFastDraw();
	}
}

void RayRenderer::toggleFastDraw() {
	m_fastDraw = !m_fastDraw;
	m_rayShader.setUniform("u_fast",m_fastDraw);
	
	int N = getWaterVertN();
	m_waterDD.getDraw(0).drawLen = N*N*6;
	m_waterShader.setUniform("u_N",N);
		
	m_repaint = true;
}
	
void RayRenderer::onArrowKey(vec3,bool) {
	
}

void RayRenderer::onUpdate(fm::Time) {
	
}

void RayRenderer::onClick(bool left,vec2 p) {
	if (!left) {
		vec2s s = m_cam.getCanvasSize();
		m_waterFbo.bind();
		glReadBuffer(GL_COLOR_ATTACHMENT1);
		vec4 val;
		glReadPixels(p.x,s.h - p.y-1,1,1,GL_RGBA,GL_FLOAT,&val);
		
		if (val.x > 0 || val.y > 0) {
			if (m_clickWaves.size() < m_maxUserWaveN) {
				m_clickWaves.push_back(ClickWave{vec2(val.x,val.y),Clock(0,m_clk.isPaused())});
				m_repaint = true;
			}
		}
	}
}