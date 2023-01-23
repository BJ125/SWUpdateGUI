/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains functions to communicate to the SwUpdate process
 *
 */

#include "rec-swupdate-client.h"

#include "rec-progress-mq.h"

#include "rec-util-linked-list.h"

#include <progress_ipc.h>
#include <network_ipc.h>

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <lvgl/lvgl.h>
#include <poll.h>

static struct progress_msg swupdate_client_ProgressMsg;
static bool swupdate_client_WaitUpdate = true;

/* the file descriptor to open a swu file */
static int swupdateFileFd = -1;

/* The poll structures for the progress and messages socket, containing file descriptors
     - the first is for the progress socket
     - the second is for the notification socket
*/
static struct pollfd swuSocketFds[2] = { { .fd = -1, .events = POLLIN },
					 { .fd = -1, .events = POLLIN } };

struct SwupdateNotificationMessages {
	/* Linked list holding the received notification messages */
	struct LinkedList *messages;

	/* Mutex to lock the access to the notification messages */
	pthread_mutex_t mutex;
};

static struct SwupdateNotificationMessages *
AccessSwuUpdateNotificationMessages(void)
{
	static struct SwupdateNotificationMessages
		*swupdateNotificationMessages = NULL;

	if (NULL == swupdateNotificationMessages) {
		swupdateNotificationMessages =
			malloc(sizeof(struct SwupdateNotificationMessages));

		pthread_mutex_init(&swupdateNotificationMessages->mutex, NULL);

		swupdateNotificationMessages->messages =
			util_linked_list_create();
	}

	return swupdateNotificationMessages;
}

static void LockSwupdateNotificationMessagesMutex(void)
{
	pthread_mutex_lock(&AccessSwuUpdateNotificationMessages()->mutex);
}

static void UnlockSwupdateNotificationMessagesMutex(void)
{
	pthread_mutex_unlock(&AccessSwuUpdateNotificationMessages()->mutex);
}

static void PushSwupdateNotificationMessagesToTheList(const char *message)
{
	LockSwupdateNotificationMessagesMutex();

	util_linked_list_push_element(
		AccessSwuUpdateNotificationMessages()->messages, message,
		strlen(message) + 1);

	UnlockSwupdateNotificationMessagesMutex();
}

static const char *swupdate_client_getUpdateSource(unsigned int Sourcetype)
{
#define IFM_INT_TO_STRING_LENGTH ((CHAR_BIT * sizeof(int) - 1) / 3 + 2)
	static char TypeTmp[IFM_INT_TO_STRING_LENGTH];

	const char *Source = NULL;
	switch (Sourcetype) {
	case SOURCE_UNKNOWN:
		Source = "UNKNOWN";
		break;

	case SOURCE_WEBSERVER:
		Source = "WEBSERVER";
		break;

	case SOURCE_SURICATTA:
		Source = "BACKEND";
		break;

	case SOURCE_DOWNLOADER:
		Source = "DOWNLOADER";
		break;

	case SOURCE_CHUNKS_DOWNLOADER:
		Source = "CHUNKS DOWNLOADER";
		break;

	case SOURCE_LOCAL:
		Source = "LOCAL";
		break;

	default:
		snprintf(TypeTmp, sizeof(TypeTmp) - 1, "(%d)", Sourcetype);
		Source = TypeTmp;
		LV_LOG_WARN("Invalid source: %d", Sourcetype);
	}
	return Source;
}

static void swupdate_client_handleUpdateStartup(void)
{
	if (swupdate_client_WaitUpdate) {
		if ((START == swupdate_client_ProgressMsg.status) ||
		    (RUN == swupdate_client_ProgressMsg.status)) {
			printf("IPCHandler: Interface: %s",
			       swupdate_client_getUpdateSource(
				       swupdate_client_ProgressMsg.source));

			swupdate_client_WaitUpdate = false;
		}
	}
}

static void swupdate_client_handleUpdateEnding(void)
{
	switch (swupdate_client_ProgressMsg.status) {
	case FAILURE:
		swupdate_client_WaitUpdate = true;
		break;

	case SUCCESS:
		swupdate_client_WaitUpdate = true;
		break;

	case DONE:
		LV_LOG("swupdate: DONE.");
		break;

	case START:
	case IDLE:
	case RUN:
	case DOWNLOAD:
	case SUBPROCESS:
	case PROGRESS:
		break;

	default:
		LV_LOG_WARN("Invalid update status %d received.",
			    swupdate_client_ProgressMsg.status);
	}
}

static void swupdate_client_ConnectToSockets(void)
{
	swuSocketFds[0].fd = progress_ipc_connect(true);

	if (swuSocketFds[0].fd > 0) {
		swuSocketFds[1].fd = ipc_notify_connect();

		if (swuSocketFds[1].fd < 0) {
			const char *errorMsg =
				"Failed to connect to the swupdate socked to get notification messages.";

			LV_LOG_ERROR("%s\n", errorMsg);

			PushSwupdateNotificationMessagesToTheList(errorMsg);
		}
	} else {
		LV_LOG_ERROR("Failed to connect to the swupdate socket.\n");
	}
}

static void swupdate_client_ProcessProgressMessage(void)
{
	const int res = progress_ipc_receive(&swuSocketFds[0].fd,
					     &swupdate_client_ProgressMsg);

	if (res > 0) {
		rec_mq_enqueueProgressMessage(&swupdate_client_ProgressMsg);

		swupdate_client_handleUpdateStartup();

		swupdate_client_handleUpdateEnding();
	}
}

