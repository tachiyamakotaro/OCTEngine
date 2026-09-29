#include "stdafx.h"
#include "system.h"

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

HWND			g_hWnd = NULL;				// Window handle.

// The one and only low-level engine instance.
// K2EngineLow::Init() sets the global g_engine to point at this object,
// so after InitGame() you can drive the frame with g_engine->BeginFrame() etc.
static K2EngineLow* g_k2EngineLow = nullptr;

ID3D12DescriptorHeap* g_imguiSrvHeap = nullptr;

struct ExampleDescriptorHeapAllocator
{
	ID3D12DescriptorHeap* Heap = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
	D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
	UINT                        HeapHandleIncrement;
	ImVector<int>               FreeIndices;

	void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
	{
		IM_ASSERT(Heap == nullptr && FreeIndices.empty());
		Heap = heap;
		D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
		HeapType = desc.Type;
		HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
		HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
		HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
		FreeIndices.reserve((int)desc.NumDescriptors);
		for (int n = desc.NumDescriptors; n > 0; n--)
			FreeIndices.push_back(n - 1);
	}
	void Destroy()
	{
		Heap = nullptr;
		FreeIndices.clear();
	}
	void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
	{
		IM_ASSERT(FreeIndices.Size > 0);
		int idx = FreeIndices.back();
		FreeIndices.pop_back();
		out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
		out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
	}
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
	{
		int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
		int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
		IM_ASSERT(cpu_idx == gpu_idx);
		FreeIndices.push_back(cpu_idx);
	}
};

static ExampleDescriptorHeapAllocator g_imguiSrvHeapAlloc;


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
///////////////////////////////////////////////////////////////////
// Window message procedure.
///////////////////////////////////////////////////////////////////
LRESULT CALLBACK MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

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

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsClassic();
	ImGui_ImplWin32_Init(g_hWnd);

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;		// テクスチャ用の窓口
	desc.NumDescriptors = 64;											// 窓口の数
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// シェーダーから見えるようにする
	g_graphicsEngine->GetD3DDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_imguiSrvHeap));

	g_imguiSrvHeapAlloc.Create(g_graphicsEngine->GetD3DDevice(), g_imguiSrvHeap);

	ImGui_ImplDX12_InitInfo info = {};
	info.Device = g_graphicsEngine->GetD3DDevice();
	info.CommandQueue = g_graphicsEngine->GetCommandQueue();
	info.NumFramesInFlight = 2;			// FRAME_BUFFER_COUNT
	info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	info.SrvDescriptorHeap = g_imguiSrvHeap;
	info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return g_imguiSrvHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
	info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return g_imguiSrvHeapAlloc.Free(cpu_handle, gpu_handle); };
	ImGui_ImplDX12_Init(&info);
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
