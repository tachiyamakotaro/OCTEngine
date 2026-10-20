#include "stdafx.h"
#include "system/system.h"

#include<dxgidebug.h>
#include<InitGUID.h>

#include "Game.h"

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"



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

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		{
			// ウィンドウの中の大きさとフレームバッファ（FRAME_BUFFER_W x FRAME_BUFFER_H）が違うと、
			// ImGui がフレームバッファの左上の一部にだけ描かれ、それが画面に引き伸ばされるので、絵とマウスの位置がずれる。
			// ウィンドウの中の大きさ（DisplaySize）からフレームバッファへの倍率を渡して、フレームバッファ全体に描かせる。
			ImGuiIO& io = ImGui::GetIO();
			if (io.DisplaySize.x > 0.0f && io.DisplaySize.y > 0.0f)
			{
				io.DisplayFramebufferScale = ImVec2(
					static_cast<float>(FRAME_BUFFER_W) / io.DisplaySize.x,
					static_cast<float>(FRAME_BUFFER_H) / io.DisplaySize.y);
			}
		}
		ImGui::NewFrame();

		g_engine->ExecuteUpdate();	// Update all game objects (IGameObject::Update).
		g_engine->ExecuteRender();	// Render all game objects (IGameObject::Render).
		RenderingEngine::GetInstance()->Execute(g_graphicsEngine->GetRenderContext());

		ImGui::Render();
#ifdef _DEBUG
		// Debug のときだけ ImGui を画面に描く（F4 で出す / 消す）
		if (Game::IsImGuiVisible())
		{
			auto* cmdList = g_graphicsEngine->GetCommandList();
			cmdList->SetDescriptorHeaps(1, &g_imguiSrvHeap);
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
		}
#endif // _DEBUG

		g_engine->EndFrame();		// End the frame: present the back buffer.
	}

	// Shut down.
	FinalizeGame();

#ifdef _DEBUG
	ReportLiveObjects();
#endif // _DEBUG
	return 0;
}
