/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains functions to create and operate file browser screen
 *
 */

#ifndef REC_SCREEN_FILE_BROWSER_H
#define REC_SCREEN_FILE_BROWSER_H

struct ConfigRecovery;

void rec_screen_file_createFileBrowserScreen(
	const struct ConfigRecovery *Config);
void rec_screen_file_showFileBrowserScreen(void);

#endif /* REC_SCREEN_FILE_BROWSER_H */
