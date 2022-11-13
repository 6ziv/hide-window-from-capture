#include "thread_window_walker.h"
#include <Windows.h>
#include <processsnapshot.h>

BOOL WalkThreads(WALKTHREADSINIT init,WALKTHREADSCALLBACK callback, LPVOID lParameter, BOOL SkipSelf) {
	HPSS hSnapshot = NULL;
	if (ERROR_SUCCESS != PssCaptureSnapshot(GetCurrentProcess(), PSS_CAPTURE_THREADS, 0, &hSnapshot))
		return FALSE;
	BOOL ok = TRUE;
	__try {
		PSS_THREAD_INFORMATION info;

		if (ERROR_SUCCESS != PssQuerySnapshot(hSnapshot, PSS_QUERY_THREAD_INFORMATION, &info, sizeof(info))) {
			ok = FALSE;
			__leave;
		}
		if (init) {
			if (!init(info.ThreadsCaptured, lParameter)) {
				ok = FALSE;
				__leave;
			}
		}
		if (callback) {
			//SIZE_T hThreadCnt = 0;

			HPSSWALK hWalk;
			if (ERROR_SUCCESS != PssWalkMarkerCreate(NULL, &hWalk)) {
				ok = FALSE;
				__leave;
			}

			PSS_THREAD_ENTRY thread;

			while (ERROR_SUCCESS == PssWalkSnapshot(hSnapshot, PSS_WALK_THREADS, hWalk, &thread, sizeof(thread))) {
				if (thread.ThreadId != GetCurrentThreadId() || !SkipSelf)
				{
					callback(thread.ThreadId, lParameter);
				}
			}

			PssWalkMarkerFree(hWalk);
		}
	}
	__finally {
		PssFreeSnapshot(GetCurrentProcess(), hSnapshot);
	}
	return ok;
}
typedef struct {
	LPARAM lParam;
	WALKWINDOWSCALLBACK callback;
}WALKTHREADWINDOWSPARAMS,*PWALKTHREADWINDOWSPARAMS;
BOOL CALLBACK WalkThreadWindows(DWORD threadId, LPVOID lParameters) {
	if (lParameters == NULL)return FALSE;
	PWALKTHREADWINDOWSPARAMS params = (PWALKTHREADWINDOWSPARAMS)(lParameters);
	EnumThreadWindows(threadId, params->callback, params->lParam);
	return TRUE;
}

BOOL WalkWindows(WALKWINDOWSCALLBACK callback, LPARAM lParam,BOOL SkipSelf) {
	WALKTHREADWINDOWSPARAMS params;
	params.lParam = lParam;
	params.callback = callback;
	return WalkThreads(NULL, WalkThreadWindows, &params, SkipSelf);
	
}