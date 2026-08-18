#pragma once

extern HWND			g_hWnd ;				// Window handle.

// Initialize the game (creates the window and the low-level engine).
void InitGame(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow, const TCHAR* appName);
// Shut down the game (destroys the low-level engine).
void FinalizeGame();
// Pump window messages. Returns false when the game should quit.
bool DispatchWindowMessage();
