/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains functions to open and read from
 *                 a keypad device
 *
 */

#ifndef REC_DRV_KEYPAD_H
#define REC_DRV_KEYPAD_H

#include <lvgl/lvgl.h>

bool rec_drv_openKeypadDevFd(const char *KeypadDev);

void rec_drv_readKeypad(lv_indev_drv_t *IndevDrv, lv_indev_data_t *Data);

#endif /* REC_DRV_KEYPAD_H */
