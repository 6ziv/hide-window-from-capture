#pragma once
#include <Windows.h>
typedef BOOL(CALLBACK* WALKTHREADSINIT)(SIZE_T cntThreads, LPVOID lParameter);
typedef BOOL(CALLBACK* WALKTHREADSCALLBACK)(DWORD dwThreadId, LPVOID lParameter);
typedef BOOL(CALLBACK* WALKWINDOWSCALLBACK)(HWND hWnd, LPARAM lParameter);

BOOL WalkThreads(WALKTHREADSINIT init,WALKTHREADSCALLBACK callback, LPVOID lParameters, BOOL SkipSelf);
BOOL WalkWindows(WALKWINDOWSCALLBACK callback, LPARAM lParam,BOOL SkipSelf);
