#pragma once
#include <Windows.h>
#ifdef __cplusplus 
extern "C" {
#endif
	typedef struct {
		UINT32 mode;
		UINT32 width;
		UINT32 height;
		UINT32 background;
	}ImageHeader;

	enum ImageResizeMode {
		MY_STRETCH_RESIZE = 0,
		MY_STRETCH_KEEPRATIO = 1,
		MY_STRETCH_NORESIZE = 2,
		MY_STRETCH_REPEAT = 3
	};
	BOOL InitImage(UINT8 I, LPVOID data, SIZE_T len);
	HBITMAP GetImage(UINT8 I, SIZE target_size_hint, BOOL force_size);
#ifdef __cplusplus 
}
#endif
