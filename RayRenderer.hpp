#ifndef RAY_RENDERER_HPP
#define RAY_RENDERER_HPP

#include <Frontier.hpp>

class RayRenderer
{
	ShaderManager m_waterShader;
	FloatTexture  m_waterUVTex;
	ShaderManager m_rayShader;
	FloatTexture  m_waterTex;
	fm::Size m_maxUserWaveN;
	FrameBuffer m_waterFbo;
	CubeTexture m_skyBlur;
	DrawData m_waterDD;
	DrawData m_scrQuad;
	CubeTexture m_sky;
	bool m_fastDraw;
	bool m_repaint;
	Camera m_cam;
	Clock m_clk;
	
	class ClickWave {
	public:
		vec2 uv;
		Clock clk;	
	};
	
	class FrameDesc {
	public:
		Camera cam;
		Time clkTime;
		Time timeStamp;
		std::vector<vec2> clickPts;
		std::vector<fm::Time> clickTimes;
		vec2i curp;
	};
	
	Clock m_vidClk;
	vec2 m_lastCurPos;
	Image m_cursorImg;
	std::deque<FrameDesc> m_frames;
	fm::Time m_vidTime;
	fm::Size m_vidFrame;
	bool m_capture;
	bool m_vidRender;
	
	std::vector<ClickWave> m_clickWaves;
	
	void toggleFastDraw();
	int getWaterVertN() const;
	void applyCapturedFrame();
	void saveCurrentFrame();
	void log_res(fm::Result res);
	fm::Result applyCam();
	void init(vec2 size);
	fm::Result loadShaders();
public:
	RayRenderer(vec2 initSize);
	
	void render();
	void update();
	
	void onArrowKey(vec3 dir,bool press);
	void onUpdate(fm::Time dt);
	void onResize(vec2 newSize);
	void onFocus(bool gained);
	void onLetter(char c);
	void onClick(bool left,vec2 p);
	
	void onDrag(vec2 prevp,vec2 delta,bool leftDown,bool rightDown);
	void onScroll(float delta);
	
	bool needRedraw();
};

#endif // RAY_RENDERER_HPP