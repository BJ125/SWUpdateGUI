/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains common functions for all UI screens
 *
 */

#include "rec-screen-common.h"

#include "rec-styles.h"

#include "rec-util-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const unsigned int HEADER_HEIGHT = 50;

const unsigned int MSG_LENGTH_MAX = 1000;

/**
 * Create new lvgl screen & set header for it
 *
 * @param[in] Screen Object containing pointers to lvgl screen & header
 * @param[in] Config Configuration data
 *
 */
void rec_screen_createHeader(struct Screen *Screen,
			     const struct ConfigRecovery *Config)
{
	Screen->Obj = lv_obj_create(NULL);

	rec_styles_applyContainerStyleBorderless(Screen->Obj);
	rec_screen_disableScrolling(Screen->Obj);
	lv_obj_set_style_pad_row(Screen->Obj, 0, LV_PART_MAIN);

	lv_obj_set_size(Screen->Obj, LV_HOR_RES, LV_VER_RES);

	Screen->Header = lv_obj_create(Screen->Obj);

	rec_styles_applyHeaderStyle(Screen->Header);

	lv_obj_set_size(Screen->Header, LV_PCT(100), HEADER_HEIGHT);
	rec_screen_disableScrolling(Screen->Header);

	char Title[2 * SETTING_STR_LENGTH_MAX + 1];
	snprintf(Title, sizeof(Title) - 1, "%s %s", Config->AppId,
		 Config->Version);
	Title[sizeof(Title) - 1] = '\0';

	Screen->Title = lv_label_create(Screen->Header);
	lv_label_set_text(Screen->Title, Title);
	lv_obj_set_align(Screen->Title, LV_ALIGN_CENTER);
}

/**
 * Load a provided screen
 *
 * @param[in] Screen HMI Screen object
 */
void rec_screen_loadScreen(struct Screen *Screen)
{
	lv_scr_load(Screen->Obj);
}

/**
 * Disable scrolling for given object
 *
 * @param[in] obj Object for which scrolling is to be disabled
 */
void rec_screen_disableScrolling(lv_obj_t *Obj)
{
	lv_obj_clear_flag(Obj, LV_OBJ_FLAG_SCROLLABLE);
}

/**
 * Create notification area
 *
 * @param[in] Screen Screen object which contains
 * notification and header
 */
void rec_screen_createNotification(struct Screen *Screen)
{
	Screen->Notification = lv_obj_create(Screen->Obj);
	lv_obj_set_size(Screen->Notification, LV_PCT(100), HEADER_HEIGHT);
	lv_obj_set_flex_flow(Screen->Notification, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(Screen->Notification, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	rec_screen_disableScrolling(Screen->Notification);

	Screen->MessageLabel = lv_label_create(Screen->Notification);
	lv_label_set_long_mode(Screen->MessageLabel, LV_LABEL_LONG_DOT);
	lv_obj_set_flex_grow(Screen->MessageLabel, 1);
}

/**
 * Show notification
 *
 * @param[in] Screen Screen object
 * @param[in] Text Message to be shown
 * @param[in] Type Type of notification
 */
void rec_screen_showNotification(struct Screen *Screen, const char *Text,
				 const enum NotifyType Type)
{
	char ErrorMsg[MSG_LENGTH_MAX];
	char MsgType[20];

	switch (Type) {
	case NOTIFY_SUCCESS:
		strncpy(MsgType, "success", sizeof(MsgType) - 1);
		rec_styles_applyNotifySuccessStyle(Screen->Notification);
		break;

	case NOTIFY_ERROR:
		LV_LOG_ERROR("error notification: %s", Text);
		strncpy(MsgType, "error", sizeof(MsgType) - 1);
		rec_styles_applyNotifyErrorStyle(Screen->Notification);
		break;

	case NOTIFY_WARNING:
		LV_LOG_WARN("warning notification: %s", Text);
		strncpy(MsgType, "warning", sizeof(MsgType) - 1);
		rec_styles_applyNotifyWarningStyle(Screen->Notification);
		break;

	case NOTIFY_INFO:
		strncpy(MsgType, "info", sizeof(MsgType) - 1);
		rec_styles_applyNotifyInfoStyle(Screen->Notification);
		break;

	default:
		LV_LOG_ERROR("Notification type is not supported: %d", Type);
		exit(1);
	}

	snprintf(ErrorMsg, sizeof(ErrorMsg) - 1, "%s: %s", MsgType, Text);
	lv_label_set_text(Screen->MessageLabel, ErrorMsg);
}

/**
 * Clear notification area
 *
 * @param[in] Screen Screen object
 */
void rec_screen_clearNotification(struct Screen *Screen)
{
	lv_obj_t *MessageLabel = lv_obj_get_child(Screen->Notification, 0);

	lv_label_set_text(MessageLabel, "info: Ready");

	rec_styles_applyNotifyInfoStyle(Screen->Notification);
}
