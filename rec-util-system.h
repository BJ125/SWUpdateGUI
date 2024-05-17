/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains utility functions for system-features
 *
 */

#ifndef REC_UTIL_SYSTEM_H
#define REC_UTIL_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <lvgl/lvgl.h>

struct EnvSettings {
	unsigned int ScreenOrientationAngle;
};

struct EnvSettings util_system_getEnvSettings(void);

lv_display_rotation_t util_system_getRotationEnum(const unsigned int Angle);

lv_key_t util_system_convertLinuxInputCodeToLvKey(const uint16_t InputCode);

lv_palette_t util_system_convertColorToLvPalette(const char *Color);

int util_system_executeScript(const char *command);
bool util_system_isReturnValueInValidList(const int returnValue,
					  const int validReturnValues[],
					  const unsigned int n);

/**
 * This macro should be used to check if the command's returned value is in the list of valid
 * return values.
 *
 * Example:
 *          util_system_checkIfReturnValueValid( "ls -aaawe ztre", 1, 0, 1, 2 )
 * where the first parameter is the executed command, the second parameter is the returned value,
 * and rest of parameters are valid returned values.
 *
 * @param[in] command executed command
 * @param[in] returnValue returned value of the command
 * @param[in] variadic_arguments list of valid return values
 * */
#define util_system_checkIfReturnValueValid(command, returnValue, ...)                    \
	do {                                                                              \
		const int v[] = { __VA_ARGS__ };                                          \
		const unsigned int n = sizeof(v) / sizeof(v[0]);                          \
		if (!util_system_isReturnValueInValidList(returnValue, v,                 \
							  n)) {                           \
			LV_LOG_ERROR(                                                     \
				"Command (%s) returned an unsupported return value (%d)", \
				command, returnValue);                                    \
			exit(1);                                                          \
		}                                                                         \
	} while (0);

#ifdef __cplusplus
}
#endif

#endif /* REC_UTIL_SYSTEM_H */
