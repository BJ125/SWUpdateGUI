/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains functions to create & operate update-progress screen
 *
 */

#include "rec-screen-progress.h"

#include "rec-screen-main.h"
#include "rec-screen-common.h"

#include "rec-styles.h"

#include "rec-util-linked-list.h"
#include "rec-util-config.h"

#include "rec-swupdate-client.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


/*
 * Progress bar with a title
 */
struct ProgressBar {
	/* Parent widget that represents this area, and acts as a layout. */
	lv_obj_t *Container;

	/* Label above the slider to explain it */
	lv_obj_t *Label;

	/* Slider to show the progress bar */
	lv_obj_t *Slider;
};

/*
 * This structure represents the content that goes between the header and the notification areas.
 */
struct ProgressBars {
	/* Parent widget that represents this area, and acts as a layout. */
	lv_obj_t *Container;

	/* Shows current step and total number of steps */
	struct ProgressBar StepsProgress;

	/* Shows progress of the current step */
	struct ProgressBar PercentProgress;
};

/*
 * Contains log messages
 * */
struct LogMessages {
	/* Parent widget that represents this area, and acts as a layout. */
	lv_obj_t *Container;

	/* this shows logs */
	lv_obj_t *TextArea;
};

/*
 * Action panels containing buttons
 */
struct ActionsPanel {
	/* Parent widget that represents this area, and acts as a layout. */
	lv_obj_t *Container;

	/* The progress screen contains following buttons:
            - OK - to close the progress screen and go back to the previous screen
            - SHOW LOGS - to show logs from swupdate program after an image update
       When logs are shown, following buttons are displayed:
            - BACK - to go back to the progress screen
            - EXPORT LOGS - the export the logs to the usb
     */
	lv_obj_t *OkButton;
	lv_obj_t *ShowLogsButton;

	lv_obj_t *backButton;
	lv_obj_t *exportLogsButton;
};

struct ProgressScreen {
	struct Screen Screen;

	struct ProgressBars ProgressBars;
	struct LogMessages logMessages;

	struct ActionsPanel ActionsPanel;

	lv_color_t PrevNotificationColor;
};

static void rec_screen_progress_onClickShowLogsButton(lv_event_t *E);
static void rec_screen_progress_onClickBackButton(lv_event_t *E);
static void rec_screen_progress_onClickExportLogsButton(lv_event_t *E);

/* this function creates a file with unique name, opens it and return a file descriptor to it */
static int create_export_log_file(char **filename)
{
	const struct RecoveryParameters *params =
		util_config_getRecoveryParameters();

	const char *template = "swupdate.XXXXXX.log";

	/* filename consists of the path to the location as defined in the configuration,
     * followed by "/", followed by the template, and '\0' at the end to terminate the string */
	const unsigned int length =
		strlen(params->Config.Mediapath) + 1 + strlen(template) + 1;
	*filename = malloc(length);
	(*filename)[0] = '\0';

	const int res = snprintf(*filename, length, "%s/%s",
				 params->Config.Mediapath, template);
	if (res < 0) {
		LV_LOG_ERROR(
			"Failed to generate unique filename. Generated this: %s",
			*filename);
		return res;
	}

	/* the sufix is ".log", which is 4 characters long */
	const int fd = mkstemps(*filename, 4);

	LV_LOG_INFO("Exporting swupdate messages to %s.", *filename);

	return fd;
}

static int write_swupdate_log_messages_to_file(const int fd)
{
	const struct LinkedList *messages =
		swupdate_client_getSwupdateMessages();

	const struct LinkedListNode *it =
		util_linked_list_get_first_element(messages);

	while (NULL != it) {
		const int res = write(fd, util_linked_list_get_data(it),
				      util_linked_list_get_data_size(it));
		if (-1 == res) {
			LV_LOG_ERROR(
				"Failed to write swupdate logs to the log file.");

			util_linked_list_release(messages);

			return res;
		}

		it = util_linked_list_iterate_to_next_element(messages, it);
	}

	util_linked_list_release(messages);

	return 0;
}

/**
 * Saves swupdate logs in the disk location as defined in the configuration.
 * It copies the filename into the first argument.
 *
 * @param[out] filename if the operation succeeds, this argument will contain allocated string,
 *                      which has to be free()-ed. When the operation fails, this pointer will
 *                      point to NULL
 *
 * @return 0 when the operation succeeded, a negative number otherwise
 *
 * @note when the operation succceeds, the pointer returned in filename has to be free()-ed
 * */
