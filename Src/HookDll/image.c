#include "image.h"
#include <simde/x86/avx.h>
#include <simde/x86/avx2.h>
typedef struct {
	BOOL inited;
	ImageHeader header;
	HBITMAP hBitmap;
} ImageData;
ImageData g_images[2] = { {.inited = FALSE},{.inited = FALSE} };

BOOL InitImage(UINT8 I, LPVOID data, SIZE_T len) {
	ImageData* pg_image = &g_images[I];
	if (data == NULL)return FALSE;
	if (len < sizeof(ImageHeader))return FALSE;

	ImageHeader* header = (ImageHeader*)data;
	memcpy(&pg_image->header, header, sizeof(ImageHeader));
	if (len < sizeof(ImageHeader) + header->width * header->height * 4)return FALSE;


	LPCBYTE img_data = (LPCBYTE)data + sizeof(ImageHeader);

	HDC hMemDC = CreateCompatibleDC(NULL);
	if (hMemDC == NULL)return FALSE;

	BITMAPINFO bmi;
	ZeroMemory(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = (LONG)header->width;
	bmi.bmiHeader.biHeight = -(LONG)header->height;  // Use a top-down DIB
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;

	PBYTE pData = NULL;
	HBITMAP hBMP = CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, (VOID**)&pData, NULL, 0);
	if (hBMP == NULL) {
		DeleteDC(hMemDC);
		return FALSE;
	}

	CopyMemory(pData, img_data, header->height * header->width * 4);

	GdiFlush();
	pg_image->hBitmap = hBMP;

	pg_image->inited = TRUE;
	return TRUE;
}

inline void FillPixels(PUINT32 data, UINT32 color, SIZE_T pixel) {
	PUINT32 end = data + pixel;
	const simde__m256i color_expand = simde_mm256_set1_epi32((INT32)color);
	while (data != end && (((UINT_PTR)data) & 31)) {
		*data = color;
		data++;
	}
	while (data != end && (((UINT_PTR)end) & 31)) {
		end--;
		*end = color;
	}
	while (data != end) {
		simde_mm256_store_si256((simde__m256i*)data, color_expand);
		data += 8;
	}
}

HBITMAP CreateDefaultBitmapT(SIZE s) {
	HDC hMemDC = CreateCompatibleDC(NULL);
	if (hMemDC == NULL)return NULL;

	BITMAPINFO bmi;
	ZeroMemory(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = s.cx;
	bmi.bmiHeader.biHeight = -s.cy;  // Use a top-down DIB
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;

	PBYTE pbDS = NULL;
	HBITMAP hBitmap = CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, (VOID**)&pbDS, NULL, 0);
	if (hBitmap == NULL)return NULL;

#if _DEBUG
	FillPixels((PUINT32)(pbDS), 0xffff00ff, s.cx * s.cy);
#else
	FillPixels((PUINT32)(pbDS), 0xffffffff, s.cx * s.cy);
#endif
	GdiFlush();

	return hBitmap;
}

HBITMAP GetImageImpl(const ImageData* img, SIZE target_size, UINT8 mode) {
	HBITMAP hBitmap = NULL;
	HDC hMemDC = CreateCompatibleDC(NULL);
	if (hMemDC == NULL)return NULL;

	if (mode == MY_STRETCH_NORESIZE) {
		target_size.cx = img->header.width;
		target_size.cy = img->header.height;
	}
	//BOOST_SCOPE_EXIT(hMemDC) { DeleteDC(hMemDC); }BOOST_SCOPE_EXIT_END;

	BITMAPINFO bmi;
	ZeroMemory(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = target_size.cx;
	bmi.bmiHeader.biHeight = -target_size.cy;  // Use a top-down DIB
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;

	PBYTE pbDS = NULL;
	hBitmap = CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, (VOID**)&pbDS, NULL, 0);
	if (hBitmap == NULL)goto err;

	FillPixels((PUINT32)pbDS, img->header.background, target_size.cx * target_size.cy);
	GdiFlush();

	HDC hSrcDC = CreateCompatibleDC(NULL);
	if (hSrcDC == NULL)goto err;
	HGDIOBJ saved1 = SelectObject(hSrcDC, img->hBitmap);
	HGDIOBJ saved2 = SelectObject(hMemDC, hBitmap);

	BLENDFUNCTION blend = { .BlendOp = AC_SRC_OVER,.BlendFlags = 0,.SourceConstantAlpha = 255,.AlphaFormat = AC_SRC_ALPHA };

	POINT offset = { .x = 0,.y = 0 };
	SIZE imgsize = { .cx = target_size.cx,.cy = target_size.cy };
	switch (mode & 3) {
	case MY_STRETCH_KEEPRATIO:
	{
		UINT64 k_height = (UINT64)target_size.cx * (UINT64)img->header.height;
		UINT64 k_width = (UINT64)target_size.cy * (UINT64)img->header.width;
		UINT64 k = min(k_height, k_width);
		imgsize.cx = (LONG)(k / img->header.height);
		imgsize.cy = (LONG)(k / img->header.width);

		offset.x = (target_size.cx - imgsize.cx) / 2;
		offset.y = (target_size.cy - imgsize.cy) / 2;
	}
	//[[ __fallthrough__ ]];	//available in C23 only
	case MY_STRETCH_NORESIZE:
	case MY_STRETCH_RESIZE:
		AlphaBlend(hMemDC, offset.x, offset.y, imgsize.cx, imgsize.cy, hSrcDC, 0, 0, img->header.width, img->header.height, blend);
		break;
	case MY_STRETCH_REPEAT:
		for (UINT32 y_id = 0; y_id < (target_size.cy - 1) / img->header.height + 1; y_id++) {
			offset.y = img->header.height * y_id;
			imgsize.cy = min((UINT32)target_size.cy, offset.y + img->header.height) - offset.y;
			for (UINT32 x_id = 0; x_id < (target_size.cx - 1) / img->header.width + 1; x_id++) {
				offset.x = img->header.width * x_id;
				imgsize.cx = min((UINT32)target_size.cx, offset.x + img->header.width) - offset.x;
				AlphaBlend(hMemDC, offset.x, offset.y, imgsize.cx, imgsize.cy, hSrcDC, 0, 0, imgsize.cx, imgsize.cy, blend);
			}
		}
		break;
	default:
		break;
	}
	SelectObject(hMemDC, saved2);
	SelectObject(hSrcDC, saved1);
	DeleteDC(hSrcDC);
err:
	DeleteDC(hMemDC);
	return hBitmap;

}
HBITMAP GetImage(UINT8 I, SIZE target_size_hint, BOOL force_size) {
	ImageData* pg_image = &g_images[I];
	if (!pg_image->inited) {
		return CreateDefaultBitmapT(target_size_hint);
	}
	//SIZE target_size = target_size_hint;
	//POINT borders = { .x = 0,.y = 0 };
	UINT32 stretch_mode = pg_image->header.mode & 3;
	if (stretch_mode == MY_STRETCH_NORESIZE && force_size)stretch_mode = MY_STRETCH_RESIZE;

	return GetImageImpl(pg_image, target_size_hint, (UINT8)stretch_mode);
}