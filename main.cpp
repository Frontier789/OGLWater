#include <Frontier.hpp>
#include <FRONTIER/OpenGL.hpp>
#include "RayRenderer.hpp"

#include <iostream>
using namespace std;

int main()
{
	GuiWindow win(vec2(640,480),"Ray");
	win.setMaxFps(60);
	win.setDepthTest(fg::LEqual);
	
	cout << glGetString(GL_VERSION) << endl;
	
	RayRenderer renderer(win.getSize());
	
	vec2 prevMP;
	bool leftPress  = false;
	bool rightPress = false;
	bool running    = true;
	
	while (running)
	{
		Event ev;
		while (win.popEvent(ev))
		{
			win.handleEvent(ev);
			
			if (ev.type == Event::Closed) running = false;
			
			if (ev.type == Event::KeyPressed || ev.type == Event::KeyReleased) {
				vec3 d = Keyboard::keyToDelta(ev.key.code);
				
				if (d.LENGTH()) renderer.onArrowKey(d,ev.type == Event::KeyPressed);
				
				if (ev.key.code == Keyboard::Escape) running = false;
				
				if (ev.type == Event::KeyPressed) {
					if (ev.key.code == Keyboard::Space) {
						renderer.onLetter(' ');
					}
					if (ev.key.code >= Keyboard::A && ev.key.code <= Keyboard::Z) {
						renderer.onLetter('a' + (ev.key.code - Keyboard::A));
					}
					if (ev.key.code == Keyboard::U) {
						win.setSize(vec2(1280,720));
					}
				}
			}
			
			if (ev.type == Event::ButtonPressed || ev.type == Event::ButtonReleased) {
				leftPress  = ev.type == Event::ButtonPressed && ev.mouse.button == Mouse::Left;
				rightPress = ev.type == Event::ButtonPressed && ev.mouse.button == Mouse::Right;
				prevMP = ev.mouse;
				
				if (ev.type == Event::ButtonPressed) {
					renderer.onClick(ev.mouse.button == Mouse::Left,ev.mouse);
				}
			}
			
			if (ev.type == Event::MouseMoved) {
				vec2 p = ev.motion;
				renderer.onDrag(prevMP,p - prevMP,leftPress,rightPress);
				prevMP = p;
			}
			
			if (ev.type == Event::MouseWheelMoved) {
				renderer.onScroll(-ev.wheel.delta);
			}
			
			if (ev.type == Event::Resized) {
				renderer.onResize(ev.size);
			}
			
			if (ev.type == Event::FocusGained) {
				renderer.onFocus(true);
			}
		}
		renderer.update();
		
		if (renderer.needRedraw()) {
			win.clear();
			renderer.render();
			win.swapBuffers();
		}
		
		win.applyFpsLimit();
	}
}
