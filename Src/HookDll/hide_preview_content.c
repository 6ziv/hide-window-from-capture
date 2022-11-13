#include "hide_preview_content.h"
#include <Windows.h>
#include <dwmapi.h>
#include "window_subclass.h"
#include "image.h"
#pragma comment(lib,"dwmapi.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"msimg32.lib")

//IMAGE* img = NULL;
//BOOL KeepRatio = TRUE;
BOOL LoadImageImpl(UINT8 I, HANDLE h) {
	LPVOID mem = MapViewOfFile(h, FILE_MAP_READ, 0, 0, 0);
	if (mem == NULL) {
		CloseHandle(h);
		return FALSE;
	}
	MEMORY_BASIC_INFORMATION basic_info;
	if (0 == VirtualQuery(mem, &basic_info, sizeof(basic_info))) {
		UnmapViewOfFile(mem);
		CloseHandle(h);
		return FALSE;
	}
	return InitImage(I, mem, basic_info.RegionSize);
}

BOOL LoadImagesFromHandles(HANDLE hPreview, HANDLE hIcon) {
	return LoadImageImpl(0, hPreview) && LoadImageImpl(1, hIcon);
}
/*
inline VOID CalcPreviewParameters(SIZE_T w, SIZE_T h, RESIZEPARAMS* params) {
	if (KeepRatio) {
		LONGLONG product1 = params->size.cy * w;
		LONGLONG product2 = params->size.cx * h;
		LONGLONG ResizeProduct = min(product1, product2);

		params->inner_size.cx = (LONG)(ResizeProduct / h);
		params->inner_size.cy = (LONG)(ResizeProduct / w);
		params->offset.x = (params->size.cx - params->inner_size.cx) / 2;
		params->offset.y = (params->size.cy - params->inner_size.cy) / 2;
	}
	else {
		params->inner_size.cx = params->size.cx;
		params->inner_size.cy = params->size.cy;
		params->offset.x = 0;
		params->offset.y = 0;
	}
	return;
}*/
LRESULT CALLBACK HidePreviewSubclass(
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
	if (uMsg == WM_DWMSENDICONICTHUMBNAIL) {
		SIZE s = { .cx = HIWORD(lParam), .cy = LOWORD(lParam) };
		HBITMAP hb = GetImage(1, s, TRUE);
		DwmSetIconicThumbnail(hWnd, hb, 0);
		DeleteObject(hb);
		return 0;
	}
	if (uMsg == WM_DWMSENDICONICLIVEPREVIEWBITMAP) {
		WINDOWPLACEMENT placement;
		placement.length = sizeof(placement);
		GetWindowPlacement(hWnd, &placement);
		/*RESIZEPARAMS params = {.size = {
				.cx = placement.rcNormalPosition.right - placement.rcNormalPosition.left,
				.cy = placement.rcNormalPosition.bottom - placement.rcNormalPosition.top
			} };*/
		POINT pt = { .x = 0,.y = 0 };
		SIZE s = {
				.cx = placement.rcNormalPosition.right - placement.rcNormalPosition.left,
				.cy = placement.rcNormalPosition.bottom - placement.rcNormalPosition.top
		};
		HBITMAP hb = GetImage(0, s, TRUE);
		//HBITMAP hb = MyResizeImage(img, &params);
		DwmSetIconicLivePreviewBitmap(hWnd, hb, &pt, 0);
		DeleteObject(hb);
		return 0;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

BOOL StartHidePreview(HWND hWnd) {
	
	BOOL b = SubClassWindow(hWnd, HidePreviewSubclass);
	if (!b)return FALSE;

	b = TRUE;
	DwmSetWindowAttribute(hWnd, DWMWA_FORCE_ICONIC_REPRESENTATION, &b, sizeof(BOOL));

	b = TRUE;
	DwmSetWindowAttribute(hWnd, DWMWA_HAS_ICONIC_BITMAP, &b, sizeof(BOOL));

	DwmInvalidateIconicBitmaps(hWnd);

	return TRUE;
}