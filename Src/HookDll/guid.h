#pragma once
#include <Windows.h>
extern const GUID HIDE_WINDOW_CONFIG_GUID;
#ifdef HIDE_WINDOW_DEFINE_GUID
const GUID HIDE_WINDOW_CONFIG_GUID =
{
	0xac241579,
	0xa009,
	0x4d77,
	{0x82,0x6b,0x37,0xb7,0xf0,0x78,0x96,0x28}
};

const GUID PREVIEW_IMAGE_GUID = {
	0x6a1a51eb,
	0x7aed,
	0x4135,
	{0xa3,0x85,0xdb,0x50,0x28,0x23,0x98,0x15}
};
#endif