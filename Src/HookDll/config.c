#include "config.h"
#include <detours/detours.h>
#define HIDE_WINDOW_DEFINE_GUID
#include "guid.h"

const CONFIG* g_config = NULL;

const CONFIG default_config = {
	.HidePreview		= TRUE,
	.HideTaskbar		= FALSE,
	.preview_mapping	= NULL,
	.icon_mapping		= NULL
};

VOID initConfig() {
	g_config = &default_config;

	DWORD config_len = 0;
	CONFIG* cfg = (CONFIG*)DetourFindPayloadEx(&HIDE_WINDOW_CONFIG_GUID, &config_len);
	if (config_len != sizeof(CONFIG))
		return;

	g_config = cfg;
}
