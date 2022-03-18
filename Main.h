#pragma once

#define GAME_NAME							"Game"
#define GAME_RES_WIDTH						384
#define GAME_RES_HEIGHT						240
#define GAME_BPP							32
#define GAME_DRAWING_AREA_MEMORY_SIZE		(GAME_RES_WIDTH * GAME_RES_HEIGHT * (GAME_BPP / 8)) // In bytes
#define CALCULATE_AVG_FPS_EVERY_X_FRAMES	100 // This means that after every 100 frames we are going to calc avg fps
#define TARGET_MICROSECONDS_PER_FRAME		16667

#pragma warning(disable: 4820) // Disable Warning about Struct padding
#pragma warning(disable: 5045) // Disable Warning about Spectre/Meltdown CPU vulnerability

typedef LONG(NTAPI* _NtQueryTimerResolution) (OUT PULONG MinimumResolution, OUT PULONG MaximumResolution, OUT PULONG CurrentResolution);

_NtQueryTimerResolution NtQueryTimerResolution;

//#pragma pack(1) // This line will tell compiler to don't pad the data structure
typedef struct GAMEBITMAP
{
	BITMAPINFO BitmapInfo;	// 44 bytes
	// 4 bytes of padding here
	void* Memory;			// 8 bytes
} GAMEBITMAP;

typedef struct PIXEL32
{
	uint8_t Blue;
	uint8_t Green;
	uint8_t Red;
	uint8_t Alpha;
} PIXEL32;

typedef struct GAMEPERFDATA
{
	uint64_t TotalFramesRendered;
	float RawFPSAverage;
	float CookedFPSAverage;
	int64_t PerfFrequency;
	MONITORINFO MonitorInfo;
	int32_t MonitorWidth;
	int32_t MonitorHeight;
	BOOL DisplayDebugInfo;
	LONG MinimumTimerResolution;
	LONG MaximumTimerResolution;
	LONG CurrentTimerResolution;
} GAMEPERFDATA;


LRESULT CALLBACK MainWindowProc(_In_ HWND WindowHandle, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam);
DWORD CreateMainGameWindow(void);
BOOL GameIsAlreadyRunning(void);
void ProcessPlayerInput(void);
void RenderGameGraphics(void);
void ClearScreen(_In_ __m128i Color);