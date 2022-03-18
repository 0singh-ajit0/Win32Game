#pragma warning(push, 3)
#include <stdio.h>

#include <Windows.h>

#include <emmintrin.h>
#pragma warning(pop)

#include <stdint.h>
#include "Main.h"

HWND gGameWindow;
BOOL gGameIsRunning;
GAMEBITMAP gBackBuffer;
GAMEPERFDATA gPerformanceData;

int __stdcall WinMain(_In_ HINSTANCE Instance, _In_opt_ HINSTANCE PreviousInstance, _In_ PSTR CommandLine, _In_ int CmdShow)
{
	UNREFERENCED_PARAMETER(Instance);
	UNREFERENCED_PARAMETER(PreviousInstance);
	UNREFERENCED_PARAMETER(CommandLine);
	UNREFERENCED_PARAMETER(CmdShow);

	MSG Message = { 0 };
	int64_t FrameStart = 0; // Time at frame startup
	int64_t FrameEnd = 0; // Time at frame completion
	int64_t ElapsedMicrosecondsPerFrame = 0;
	int64_t ElapsedMicorsecondsPerFrameAccumulatorRaw = 0;
	int64_t ElapsedMicorsecondsPerFrameAccumulatorCooked = 0;

	HMODULE NtDllModuleHandle;

	if ((NtDllModuleHandle = GetModuleHandleA("ntdll.dll")) == NULL)
	{
		MessageBoxA(NULL, "Could not load ntdll.dll !", "Error!", MB_ICONEXCLAMATION | MB_OK);

		goto Exit;
	}

	if ((NtQueryTimerResolution = (_NtQueryTimerResolution)GetProcAddress(NtDllModuleHandle, "NtQueryTimerResolution")) == NULL)
	{
		MessageBoxA(NULL, "Could not find NtQueryTimerResolution function in ntdll.dll !", "Error!", MB_ICONEXCLAMATION | MB_OK);

		goto Exit;
	}

	NtQueryTimerResolution(&gPerformanceData.MinimumTimerResolution, &gPerformanceData.MaximumTimerResolution, &gPerformanceData.CurrentTimerResolution);

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

	QueryPerformanceFrequency((LARGE_INTEGER*) & gPerformanceData.PerfFrequency);

	gPerformanceData.DisplayDebugInfo = TRUE;

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

	gGameIsRunning = TRUE;

	while (gGameIsRunning)
	// Eg. People may write above cond. like this -> gGameIsRunning = TRUE but compiler won't warn them about this. But if he/she writes TRUE = gGameIsRunning compiler will warn
	{
		QueryPerformanceCounter((LARGE_INTEGER*) &FrameStart);
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

		QueryPerformanceCounter((LARGE_INTEGER*) &FrameEnd);

		ElapsedMicrosecondsPerFrame = FrameEnd - FrameStart;
		// FrameStart and FrameEnd are in seconds. Convert it to microseconds to guard against loss-of-precision before dividing it by number of ticks-per-second.

		ElapsedMicrosecondsPerFrame *= 1000000;

		ElapsedMicrosecondsPerFrame /= gPerformanceData.PerfFrequency;

		gPerformanceData.TotalFramesRendered++;

		ElapsedMicorsecondsPerFrameAccumulatorRaw += ElapsedMicrosecondsPerFrame;

		while (ElapsedMicrosecondsPerFrame <= TARGET_MICROSECONDS_PER_FRAME)
		{
			ElapsedMicrosecondsPerFrame = FrameEnd - FrameStart;
			ElapsedMicrosecondsPerFrame *= 1000000;
			ElapsedMicrosecondsPerFrame /= gPerformanceData.PerfFrequency;

			QueryPerformanceCounter((LARGE_INTEGER*) & FrameEnd);

			if (ElapsedMicrosecondsPerFrame <= (TARGET_MICROSECONDS_PER_FRAME - gPerformanceData.CurrentTimerResolution))
			{
				Sleep(1); // This sleep could be anywhere between 1ms and full system timer tick(15 ms)
			}
		}

		ElapsedMicorsecondsPerFrameAccumulatorCooked += ElapsedMicrosecondsPerFrame;

		if ((gPerformanceData.TotalFramesRendered % CALCULATE_AVG_FPS_EVERY_X_FRAMES) == 0)
		{
			gPerformanceData.RawFPSAverage = 1.0f / ((ElapsedMicorsecondsPerFrameAccumulatorRaw / CALCULATE_AVG_FPS_EVERY_X_FRAMES) * 0.000001f);
			gPerformanceData.CookedFPSAverage = 1.0f / ((ElapsedMicorsecondsPerFrameAccumulatorCooked / CALCULATE_AVG_FPS_EVERY_X_FRAMES) * 0.000001f);

			ElapsedMicorsecondsPerFrameAccumulatorRaw = 0;
			ElapsedMicorsecondsPerFrameAccumulatorCooked = 0;
		}
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
		NULL,
		WindowClass.lpszClassName,
		"Game Title",
		WS_VISIBLE,
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

	gPerformanceData.MonitorInfo.cbSize = sizeof(MONITORINFO);

	if (GetMonitorInfoA(MonitorFromWindow(gGameWindow, MONITOR_DEFAULTTOPRIMARY), &gPerformanceData.MonitorInfo) == 0)
	{
		Result = ERROR_MONITOR_NO_DESCRIPTOR;
		
		goto Exit;
	}

	gPerformanceData.MonitorWidth = gPerformanceData.MonitorInfo.rcMonitor.right - gPerformanceData.MonitorInfo.rcMonitor.left;
	gPerformanceData.MonitorHeight = gPerformanceData.MonitorInfo.rcMonitor.bottom - gPerformanceData.MonitorInfo.rcMonitor.top;

	// (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_OVERLAPPEDWINDOW = WS_VISIBLE
	if (SetWindowLongPtrA(gGameWindow, GWL_STYLE, WS_VISIBLE) == 0)
	{
		Result = GetLastError();

		goto Exit;
	}

	// For changing size of window
	if (SetWindowPos(
		gGameWindow, 
		HWND_TOP, 
		gPerformanceData.MonitorInfo.rcMonitor.left, 
		gPerformanceData.MonitorInfo.rcMonitor.top, 
		gPerformanceData.MonitorWidth, 
		gPerformanceData.MonitorHeight, 
		SWP_FRAMECHANGED | SWP_NOOWNERZORDER) == 0
	)
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

	int16_t DebugKeyIsDown = GetAsyncKeyState(VK_F1);

	static int16_t DebugKeyWasDown = 0;

	if (EscapeKeyIsDown)
	{
		SendMessageA(gGameWindow, WM_CLOSE, 0, 0);
	}

	if (DebugKeyIsDown && !DebugKeyWasDown)
	{
		gPerformanceData.DisplayDebugInfo = !gPerformanceData.DisplayDebugInfo;
	}

	DebugKeyWasDown = DebugKeyIsDown;
}

void RenderGameGraphics(void)
{

	//memset(gBackBuffer.Memory, 0xFF, GAME_RES_WIDTH * GAME_RES_HEIGHT * 4);

	__m128i QuadPixel = { 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff};

	//ClearScreen(QuadPixel);
	for (int i = 0; i < GAME_RES_WIDTH * GAME_RES_HEIGHT; i += 4)
	{
		_mm_store_si128((PIXEL32*)gBackBuffer.Memory + i, QuadPixel);
	}

	int32_t ScreenX = 25;
	int32_t ScreenY = 25;

	int32_t StartingScreenPixel = ((GAME_RES_WIDTH * GAME_RES_HEIGHT) - GAME_RES_WIDTH) - (GAME_RES_WIDTH * ScreenY) + ScreenX;

	for (int32_t y = 0; y < 16; y++)
	{
		for (int32_t x = 0; x < 16; x++)
		{
			memset((PIXEL32*)gBackBuffer.Memory + (uintptr_t)StartingScreenPixel + x - ((uintptr_t)GAME_RES_WIDTH * y), 0xFF, sizeof(PIXEL32));
		}
	}

	HDC DeviceContext = GetDC(gGameWindow);

	StretchDIBits(
		DeviceContext, 
		0, 
		0, 
		gPerformanceData.MonitorWidth, 
		gPerformanceData.MonitorHeight,
		0, 
		0, 
		GAME_RES_WIDTH, 
		GAME_RES_HEIGHT, 
		gBackBuffer.Memory, 
		&gBackBuffer.BitmapInfo, 
		DIB_RGB_COLORS, 
		SRCCOPY
	);

	if (gPerformanceData.DisplayDebugInfo)
	{
		SelectObject(DeviceContext, (HFONT)GetStockObject(ANSI_FIXED_FONT));

		char DebugTextBuffer[64] = { 0 };

		sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Raw FPS:    %.01f", gPerformanceData.RawFPSAverage);

		TextOutA(DeviceContext, 0, 0, DebugTextBuffer, (int)strlen(DebugTextBuffer));
	
		sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Cooked FPS: %.01f", gPerformanceData.CookedFPSAverage);

		TextOutA(DeviceContext, 0, 13, DebugTextBuffer, (int)strlen(DebugTextBuffer));

		sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Min Timer Res: %.02f", gPerformanceData.MinimumTimerResolution / 10000.0f);

		TextOutA(DeviceContext, 0, 26, DebugTextBuffer, (int)strlen(DebugTextBuffer));

		sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Max Timer Res: %.02f", gPerformanceData.MaximumTimerResolution / 10000.0f);

		TextOutA(DeviceContext, 0, 39, DebugTextBuffer, (int)strlen(DebugTextBuffer));

		sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Cur Timer Res: %.02f", gPerformanceData.CurrentTimerResolution / 10000.0f);

		TextOutA(DeviceContext, 0, 52, DebugTextBuffer, (int)strlen(DebugTextBuffer));
	}

	ReleaseDC(gGameWindow, DeviceContext);
}

__forceinline void ClearScreen(_In_ __m128i Color)
{
	/*GAME_RES_WIDTH * GAME_RES_HEIGHT = Number of PIXEL32 pixels in back buffer*/
	for (int i = 0; i < GAME_RES_WIDTH * GAME_RES_HEIGHT; i += 4)
	{
		_mm_store_si128((PIXEL32*)gBackBuffer.Memory + i, Color);
	}
}
