#include <stdio.h>

#pragma warning(push, 3)
#include <Windows.h>
#pragma warning(pop)

#include <stdint.h>

#include "Main.h"

HWND gGameWindow;
BOOL gGameIsRunning;
GAMEBITMAP gBackBuffer;
MONITORINFO gMonitorInfo = { sizeof(MONITORINFO), 0, 0, 0};
int32_t gMonitorWidth;
int32_t gMonitorHeight;

int __stdcall WinMain(_In_ HINSTANCE Instance, _In_opt_ HINSTANCE PreviousInstance, _In_ PSTR CommandLine, _In_ int CmdShow)
{
	UNREFERENCED_PARAMETER(Instance);
	UNREFERENCED_PARAMETER(PreviousInstance);
	UNREFERENCED_PARAMETER(CommandLine);
	UNREFERENCED_PARAMETER(CmdShow);

	if (GameIsAlreadyRunning() == TRUE)
	{
		MessageBoxA(NULL, "Another Instance of this game is already running!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		goto Exit;
	}


	/* Things MUST for a WIN32 application
	 * 1. Message Loop
	 * 2. Callback function to handle messages(Window Procedure)
	*/

	if (CreateMainGameWindow() != ERROR_SUCCESS)
	{
		goto Exit;
	}

	gBackBuffer.BitmapInfo.bmiHeader.biSize = sizeof(gBackBuffer.BitmapInfo.bmiHeader);
	gBackBuffer.BitmapInfo.bmiHeader.biWidth = GAME_RES_WIDTH;
	gBackBuffer.BitmapInfo.bmiHeader.biHeight = GAME_RES_HEIGHT;
	gBackBuffer.BitmapInfo.bmiHeader.biBitCount = GAME_BPP;
	gBackBuffer.BitmapInfo.bmiHeader.biCompression = BI_RGB;
	gBackBuffer.BitmapInfo.bmiHeader.biPlanes = 1;
	gBackBuffer.Memory = VirtualAlloc(
		NULL,
		GAME_DRAWING_AREA_MEMORY_SIZE,
		MEM_RESERVE | MEM_COMMIT,
		PAGE_READWRITE
	);

	if (gBackBuffer.Memory == NULL)
	{
		MessageBoxA(NULL, "Failed to allocate memory for drawing surface!", "Error!", MB_ICONEXCLAMATION | MB_OK);

		goto Exit;
	}

	memset(gBackBuffer.Memory, 0x7F, GAME_DRAWING_AREA_MEMORY_SIZE);

	/*
	* In html and others color code is in the form RGBA, but here in win32 we have BGRA
	*/

	//ShowWindow(WindowHandle, CmdShow);
	//UpdateWindow(WindowHandle);

	MSG Message = { 0 };

	gGameIsRunning = TRUE;

	while (gGameIsRunning)
	// Eg. People may write above cond. like this -> gGameIsRunning = TRUE but compiler won't warn them about this. But if he/she writes TRUE = gGameIsRunning compiler will warn
	{
		/*Using PeekMessage instead of GetMessage because unlike GetMessage, the PeekMessage function does not wait for a message to be posted before returning.*/
		while (PeekMessageA(&Message, gGameWindow, 0, 0, PM_REMOVE))
		{
			DispatchMessageA(&Message);
		}

		/* TODO in game running
		 * 1. ProcessPlayerInput();
		 * 2. RenderGameGraphics();
		*/

		ProcessPlayerInput();
		RenderGameGraphics();
		Sleep(1);
	}

	Exit:

	return(0);
}


LRESULT CALLBACK MainWindowProc(
	_In_ HWND WindowHandle,        // handle to window
	_In_ UINT Message,             // message identifier
	_In_ WPARAM WParam,            // first message parameter
	_In_ LPARAM LParam)            // second message parameter
{
	LRESULT Result = 0;

	switch (Message)
	{
		case WM_CLOSE:
		{
			gGameIsRunning = FALSE;

			PostQuitMessage(0);

			break;
		}

		default:
			Result = DefWindowProcA(WindowHandle, Message, WParam, LParam);
	}
	return (Result);
}

DWORD CreateMainGameWindow(void)
{
	DWORD Result = ERROR_SUCCESS;

	WNDCLASSEXA WindowClass = { 0 };

	WindowClass.cbSize = sizeof(WNDCLASSEXA);
	WindowClass.style = 0;
	WindowClass.lpfnWndProc = MainWindowProc;
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.hInstance = GetModuleHandleA(NULL); // Can be Instance also
	WindowClass.hIcon = LoadIconA(NULL, IDI_APPLICATION);
	WindowClass.hIconSm = LoadIconA(NULL, IDI_APPLICATION);
	WindowClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
	WindowClass.hbrBackground = CreateSolidBrush(RGB(255, 0, 255));
	WindowClass.lpszMenuName = NULL;
	WindowClass.lpszClassName = GAME_NAME "_CLASS";

	if (RegisterClassExA(&WindowClass) == 0)
	{
		Result = GetLastError();

		MessageBoxA(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

		goto Exit;
	}

	gGameWindow = CreateWindowExA(
		WS_EX_WINDOWEDGE,
		WindowClass.lpszClassName,
		"Game Title",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		640,
		480,
		NULL,
		NULL,
		GetModuleHandleA(NULL),
		NULL
	);

	if (gGameWindow == NULL)
	{
		Result = GetLastError();

		MessageBoxA(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

		goto Exit;
	}

	if (GetMonitorInfoA(MonitorFromWindow(gGameWindow, MONITOR_DEFAULTTOPRIMARY), &gMonitorInfo) == 0)
	{
		Result = ERROR_MONITOR_NO_DESCRIPTOR;
		
		goto Exit;
	}

	gMonitorWidth = gMonitorInfo.rcMonitor.right - gMonitorInfo.rcMonitor.left;
	gMonitorHeight = gMonitorInfo.rcMonitor.bottom - gMonitorInfo.rcMonitor.top;

	// (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_OVERLAPPEDWINDOW = WS_VISIBLE
	if (SetWindowLongPtrA(gGameWindow, GWL_STYLE, WS_VISIBLE) == 0)
	{
		Result = GetLastError();

		goto Exit;
	}

	// For changing size of window
	if (SetWindowPos(gGameWindow, HWND_TOP, gMonitorInfo.rcMonitor.left, gMonitorInfo.rcMonitor.top, gMonitorWidth, gMonitorHeight, SWP_FRAMECHANGED) == 0)
	{
		Result = GetLastError();

		goto Exit;
	}

Exit:

	return (Result);
}

BOOL GameIsAlreadyRunning(void)
{
	HANDLE Mutex = NULL;

	Mutex = CreateMutexA(NULL, TRUE, GAME_NAME "_MUTEX");

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
}

void ProcessPlayerInput(void)
{
	int16_t EscapeKeyIsDown = GetAsyncKeyState(VK_ESCAPE);

	if (EscapeKeyIsDown)
	{
		SendMessageA(gGameWindow, WM_CLOSE, 0, 0);
	}
}

void RenderGameGraphics(void)
{

	//memset(gBackBuffer.Memory, 0xFF, GAME_RES_WIDTH * GAME_RES_HEIGHT * 4);

	PIXEL32 Pixel = { 0 };

	Pixel.Blue = 0xff;
	Pixel.Green = 0;
	Pixel.Red = 0;
	Pixel.Alpha = 0xff;

	for (int i = 0; i < GAME_RES_WIDTH * GAME_RES_HEIGHT; i++)
	{
		memcpy((PIXEL32*)gBackBuffer.Memory + i, &Pixel, sizeof(PIXEL32));
	}

	HDC DeviceContext = GetDC(gGameWindow);

	StretchDIBits(
		DeviceContext, 
		0, 
		0, 
		gMonitorWidth, 
		gMonitorHeight, 
		0, 
		0, 
		GAME_RES_WIDTH, 
		GAME_RES_HEIGHT, 
		gBackBuffer.Memory, 
		&gBackBuffer.BitmapInfo, 
		DIB_RGB_COLORS, 
		SRCCOPY
	);

	ReleaseDC(gGameWindow, DeviceContext);
}
