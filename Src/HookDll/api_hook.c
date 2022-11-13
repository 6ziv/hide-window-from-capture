#define _CRT_SECURE_NO_WARNINGS
#include "api_hook.h"
#include <detours/detours.h>
#include <dwmapi.h>
#include <strsafe.h>
#include "thread_window_walker.h"
#include "hide_preview_content.h"
#include "config.h"
#include "guid.h"

typedef BOOL(WINAPI* SetWindowDisplayAffinityT)(HWND, DWORD);
static SetWindowDisplayAffinityT TrueSetWindowDisplayAffinity = NULL;
BOOL WINAPI FakeSetWindowDisplayAffinity(HWND hwnd, DWORD affinity) {
	(void)affinity;
	return TrueSetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
}

LPCSTR dll_path = NULL;
#define DETOUR_DOCUMENTED_APIS

typedef BOOL(WINAPI* CreateProcessAType)(LPCSTR, LPSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCSTR, LPSTARTUPINFOA, LPPROCESS_INFORMATION);
typedef BOOL(WINAPI* CreateProcessWType)(LPCWSTR, LPWSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCWSTR, LPSTARTUPINFOW, LPPROCESS_INFORMATION);
typedef BOOL(WINAPI* CreateProcessAsUserAType)(HANDLE, LPCSTR, LPSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCSTR, LPSTARTUPINFOA, LPPROCESS_INFORMATION);
typedef BOOL(WINAPI* CreateProcessAsUserWType)(HANDLE, LPCWSTR, LPWSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCWSTR, LPSTARTUPINFOW, LPPROCESS_INFORMATION);
typedef BOOL(WINAPI* CreateProcessWithTokenWType)(HANDLE, DWORD, LPCWSTR, LPWSTR, DWORD, LPVOID, LPCWSTR, LPSTARTUPINFOW, LPPROCESS_INFORMATION);
typedef BOOL(WINAPI* CreateProcessWithLogonWType)(LPCWSTR, LPCWSTR, LPCWSTR, DWORD, LPCWSTR, LPWSTR, DWORD, LPVOID, LPCWSTR, LPSTARTUPINFOW, LPPROCESS_INFORMATION);
CreateProcessAType TrueCreateProcessA = NULL;
CreateProcessWType TrueCreateProcessW = NULL;
CreateProcessAsUserAType TrueCreateProcessAsUserA = NULL;
CreateProcessAsUserWType TrueCreateProcessAsUserW = NULL;
CreateProcessWithTokenWType TrueCreateProcessWithTokenW = NULL;
CreateProcessWithLogonWType TrueCreateProcessWithLogonW = NULL;
#define ImplementFakeCreateProcess(InvokeRealFunc)\
{\
	PROCESS_INFORMATION backup;\
	if (lpProcessInformation == NULL)\
	lpProcessInformation = &backup;\
	if (!(InvokeRealFunc))\
	{\
		return FALSE;\
	}\
	CONFIG child_cfg;\
	CopyMemory(&child_cfg, g_config, sizeof(CONFIG));\
	HANDLE hChildPreviewMapping = NULL;\
	HANDLE hChildIconMapping = NULL;\
	if (g_config->preview_mapping == NULL || g_config->preview_mapping == INVALID_HANDLE_VALUE)\
	{\
		child_cfg.preview_mapping = NULL;\
	}\
	else\
	{\
		if (DuplicateHandle(GetCurrentProcess(), g_config->preview_mapping, lpProcessInformation->hProcess, &hChildPreviewMapping, 0, FALSE, DUPLICATE_SAME_ACCESS)) \
		{\
			child_cfg.preview_mapping = hChildPreviewMapping;\
		}\
		else \
		{\
			child_cfg.preview_mapping = NULL;\
		}\
	}\
	if (g_config->icon_mapping == NULL || g_config->icon_mapping == INVALID_HANDLE_VALUE)\
	{\
		child_cfg.icon_mapping = NULL;\
	}\
	else\
	{\
		if (DuplicateHandle(GetCurrentProcess(), g_config->icon_mapping, lpProcessInformation->hProcess, &hChildIconMapping, 0, FALSE, DUPLICATE_SAME_ACCESS)) \
		{\
			child_cfg.icon_mapping = hChildIconMapping;\
		}\
		else\
		{\
			child_cfg.icon_mapping = NULL;\
		}\
	}\
	DetourCopyPayloadToProcess(lpProcessInformation->hProcess, &HIDE_WINDOW_CONFIG_GUID, &child_cfg, sizeof(CONFIG));\
	if (!DetourUpdateProcessWithDll(lpProcessInformation->hProcess, &dll_path, 1) && !DetourProcessViaHelperA(lpProcessInformation->dwProcessId, dll_path, TrueCreateProcessA))\
	OutputDebugStringA("Detour failed.");\
	if (0 == (dwCreationFlags & CREATE_SUSPENDED)) \
	{\
		ResumeThread(lpProcessInformation->hThread);\
	}\
	if (lpProcessInformation == &backup) \
	{\
		CloseHandle(lpProcessInformation->hThread);\
		CloseHandle(lpProcessInformation->hProcess);\
	}\
	return TRUE;\
}

