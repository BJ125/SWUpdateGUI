/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: These functions set the user's interface. Show the GUI of the program.
 *
 */

#ifndef REC_PROGRESS_MQ_H
#define REC_PROGRESS_MQ_H

#include <progress_ipc.h>

void rec_mq_createProgressMq(void);

void rec_mq_processProgressMessages(void);

void rec_mq_enqueueProgressMessage(struct progress_msg *Message);

#endif /* REC_PROGRESS_MQ_H */
