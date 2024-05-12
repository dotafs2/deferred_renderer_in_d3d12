#pragma once
#include "windowsApp.h"
#include "DirectxHelper.h"
#include <stdio.h>
#include "DeferredRender.h"
#include "CameraManager.h"
#include "Wave.h"
#include <DirectXMath.h>
#include "VertexStructures.h"
#include "GameTimer.h"
#include "MathHelper.h"

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;


std::wstring mMainWndCaption = L"d3d App";
D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;


class directxProcess :public windowsApp
{
	ComPtr<ID3D12GraphicsCommandList> mCommandList;

	DeferredRender mDeferredTech;
	CameraData mCamData;
	LightData mLightData;
	GCamera mCamera;
	bool mInit = false;
	std::unique_ptr<Wave> mWave;
	GameTimer mTimer;
	std::shared_ptr<D3dDeviceManager> GetDeviceResources()
	{
		if (g_d3dObjects != nullptr && g_d3dObjects->IsDeviceRemoved())
		{
			// All references to the existing D3D device must be released before a new device
			// can be created.

			g_d3dObjects = nullptr;
			OnDeviceRemoved();
		}

		if (g_d3dObjects == nullptr)
		{
			g_d3dObjects = std::make_shared<D3dDeviceManager>();
			g_d3dObjects->SetWindows(mHwnd, mWidth, mHeight);
		}
		return g_d3dObjects;

	};
public:
	int Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR    lpCmdLine, int     nCmdShow)
	{
		appPointer = this;
		UNREFERENCED_PARAMETER(hPrevInstance);
		UNREFERENCED_PARAMETER(lpCmdLine);
		MSG msg;

		WNDCLASSEX wc;
		ZeroMemory(&wc, sizeof(wc));
		ZeroMemory(&msg, sizeof(msg));
		mTimer.Reset();

		wc.cbSize = sizeof(wc);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = &windowsApp::WndProc;
		wc.hInstance = hInstance;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszClassName = "Deferred Shading";
		RegisterClassEx(&wc);

		RECT rect = { 0, 0, mWidth, mHeight };
		DWORD dwStyle = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX;
		AdjustWindowRect(&rect, dwStyle, FALSE);
		mHwnd = CreateWindow(wc.lpszClassName,
			wc.lpszClassName,
			dwStyle,
			CW_USEDEFAULT, CW_USEDEFAULT,
			rect.right - rect.left, rect.bottom - rect.top,
			NULL, NULL, hInstance, NULL);

		ShowWindow(mHwnd, nCmdShow);
		UpdateWindow(mHwnd);
		Setup();

		while (msg.message != WM_QUIT) {
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);

			}
			else {
				mTimer.Tick();
				CalculateFrameStats();
				Update(mTimer);
				Render(mTimer);

			}
		}



		return (int)msg.wParam;
	}

