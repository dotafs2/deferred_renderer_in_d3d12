#pragma once
#include <windows.h>


class windowsApp;
extern windowsApp* appPointer;
class windowsApp
{

public:
	windowsApp();
	
	virtual int Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) = 0;
protected:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void Setup(){ return ; }
	virtual void Update(){ return; }
	virtual void Render(){ return; }
	virtual void Resize(UINT width, UINT height) { return; }
	virtual void KeyDown(UINT key) { return; }
	virtual void KeyUp(UINT key) { return; }
	virtual void MouseMove(UINT xpos,UINT ypos) { return; }
	virtual void MousePress(UINT xpos, UINT ypos) { return; }
	virtual void MouseRelease(UINT key) { return; }
	int mWidth;
	int mHeight;
	HWND mHwnd = nullptr;
};