#include "stdafx.h"
#include "system/system.h"

#include<InitGUID.h>
#include<dxgidebug.h>

#include "Game.h"



void ReportLiveObjects()
{
	IDXGIDebug* pDxgiDebug;

	typedef HRESULT(__stdcall* fPtr)(const IID&, void**);
	HMODULE hDll = GetModuleHandleW(L"dxgidebug.dll");
	fPtr DXGIGetDebugInterface = (fPtr)GetProcAddress(hDll, "DXGIGetDebugInterface");

	DXGIGetDebugInterface(__uuidof(IDXGIDebug), (void**)&pDxgiDebug);

	pDxgiDebug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_DETAIL);
}

///////////////////////////////////////////////////////////////////
// Windows application entry point.
///////////////////////////////////////////////////////////////////
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	// Initialize the game (window + K2EngineLow).
	InitGame(hInstance, hPrevInstance, lpCmdLine, nCmdShow, TEXT("Game"));
	//////////////////////////////////////
	// Write your own start-up code from here.
	//////////////////////////////////////

	// Create the Game object.
	NewGO<Game>(0, "game");

	//////////////////////////////////////
	// End of start-up code.
	//////////////////////////////////////

	// Game loop.
	while (DispatchWindowMessage())
	{
		if (g_pad[0]->IsTrigger(enButtonA)) {
			g_pad[0]->SetVibration(/*durationSec=*/0.5f, /*normalizedPower=*/1.0f);
		}

		// Drive one frame of the low-level engine yourself.
		// (K2Engine used to hide this behind K2Engine::Execute().)
		g_engine->BeginFrame();		// Begin the frame: clear the screen, update input, etc.
		g_engine->ExecuteUpdate();	// Update all game objects (IGameObject::Update).
		g_engine->ExecuteRender();	// Render all game objects (IGameObject::Render).
		g_engine->EndFrame();		// End the frame: present the back buffer.
	}

	// Shut down.
	FinalizeGame();

#ifdef _DEBUG
	ReportLiveObjects();
#endif // _DEBUG
	return 0;
}
