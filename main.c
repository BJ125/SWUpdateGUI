/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: The main() function for the program.
 *
 */

#include "rec-lvgl-init.h"
#include "rec-screen-main.h"
#include "rec-swupdate-client.h"
#include "rec-progress-mq.h"

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	rec_lvgl_initialize();

	rec_mq_createProgressMq();
	swupdate_client_startIPCThread();

	rec_screen_main_createScreen();
	rec_screen_main_showScreen();

	rec_lvgl_processLvglEventsInLoop();

	return 0;
}
