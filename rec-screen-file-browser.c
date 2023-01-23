/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains functions to create and operate file browser screen
 *
 */

#include "rec-screen-file-browser.h"

#include "rec-screen-common.h"
#include "rec-screen-progress.h"
#include "rec-screen-main.h"

#include "rec-swupdate-client.h"

#include "rec-styles.h"

#include "rec-util-files.h"
#include "rec-util-config.h"

#include <stdio.h>
#include <string.h>

/*
 * Dry-run checkbox
 *
 */
struct DryRun {
	lv_obj_t *Panel;
	lv_obj_t *Checkbox;

	bool IsDryRunEnabled;
};

/*
 * Buttons for start/rescan/cancel actions
 *
 */
struct Actions {
	lv_obj_t *StartButton;
	lv_obj_t *RescanButton;
	lv_obj_t *CancelButton;
};

/*
 * List of files/folders
 *
 */
struct Contents {
	lv_obj_t *Panel;
	lv_obj_t *DirPathLabel;
	lv_obj_t *DirContentsPanel;
	lv_obj_t *RowHeader;
	lv_obj_t *FileNameHeader;
	lv_obj_t *SizeHeader;
	lv_obj_t *FileEntries[FENTRIES_MAX];

	char SelectedFile[FNAME_MAX];
	char CurrentDirPath[FILEPATH_MAX];
	char TopDirPath[SETTING_STR_LENGTH_MAX];
	struct DirInfo DirInfo;
	unsigned int NumberOfFiles;
};

struct FileBrowserScreen {
	struct Screen Screen;
	struct DryRun DryRun;
	struct Contents Contents;
	struct Actions Actions;
};

static void
rec_screen_file_createFileEntries(struct FileBrowserScreen *FileBrowser);
static void
rec_screen_file_deleteFileEntries(struct FileBrowserScreen *FileBrowser);
static void
rec_screen_file_enableNavigationViaKeys(struct FileBrowserScreen *FileBrowser);
static void rec_screen_file_onClickButtonStart(lv_event_t *E);

static struct FileBrowserScreen *AccessFileBrowserScreen(void)
{
	static struct FileBrowserScreen Obj;
	return &Obj;
}

static void
rec_screen_file_updateDirPathLabel(struct FileBrowserScreen *FileBrowser)
{
	lv_label_set_text(FileBrowser->Contents.DirPathLabel,
			  FileBrowser->Contents.CurrentDirPath);
}

static void
rec_screen_file_createDirPathLabel(struct FileBrowserScreen *FileBrowser)
{
	FileBrowser->Contents.DirPathLabel =
		lv_label_create(FileBrowser->Contents.Panel);
	lv_obj_set_size(FileBrowser->Contents.DirPathLabel, LV_PCT(100),
			LV_PCT(10));
}

