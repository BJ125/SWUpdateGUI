/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains common functions for all UI screens
 *
 */

#ifndef REC_SCREEN_COMMON_H
#define REC_SCREEN_COMMON_H

#include <lvgl/lvgl.h>

extern const unsigned int HEADER_HEIGHT;
extern const unsigned int MSG_LENGTH_MAX;

struct ConfigRecovery;

struct Screen {
	/* This is the top widget, that is the parent for all widgets and represents the screen. */
	lv_obj_t *Obj;

	/* Widget that represents the top header area. */
	lv_obj_t *Header;
	/* This is part of the header, and it shows the header text. */
	lv_obj_t *Title;

	/* Widget that represents the bottom notification area. */
	lv_obj_t *Notification;
	/* Text shown in notification area. */
	lv_obj_t *MessageLabel;

	/* Widget to hold buttons */
	lv_obj_t *ActionsPanel;
};

/**
 * Enum for status-message style selection
 *
 * Status-message background color is set according to type
 */
enum NotifyType {
	NOTIFY_INFO, /* grey */
	NOTIFY_SUCCESS, /* green */
	NOTIFY_WARNING, /* yellow */
	NOTIFY_ERROR, /* red */
};

void rec_screen_createHeader(struct Screen *Screen,
			     const struct ConfigRecovery *Config);
void rec_screen_loadScreen(struct Screen *Screen);
void rec_screen_disableScrolling(lv_obj_t *Obj);
void rec_screen_createNotification(struct Screen *Screen);
void rec_screen_showNotification(struct Screen *Screen, const char *Text,
				 const enum NotifyType Type);
void rec_screen_clearNotification(struct Screen *Screen);

#endif /* REC_SCREEN_COMMON_H */