BOOL WINAPI FakeCreateProcessA(
	LPCSTR               lpApplicationName,
	LPSTR                lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL                  bInheritHandles,
	DWORD                 dwCreationFlags,
	LPVOID                lpEnvironment,
	LPCSTR               lpCurrentDirectory,
	LPSTARTUPINFOA        lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
) {
	ImplementFakeCreateProcess(TrueCreateProcessA(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags | CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
}
BOOL WINAPI FakeCreateProcessW(
	LPCWSTR               lpApplicationName,
	LPWSTR                lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL                  bInheritHandles,
	DWORD                 dwCreationFlags,
	LPVOID                lpEnvironment,
	LPCWSTR               lpCurrentDirectory,
	LPSTARTUPINFOW        lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
) {
	ImplementFakeCreateProcess(TrueCreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags | CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
}
BOOL FakeCreateProcessAsUserA(
	HANDLE                hToken,
	LPCSTR                lpApplicationName,
	LPSTR                 lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL                  bInheritHandles,
	DWORD                 dwCreationFlags,
	LPVOID                lpEnvironment,
	LPCSTR                lpCurrentDirectory,
	LPSTARTUPINFOA        lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
) {
	ImplementFakeCreateProcess(TrueCreateProcessAsUserA(hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags | CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
}
BOOL FakeCreateProcessAsUserW(
	HANDLE                hToken,
	LPCWSTR                lpApplicationName,
	LPWSTR                 lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL                  bInheritHandles,
	DWORD                 dwCreationFlags,
	LPVOID                lpEnvironment,
	LPCWSTR                lpCurrentDirectory,
	LPSTARTUPINFOW        lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
) {
	ImplementFakeCreateProcess(TrueCreateProcessAsUserW(hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags | CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
}
BOOL FakeCreateProcessWithTokenW(
	HANDLE                hToken,
	DWORD                 dwLogonFlags,
	LPCWSTR               lpApplicationName,
	LPWSTR                lpCommandLine,
	DWORD                 dwCreationFlags,
	LPVOID                lpEnvironment,
	LPCWSTR               lpCurrentDirectory,
	LPSTARTUPINFOW        lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
) {
	ImplementFakeCreateProcess(TrueCreateProcessWithTokenW(hToken, dwLogonFlags, lpApplicationName, lpCommandLine, dwCreationFlags | CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
}
BOOL FakeCreateProcessWithLogonW(
	LPCWSTR               lpUsername,
	LPCWSTR               lpDomain,
	LPCWSTR               lpPassword,
	DWORD                 dwLogonFlags,
	LPCWSTR               lpApplicationName,
	LPWSTR                lpCommandLine,
	DWORD                 dwCreationFlags,
	LPVOID                lpEnvironment,
	LPCWSTR               lpCurrentDirectory,
	LPSTARTUPINFOW        lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
) {
	ImplementFakeCreateProcess(TrueCreateProcessWithLogonW(lpUsername, lpDomain, lpPassword, dwLogonFlags, lpApplicationName, lpCommandLine, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
}
#undef ImplementFakeCreateProcess

typedef struct {
	SIZE_T size;
	SIZE_T capacity;
	HANDLE* hThreads;
}WalkThreadContext;
BOOL CALLBACK WalkThreadInitCallback(SIZE_T cntThreads, LPVOID lParameter) {
	WalkThreadContext* ctx = (WalkThreadContext*)(lParameter);
	ctx->capacity = cntThreads;
	ctx->size = 0;
	ctx->hThreads = HeapAlloc(GetProcessHeap(), 0, sizeof(SIZE_T) + cntThreads * sizeof(HANDLE));
	if (ctx->hThreads == NULL)return FALSE;
	return TRUE;
}
BOOL CALLBACK WalkThreadCallback(DWORD dwThreadId, LPVOID lParameter) {
	WalkThreadContext* ctx = (WalkThreadContext*)(lParameter);
	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, dwThreadId);
	if (ctx->size >= ctx->capacity)
		return FALSE;
	if (hThread == NULL || hThread == INVALID_HANDLE_VALUE)
		return FALSE;
	DetourUpdateThread(hThread);
	ctx->hThreads[ctx->size] = hThread;
	ctx->size++;
	return TRUE;
}

typedef LRESULT(WINAPI* DefWindowProcT)(HWND, UINT, WPARAM, LPARAM);

BOOL SetupAPIHook(HMODULE hMod) {
	{
		DWORD len = MAX_PATH;
		while (dll_path == NULL) {
			LPSTR path = malloc(len + 1);
			DWORD retlen = GetModuleFileNameA(hMod, path, len + 1);
			if (retlen == 0) {
				//Error
				break;
			}
			if (retlen < len + 1) {
				dll_path = path;
				break;
			}
			len = len * 2;
		}
	}
	TrueCreateProcessA = CreateProcessA;
	TrueCreateProcessW = CreateProcessW;
	TrueCreateProcessAsUserA = CreateProcessAsUserA;
	TrueCreateProcessAsUserW = CreateProcessAsUserW;
	TrueCreateProcessWithTokenW = CreateProcessWithTokenW;
	TrueCreateProcessWithLogonW = CreateProcessWithLogonW;
	
	DetourTransactionBegin();
	WalkThreadContext ctx;
	ctx.hThreads = NULL;
	BOOL get_threads = WalkThreads(WalkThreadInitCallback, WalkThreadCallback, &ctx, TRUE);
	TrueSetWindowDisplayAffinity = SetWindowDisplayAffinity;

	DetourUpdateThread(GetCurrentThread());

	DetourAttach((PVOID*)&TrueSetWindowDisplayAffinity, (PVOID)FakeSetWindowDisplayAffinity);
	if (dll_path) {
		DetourAttach((PVOID*)&TrueCreateProcessA, (PVOID)FakeCreateProcessA);
		DetourAttach((PVOID*)&TrueCreateProcessW, (PVOID)FakeCreateProcessW);
		DetourAttach((PVOID*)&TrueCreateProcessAsUserA, (PVOID)FakeCreateProcessAsUserA);
		DetourAttach((PVOID*)&TrueCreateProcessAsUserW, (PVOID)FakeCreateProcessAsUserW);
		DetourAttach((PVOID*)&TrueCreateProcessWithTokenW, (PVOID)FakeCreateProcessWithTokenW);
		DetourAttach((PVOID*)&TrueCreateProcessWithLogonW, (PVOID)FakeCreateProcessWithLogonW);
}
	DetourTransactionCommit();
	if (get_threads && ctx.hThreads != NULL) {
		for (SIZE_T i = 0; i < ctx.size; i++) {
			CloseHandle(ctx.hThreads[i]);
		}
		HeapFree(GetProcessHeap(), 0, ctx.hThreads);
	}
	return TRUE;
}