protected:
	
	void OnDeviceRemoved()
	{

	};
	void directxProcess::CalculateFrameStats()
	{
		static int frameCnt = 0;
		static float timeElapsed = 0.0f;

		frameCnt++;

		// Compute averages over one second period.
		if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
		{
			float fps = float(frameCnt); // fps = frameCnt / 1
			float mspf = 1000.0f / fps;

			std::wstring fpsStr = std::to_wstring(fps);
			std::wstring mspfStr = std::to_wstring(mTimer.DeltaTime());

			std::wstring windowText = mMainWndCaption +
				L"    fps: " + fpsStr +
				L"   mspf: " + mspfStr;

		SetWindowTextW(mHwnd, windowText.c_str());


			// Reset for next average.
			frameCnt = 0;
			timeElapsed += 1.0f;
		}
	}






	void Setup()
	{
		// reset the timer
		mTimer.Reset();
		auto commandQueue = GetDeviceResources()->GetCommandQueue();
		
		PIXBeginEvent(commandQueue, 0, L"Setup");
		{
			ThrowIfFailed(GetDeviceResources()->GetD3DDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_d3dObjects->GetCommandAllocator(), nullptr, IID_PPV_ARGS(mCommandList.GetAddressOf())));

			g_ShaderManager.LoadFolder("Shaders");

			
			mCamera.InitProjMatrix(3.14f*0.45f, mWidth, mHeight, 0.01f, 10000.0f);
			mCamera.Position(DirectX::XMFLOAT3(0,0.01f,-25.1f));
			mCamData.MVP = mCamera.ProjView();
			mCamData.InvPV = mCamera.InvScreenProjView();
			mCamData.CamPos = mCamera.Position();

			mLightData.pos = XMFLOAT3(0, 15, -7);

			mDeferredTech.Init();
			mDeferredTech.UpdateConstantBuffer(mCamData, mLightData);

			mWave->Init(128,128,1.0f,0.03f,4.0f,0.2f);

			ThrowIfFailed(mCommandList->Close());
			ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
			GetDeviceResources()->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

			g_d3dObjects->WaitForGPU();
		}
		PIXEndEvent(commandQueue);
		mInit = true;
	}
	void Render(const GameTimer& gt)
	{
		auto commandQueue = GetDeviceResources()->GetCommandQueue();
		PIXBeginEvent(commandQueue, 0, L"Render");
		{
			ThrowIfFailed(g_d3dObjects->GetCommandAllocator()->Reset());
			ThrowIfFailed(mCommandList->Reset(g_d3dObjects->GetCommandAllocator(), mDeferredTech.getPSO()));
	
			
			AddResourceBarrier(mCommandList.Get(), g_d3dObjects->GetRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			
			float clearColor[4]={ 0.0f,0.0f,0.0f,0.0f };
			mCommandList->ClearRenderTargetView(g_d3dObjects->GetRenderTargetView(), clearColor,0,nullptr);
			D3D12_RECT rect = { 0, 0, mWidth, mHeight };
			
			
		
			mCommandList->RSSetScissorRects(1, &rect);
			mCommandList->RSSetViewports(1, &g_d3dObjects->GetScreenViewport());

			mDeferredTech.ApplyGBufferPSO(mCommandList.Get());
			mWave->Render(mCommandList);
		

			mCommandList->OMSetRenderTargets(1, &g_d3dObjects->GetRenderTargetView(),true, nullptr);
			mDeferredTech.ApplyLightingPSO(mCommandList.Get());

			AddResourceBarrier(mCommandList.Get(), g_d3dObjects->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	
			ThrowIfFailed(mCommandList->Close());
			ID3D12CommandList* ppCommandLists[1] = { mCommandList.Get() };

			commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

			g_d3dObjects->WaitForGPU();
		
			g_d3dObjects->Present();
			
		}
		PIXEndEvent(commandQueue);
	}

	void Update(const GameTimer& gt)
	{
		mTimer.Tick();

		auto commandQueue = GetDeviceResources()->GetCommandQueue();
	
		PIXBeginEvent(commandQueue, 0, L"Update");
		{
			mCamera.Update();
			mCamData.MVP = mCamera.ProjView();
			mCamData.InvPV = mCamera.InvScreenProjView();
			mCamData.CamPos = mCamera.Position();
			mDeferredTech.UpdateConstantBuffer(mCamData, mLightData);
			UpdateWaves(gt);
		}
		PIXEndEvent(commandQueue);

	}


	void UpdateWaves(const GameTimer& gt)
	{
		// Every quarter second, generate a random wave.
		static float t_base = 0.0f;
		if ((mTimer.TotalTime() - t_base) >= 0.25f)
		{
			t_base += 0.25f;

			int i = MathHelper::Rand(4, mWave->RowCount() - 5);
			int j = MathHelper::Rand(4, mWave->ColumnCount() - 5);

			float r = MathHelper::RandF(0.2f, 0.5f);

			mWave->Disturb(i, j, r);
		}

		// Update the wave simulation.
		mWave->Update(gt.DeltaTime());

		// Update the wave vertex buffer with the new solution.
		auto currWavesVB = mCurrFrameResource->WavesVB.get();
		for (int i = 0; i < mWave->VertexCount(); ++i)
		{
			NormalVertex v;

			v.position = DirectX::XMFLOAT4(mWave->Position(i).x,mWave->Position(i).y, mWave->Position(i).z,1.0f);
		

			currWavesVB->CopyData(i, v);
		}

		// Set the dynamic VB of the wave renderitem to the current frame VB.
		mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
	}



	void Resize(UINT width, UINT height)
	{
		mWidth = width;
		mHeight = height;
		GetDeviceResources()->SetWindows(mHwnd,width,height);
		if (mInit)
		{
			mCamera.OnResize(mWidth, mHeight);
			mDeferredTech.InitWindowSizeDependentResources();
		}

	}

	void KeyDown(UINT key)
	{ 
		mCamera.KeyDown(key);
	return; }

    void MouseMove(UINT xpos, UINT ypos) {
		mCamera.InputMove(xpos, ypos);
		return; }
	void MousePress(UINT xpos, UINT ypos) {
		mCamera.InputPress(xpos,ypos);
		return; }


	
};



