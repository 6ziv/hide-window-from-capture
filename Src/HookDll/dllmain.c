// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include <Windows.h>
#include <stdio.h>
#include <detours/detours.h>
#include <processsnapshot.h>
#include <dwmapi.h>
#include "hide_preview_content.h"
#include "thread_window_walker.h"
#include "api_hook.h"
#include "win_event_hook.h"
#include "hiding_from_taskbar.h"

#include "config.h"
#pragma comment(lib,"dwmapi.lib")
#if _DLL
#warning "Using /MT is suggested."
#endif

VOID WorkWithHWND(HWND hWnd) {
	SetWindowDisplayAffinity(hWnd, WDA_EXCLUDEFROMCAPTURE);
	if(g_config->HidePreview)
		StartHidePreview(hWnd);
	if(g_config->HideTaskbar)
		StartHideTaskbar(hWnd);
}


BOOL CALLBACK windowInitializerRun(HWND hWnd, LPARAM lParameter){
	(void)lParameter;
	WorkWithHWND(hWnd);
	return TRUE;
}

DWORD APIENTRY SetupHook(LPVOID lpUnused) {
	(void)lpUnused;
	if (g_config->HidePreview) {
		LoadImagesFromHandles(g_config->preview_mapping, g_config->icon_mapping);
		//LoadPreviewImage(g_config->ImagePath, g_config->BackgroundColor, g_config->KeepRatio,g_config->ImageType);
	}

	if (!SetupWinEventHook())
		return GetLastError();
	if(!WalkWindows(windowInitializerRun, 0, TRUE))
		return GetLastError();
	return ERROR_SUCCESS;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	(void)lpReserved;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if (DetourIsHelperProcess())
			return TRUE;

		initConfig();

		DisableThreadLibraryCalls(hModule);
		BOOL is_injected = TRUE;
		if(FALSE == DetourRestoreAfterWith() && GetLastError()==ERROR_MOD_NOT_FOUND)is_injected = FALSE;	//Detours inject is not suitable. Because of the loader lock.
		SetupAPIHook(hModule);
		//if (!DoDetours())return FALSE;
		CreateThread(NULL, 0, SetupHook, NULL, 0, NULL);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

