/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Fully initialize LVGL library to show the GUI using /dev/fb and to take inputs using evdev.
 *
 */

#ifndef REC_LVGL_INIT_H
#define REC_LVGL_INIT_H

void rec_lvgl_initialize(void);
void rec_lvgl_processLvglEventsInLoop(void);

#endif /* REC_LVGL_INIT_H */
