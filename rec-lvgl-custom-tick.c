/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: If the LVGL is configured (by defining macros LV_TICK_CUSTOM and
 *                 LV_TICK_CUSTOM_SYS_TIME_EXPR) this function will be used as a custom
 *                 tick function.
 *
 */

#include <lvgl/lvgl.h>

#include <sys/time.h>

#if (defined(LV_TICK_CUSTOM) && defined(LV_TICK_CUSTOM_SYS_TIME_EXPR))

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
	static uint64_t StartMs = 0;
	if (0 == StartMs) {
		struct timeval StartTime;
		gettimeofday(&StartTime, NULL);
		StartMs =
			(StartTime.tv_sec * 1000000 + StartTime.tv_usec) / 1000;
	}

	struct timeval CurrentTime;
	gettimeofday(&CurrentTime, NULL);
	uint64_t CurrentTimeMs;
	CurrentTimeMs =
		(CurrentTime.tv_sec * 1000000 + CurrentTime.tv_usec) / 1000;

	uint32_t TickMs = CurrentTimeMs - StartMs;
	return TickMs;
}

#endif