static int util_files_export_swupdate_logs(char **filename)
{
	const int fd = create_export_log_file(filename);

	if (fd < 0) {
		LV_LOG_ERROR(
			"Failed to generate unique filename, or create the file with such filename.");

		free(*filename);
		*filename = NULL;

		return fd;
	}

	const int res = write_swupdate_log_messages_to_file(fd);

	if (res < 0) {
		free(*filename);
		*filename = NULL;
	}

	close(fd);

	/* force synchronization of disk caches */
	sync();

	return res;
}

static struct ProgressScreen *access_progress_screen(void)
{
	static struct ProgressScreen *obj = NULL;

	if (NULL == obj) {
		obj = malloc(sizeof(*obj));
		memset(obj, 0, sizeof(*obj));
	}

	return obj;
}

static void
rec_screen_progress_createStepsProgressLabel(struct ProgressScreen *Progress)
{
	Progress->ProgressBars.StepsProgress.Label =
		lv_label_create(Progress->ProgressBars.StepsProgress.Container);

	lv_obj_set_align(Progress->ProgressBars.StepsProgress.Label,
			 LV_ALIGN_CENTER);
	lv_label_set_text(Progress->ProgressBars.StepsProgress.Label,
			  "NUMBER OF STEPS(0/0)");
}

static void
rec_screen_progress_createPercentProgressLabel(struct ProgressScreen *Progress)
{
	Progress->ProgressBars.PercentProgress.Label = lv_label_create(
		Progress->ProgressBars.PercentProgress.Container);

	lv_obj_set_align(Progress->ProgressBars.PercentProgress.Label,
			 LV_ALIGN_CENTER);
	lv_label_set_text(Progress->ProgressBars.PercentProgress.Label,
			  "CURRENT STEP(0%)");
}

static void
rec_screen_progress_createStepsProgressPanel(struct ProgressScreen *Progress)
{
	Progress->ProgressBars.StepsProgress.Container =
		lv_obj_create(Progress->ProgressBars.Container);

	rec_styles_applyContainerStyleBorderless(
		Progress->ProgressBars.StepsProgress.Container);
	lv_obj_set_flex_flow(Progress->ProgressBars.StepsProgress.Container,
			     LV_FLEX_FLOW_COLUMN);
	lv_obj_set_width(Progress->ProgressBars.StepsProgress.Container,
			 LV_PCT(100));

	rec_screen_disableScrolling(
		Progress->ProgressBars.StepsProgress.Container);
}

static void
rec_screen_progress_createPercentProgressPanel(struct ProgressScreen *Progress)
{
	Progress->ProgressBars.PercentProgress.Container =
		lv_obj_create(Progress->ProgressBars.Container);

	rec_styles_applyContainerStyleBorderless(
		Progress->ProgressBars.PercentProgress.Container);

	lv_obj_set_flex_flow(Progress->ProgressBars.PercentProgress.Container,
			     LV_FLEX_FLOW_COLUMN);
	lv_obj_set_width(Progress->ProgressBars.PercentProgress.Container,
			 LV_PCT(100));

	rec_screen_disableScrolling(
		Progress->ProgressBars.PercentProgress.Container);
}

static void
rec_screen_progress_createPercentProgressSlider(struct ProgressScreen *Progress)
{
	Progress->ProgressBars.PercentProgress.Slider = lv_slider_create(
		Progress->ProgressBars.PercentProgress.Container);

	lv_obj_set_size(Progress->ProgressBars.PercentProgress.Slider,
			LV_PCT(100), 30);

	lv_obj_clear_flag(Progress->ProgressBars.PercentProgress.Slider,
			  LV_OBJ_FLAG_CLICKABLE);
}

static void
rec_screen_progress_createPercentProgress(struct ProgressScreen *Progress)
{
	rec_screen_progress_createPercentProgressPanel(Progress);

	rec_screen_progress_createPercentProgressLabel(Progress);

	rec_screen_progress_createPercentProgressSlider(Progress);
}

static void
rec_screen_progress_createContentsPanel(struct ProgressScreen *Progress)
{
	Progress->ProgressBars.Container = lv_obj_create(Progress->Screen.Obj);

	lv_obj_set_flex_flow(Progress->ProgressBars.Container,
			     LV_FLEX_FLOW_COLUMN);

	rec_styles_applyContainerStyleBorderless(
		Progress->ProgressBars.Container);

	rec_screen_disableScrolling(Progress->ProgressBars.Container);
}

