/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains utility functions for configuration file
 *
 */

#include "rec-util-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include <lvgl/lvgl.h>

/* This is the configuration file that is loaded at start of recovery */
static const char *RECOVERY_CONFIG_FILE_PATH = "/etc/recovery_gui/config.txt";

/* This variable holds the only instance on the structure holding all parameters for the GUI */
static struct RecoveryParameters *RecoveryParameters = NULL;

/**
 * Check if provided config-file exists or not
 *
 * @param[in] ConfigPath Filepath to config-file
 *
 * @return true if file exists, else false
 */
bool util_config_isAvailable(const char *ConfigPath)
{
	struct stat Buffer;
	return (0 == stat(ConfigPath, &Buffer));
}

/**
 * Trim leading & trailing whitespaces
 * Function returns a substring from provided string
 * Note: When using with dynamically allocated strings,
 * copy result to a new string and deallocate memory
 * using original pointer
 *
 * @param[in] Input String to be trimmed
 *
 * @return String with no leading/trailing whitespaces
 */
char *util_config_trimWhitespaces(char *Input)
{
	char *End;

	while (isspace((unsigned char)*Input)) {
		Input++;
	}

	if (0 == *Input) {
		return Input;
	}

	End = Input + strlen(Input) - 1;
	while ((End > Input) && isspace((unsigned char)*End)) {
		End--;
	}

	End[1] = '\0';

	return Input;
}

/**
 * Remove all instances of token from a string
 *
 * @param[in|out] Input String to be modified
 * @param[in] Token Character to be removed
 */
void util_config_removeChar(char *Input, const char Token)
{
	char *Current = Input;
	do {
		while (*Current == Token) {
			++Current;
		}
	} while ((*Input++ = *Current++));
}

static void util_config_parseConfigLine(char *Line, unsigned int Length,
					char *Tag, char *Value)
{
	char TrimmedLine[SETTING_STR_LENGTH_MAX];
	char TmpTag[SETTING_STR_LENGTH_MAX];
	char TmpValue[SETTING_STR_LENGTH_MAX];
	char *Token = NULL;
	char *SanitizedValue = NULL;
	int TokenLen = 0;

	/* Remove all doublequotes */
	strncpy(TrimmedLine, Line, Length - 1);
	TrimmedLine[Length - 1] = '\0';
	util_config_removeChar(TrimmedLine, '\"');

	/* Get configuration tag */
	strncpy(TmpTag, TrimmedLine, SETTING_STR_LENGTH_MAX);
	Token = strtok(TmpTag, "=");

	if (NULL == Token) {
		LV_LOG_INFO("No tag found in config-line: %s\n", Line);
		return;
	}

	TokenLen = strlen(Token);
	strncpy(Tag, Token, SETTING_STR_LENGTH_MAX);
	Tag[TokenLen] = '\0';

	util_config_removeChar(Tag, ' ');

	/* Get configuration value */
	strncpy(TmpValue, TrimmedLine + TokenLen + 1,
		SETTING_STR_LENGTH_MAX - TokenLen);
	TmpValue[SETTING_STR_LENGTH_MAX - 1] = '\0';

	SanitizedValue = util_config_trimWhitespaces(TmpValue);

	strncpy(Value, SanitizedValue, strlen(SanitizedValue));
	Value[strlen(SanitizedValue)] = '\0';
}

static void util_config_setValueInConfig(struct ConfigRecovery *Config,
					 const char *Tag, const char *Value)
{
	if (0 == strcmp("VERSION", Tag)) {
		strncpy(Config->Version, Value, sizeof(Config->Version) - 1);
		Config->Version[sizeof(Config->Version) - 1] = '\0';
	} else if (0 == strcmp("APP_ID", Tag)) {
		strncpy(Config->AppId, Value, sizeof(Config->AppId) - 1);
		Config->AppId[sizeof(Config->AppId) - 1] = '\0';
	} else if (0 == strcmp("LOGO", Tag)) {
		strncpy(Config->Logo, Value, sizeof(Config->Logo) - 1);
		Config->Logo[sizeof(Config->Logo) - 1] = '\0';
	} else if (0 == strcmp("NETWORK_INTERFACES", Tag)) {
		strncpy(Config->Interfaces, Value,
			sizeof(Config->Interfaces) - 1);
		Config->Interfaces[sizeof(Config->Interfaces) - 1] = '\0';
	} else if (0 == strcmp("MEDIAPATH", Tag)) {
		strncpy(Config->Mediapath, Value,
			sizeof(Config->Mediapath) - 1);
		Config->Mediapath[sizeof(Config->Mediapath) - 1] = '\0';
	} else if (0 == strcmp("KEYPAD_DEVICE", Tag)) {
		strncpy(Config->KeypadDev, Value,
			sizeof(Config->KeypadDev) - 1);
		Config->KeypadDev[sizeof(Config->KeypadDev) - 1] = '\0';
	} else if (0 == strcmp("THEME_COLOR", Tag)) {
		strncpy(Config->ThemeColor, Value,
			sizeof(Config->ThemeColor) - 1);
		Config->ThemeColor[sizeof(Config->ThemeColor) - 1] = '\0';
	} else {
		LV_LOG_ERROR("Invalid tag in config: %s\nPlease check %s", Tag,
			     RECOVERY_CONFIG_FILE_PATH);
		exit(-1);
	}
}

