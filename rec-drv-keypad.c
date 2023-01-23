/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains functions to open and read from
 *                 a keypad device
 *
 */

#include "rec-drv-keypad.h"

#include "rec-util-system.h"

#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

/* keypad uinput device file descriptor */
static int KeypadInputFd = -1;

/**
 * Open keypad device node for reading
 *
 * @param[in] KeypadDev Path to keypad device
 *
 * @return true for success; false for error
 */
bool rec_drv_openKeypadDevFd(const char *KeypadDev)
{
	KeypadInputFd = open(KeypadDev, O_RDONLY | O_NONBLOCK);

	if (-1 == KeypadInputFd) {
		LV_LOG_WARN(
			"Failed to open keypad device. Please check configuration or default keypad device %s",
			KeypadDev);
		return false;
	}
	return true;
}

/**
 * Read keypad device and send received key event to LVGL
 *
 * @param[in] IndevDrv Driver object
 * @param[out] Data Event data
 */
void rec_drv_readKeypad(lv_indev_drv_t *IndevDrv, lv_indev_data_t *Data)
{
	(void)IndevDrv;
	struct input_event Ev = { 0 };

	int Ret = read(KeypadInputFd, &Ev, sizeof(Ev));
	if (-1 != Ret) {
		if (EV_KEY == Ev.type) {
			Data->key = util_system_convertLinuxInputCodeToLvKey(
				Ev.code);

			if (0 == Ev.value) // Release
			{
				Data->state = LV_INDEV_STATE_REL;
			} else if (1 == Ev.value) // press
			{
				Data->state = LV_INDEV_STATE_PR;
			}
		}
	}
}
