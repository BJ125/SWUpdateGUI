/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains utility functions for configuration file
 *
 */

#ifndef REC_UTIL_CONFIG_H
#define REC_UTIL_CONFIG_H

#include "rec-util-system.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IFACE_STR_LENGTH_MAX 32
#define IFACE_COUNT_MAX 10
#define IFACE_ADDR_MAX 17
#define SETTING_STR_LENGTH_MAX 1024

struct ConfigRecovery {
	char Version[SETTING_STR_LENGTH_MAX];
	char AppId[SETTING_STR_LENGTH_MAX];
	char Logo[SETTING_STR_LENGTH_MAX];
	char Interfaces[SETTING_STR_LENGTH_MAX];
	char Mediapath[SETTING_STR_LENGTH_MAX];
	char KeypadDev[SETTING_STR_LENGTH_MAX];
	char ThemeColor[SETTING_STR_LENGTH_MAX];
};

struct RecoveryParameters {
	struct ConfigRecovery Config;
	struct EnvSettings Env;
};

bool util_config_isAvailable(const char *ConfigPath);

void util_config_setDefaults(struct ConfigRecovery *Config);

struct ConfigRecovery util_config_get(const char *ConfigPath);

void util_config_removeChar(char *Input, const char Token);

char *util_config_trimWhitespaces(char *Str);

void util_config_parseInterfaces(const char *Interfaces,
				 char Output[][IFACE_STR_LENGTH_MAX]);

const struct RecoveryParameters *util_config_getRecoveryParameters(void);

#ifdef __cplusplus
}
#endif

#endif /* REC_UTIL_CONFIG_H */
