/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains functions to communicate to the SwUpdate process
 *
 */

#ifndef REC_SWUPDATE_CLIENT_H
#define REC_SWUPDATE_CLIENT_H

#include <stdbool.h>

struct LinkedList;

void swupdate_client_startIPCThread(void);

void swupdate_client_startLocalUpdate(const char *Filename,
				      const bool IsDryRunEnabled);

const struct LinkedList *swupdate_client_getSwupdateMessages(void);

#endif /* REC_SWUPDATE_CLIENT_H */
