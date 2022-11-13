#include "hiding_from_taskbar.h"
#include "window_subclass.h"
UINT WM_MY_SHOWHIDE = 0;
LRESULT CALLBACK HideTaskbarSubclass(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
)
{
	(void)uIdSubclass;
	(void)dwRefData;
	if (uMsg == WM_MY_SHOWHIDE) {
		if (wParam == 1) {
			if (GetWindow(hWnd, GW_OWNER) == NULL) {
				SetWindowLongPtrA(hWnd, GWLP_HWNDPARENT, (LONG_PTR)GetDesktopWindow());
			}
		}
		else if (wParam == 0) {
			if (GetWindow(hWnd, GW_OWNER) == GetDesktopWindow()) {
				SetWindowLongPtrA(hWnd, GWLP_HWNDPARENT, (LONG_PTR)NULL);
			}
		}
		else return (LRESULT)(hWnd);
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
BOOL StartHideTaskbar(HWND hWnd) {
	if (WS_CHILD & GetWindowLongA(hWnd, GWL_STYLE))
		return FALSE;
	HWND hOwner = GetWindow(hWnd, GW_OWNER);
	if (hOwner != NULL)return FALSE;
	if (WM_MY_SHOWHIDE == 0)WM_MY_SHOWHIDE = RegisterWindowMessageA("SetWindowOwnerToDesktop");
	SetWindowLongPtrA(hWnd, GWLP_HWNDPARENT, (LONG_PTR)GetDesktopWindow());
	return SubClassWindow(hWnd, HideTaskbarSubclass);
}