#pragma once
#include <Windows.h>
typedef struct {
	BOOLEAN HideTaskbar;
	BOOLEAN HidePreview;
	
	void* POINTER_64 preview_mapping;
	void* POINTER_64 icon_mapping;
}CONFIG;
extern const CONFIG* g_config;
VOID initConfig();