static void swupdate_client_ProcessNotificationMessage(void)
{
	ipc_message message;

	const int res = ipc_notify_receive(&swuSocketFds[1].fd, &message);

	if (res > 0) {
		PushSwupdateNotificationMessagesToTheList(
			message.data.notify.msg);
	} else {
		const char *errorMsg =
			"Failed to read a message from the swupdate socked. Stopping collection of further messages.";

		LV_LOG_ERROR("%s\n", errorMsg);

		PushSwupdateNotificationMessagesToTheList(errorMsg);
	}
}

static void swupdate_client_ProcessSocketMessages(void)
{
	const int nReady = poll(swuSocketFds, 2, -1);

	if (nReady > 0) {
		if (swuSocketFds[0].revents & POLLIN) {
			swupdate_client_ProcessProgressMessage();
		}
		if (swuSocketFds[1].revents & POLLIN) {
			swupdate_client_ProcessNotificationMessage();
		}
	}
}

static void *swupdate_client_handleIPCs(void *Arg)
{
	(void)Arg;

	while (1) {
		if (swuSocketFds[0].fd < 0) {
			swupdate_client_ConnectToSockets();
		} else {
			swupdate_client_ProcessSocketMessages();
		}
	}
	return NULL;
}

static void swupdate_client_setStatus(RECOVERY_STATUS Status,
				      const char *MessageInfo,
				      unsigned int MessageLen)
{
	struct progress_msg Message;

	Message.status = Status;
	strncpy(Message.info, MessageInfo, MessageLen);
	Message.info[MessageLen] = '\0';
	Message.infolen = strlen(MessageInfo);

	rec_mq_enqueueProgressMessage(&Message);
}

/**
 * Function to read next buffer chunk of local update file
 *
 * @param[out] BufPointer Buffer pointer containing next data chunk
 * @param[out] ChunkSize Size of data in buffer
 *
 * @return Unused
 */
static int swupdate_client_getNextChunk(char **BufPointer, int *ChunkSize)
{
	static char buffer[256];

	const int ret = read(swupdateFileFd, buffer, sizeof(buffer));

	if (ret < 0) {
		const char *errorMsg = "Error reading from the swu file.";

		LV_LOG_ERROR("%s Error code: %d\n", errorMsg, ret);

		PushSwupdateNotificationMessagesToTheList(errorMsg);

		*ChunkSize = 0;
	} else {
		*BufPointer = buffer;

		*ChunkSize = ret;
	}

	return 0;
}

/**
 * Callback function executed on update process end.
 *
 * @param[in] Status Status of update-process
 *
 * @return Unused
 */
static int swupdate_client_handleEndOfUpdate(RECOVERY_STATUS Status)
{
	if (SUCCESS == Status) {
		LV_LOG("EndOfUpdate: SWUpdate  was successful!");
	} else {
		LV_LOG("EndOfUpdate: SWUpdate failed!");
	}

	close(swupdateFileFd);
	swupdateFileFd = -1;

	return 0;
}

static void swupdate_client_ResetSwupdateNotificationMessages(void)
{
	LockSwupdateNotificationMessagesMutex();

	util_linked_list_release(
		AccessSwuUpdateNotificationMessages()->messages);
	AccessSwuUpdateNotificationMessages()->messages =
		util_linked_list_create();

	UnlockSwupdateNotificationMessagesMutex();
}

/**
 * Starts SWUpdate IPC message monitoring thread
 *
 */
void swupdate_client_startIPCThread(void)
{
	static pthread_t IPCThread;
	const int ret = pthread_create(&IPCThread, NULL,
				       swupdate_client_handleIPCs, NULL);

	if (0 != ret) {
		LV_LOG_ERROR(
			"Failed to create a thread to communicate with the swupdate process. Error code: %d\n",
			ret);
		exit(-1);
	}
}

/**
 * Function to start update via swupdate_sync_start with given local file
 *
 * @param[in] Filename Local update file
 * @param[in] IsDryRunEnabled Flag to enable dry-run
 *
 */
void swupdate_client_startLocalUpdate(const char *Filename,
				      const bool IsDryRunEnabled)
{
	swupdate_client_ResetSwupdateNotificationMessages();

	swupdateFileFd = open(Filename, O_RDONLY);

	if (swupdateFileFd < 0) {
		LV_LOG_WARN("Local-update: Unable to open file %s", Filename);

		const char *Message = "Unable to open file";
		swupdate_client_setStatus(FAILURE, Message, strlen(Message));
	} else {
		struct swupdate_request Request;

		swupdate_prepare_req(&Request);

		Request.dry_run = IsDryRunEnabled;

		const int rc =
			swupdate_async_start(swupdate_client_getNextChunk, NULL,
					     swupdate_client_handleEndOfUpdate,
					     &Request, sizeof(Request));

		if (rc < 0) {
			close(swupdateFileFd);
			swupdateFileFd = -1;

			const char *errorMsg =
				"The swupdate_async_start() function failed. Check the swupdate service state.";

			LV_LOG_ERROR("%s The return code: %d", errorMsg, rc);

			swupdate_client_setStatus(FAILURE, errorMsg,
						  strlen(errorMsg));
		}
	}
}

/**
 * Get all received messages from swupdate.
 * When done with it, the linked list with messages has to be released with util_linked_list_release() function
 *
 * @return pointer to the struct LinkedList containing all swupdate notification messages
 */
const struct LinkedList *swupdate_client_getSwupdateMessages(void)
{
	LockSwupdateNotificationMessagesMutex();

	const struct LinkedList *messages = util_linked_list_copy(
		AccessSwuUpdateNotificationMessages()->messages);

	UnlockSwupdateNotificationMessagesMutex();

	return messages;
}