static void
rec_screen_progress_createStepsProgressSlider(struct ProgressScreen *Progress)
{
	Progress->ProgressBars.StepsProgress.Slider = lv_slider_create(
		Progress->ProgressBars.StepsProgress.Container);

	lv_obj_set_size(Progress->ProgressBars.StepsProgress.Slider,
			LV_PCT(100), 30);

	lv_obj_clear_flag(Progress->ProgressBars.StepsProgress.Slider,
			  LV_OBJ_FLAG_CLICKABLE);
}

static void rec_progress_createStepsProgress(struct ProgressScreen *progress)
{
	rec_screen_progress_createStepsProgressPanel(progress);

	rec_screen_progress_createStepsProgressLabel(progress);

	rec_screen_progress_createStepsProgressSlider(progress);
}

static void rec_screen_progress_onClickedOkButton(lv_event_t *E)
{
	(void)E;

	rec_screen_main_showScreen();
}

static void
rec_screen_progress_createActionsPanel(struct ProgressScreen *Progress)
{
	Progress->ActionsPanel.Container = lv_obj_create(Progress->Screen.Obj);

	rec_styles_applyContainerStyle(Progress->ActionsPanel.Container);

	rec_screen_disableScrolling(Progress->ActionsPanel.Container);

	lv_obj_set_flex_flow(Progress->ActionsPanel.Container,
			     LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(Progress->ActionsPanel.Container,
			      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_CENTER);
}

static void
rec_screen_progress_createOkButton(struct ProgressScreen *progressScreen)
{
	progressScreen->ActionsPanel.OkButton =
		lv_btn_create(progressScreen->ActionsPanel.Container);

	rec_styles_applyButtonStyle(progressScreen->ActionsPanel.OkButton);

	lv_obj_center(progressScreen->ActionsPanel.OkButton);
	lv_obj_set_size(progressScreen->ActionsPanel.OkButton, LV_PCT(50), 40);
	lv_obj_add_state(progressScreen->ActionsPanel.OkButton,
			 LV_STATE_DISABLED);

	lv_obj_t *label =
		lv_label_create(progressScreen->ActionsPanel.OkButton);
	lv_obj_center(label);
	lv_label_set_text_static(label, "OK");

	lv_obj_add_event_cb(progressScreen->ActionsPanel.OkButton,
			    rec_screen_progress_onClickedOkButton,
			    LV_EVENT_CLICKED, NULL);
}

static void
rec_screen_progress_createShowLogsButton(struct ProgressScreen *Progress)
{
	Progress->ActionsPanel.ShowLogsButton =
		lv_btn_create(Progress->ActionsPanel.Container);

	rec_styles_applyButtonStyle(Progress->ActionsPanel.ShowLogsButton);

	lv_obj_center(Progress->ActionsPanel.ShowLogsButton);
	lv_obj_set_size(Progress->ActionsPanel.ShowLogsButton, LV_PCT(50), 40);
	lv_obj_add_state(Progress->ActionsPanel.ShowLogsButton,
			 LV_STATE_DISABLED);

	lv_obj_t *label =
		lv_label_create(Progress->ActionsPanel.ShowLogsButton);
	lv_obj_center(label);
	lv_label_set_text_static(label, "SHOW LOGS");

	lv_obj_add_event_cb(Progress->ActionsPanel.ShowLogsButton,
			    rec_screen_progress_onClickShowLogsButton,
			    LV_EVENT_CLICKED, Progress);
}

static void
rec_screen_progress_createBackButton(struct ProgressScreen *Progress)
{
	Progress->ActionsPanel.backButton =
		lv_btn_create(Progress->ActionsPanel.Container);

	rec_styles_applyButtonStyle(Progress->ActionsPanel.backButton);

	lv_obj_center(Progress->ActionsPanel.backButton);
	lv_obj_set_size(Progress->ActionsPanel.backButton, LV_PCT(50), 40);

	lv_obj_t *label = lv_label_create(Progress->ActionsPanel.backButton);
	lv_obj_center(label);
	lv_label_set_text_static(label, "BACK");

	lv_obj_add_event_cb(Progress->ActionsPanel.backButton,
			    rec_screen_progress_onClickBackButton,
			    LV_EVENT_CLICKED, Progress);
}

static void
rec_screen_progress_createExportLogsButton(struct ProgressScreen *Progress)
{
	Progress->ActionsPanel.exportLogsButton =
		lv_btn_create(Progress->ActionsPanel.Container);

	rec_styles_applyButtonStyle(Progress->ActionsPanel.exportLogsButton);

	lv_obj_center(Progress->ActionsPanel.exportLogsButton);
	lv_obj_set_size(Progress->ActionsPanel.exportLogsButton, LV_PCT(50),
			40);

	lv_obj_t *label =
		lv_label_create(Progress->ActionsPanel.exportLogsButton);
	lv_obj_center(label);
	lv_label_set_text_static(label, "EXPORT LOGS");

	lv_obj_add_event_cb(Progress->ActionsPanel.exportLogsButton,
			    rec_screen_progress_onClickExportLogsButton,
			    LV_EVENT_CLICKED, Progress);
}

static void rec_screen_progress_createActions(struct ProgressScreen *progress)
{
	rec_screen_progress_createActionsPanel(progress);

	rec_screen_progress_createOkButton(progress);
	rec_screen_progress_createShowLogsButton(progress);
	rec_screen_progress_createBackButton(progress);
	rec_screen_progress_createExportLogsButton(progress);
}

static void
rec_screen_progress_enableNavigation(struct ProgressScreen *Progress)
{
	lv_group_t *Group = lv_group_get_default();

	lv_group_remove_all_objs(Group);

	lv_group_add_obj(Group, Progress->ActionsPanel.Container);

	lv_gridnav_add(Progress->ActionsPanel.Container, LV_GRIDNAV_CTRL_NONE);

	lv_obj_add_state(Progress->ActionsPanel.OkButton, LV_STATE_FOCUS_KEY);
	lv_obj_add_state(Progress->ActionsPanel.ShowLogsButton,
			 LV_STATE_FOCUS_KEY);
}

static void
rec_screen_progress_resetProgressBars(struct ProgressScreen *Progress)
{
	lv_slider_set_value(Progress->ProgressBars.StepsProgress.Slider, 0,
			    LV_ANIM_OFF);
	lv_slider_set_value(Progress->ProgressBars.PercentProgress.Slider, 0,
			    LV_ANIM_OFF);

	lv_obj_add_state(Progress->ActionsPanel.OkButton, LV_STATE_DISABLED);
	lv_obj_add_state(Progress->ActionsPanel.ShowLogsButton,
			 LV_STATE_DISABLED);

	rec_screen_showNotification(&Progress->Screen, "UPDATE IN PROGRESS",
				    NOTIFY_INFO);
}

static void
rec_screen_progress_createProgressBars(struct ProgressScreen *Progress)
{
	rec_screen_progress_createContentsPanel(Progress);

	rec_progress_createStepsProgress(Progress);

	rec_screen_progress_createPercentProgress(Progress);
}

static void rec_screen_progress_setLayout(struct ProgressScreen *ProgressScreen)
{
	/* Set screen layout */
	lv_obj_set_flex_flow(ProgressScreen->Screen.Obj, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(ProgressScreen->Screen.Obj, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND);

	/* header goes at the top */
	lv_obj_set_align(ProgressScreen->Screen.Header, LV_ALIGN_TOP_MID);
	/* prevent header from expanding it's size */
	lv_obj_set_flex_grow(ProgressScreen->Screen.Header, 0);

	/* progress bars and log messages go in the middle. when one is shown, the other is hidden */
	lv_obj_align_to(ProgressScreen->ProgressBars.Container,
			ProgressScreen->Screen.Header, LV_ALIGN_OUT_BOTTOM_MID,
			0, 0);
	lv_obj_set_flex_flow(ProgressScreen->ProgressBars.Container,
			     LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_grow(ProgressScreen->ProgressBars.Container, 1);
	lv_obj_set_width(ProgressScreen->ProgressBars.Container, LV_PCT(100));

	/* progress bars and log messages go in the middle. when one is shown, the other is hidden */
	lv_obj_align_to(ProgressScreen->logMessages.Container,
			ProgressScreen->Screen.Header, LV_ALIGN_OUT_BOTTOM_MID,
			0, 0);
	lv_obj_set_flex_flow(ProgressScreen->logMessages.Container,
			     LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_grow(ProgressScreen->logMessages.Container, 1);
	lv_obj_set_width(ProgressScreen->logMessages.Container, LV_PCT(100));

	/* notification bar goes at the bottom */
	lv_obj_set_align(ProgressScreen->Screen.Notification,
			 LV_ALIGN_BOTTOM_MID);
	/* prevent header from expanding it's size */
	lv_obj_set_flex_grow(ProgressScreen->Screen.Notification, 0);

	/* above notification area goes area with buttons */
	lv_obj_align_to(ProgressScreen->ActionsPanel.Container,
			ProgressScreen->Screen.Notification,
			LV_ALIGN_OUT_TOP_MID, 0, 0);
	lv_obj_set_width(ProgressScreen->ActionsPanel.Container, LV_PCT(100));
	lv_obj_set_height(ProgressScreen->ActionsPanel.Container, LV_PCT(15));
	/* prevent it from changing it's size */
	lv_obj_set_flex_grow(ProgressScreen->ActionsPanel.Container, 0);
}

static void rec_screen_progressScreen_showProgressBars(
	struct ProgressScreen *ProgressScreen)
{
	rec_screen_progress_setLayout(ProgressScreen);

	/* show needed panels */
	lv_obj_clear_flag(ProgressScreen->ProgressBars.Container,
			  LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(ProgressScreen->logMessages.Container,
			LV_OBJ_FLAG_HIDDEN);

	/* show needed buttons */
	lv_obj_clear_flag(ProgressScreen->ActionsPanel.OkButton,
			  LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(ProgressScreen->ActionsPanel.ShowLogsButton,
			  LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(ProgressScreen->ActionsPanel.backButton,
			LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(ProgressScreen->ActionsPanel.exportLogsButton,
			LV_OBJ_FLAG_HIDDEN);
}

static void
rec_screen_progressScreen_showLogMessages(struct ProgressScreen *ProgressScreen)
{
	rec_screen_progress_setLayout(ProgressScreen);

	/* show needed panels */
	lv_obj_add_flag(ProgressScreen->ProgressBars.Container,
			LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(ProgressScreen->logMessages.Container,
			  LV_OBJ_FLAG_HIDDEN);

	/* show needed buttons */
	lv_obj_add_flag(ProgressScreen->ActionsPanel.OkButton,
			LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(ProgressScreen->ActionsPanel.ShowLogsButton,
			LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(ProgressScreen->ActionsPanel.backButton,
			  LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(ProgressScreen->ActionsPanel.exportLogsButton,
			  LV_OBJ_FLAG_HIDDEN);
}

static void rec_screen_progress_onClickShowLogsButton(lv_event_t *E)
{
	struct ProgressScreen *ProgressScreen = E->user_data;

	rec_screen_progressScreen_showLogMessages(ProgressScreen);
}

static void rec_screen_progress_onClickBackButton(lv_event_t *E)
{
	struct ProgressScreen *ProgressScreen = E->user_data;

	rec_screen_progressScreen_showProgressBars(ProgressScreen);
}

static void rec_screen_progress_onClickExportLogsButton(lv_event_t *E)
{
	struct ProgressScreen *screen = E->user_data;

	char *filename = NULL;
	const int res = util_files_export_swupdate_logs(&filename);

	if (0 == res) {
		char msg[MSG_LENGTH_MAX];
		snprintf(msg, sizeof(msg), "Logs written to %s.", filename);
		rec_screen_showNotification(&screen->Screen, msg,
					    NOTIFY_SUCCESS);
	} else {
		rec_screen_showNotification(&screen->Screen,
					    "Failed to export logs.",
					    NOTIFY_ERROR);
	}

	free(filename);
}

static void rec_screen_progressScreen_createLogsMessagesTextArea(
	struct ProgressScreen *ProgressScreen)
{
	ProgressScreen->logMessages.Container =
		lv_obj_create(ProgressScreen->Screen.Obj);

	ProgressScreen->logMessages.TextArea =
		lv_textarea_create(ProgressScreen->logMessages.Container);

	rec_styles_applyTextLogStyle(ProgressScreen->logMessages.TextArea);
	rec_styles_applyScrollbarStyle(ProgressScreen->logMessages.TextArea);

	/* center the text and allow it to expand as much as possible */
	lv_obj_set_size(ProgressScreen->logMessages.TextArea, LV_PCT(100),
			LV_PCT(100));
	lv_obj_center(ProgressScreen->logMessages.TextArea);

	lv_obj_set_style_text_align(ProgressScreen->logMessages.TextArea,
				    LV_ALIGN_LEFT_MID, LV_PART_MAIN);
}

/**
 * Create progress screen
 *
 * @param[in] Config Configuration data
 */
void rec_screen_progress_createScreen(const struct ConfigRecovery *Config)
{
	struct ProgressScreen *progressScreen = access_progress_screen();

	rec_screen_createHeader(&progressScreen->Screen, Config);
	rec_screen_progress_createProgressBars(progressScreen);
	rec_screen_progressScreen_createLogsMessagesTextArea(progressScreen);
	rec_screen_progress_createActions(progressScreen);
	rec_screen_createNotification(&progressScreen->Screen);

	rec_screen_progress_resetProgressBars(progressScreen);
}

/**
 * Show progress screen
 *
 */
void rec_screen_progress_showScreen(void)
{
	struct ProgressScreen *Progress = access_progress_screen();

	rec_screen_loadScreen(&Progress->Screen);

	rec_screen_progressScreen_showProgressBars(Progress);

	rec_screen_progress_enableNavigation(Progress);
}

/**
 * Starting new image update. This function will reset all fields to show the new image
 * update progress.
 * */
void rec_screen_progress_startingNewUpdate(void)
{
	struct ProgressScreen *progressScreen = access_progress_screen();

	rec_screen_progress_clearUpdateMessages();

	rec_screen_showNotification(&progressScreen->Screen,
				    "UPDATE IN PROGRESS", NOTIFY_INFO);

	lv_obj_add_state(progressScreen->ActionsPanel.OkButton,
			 LV_STATE_DISABLED);
	lv_obj_add_state(progressScreen->ActionsPanel.ShowLogsButton,
			 LV_STATE_DISABLED);
}

/**
 * Update progress on the progress screen with latest information
 *
 * @param[in] Message Latest progress data
 */
void rec_screen_progress_updateUpdateProgress(const struct progress_msg *Message)
{
	LV_LOG_INFO("Update message: %d/%d   %d%%", Message->cur_step,
		    Message->nsteps, Message->cur_percent);

	struct ProgressScreen *Progress = access_progress_screen();

	unsigned int StepsPercent = 0;
	char StepsText[50];
	char PercentText[50];
	if (Message->nsteps > 0) {
		StepsPercent = 100 * Message->cur_step / Message->nsteps;
		snprintf(StepsText, sizeof(StepsText) - 1,
			 "NUMBER OF STEPS(%d/%d)", Message->cur_step,
			 Message->nsteps);
		snprintf(PercentText, sizeof(PercentText) - 1,
			 "CURRENT STEP(%d%%)", Message->cur_percent);

		lv_label_set_text(Progress->ProgressBars.StepsProgress.Label,
				  StepsText);
		lv_label_set_text(Progress->ProgressBars.PercentProgress.Label,
				  PercentText);
		lv_slider_set_value(
			Progress->ProgressBars.PercentProgress.Slider,
			Message->cur_percent, LV_ANIM_OFF);
		lv_slider_set_value(Progress->ProgressBars.StepsProgress.Slider,
				    StepsPercent, LV_ANIM_OFF);
	} else {
		lv_slider_set_value(
			Progress->ProgressBars.PercentProgress.Slider, 0,
			LV_ANIM_OFF);
		lv_slider_set_value(Progress->ProgressBars.StepsProgress.Slider,
				    0, LV_ANIM_OFF);
	}
}

/**
 * Show update result after the image update finishes
 *
 * @param[in] isSuccess true when the update was successful, false otherwise
 *
 */
void rec_screen_progress_finishUpdate(const bool isSuccess)
{
	struct ProgressScreen *progressScreen = access_progress_screen();

	if (isSuccess) {
		rec_screen_showNotification(&progressScreen->Screen, "Success!",
					    NOTIFY_SUCCESS);
	} else {
		rec_screen_showNotification(&progressScreen->Screen,
					    "Update failed!", NOTIFY_ERROR);
	}

	lv_obj_clear_state(progressScreen->ActionsPanel.OkButton,
			   LV_STATE_DISABLED);
	lv_obj_clear_state(progressScreen->ActionsPanel.ShowLogsButton,
			   LV_STATE_DISABLED);
}

/**
 * Add swupdate-messages to extended-notification
 *
 * @param[in] Text SWUpdate message
 */
void rec_screen_progress_addMessage(const char *Text)
{
	struct ProgressScreen *Progress = access_progress_screen();

	lv_textarea_add_text(Progress->logMessages.TextArea, Text);
	lv_textarea_add_text(Progress->logMessages.TextArea, "\n");
}

/**
 * Clear messages in extended-notification
 *
 */
void rec_screen_progress_clearUpdateMessages(void)
{
	struct ProgressScreen *Progress = access_progress_screen();

	lv_textarea_set_text(Progress->logMessages.TextArea, "");
}
