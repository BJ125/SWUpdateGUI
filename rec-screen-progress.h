/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains functions to create & operate update-progress screen
 *
 */

#ifndef REC_SCREEN_PROGRESS_H
#define REC_SCREEN_PROGRESS_H

#include <progress_ipc.h>

struct ConfigRecovery;

void rec_screen_progress_createScreen(const struct ConfigRecovery *Config);

void rec_screen_progress_showScreen(void);

void rec_screen_progress_startingNewUpdate(void);
void rec_screen_progress_updateUpdateProgress(
	const struct progress_msg *Message);
void rec_screen_progress_finishUpdate(const bool isSuccess);

void rec_screen_progress_addMessage(const char *Text);

void rec_screen_progress_clearUpdateMessages(void);

#endif /* REC_SCREEN_PROGRESS_H */
