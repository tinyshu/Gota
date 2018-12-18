#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../types_service.h"
#include "gw_config.h"

struct server_module_config modules[] = {
	{
		SYPTE_CENTER,
		"127.0.0.1",
		6080,
		"center server",
	},
	{
		STYPE_SYSTEM,
		"127.0.0.1",
		6081,
		"system server",
	},
	{
		STYPE_GAME1,
		"127.0.0.1",
		6082,
		"game1 server",
	},

	{
		STYPE_GAME2,
		"127.0.0.1",
		6082,
		"game1 server",
	},
};

struct gw_config GW_CONFIG = {
	"127.0.0.1",
	6010,
	sizeof(modules) / sizeof(struct server_module_config),
	modules,
};