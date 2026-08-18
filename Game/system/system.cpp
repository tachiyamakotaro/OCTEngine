#include "stdafx.h"
#include "system.h"

HWND			g_hWnd = NULL;				// Window handle.

// The one and only low-level engine instance.
// K2EngineLow::Init() sets the global g_engine to point at this object,
// so after InitGame() you can drive the frame with g_engine->BeginFrame() etc.
static K2EngineLow* g_k2EngineLow = nullptr;

///////////////////////////////////////////////////////////////////
// Window message procedure.
///////////////////////////////////////////////////////////////////
LRESULT CALLBACK MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	return 0;
}

///////////////////////////////////////////////////////////////////
// Create the application window.
///////////////////////////////////////////////////////////////////
void InitWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow, const TCHAR* appName)
{
	WNDCLASSEX wc =
	{
		sizeof(WNDCLASSEX),
		CS_CLASSDC,
		MsgProc,
		0,
		0,
		GetModuleHandle(NULL),
		NULL,
		NULL,
		NULL,
		NULL,
		appName,
		NULL
	};
	RegisterClassEx(&wc);

	g_hWnd = CreateWindow(
		appName,
		appName,
		WS_OVERLAPPEDWINDOW,
		0,
		0,
		FRAME_BUFFER_W,
		FRAME_BUFFER_H,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	ShowWindow(g_hWnd, nCmdShow);
}

// Initialize the game.
void InitGame(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow, const TCHAR* appName)
{
	// Create the window.
	InitWindow(hInstance, hPrevInstance, lpCmdLine, nCmdShow, appName);

	// Create the low-level engine.
	g_k2EngineLow = new K2EngineLow;

	// We do NOT use ray tracing in this template, but GraphicsEngine::Init still
	// initializes the ray tracing engine, so hand it a tiny dummy resource
	// (a valid pointer with a non-zero size) to keep the internal buffers happy.
	static int dummyExpandShaderResource = 0;
	raytracing::InitData raytracingInitData;
	raytracingInitData.m_expandShaderResource = &dummyExpandShaderResource;
	raytracingInitData.m_expandShaderResourceSize = sizeof(dummyExpandShaderResource);

	g_k2EngineLow->Init(g_hWnd, FRAME_BUFFER_W, FRAME_BUFFER_H, raytracingInitData);

	// Give the 3D camera a sensible default position so that anything you draw
	// later shows up on screen. Change this freely once you have your own camera.
	g_camera3D->SetPosition({ 0.0f, 100.0f, -200.0f });
	g_camera3D->SetTarget({ 0.0f, 50.0f, 0.0f });
}

// Destroy the low-level engine.
void FinalizeGame()
{
	delete g_k2EngineLow;
	g_k2EngineLow = nullptr;
}

// Pump window messages. Returns false when the game should quit.
bool DispatchWindowMessage()
{
	MSG msg = { 0 };
	while (WM_QUIT != msg.message) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			break;
		}
	}
	return msg.message != WM_QUIT;
}