static void
rec_screen_file_createListHeader(struct FileBrowserScreen *FileBrowser)
{
	FileBrowser->Contents.DirContentsPanel =
		lv_obj_create(FileBrowser->Contents.Panel);
	lv_obj_set_flex_flow(FileBrowser->Contents.DirContentsPanel,
			     LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(FileBrowser->Contents.DirContentsPanel,
			      LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_SPACE_AROUND);

	rec_styles_applyContainerStyle(FileBrowser->Contents.DirContentsPanel);
	rec_styles_applyScrollbarStyle(FileBrowser->Contents.DirContentsPanel);

	lv_obj_set_size(FileBrowser->Contents.DirContentsPanel, LV_PCT(100),
			LV_PCT(90));

	FileBrowser->Contents.RowHeader =
		lv_obj_create(FileBrowser->Contents.DirContentsPanel);
	lv_obj_clear_flag(FileBrowser->Contents.RowHeader,
			  LV_OBJ_FLAG_CLICK_FOCUSABLE);

	rec_styles_applyContainerStyleBorderless(
		FileBrowser->Contents.RowHeader);
	lv_obj_set_style_pad_all(FileBrowser->Contents.RowHeader, 0,
				 LV_PART_MAIN);

	lv_obj_set_size(FileBrowser->Contents.RowHeader, LV_PCT(100), 30);
	lv_obj_set_flex_flow(FileBrowser->Contents.RowHeader, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(FileBrowser->Contents.RowHeader,
			      LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_SPACE_EVENLY);

	FileBrowser->Contents.FileNameHeader =
		lv_label_create(FileBrowser->Contents.RowHeader);
	lv_label_set_text(FileBrowser->Contents.FileNameHeader, "FILENAME");

	FileBrowser->Contents.SizeHeader =
		lv_label_create(FileBrowser->Contents.RowHeader);
	lv_label_set_text(FileBrowser->Contents.SizeHeader, "(SIZE in Bytes)");
}

static void
rec_screen_file_createListContainer(struct FileBrowserScreen *FileBrowser)
{
	FileBrowser->Contents.Panel = lv_obj_create(FileBrowser->Screen.Obj);

	rec_styles_applyContainerStyle(FileBrowser->Contents.Panel);

	rec_screen_disableScrolling(FileBrowser->Contents.Panel);

	lv_obj_set_size(FileBrowser->Contents.Panel, LV_PCT(100), LV_PCT(55));
	lv_obj_set_flex_flow(FileBrowser->Contents.Panel, LV_FLEX_FLOW_COLUMN);
}

static void
rec_screen_file_clearDryRunChk(struct FileBrowserScreen *FileBrowser)
{
	FileBrowser->DryRun.IsDryRunEnabled = false;
	lv_obj_clear_state(FileBrowser->DryRun.Checkbox, LV_STATE_CHECKED);
}

static void rec_screen_file_onClickDryRunChk(lv_event_t *E)
{
	struct FileBrowserScreen *FileBrowser =
		(struct FileBrowserScreen *)E->user_data;

	lv_event_code_t Code = lv_event_get_code(E);
	if (LV_EVENT_VALUE_CHANGED == Code) {
		lv_obj_t *Obj = lv_event_get_target(E);
		FileBrowser->DryRun.IsDryRunEnabled =
			(lv_obj_get_state(Obj) & LV_STATE_CHECKED);
	}
}

static void
rec_screen_file_createDryRunChk(struct FileBrowserScreen *FileBrowser)
{
	FileBrowser->DryRun.Panel = lv_obj_create(FileBrowser->Screen.Obj);

	rec_styles_applyContainerStyleBorderless(FileBrowser->DryRun.Panel);

	lv_obj_set_size(FileBrowser->DryRun.Panel, LV_PCT(100), LV_PCT(10));
	lv_obj_set_flex_flow(FileBrowser->DryRun.Panel, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(FileBrowser->DryRun.Panel, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY);

	FileBrowser->DryRun.Checkbox =
		lv_checkbox_create(FileBrowser->DryRun.Panel);

	rec_styles_applyCheckboxStyle(FileBrowser->DryRun.Checkbox);

	lv_checkbox_set_text(FileBrowser->DryRun.Checkbox, "Enable DRY-RUN");

	lv_obj_add_event_cb(FileBrowser->DryRun.Checkbox,
			    rec_screen_file_onClickDryRunChk, LV_EVENT_ALL,
			    FileBrowser);

	FileBrowser->DryRun.IsDryRunEnabled = false;
}

static void
rec_screen_file_setStartButtonState(struct FileBrowserScreen *FileBrowser,
				    bool State)
{
	if (State) {
		lv_obj_add_event_cb(FileBrowser->Actions.StartButton,
				    rec_screen_file_onClickButtonStart,
				    LV_EVENT_CLICKED, FileBrowser);

		lv_obj_clear_state(FileBrowser->Actions.StartButton,
				   LV_STATE_DISABLED);

		lv_obj_add_flag(FileBrowser->Actions.StartButton,
				LV_OBJ_FLAG_CLICK_FOCUSABLE);
		lv_obj_add_flag(FileBrowser->Actions.StartButton,
				LV_OBJ_FLAG_CLICKABLE);
	} else {
		lv_obj_remove_event_cb(FileBrowser->Actions.StartButton,
				       rec_screen_file_onClickButtonStart);

		lv_obj_add_state(FileBrowser->Actions.StartButton,
				 LV_STATE_DISABLED);

		lv_obj_clear_flag(FileBrowser->Actions.StartButton,
				  LV_OBJ_FLAG_CLICK_FOCUSABLE);
		lv_obj_clear_flag(FileBrowser->Actions.StartButton,
				  LV_OBJ_FLAG_CLICKABLE);
	}
}

static void
rec_screen_file_clearOtherFileEntries(struct FileBrowserScreen *FileBrowser,
				      lv_obj_t *currentEntry)
{
	lv_obj_t **It = FileBrowser->Contents.FileEntries;
	lv_obj_t **End = It + FileBrowser->Contents.NumberOfFiles;

	while (End != It) {
		if ((NULL != *It) && (currentEntry != *It)) {
			lv_obj_clear_state(*It, LV_STATE_CHECKED);
		}
		++It;
	}
}

static void
rec_screen_file_setSelectedFile(struct FileBrowserScreen *FileBrowser,
				const char *CheckboxText)
{
	char FileInfo[FNAME_MAX];
	strncpy(FileInfo, CheckboxText, sizeof(FileInfo) - 1);
	FileInfo[sizeof(FileInfo) - 1] = '\0';

	const char *Filename = strtok(FileInfo, " ");
	strncpy(FileBrowser->Contents.SelectedFile, Filename,
		sizeof(FileBrowser->Contents.SelectedFile) - 1);
	FileBrowser->Contents
		.SelectedFile[sizeof(FileBrowser->Contents.SelectedFile) - 1] =
		'\0';
}

static void
rec_screen_file_clearSelectedFile(struct FileBrowserScreen *FileBrowser)
{
	strcpy(FileBrowser->Contents.SelectedFile, "");
}

static bool
rec_screen_file_updateCurrentDir(struct FileBrowserScreen *FileBrowser,
				 const char *SelectedDir)
{
	const bool Status = util_files_updateCurrentDir(
		FileBrowser->Contents.CurrentDirPath,
		sizeof(FileBrowser->Contents.CurrentDirPath), SelectedDir);

	if (Status) {
		rec_screen_file_updateDirPathLabel(FileBrowser);
	}

	return Status;
}

static void rec_screen_file_onClickDirEntry(lv_event_t *E)
{
	const char *SelectedDir = (const char *)E->user_data;
	struct FileBrowserScreen *FileBrowser = AccessFileBrowserScreen();

	const bool Status =
		rec_screen_file_updateCurrentDir(FileBrowser, SelectedDir);
	if (!Status) {
		rec_screen_showNotification(
			&FileBrowser->Screen,
			"The selected directory path is not allowed.",
			NOTIFY_ERROR);
		return;
	}

	rec_screen_file_deleteFileEntries(FileBrowser);

	rec_screen_file_setStartButtonState(FileBrowser, false);

	rec_screen_clearNotification(&FileBrowser->Screen);

	rec_screen_file_createFileEntries(FileBrowser);

	rec_screen_file_enableNavigationViaKeys(FileBrowser);
}

static void rec_screen_file_onClickFileEntry(lv_event_t *E)
{
	struct FileBrowserScreen *FileBrowser =
		(struct FileBrowserScreen *)E->user_data;

	lv_obj_t *CheckBox = lv_event_get_target(E);
	const char *FileInfo = lv_checkbox_get_text(CheckBox);

	if (0 != (LV_STATE_CHECKED & lv_obj_get_state(CheckBox))) {
		rec_screen_file_setSelectedFile(FileBrowser, FileInfo);

		rec_screen_file_clearOtherFileEntries(FileBrowser, CheckBox);

		rec_screen_file_setStartButtonState(FileBrowser, true);
	} else {
		rec_screen_file_clearSelectedFile(FileBrowser);

		rec_screen_file_setStartButtonState(FileBrowser, false);
	}
}

static void
rec_screen_file_createDirButton_(struct FileBrowserScreen *FileBrowser,
				 const char *DirName, int Index)
{
	if (!util_files_isValidDirNameLength(DirName)) {
		rec_screen_showNotification(&FileBrowser->Screen,
					    "The directory name is too long.",
					    NOTIFY_WARNING);
	} else {
		FileBrowser->Contents.FileEntries[Index] =
			lv_btn_create(FileBrowser->Contents.DirContentsPanel);

		rec_styles_applyButtonStyle(
			FileBrowser->Contents.FileEntries[Index]);

		lv_obj_set_size(FileBrowser->Contents.FileEntries[Index],
				LV_PCT(100), 40);

		lv_obj_t *Label = lv_label_create(
			FileBrowser->Contents.FileEntries[Index]);
		lv_obj_align(Label, LV_ALIGN_LEFT_MID, 0, 0);
		lv_label_set_text(Label, DirName);

		lv_obj_add_event_cb(FileBrowser->Contents.FileEntries[Index],
				    rec_screen_file_onClickDirEntry,
				    LV_EVENT_CLICKED, (void *)DirName);
	}
}

static void
rec_screen_file_createFileCheckbox(struct FileBrowserScreen *FileBrowser,
				   const char *FileName, int FileSize,
				   int Index)
{
	char text[FILEPATH_MAX];
	bool Success =
		util_files_createFileEntryLabel(text, FileName, FileSize);
	if (!Success) {
		rec_screen_showNotification(&FileBrowser->Screen,
					    "Filename is too long.",
					    NOTIFY_WARNING);
	} else {
		FileBrowser->Contents.FileEntries[Index] = lv_checkbox_create(
			FileBrowser->Contents.DirContentsPanel);

		rec_styles_applyCheckboxStyle(
			FileBrowser->Contents.FileEntries[Index]);

		lv_obj_set_width(FileBrowser->Contents.FileEntries[Index],
				 LV_PCT(100));

		lv_checkbox_set_text(FileBrowser->Contents.FileEntries[Index],
				     text);

		lv_obj_add_event_cb(FileBrowser->Contents.FileEntries[Index],
				    rec_screen_file_onClickFileEntry,
				    LV_EVENT_CLICKED, FileBrowser);
	}
}

static void rec_screen_file_clearDirInfo(struct FileBrowserScreen *FileBrowser)
{
	memset(&FileBrowser->Contents.DirInfo, 0, sizeof(struct DirInfo));
}

static void
rec_screen_file_refreshDirInfo(struct FileBrowserScreen *FileBrowser)
{
	rec_screen_file_clearDirInfo(FileBrowser);

	util_files_listAllSwuFiles(FileBrowser->Contents.CurrentDirPath,
				   &FileBrowser->Contents.DirInfo);

	FileBrowser->Contents.NumberOfFiles =
		FileBrowser->Contents.DirInfo.DirCount +
		FileBrowser->Contents.DirInfo.SwuCount;

	if (FENTRIES_MAX < FileBrowser->Contents.NumberOfFiles) {
		char Message[MSG_LENGTH_MAX];
		snprintf(
			Message, sizeof(Message) - 1,
			"Too many Files/Directories: %d. Only first %d entries will be visible.",
			FileBrowser->Contents.NumberOfFiles, FENTRIES_MAX);

		rec_screen_showNotification(&FileBrowser->Screen, Message,
					    NOTIFY_WARNING);

		FileBrowser->Contents.NumberOfFiles = FENTRIES_MAX;
	}

	if (0 < FileBrowser->Contents.NumberOfFiles) {
		util_files_removeParentEntry(
			&FileBrowser->Contents.DirInfo,
			FileBrowser->Contents.TopDirPath,
			FileBrowser->Contents.CurrentDirPath,
			&FileBrowser->Contents.NumberOfFiles);
	}

	if (0 == FileBrowser->Contents.DirInfo.SwuCount) {
		rec_screen_showNotification(&FileBrowser->Screen,
					    "No swu files available!",
					    NOTIFY_WARNING);
	}
}

static void
rec_screen_file_createFileEntries(struct FileBrowserScreen *FileBrowser)
{
	rec_screen_file_refreshDirInfo(FileBrowser);

	unsigned int Index = 0;
	struct DirEntry *CurrentDir = FileBrowser->Contents.DirInfo.Dirs;
	while ((NULL != CurrentDir) && (FENTRIES_MAX > Index)) {
		rec_screen_file_createDirButton_(FileBrowser, CurrentDir->Name,
						 Index);

		CurrentDir = CurrentDir->Next;
		Index++;
	}

	struct FileEntry *CurrentFile = FileBrowser->Contents.DirInfo.SwuFiles;
	while ((NULL != CurrentFile) && (FENTRIES_MAX > Index)) {
		rec_screen_file_createFileCheckbox(FileBrowser,
						   CurrentFile->FileInfo.Name,
						   CurrentFile->FileInfo.Size,
						   Index++);
		CurrentFile = CurrentFile->Next;
	}

	rec_screen_file_clearSelectedFile(FileBrowser);
}

static void
rec_screen_file_createFileList(struct FileBrowserScreen *FileBrowser)
{
	rec_screen_file_createListContainer(FileBrowser);

	rec_screen_file_createDirPathLabel(FileBrowser);

	rec_screen_file_createListHeader(FileBrowser);
}

static void rec_screen_file_onClickButtonStart(lv_event_t *E)
{
	struct FileBrowserScreen *FileBrowser =
		(struct FileBrowserScreen *)E->user_data;

	rec_screen_progress_showScreen();

	char Filepath[FILEPATH_MAX + FNAME_MAX + 1];
	snprintf(Filepath, sizeof(Filepath) - 1, "%s/%s",
		 FileBrowser->Contents.CurrentDirPath,
		 FileBrowser->Contents.SelectedFile);

	swupdate_client_startLocalUpdate(Filepath,
					 FileBrowser->DryRun.IsDryRunEnabled);
}

static void
rec_screen_file_resetCurrentDir(struct FileBrowserScreen *FileBrowser)
{
	strncpy(FileBrowser->Contents.CurrentDirPath,
		FileBrowser->Contents.TopDirPath,
		sizeof(FileBrowser->Contents.CurrentDirPath) - 1);
	FileBrowser->Contents
		.CurrentDirPath[sizeof(FileBrowser->Contents.CurrentDirPath) -
				1] = '\0';

	rec_screen_file_updateDirPathLabel(FileBrowser);
}

static void
rec_screen_file_deleteFileEntries(struct FileBrowserScreen *FileBrowser)
{
	lv_obj_t **It = FileBrowser->Contents.FileEntries;
	lv_obj_t **End = It + FileBrowser->Contents.NumberOfFiles;

	while (End != It) {
		if (NULL != *It) {
			lv_obj_del(*It);
			*It = NULL;
		}
		++It;
	}

	util_files_deallocate(&FileBrowser->Contents.DirInfo);
}

static void
rec_screen_file_createStartButton(struct FileBrowserScreen *FileBrowser)
{
	FileBrowser->Actions.StartButton =
		lv_btn_create(FileBrowser->Screen.ActionsPanel);

	rec_styles_applyButtonStyle(FileBrowser->Actions.StartButton);

	lv_obj_set_size(FileBrowser->Actions.StartButton, LV_PCT(33), 50);

	lv_obj_t *LabelStart =
		lv_label_create(FileBrowser->Actions.StartButton);
	lv_label_set_text(LabelStart, "START");
	lv_obj_align(LabelStart, LV_ALIGN_CENTER, 0, 0);

	rec_screen_file_setStartButtonState(FileBrowser, false);
}

static void rec_screen_file_onClickRescanButton(lv_event_t *E)
{
	struct FileBrowserScreen *FileBrowser =
		(struct FileBrowserScreen *)E->user_data;

	rec_screen_file_deleteFileEntries(FileBrowser);

	rec_screen_file_setStartButtonState(FileBrowser, false);

	rec_screen_file_createFileEntries(FileBrowser);

	rec_screen_file_enableNavigationViaKeys(FileBrowser);
}

static void
rec_screen_file_createRescanButton(struct FileBrowserScreen *FileBrowser)
{
	FileBrowser->Actions.RescanButton =
		lv_btn_create(FileBrowser->Screen.ActionsPanel);

	rec_styles_applyButtonStyle(FileBrowser->Actions.RescanButton);

	lv_obj_set_size(FileBrowser->Actions.RescanButton, LV_PCT(33), 50);

	lv_obj_t *LabelRescan =
		lv_label_create(FileBrowser->Actions.RescanButton);
	lv_label_set_text(LabelRescan, "RESCAN");
	lv_obj_align(LabelRescan, LV_ALIGN_CENTER, 0, 0);

	lv_obj_add_event_cb(FileBrowser->Actions.RescanButton,
			    rec_screen_file_onClickRescanButton,
			    LV_EVENT_CLICKED, FileBrowser);
}

static void rec_screen_file_onClickCancelButton(lv_event_t *E)
{
	struct FileBrowserScreen *FileBrowser =
		(struct FileBrowserScreen *)E->user_data;
	rec_screen_file_deleteFileEntries(FileBrowser);

	rec_screen_main_showScreen();
}

static void
rec_screen_file_createCancelButton(struct FileBrowserScreen *FileBrowser)
{
	FileBrowser->Actions.CancelButton =
		lv_btn_create(FileBrowser->Screen.ActionsPanel);

	rec_styles_applyButtonStyle(FileBrowser->Actions.CancelButton);

	lv_obj_set_size(FileBrowser->Actions.CancelButton, LV_PCT(33), 50);

	lv_obj_t *LabelCancel =
		lv_label_create(FileBrowser->Actions.CancelButton);
	lv_label_set_text(LabelCancel, "CANCEL");
	lv_obj_align(LabelCancel, LV_ALIGN_CENTER, 0, 0);

	lv_obj_add_event_cb(FileBrowser->Actions.CancelButton,
			    rec_screen_file_onClickCancelButton,
			    LV_EVENT_CLICKED, FileBrowser);
}

static void
rec_screen_file_createButtonContainer(struct FileBrowserScreen *FileBrowser)
{
	FileBrowser->Screen.ActionsPanel =
		lv_obj_create(FileBrowser->Screen.Obj);

	rec_styles_applyContainerStyleBorderless(
		FileBrowser->Screen.ActionsPanel);

	lv_obj_set_size(FileBrowser->Screen.ActionsPanel, LV_PCT(100),
			LV_PCT(10));
	lv_obj_set_flex_flow(FileBrowser->Screen.ActionsPanel,
			     LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(FileBrowser->Screen.ActionsPanel,
			      LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_SPACE_EVENLY);

	rec_screen_disableScrolling(FileBrowser->Screen.ActionsPanel);
}

static void
rec_screen_file_createButtonList(struct FileBrowserScreen *FileBrowser)
{
	rec_screen_file_createButtonContainer(FileBrowser);

	rec_screen_file_createStartButton(FileBrowser);

	rec_screen_file_createRescanButton(FileBrowser);

	rec_screen_file_createCancelButton(FileBrowser);
}

static void
rec_screen_file_enableNavigationViaKeys(struct FileBrowserScreen *FileBrowser)
{
	lv_group_t *Group = lv_group_get_default();
	lv_group_remove_all_objs(Group);

	lv_group_add_obj(Group, FileBrowser->DryRun.Panel);
	lv_gridnav_add(FileBrowser->DryRun.Panel, LV_GRIDNAV_CTRL_SCROLL_FIRST);

	if (0 < FileBrowser->Contents.NumberOfFiles) {
		lv_group_add_obj(Group, FileBrowser->Contents.DirContentsPanel);
		lv_gridnav_add(FileBrowser->Contents.DirContentsPanel,
			       LV_GRIDNAV_CTRL_SCROLL_FIRST);
	}

	lv_group_add_obj(Group, FileBrowser->Screen.ActionsPanel);
	lv_gridnav_add(FileBrowser->Screen.ActionsPanel,
		       LV_GRIDNAV_CTRL_SCROLL_FIRST);

	lv_obj_add_state(FileBrowser->DryRun.Checkbox, LV_STATE_FOCUS_KEY);
}

static void rec_screen_file_setLayout(struct FileBrowserScreen *FileBrowser)
{
	// Set screen layout
	lv_obj_set_flex_flow(FileBrowser->Screen.Obj, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(FileBrowser->Screen.Obj, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND);

	// Set grow-factor for header
	lv_obj_set_flex_grow(FileBrowser->Screen.Header, 0);

	// Set layout of dry-run panel & file-list
	lv_obj_set_flex_grow(FileBrowser->DryRun.Panel, 0);
	lv_obj_set_flex_grow(FileBrowser->Contents.Panel, 1);

	// Set grow-factor of buttons-panel
	lv_obj_set_flex_grow(FileBrowser->Screen.ActionsPanel, 0);

	// Set grow-factor for notification
	lv_obj_set_flex_grow(FileBrowser->Screen.Notification, 0);
}

/**
 * Create file-browser screen
 *
 * @param[in] Config Configuration data
 */
void rec_screen_file_createFileBrowserScreen(const struct ConfigRecovery *Config)
{
	struct FileBrowserScreen *FileBrowser = AccessFileBrowserScreen();

	strncpy(FileBrowser->Contents.TopDirPath, Config->Mediapath,
		sizeof(FileBrowser->Contents.TopDirPath) - 1);
	FileBrowser->Contents
		.TopDirPath[sizeof(FileBrowser->Contents.TopDirPath) - 1] =
		'\0';

	rec_screen_createHeader(&FileBrowser->Screen, Config);
	rec_screen_file_createDryRunChk(FileBrowser);
	rec_screen_file_createFileList(FileBrowser);
	rec_screen_file_createButtonList(FileBrowser);
	rec_screen_createNotification(&FileBrowser->Screen);

	rec_screen_file_setLayout(FileBrowser);
}

/**
 * Load file-browser screen
 *
 */
void rec_screen_file_showFileBrowserScreen(void)
{
	struct FileBrowserScreen *FileBrowser = AccessFileBrowserScreen();

	rec_screen_clearNotification(&FileBrowser->Screen);

	rec_screen_file_resetCurrentDir(FileBrowser);

	rec_screen_file_deleteFileEntries(FileBrowser);

	rec_screen_file_clearDryRunChk(FileBrowser);

	rec_screen_file_setStartButtonState(FileBrowser, false);

	rec_screen_file_createFileEntries(FileBrowser);

	rec_screen_file_enableNavigationViaKeys(FileBrowser);

	rec_screen_loadScreen(&FileBrowser->Screen);
}
