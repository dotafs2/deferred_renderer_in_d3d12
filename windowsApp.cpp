#include "stdafx.h"
#include "windowsApp.h"
#include <Windowsx.h>
windowsApp* appPointer;

 LRESULT CALLBACK windowsApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		appPointer->Resize(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_KEYDOWN:
		appPointer->KeyDown(wParam);
	case WM_KEYUP:
		//appPointer->KeyUp(wParam);
	case WM_LBUTTONDOWN:
		appPointer->MousePress(LOWORD(lParam), HIWORD(lParam));
	case WM_LBUTTONUP:
		appPointer->MouseRelease(wParam);
	case WM_MOUSEMOVE:
		if (wParam == MK_LBUTTON)
		{
			appPointer->MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


windowsApp::windowsApp()
{
mWidth=800;
mHeight=600;
}

