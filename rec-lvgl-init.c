/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Fully initialize LVGL library to show the GUI using /dev/fb and to take inputs using evdev.
 *
 */

#include "rec-lvgl-init.h"

#include "rec-progress-mq.h"

#include "rec-util-system.h"
#include "rec-util-config.h"

#include "rec-drv-keypad.h"

#include <lvgl/lvgl.h>

#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

/* A driver for the mouse */
static lv_indev_t *gui_lvgl_MouseDrv = NULL;

/* A driver for the keypad */
static lv_indev_t *gui_lvgl_KeypadDrv = NULL;

/* Sleep in the endless loop, which process LVGL events */
static const unsigned int LVGL_LOOP_SLEEP_US = 5000;

static void gui_lvgl_initializeDisplayDriver(void)
{
	const struct RecoveryParameters *RecoveryParameters =
		util_config_getRecoveryParameters();

	lv_display_t *displayObj = lv_linux_fbdev_create();
	if (NULL == displayObj) {
		LV_LOG_ERROR("lv_linux_fbdev_create() failed.");
		exit(1);
	}

	lv_linux_fbdev_set_file(displayObj, "/dev/fb");
	lv_display_set_default(displayObj);

	const lv_display_rotation_t displayRotation =
		util_system_getRotationEnum(
			RecoveryParameters->Env.ScreenOrientationAngle);
	lv_display_set_rotation(displayObj, displayRotation);

	LV_LOG_INFO("Display initialisation done.");
}

static void gui_lvgl_initializeKeypadDriver(void)
{
	const char *keyboardFile = "/dev/input/keyboard0";

	gui_lvgl_KeypadDrv =
		lv_evdev_create(LV_INDEV_TYPE_KEYPAD, keyboardFile);

	if (NULL == gui_lvgl_KeypadDrv) {
		LV_LOG_ERROR(
			"lv_evdev_create() failed for keypad. The device used: %s",
			keyboardFile);
	} else {
		lv_indev_set_group(gui_lvgl_KeypadDrv, lv_group_get_default());

		LV_LOG_INFO("Keyboard initialized successfully using [%s].",
			    keyboardFile);
	}
}

static void gui_lvgl_initializeTouchscreenDriver(void)
{
	const char *touchscreenFile = "/dev/input/touchscreen0";

	gui_lvgl_MouseDrv =
		lv_evdev_create(LV_INDEV_TYPE_POINTER, touchscreenFile);

	if (NULL == gui_lvgl_MouseDrv) {
		LV_LOG_ERROR(
			"lv_evdev_create() failed for the touchscreen at [%s]",
			touchscreenFile);
	} else {
		/*Set a cursor for the mouse*/
		LV_IMG_DECLARE(MouseCursorIcon)
		lv_obj_t *CursorObj = lv_img_create(lv_scr_act());
		lv_img_set_src(CursorObj, &MouseCursorIcon);
		lv_indev_set_cursor(gui_lvgl_MouseDrv, CursorObj);

		LV_LOG_INFO("Touchscreen initialized successfully using [%s]",
			    touchscreenFile);
	}
}

static void gui_lvgl_createDefaultNavigationGroup()
{
	lv_group_t *Group = lv_group_create();
	lv_group_set_default(Group);
}

/**
 * Fully initialize LVGL library to show the image and handle keyboard and mouse or touch screen.
 *
 * */
void rec_lvgl_initialize(void)
{
	lv_init();

	gui_lvgl_createDefaultNavigationGroup();

	gui_lvgl_initializeDisplayDriver();

	gui_lvgl_initializeTouchscreenDriver();

	gui_lvgl_initializeKeypadDriver();
}

/**
 * Process LVGL events in a loop.
 *
 * @warning this function doesn't end
 * */
void rec_lvgl_processLvglEventsInLoop(void)
{
	while (1) {
		lv_timer_handler();

		rec_mq_processProgressMessages();

		usleep(LVGL_LOOP_SLEEP_US);
	}
}