/**
 * Set default values in configuration structure
 *
 * @param[in] Config Configuration object
 */
void util_config_setDefaults(struct ConfigRecovery *Config)
{
	memset(Config, 0, sizeof(struct ConfigRecovery));

	strcpy(Config->AppId, "Recovery App");
	strcpy(Config->Version, "1.0");
	strcpy(Config->Mediapath, "/media/usb/sda");
	strcpy(Config->ThemeColor, "orange");
	strcpy(Config->KeypadDev, "/dev/input/keyboard0");
}

/**
 * Parse config-file and fill values into a structure
 *
 * @param[in] ConfigPath Path to config-file
 *
 * @return ConfigRecovery object containing settings read from configPath
 */
struct ConfigRecovery util_config_get(const char *ConfigPath)
{
	struct ConfigRecovery Config;
	util_config_setDefaults(&Config);

	FILE *Fp;
	char *Line = NULL;
	size_t Len = 0;
	ssize_t Read;

	Fp = fopen(ConfigPath, "r");
	if (NULL == Fp) {
		printf("Failed to open configuration!\n");
		return Config;
	}

	while (-1 != (Read = getline(&Line, &Len, Fp))) {
		char Tag[SETTING_STR_LENGTH_MAX];
		char Value[SETTING_STR_LENGTH_MAX];

		util_config_parseConfigLine(Line, Read, Tag, Value);

		util_config_setValueInConfig(&Config, Tag, Value);
	}

	free(Line);
	fclose(Fp);

	return Config;
}

/**
 * Parse comma separated interfaces string
 *
 * @param[in] Interfaces Comma separated interfaces string
 * @param[out] Output Array of interface strings
 *             Note: Leading & trailing whitespaces are trimmed off
 */
void util_config_parseInterfaces(const char *Interfaces,
				 char Output[][IFACE_STR_LENGTH_MAX])
{
	char Tmp[SETTING_STR_LENGTH_MAX];
	unsigned int Index = 0;
	char *Interface = NULL;

	strncpy(Tmp, Interfaces, sizeof(Tmp) - 1);

	Interface = strtok(Tmp, ",");
	while ((NULL != Interface) && (IFACE_COUNT_MAX > Index)) {
		char IfaceValue[IFACE_STR_LENGTH_MAX];
		strncpy(IfaceValue, Interface, sizeof(IfaceValue) - 1);
		IfaceValue[sizeof(IfaceValue) - 1] = '\0';

		char *SanitizedValue = util_config_trimWhitespaces(IfaceValue);

		strncpy(Output[Index], SanitizedValue,
			IFACE_STR_LENGTH_MAX - 1);
		Output[Index++][IFACE_STR_LENGTH_MAX - 1] = '\0';

		Interface = strtok(NULL, ",");
	}
}

/**
 * This is a singleton function, providing access to all parameters for the recovery program.
 * On the first execution, it is going to parse all parameters using the configuration file at the
 * default location.
 *
 * @return pointer to the structure with all parameters
 * */
const struct RecoveryParameters *util_config_getRecoveryParameters(void)
{
	if (NULL == RecoveryParameters) {
		RecoveryParameters = malloc(sizeof(struct RecoveryParameters));

		RecoveryParameters->Config =
			util_config_get(RECOVERY_CONFIG_FILE_PATH);
		RecoveryParameters->Env = util_system_getEnvSettings();
	}

	return RecoveryParameters;
}
