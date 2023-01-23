/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: These functions set the user's interface. Show the GUI of the program.
 *
 */

#include "rec-progress-mq.h"

#include "rec-screen-main.h"
#include "rec-screen-progress.h"

#include "rec-swupdate-client.h"

#include "rec-util-linked-list.h"

#include <lvgl/lvgl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mqueue.h>
#include <errno.h>

#define MSG_BUFFER_SIZE sizeof(struct progress_msg)

/* flag telling whether the image update is in progress */
static bool imageUpdateInProgress = false;

/* There are two threads involved: one that displays the gui, and another to communicate with
 * swupdate. The communication between these threads is implemented using a message queue.
 * The thread to display the gui uses the consumer file descriptor to receive update
 * notification messages
 * The thread that receive messages from swupdate uses the producer file descriptor to send
 * notification messages.
 */
static mqd_t IfmUi_ConsumerFd;
static mqd_t IfmUi_ProducerFd;

static bool IfmUi_dequeueProgressMessage(struct progress_msg *Message)
{
	static char InBuffer[MSG_BUFFER_SIZE];
	int Status =
		mq_receive(IfmUi_ConsumerFd, InBuffer, MSG_BUFFER_SIZE, NULL);

	if (-1 == Status) {
		if (errno != EAGAIN) {
			LV_LOG_ERROR(
				"Error receiving update progress message, code %d\n",
				errno);
		}
		return false;
	}

	memcpy(Message, InBuffer, sizeof(struct progress_msg));
	return true;
}

static void IfmUi_copyLogMessagesToTextArea(void)
{
	const struct LinkedList *MessageList =
		swupdate_client_getSwupdateMessages();
	const struct LinkedListNode *it =
		util_linked_list_get_first_element(MessageList);

	rec_screen_progress_clearUpdateMessages();

	while (it) {
		const char *Text = util_linked_list_get_data(it);
		rec_screen_progress_addMessage(Text);
		it = util_linked_list_iterate_to_next_element(MessageList, it);
	}
	util_linked_list_release(MessageList);
}

static void IfmIUi_startNewUpdate(void)
{
	rec_screen_progress_startingNewUpdate();

	rec_screen_progress_showScreen();

	imageUpdateInProgress = true;
}
static void IfmIUi_finishUpdate(const bool success)
{
	IfmUi_copyLogMessagesToTextArea();

	rec_screen_progress_finishUpdate(success);

	imageUpdateInProgress = false;
}

/**
 * Enqueue progress message for consumption by main UI thread
 *
 * @param[in] Message Latest update status
 */
void rec_mq_enqueueProgressMessage(struct progress_msg *Message)
{
	static char OutBuffer[MSG_BUFFER_SIZE];

	memcpy(OutBuffer, Message, sizeof(struct progress_msg));

	int status = mq_send(IfmUi_ProducerFd, OutBuffer,
			     sizeof(struct progress_msg), 0);
	if (-1 == status) {
		LV_LOG_ERROR("Not able to send progress message, code %d\n",
			     errno);
	}
}

/**
 * Function to process messages from swupdate.
 *
 */
void rec_mq_processProgressMessages(void)
{
	struct progress_msg Message;

	while (IfmUi_dequeueProgressMessage(&Message)) {
		switch (Message.status) {
		case IDLE:
			break;

		case START: {
			LV_LOG("swupdate: Start update, source: %d \n",
			       Message.source);
			IfmIUi_startNewUpdate();
		} break;

		case RUN:
		case DOWNLOAD:
		case PROGRESS: {
			if (!imageUpdateInProgress) {
				IfmIUi_startNewUpdate();
			}
		}

			rec_screen_progress_updateUpdateProgress(&Message);
			break;

		case SUCCESS: {
			LV_LOG("swupdate: Success");
			IfmIUi_finishUpdate(true);
		} break;

		case FAILURE: {
			LV_LOG("swupdate: Failure");
			IfmIUi_finishUpdate(false);
		} break;

		case DONE:
		case SUBPROCESS:
			break;

		default:
			LV_LOG_ERROR("Invalid update state %d", Message.status);
		}
	}
}

/**
 * Creates message queue to receive progress messages from swupdate.
 *
 * */
void rec_mq_createProgressMq(void)
{
	static const char *QUEUE_NAME = "/recovery-gui-messages";
	static const unsigned int QUEUE_PERMISSIONS = 0660;
	struct mq_attr Attr;

	Attr.mq_flags = 0;
	Attr.mq_maxmsg = 150;
	Attr.mq_msgsize = MSG_BUFFER_SIZE;
	Attr.mq_curmsgs = 0;

	IfmUi_ProducerFd = mq_open(QUEUE_NAME, O_WRONLY | O_CREAT | O_NONBLOCK,
				   QUEUE_PERMISSIONS, &Attr);

	if (-1 == IfmUi_ProducerFd) {
		LV_LOG_ERROR("Error in producer mq_open, code: %d\n", errno);
		exit(-1);
	}

	IfmUi_ConsumerFd = mq_open(QUEUE_NAME, O_RDONLY | O_NONBLOCK);

	if (-1 == IfmUi_ConsumerFd) {
		LV_LOG_ERROR("Error in consumer mq_open, code: %d\n", errno);
		exit(-1);
	}
}
