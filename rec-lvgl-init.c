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
#include <lv_drivers/display/fbdev.h>
#include <lv_drivers/indev/evdev.h>

#include <unistd.h>
#include <sys/time.h>

/* A small buffer for LVGL to draw the screen's content */
#define DISP_BUF_SIZE (128 * 1024)
static lv_color_t gui_lvgl_DisplayBuf[DISP_BUF_SIZE];

/* A descriptor for the buffer */
static lv_disp_draw_buf_t gui_lvgl_DisplayBufFd;

/* A display driver */
static lv_disp_drv_t gui_lvgl_DispDrv;

/* A driver for the inputs */
static lv_indev_drv_t gui_lvgl_IndevDrv;

/* A driver for the keypad */
static lv_indev_drv_t gui_lvgl_KeypadDrv;

/* Sleep in the endless loop, which process LVGL events */
static const unsigned int LVGL_LOOP_SLEEP_US = 5000;

static void gui_lvgl_initializeDisplayDriver(void)
{
	const struct RecoveryParameters *RecoveryParameters =
		util_config_getRecoveryParameters();

	fbdev_init();

	/*Initialize a descriptor for the buffer*/
	lv_disp_draw_buf_init(&gui_lvgl_DisplayBufFd, gui_lvgl_DisplayBuf, NULL,
			      DISP_BUF_SIZE);

	/*Initialize and register a display driver*/

	lv_disp_drv_init(&gui_lvgl_DispDrv);
	gui_lvgl_DispDrv.draw_buf = &gui_lvgl_DisplayBufFd;
	gui_lvgl_DispDrv.flush_cb = fbdev_flush;
	gui_lvgl_DispDrv.hor_res = RecoveryParameters->Env.ScreenWidth;
	gui_lvgl_DispDrv.ver_res = RecoveryParameters->Env.ScreenHeight;
	gui_lvgl_DispDrv.rotated = util_system_getRotationEnum(
		RecoveryParameters->Env.ScreenOrientationAngle);
	gui_lvgl_DispDrv.sw_rotate = 1;

	lv_disp_drv_register(&gui_lvgl_DispDrv);
}

static void gui_lvgl_initializeKeypadDriver(void)
{
	const struct RecoveryParameters *RecoveryParameters =
		util_config_getRecoveryParameters();

	const bool Success =
		rec_drv_openKeypadDevFd(RecoveryParameters->Config.KeypadDev);

	if (!Success) {
		return;
	}

	lv_indev_drv_init(&gui_lvgl_KeypadDrv);
	gui_lvgl_KeypadDrv.type = LV_INDEV_TYPE_KEYPAD;
	gui_lvgl_KeypadDrv.read_cb = rec_drv_readKeypad;

	lv_indev_t *KeypadIndev = lv_indev_drv_register(&gui_lvgl_KeypadDrv);
	if (NULL == KeypadIndev) {
		LV_LOG_ERROR(
			"lv_indev_drv_register() failed for keypad. The device used: %s",
			RecoveryParameters->Config.KeypadDev);
	} else {
		lv_indev_set_group(KeypadIndev, lv_group_get_default());
	}
}

static void gui_lvgl_initializeTouchscreenDriver(void)
{
	evdev_init();

	lv_indev_drv_init(&gui_lvgl_IndevDrv);
	gui_lvgl_IndevDrv.type = LV_INDEV_TYPE_POINTER;
	gui_lvgl_IndevDrv.read_cb = evdev_read;
	lv_indev_t *MouseIndev = lv_indev_drv_register(&gui_lvgl_IndevDrv);
	if (NULL == MouseIndev) {
		LV_LOG_ERROR(
			"lv_indev_drv_register() failed for touchscreen/mouse.");
	} else {
		/*Set a cursor for the mouse*/
		LV_IMG_DECLARE(MouseCursorIcon)
		lv_obj_t *CursorObj = lv_img_create(lv_scr_act());
		lv_img_set_src(CursorObj, &MouseCursorIcon);
		lv_indev_set_cursor(MouseIndev, CursorObj);
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
