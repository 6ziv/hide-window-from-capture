#include "win_event_hook.h"
#include <stdio.h>
VOID WorkWithHWND(HWND hWnd);
VOID CALLBACK WinEventProc(
	HWINEVENTHOOK hWinEventHook,
	DWORD event,
	HWND hwnd,
	LONG idObject,
	LONG idChild,
	DWORD idEventThread,
	DWORD dwmsEventTime
)
{
	(void)hWinEventHook;
	(void)idChild;
	(void)idEventThread;
	(void)dwmsEventTime;

	if (event == EVENT_OBJECT_CREATE && idObject == OBJID_WINDOW) {
		if (hwnd && ((GetWindowLongA(hwnd, GWL_STYLE) & WS_CHILD) == 0))
			WorkWithHWND(hwnd);
	}
	if (event == EVENT_OBJECT_IME_SHOW && idObject == OBJID_WINDOW) {
		if (hwnd && ((GetWindowLongA(hwnd, GWL_STYLE) & WS_CHILD) == 0)) {
			DWORD pid;
			GetWindowThreadProcessId(hwnd, &pid);
			if (pid == GetCurrentProcessId()) {
				WorkWithHWND(hwnd);
			}
		}
	}
}
typedef struct {
	HANDLE ev;
}ThreadParams;
DWORD APIENTRY WinEventHookListenerProc(LPVOID lpParameter) {
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_CREATE, GetModuleHandleA(NULL), WinEventProc, GetCurrentProcessId(), 0, WINEVENT_SKIPOWNTHREAD | WINEVENT_INCONTEXT);
	SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW, GetModuleHandleA(NULL), WinEventProc, GetCurrentProcessId(), 0, WINEVENT_SKIPOWNTHREAD | WINEVENT_INCONTEXT);
	SetWinEventHook(EVENT_OBJECT_IME_SHOW, EVENT_OBJECT_IME_SHOW, GetModuleHandleA(NULL), WinEventProc, GetCurrentProcessId(), 0, WINEVENT_SKIPOWNTHREAD | WINEVENT_INCONTEXT);
	ThreadParams* params = (ThreadParams*)(lpParameter);
	SetEvent(params->ev);
	
	while (1) {
		GetMessage(&msg, NULL, 0, 0);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}
BOOL SetupWinEventHook() {
	ThreadParams params;
	params.ev = CreateEventA(NULL, FALSE, FALSE, NULL);
	if (!params.ev)
		return FALSE;
	
	HANDLE hThread = CreateThread(NULL, 0, WinEventHookListenerProc, &params, 0, NULL);
	if(hThread==NULL || hThread==INVALID_HANDLE_VALUE){
		CloseHandle(params.ev);
		return FALSE;
	}
	WaitForSingleObject(params.ev, 3000);
	CloseHandle(params.ev);
	
	return TRUE;
}