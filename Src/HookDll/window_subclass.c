#include "window_subclass.h"
#include <stdio.h>

BOOL isUnicode = FALSE;
WNDPROC oldWndProc = NULL;
SUBCLASSPROC SubClassProc = NULL;
//Pass arguments with global variable.
//We only run SubclassWindow from single thread.

LRESULT CALLBACK SubClassHelper(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
) 
{
	SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
	SetWindowSubclass(hWnd, SubClassProc, 0, 0);
	LRESULT ret = 0;
	if (oldWndProc) {
		if (isUnicode)
			ret = CallWindowProcW(oldWndProc, hWnd, uMsg, wParam, lParam);
		else
			ret = CallWindowProcA(oldWndProc, hWnd, uMsg, wParam, lParam);
	}
	
	oldWndProc = NULL;
	return ret;
}
BOOL SubClassWindow(HWND hWnd, SUBCLASSPROC proc) {
	isUnicode = IsWindowUnicode(hWnd);
	SubClassProc = proc;
	oldWndProc = (WNDPROC)SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)SubClassHelper);
	if (oldWndProc == NULL) {
		return FALSE;
	}

	if (isUnicode)
		SendMessageW(hWnd, WM_NULL, 0, 0);
	else
		SendMessageA(hWnd, WM_NULL, 0, 0);
	
	DWORD_PTR ptr;
	return GetWindowSubclass(hWnd, proc, 0, &ptr);
}