/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains utility functions for system-features
 *
 */

#include "rec-util-system.h"

#include <stdlib.h>
#include <string.h>
#include <linux/input.h>

/**
 * Get settings values from environment
 *
 * @return EnvSettings structure containing various settings
 */
struct EnvSettings util_system_getEnvSettings(void)
{
	struct EnvSettings EnvSettings = { .ScreenWidth = 800,
					   .ScreenHeight = 480,
					   .ScreenOrientationAngle = 0 };

	const char *OrientationAngleString = getenv("SCREEN_ORIENTATION_ANGLE");
	if (NULL != OrientationAngleString) {
		EnvSettings.ScreenOrientationAngle =
			atoi(OrientationAngleString);
	}

	const char *ScreenWidthString = getenv("SCREEN_WIDTH");
	if (NULL != ScreenWidthString) {
		EnvSettings.ScreenWidth = atoi(ScreenWidthString);
	}

	const char *ScreenHeightString = getenv("SCREEN_HEIGHT");
	if (NULL != ScreenHeightString) {
		EnvSettings.ScreenHeight = atoi(ScreenHeightString);
	}

	return EnvSettings;
}

/**
 * Convert angle in degrees to lvgl enum
 *
 * @param[in] Angle Rotation angle in degrees; Allowed values: 0, 90, 180, 270
 *
 * @return angle as lv_disp_rot_t enum
 */
lv_display_rotation_t util_system_getRotationEnum(const unsigned int Angle)
{
	switch (Angle) {
	case 0:
		return LV_DISPLAY_ROTATION_0;

	case 90:
		return LV_DISPLAY_ROTATION_90;

	case 180:
		return LV_DISPLAY_ROTATION_180;

	case 270:
		return LV_DISPLAY_ROTATION_270;

	default:
		LV_LOG_ERROR("Invalid screen rotation angle: %u.\n", Angle);
		LV_LOG_ERROR(
			"Please provide correct value in SCREEN_ORIENTATION_ANGLE environment variable.");
		LV_LOG_ERROR("Valid values are: 0, 90, 180, 270.");
		exit(-1);
	}
}

/**
 * Convert Linux input code to LVGL key-type
 *
 * @param[in] InputCode Linux input code
 *
 * @return LVGL Key enum
 */
lv_key_t util_system_convertLinuxInputCodeToLvKey(const uint16_t InputCode)
{
	switch (InputCode) {
	case KEY_UP:
		return LV_KEY_UP;

	case KEY_DOWN:
		return LV_KEY_DOWN;

	case KEY_LEFT:
		return LV_KEY_LEFT;

	case KEY_RIGHT:
		return LV_KEY_RIGHT;

	case KEY_ENTER:
		return LV_KEY_ENTER;

	case KEY_P:
		return LV_KEY_PREV;

	case KEY_N:
		return LV_KEY_NEXT;

	default:
		return 0;
	}
}

/**
 * Convert color to lv_palette
 *
 * @param[in] Color Name of color: orange, red, pink, purple, indigo,
 *                                 blue, cyan, teal, green, brown
 *
 * @return palette enum value for color
 */
lv_palette_t util_system_convertColorToLvPalette(const char *Color)
{
	if (0 == strcmp(Color, "orange")) {
		return LV_PALETTE_ORANGE;
	} else if (0 == strcmp(Color, "red")) {
		return LV_PALETTE_RED;
	} else if (0 == strcmp(Color, "pink")) {
		return LV_PALETTE_PINK;
	} else if (0 == strcmp(Color, "purple")) {
		return LV_PALETTE_PURPLE;
	} else if (0 == strcmp(Color, "indigo")) {
		return LV_PALETTE_INDIGO;
	} else if (0 == strcmp(Color, "blue")) {
		return LV_PALETTE_BLUE;
	} else if (0 == strcmp(Color, "cyan")) {
		return LV_PALETTE_CYAN;
	} else if (0 == strcmp(Color, "teal")) {
		return LV_PALETTE_TEAL;
	} else if (0 == strcmp(Color, "green")) {
		return LV_PALETTE_GREEN;
	} else if (0 == strcmp(Color, "brown")) {
		return LV_PALETTE_BROWN;
	} else {
		LV_LOG_ERROR("Invalid palette-color value: %s.", Color);
		LV_LOG_ERROR(
			"Valid values are: orange, red, pink, purple, indigo, blue, cyan, teal, green, brown.");
		exit(-1);
	}
}

/**
 * Executes a shell command. It is going to call exit() if the shell command was terminated with a signal.
 *
 * @param[in] command a shell command to execute
 *
 * @return return value
 * */
int util_system_executeScript(const char *command)
{
	const int res = system(command);

	if (!WIFEXITED(res)) {
		LV_LOG_ERROR(
			"The command (%s) was stopped by a signal. Return value = %d",
			command, res);
		exit(-1);
	}

	return WEXITSTATUS(res);
}

/**
 * Checks whether the returned value is in the list of returned values.
 *
 * @param[in] returnValue returned value
 * @param[in] validReturnValues list of valid returned values
 * @param[in] n number of elements in the list of valid returned values
 *
 * @return true when the returned value is found in the list of valid returned values, false otherwise
 * */
bool util_system_isReturnValueInValidList(const int returnValue,
					  const int validReturnValues[],
					  const unsigned int n)
{
	const int *it = validReturnValues;
	const int *const end = it + n;

	while (end != it) {
		if (returnValue == *it++) {
			return true;
		}
	}

	return false;
}
