/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains functions to create and operate main screen UI
 *
 */

#include "rec-screen-main.h"

#include "rec-screen-common.h"
#include "rec-screen-file-browser.h"
#include "rec-screen-progress.h"
#include "rec-screen-network-setup.h"

#include "rec-styles.h"

#include "rec-util-config.h"
#include "rec-util-networking.h"

#include <stdlib.h>
#include <stdio.h>

/**
 * List of buttons for install-update, edit network-settings
 * & restart-device operations
 *
 */
struct Actions {
	lv_obj_t *Container;
	lv_obj_t *InstallButton;
	lv_obj_t *NetworkSetupButton;
	lv_obj_t *RestartButton;
};

/**
 * Area to show network-information for available interfaces
 *
 */
struct NetworkInfo {
	lv_obj_t *Container;
	lv_obj_t *InterfaceList;
};

/**
 * Main-screen backend data
 *
 */
struct Data {
	struct Ipv4Info *Interfaces;
	unsigned int InterfaceCount;
};

/**
 * Structure containing all widgets and backend-data
 * for MainScreen
 *
 */
struct MainScreen {
	struct Screen Screen;
	struct Actions Actions;
	struct NetworkInfo NetworkInfo;
	struct Data Data;
};

static void rec_screen_main_onClickInstallButton(lv_event_t *Event)
{
	(void)Event;

	rec_screen_file_showFileBrowserScreen();
}

static void rec_screen_main_OnClickNetworkSettingButton(lv_event_t *Event)
{
	(void)Event;
	rec_screen_network_showNetworkScreen();
}

static void rec_screen_main_onClickRestartButton(lv_event_t *Event)
{
	struct MainScreen *MainScreen = (struct MainScreen *)Event->user_data;

	const int rebootStatus = system("reboot");
	if (0 != rebootStatus) {
		rec_screen_showNotification(&MainScreen->Screen,
					    "Error rebooting the device",
					    NOTIFY_ERROR);
	}
}

static void rec_screen_main_createInstallButton(struct MainScreen *MainScreen)
{
	MainScreen->Actions.InstallButton =
		lv_btn_create(MainScreen->Actions.Container);

	rec_styles_applyButtonStyle(MainScreen->Actions.InstallButton);

	lv_obj_set_size(MainScreen->Actions.InstallButton, LV_PCT(100), 40);
	lv_obj_t *Label = lv_label_create(MainScreen->Actions.InstallButton);
	lv_label_set_text(Label, "INSTALL FROM FILE");
	lv_obj_center(Label);
	lv_obj_add_event_cb(MainScreen->Actions.InstallButton,
			    rec_screen_main_onClickInstallButton,
			    LV_EVENT_CLICKED, NULL);
}

static void
rec_screen_main_createNetworkSetupButton(struct MainScreen *MainScreen)
{
	MainScreen->Actions.NetworkSetupButton =
		lv_btn_create(MainScreen->Actions.Container);

	rec_styles_applyButtonStyle(MainScreen->Actions.NetworkSetupButton);

	lv_obj_set_size(MainScreen->Actions.NetworkSetupButton, LV_PCT(100),
			40);
	lv_obj_add_flag(MainScreen->Actions.NetworkSetupButton,
			LV_OBJ_FLAG_HIDDEN);

	lv_obj_t *Label =
		lv_label_create(MainScreen->Actions.NetworkSetupButton);
	lv_label_set_text(Label, "NETWORK SETUP");
	lv_obj_center(Label);
	lv_obj_add_event_cb(MainScreen->Actions.NetworkSetupButton,
			    rec_screen_main_OnClickNetworkSettingButton,
			    LV_EVENT_CLICKED, NULL);
}

static void rec_screen_main_createRestartButton(struct MainScreen *MainScreen)
{
	MainScreen->Actions.RestartButton =
		lv_btn_create(MainScreen->Actions.Container);

	rec_styles_applyButtonStyle(MainScreen->Actions.RestartButton);

	lv_obj_set_size(MainScreen->Actions.RestartButton, LV_PCT(100), 40);
	lv_obj_t *Label = lv_label_create(MainScreen->Actions.RestartButton);
	lv_label_set_text(Label, "RESTART");
	lv_obj_center(Label);
	lv_obj_add_event_cb(MainScreen->Actions.RestartButton,
			    rec_screen_main_onClickRestartButton,
			    LV_EVENT_CLICKED, MainScreen);
}

static void
rec_screen_main_createActionsContainer(struct MainScreen *MainScreen)
{
	MainScreen->Actions.Container = lv_obj_create(MainScreen->Screen.Obj);

	rec_styles_applyContainerStyleBorderless(MainScreen->Actions.Container);

	rec_screen_disableScrolling(MainScreen->Actions.Container);
}

static void rec_screen_main_createActions(struct MainScreen *MainScreen)
{
	rec_screen_main_createActionsContainer(MainScreen);

	rec_screen_main_createInstallButton(MainScreen);

	rec_screen_main_createNetworkSetupButton(MainScreen);

	rec_screen_main_createRestartButton(MainScreen);
}

static struct MainScreen *AccessMainScreen(void)
{
	static struct MainScreen *Obj = NULL;
	if (NULL == Obj) {
		Obj = (struct MainScreen *)malloc(sizeof(struct MainScreen));
		memset(Obj, 0, sizeof(struct MainScreen));
	}
	return Obj;
}

static void rec_screen_main_createOtherScreens()
{
	const struct RecoveryParameters *RecoveryParameters =
		util_config_getRecoveryParameters();

	rec_screen_file_createFileBrowserScreen(&RecoveryParameters->Config);

	rec_screen_progress_createScreen(&RecoveryParameters->Config);

	rec_screen_network_createNetworkScreen();
}

static void
rec_screen_main_enableNavigationViaKeys(struct MainScreen *MainScreen)
{
	lv_group_t *Group = lv_group_get_default();
	lv_group_remove_all_objs(Group);

	lv_group_add_obj(Group, MainScreen->Actions.Container);
	lv_gridnav_add(MainScreen->Actions.Container, LV_GRIDNAV_CTRL_ROLLOVER);

	lv_obj_clear_state(MainScreen->Actions.RestartButton,
			   LV_STATE_FOCUS_KEY);
	if (0 != MainScreen->Data.InterfaceCount) {
		lv_obj_clear_state(MainScreen->Actions.NetworkSetupButton,
				   LV_STATE_FOCUS_KEY);
	}
	lv_obj_add_state(MainScreen->Actions.InstallButton, LV_STATE_FOCUS_KEY);
}

static void rec_screen_main_populateNetworkInfoEntryRow(
	lv_obj_t *Text, const struct Ipv4Info *InterfaceInfo)
{
	// create and configure new row in the list of network interfaces
	lv_obj_t *Row = lv_obj_create(Text);

	rec_styles_applyNetworkListEntriesStyle(Row);

	lv_obj_set_size(Row, LV_PCT(100), LV_PCT(100));
	lv_obj_set_flex_flow(Row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(Row, LV_FLEX_ALIGN_SPACE_EVENLY,
			      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	rec_screen_disableScrolling(Row);

	// add network interface name into the row
	lv_obj_t *Interface = lv_label_create(Row);
	lv_label_set_text(Interface, InterfaceInfo->Name);
	lv_label_set_long_mode(Interface, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_obj_set_width(Interface, LV_PCT(25));

	// add ip address into the row
	lv_obj_t *IPAddress = lv_label_create(Row);
	lv_label_set_text(IPAddress, InterfaceInfo->Address);
	lv_label_set_long_mode(Interface, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_obj_set_width(IPAddress, LV_PCT(25));

	// add netmask into the row
	lv_obj_t *Netmask = lv_label_create(Row);
	lv_label_set_text(Netmask, InterfaceInfo->Netmask);
	lv_label_set_long_mode(Interface, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_obj_set_width(Netmask, LV_PCT(25));
}

static void rec_screen_main_createNetworkInfoEntryRow(
	lv_obj_t *list, const struct Ipv4Info *InterfaceInfo,
	const lv_coord_t width, const lv_coord_t height)
{
	lv_obj_t *Entry = lv_list_add_text(list, "");

	rec_styles_applyNetworkListEntriesStyle(Entry);

	lv_obj_set_size(Entry, width, height);

	rec_screen_main_populateNetworkInfoEntryRow(Entry, InterfaceInfo);
}

static void rec_screen_main_createNetworkInfoList(struct MainScreen *MainScreen)
{
	if (NULL != MainScreen->NetworkInfo.InterfaceList) {
		lv_obj_del(MainScreen->NetworkInfo.InterfaceList);
	}

	MainScreen->NetworkInfo.InterfaceList =
		lv_list_create(MainScreen->NetworkInfo.Container);

	rec_styles_applyNetworkListStyle(MainScreen->NetworkInfo.InterfaceList);

	lv_obj_set_size(MainScreen->NetworkInfo.InterfaceList, LV_PCT(100),
			LV_PCT(100));

	// since the list is vertically oriented, the width of each element stretches over whole width
	// and there are N elements to equally distrubute vertically
	// where N is number of network interfaces plus one for the default gateway
	const lv_coord_t listElementWidth = LV_PCT(100);
	const lv_coord_t listElementHeight =
		LV_PCT(100 / (MainScreen->Data.InterfaceCount + 1));

	// populate list with interface infos
	struct Ipv4Info *InterfaceInfo = MainScreen->Data.Interfaces;
	while (NULL != InterfaceInfo) {
		rec_screen_main_createNetworkInfoEntryRow(
			MainScreen->NetworkInfo.InterfaceList, InterfaceInfo,
			listElementWidth, listElementHeight);

		InterfaceInfo = InterfaceInfo->Next;
	}

	// add default gateway to the list
	const struct Ipv4Info GatewayInfo =
		util_networking_getDefaultGatewayInfo();

	rec_screen_main_createNetworkInfoEntryRow(
		MainScreen->NetworkInfo.InterfaceList, &GatewayInfo,
		listElementWidth, listElementHeight);
}

static void rec_screen_main_refreshInterfaces(struct MainScreen *MainScreen)
{
	const struct RecoveryParameters *RecoveryParameters =
		util_config_getRecoveryParameters();

	util_networking_createInterfaceList(
		RecoveryParameters->Config.Interfaces,
		&MainScreen->Data.InterfaceCount, &MainScreen->Data.Interfaces);

	if (0 != MainScreen->Data.InterfaceCount) {
		rec_screen_main_createNetworkInfoList(MainScreen);

		lv_obj_clear_flag(MainScreen->Actions.NetworkSetupButton,
				  LV_OBJ_FLAG_HIDDEN);
	}
}

static void
rec_screen_main_createNetworkInfoPanel(struct MainScreen *MainScreen)
{
	MainScreen->NetworkInfo.Container =
		lv_obj_create(MainScreen->Screen.Obj);

	rec_styles_applyContainerStyleNetworkPanel(
		MainScreen->NetworkInfo.Container);

	rec_screen_disableScrolling(MainScreen->NetworkInfo.Container);
}

static void rec_screen_main_setLayout(struct MainScreen *MainScreen)
{
	// Set screen layout
	lv_obj_set_flex_flow(MainScreen->Screen.Obj, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(MainScreen->Screen.Obj, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND);

	// Set grow-factor for header
	lv_obj_set_flex_grow(MainScreen->Screen.Header, 0);

	// set the layout of the panel with buttons
	lv_obj_set_width(MainScreen->Actions.Container, LV_PCT(40));
	lv_obj_set_flex_grow(MainScreen->Actions.Container, 1);
	lv_obj_set_flex_flow(MainScreen->Actions.Container,
			     LV_FLEX_FLOW_COLUMN);

	// set the size and grow-factor of the area with network information
	lv_obj_set_size(MainScreen->NetworkInfo.Container, LV_PCT(100),
			LV_PCT(20));
	lv_obj_set_flex_grow(MainScreen->NetworkInfo.Container, 0);

	// Set grow-factor for notification
	lv_obj_set_flex_grow(MainScreen->Screen.Notification, 0);
}

/**
 * Create recovery main screen UI
 *
 */
void rec_screen_main_createScreen(void)
{
	struct MainScreen *MainScreen = AccessMainScreen();
	const struct RecoveryParameters *RecoveryParameters =
		util_config_getRecoveryParameters();

	rec_screen_createHeader(&MainScreen->Screen,
				&RecoveryParameters->Config);
	rec_screen_main_createActions(MainScreen);
	rec_screen_main_createNetworkInfoPanel(MainScreen);
	rec_screen_createNotification(&MainScreen->Screen);

	rec_screen_main_setLayout(MainScreen);

	rec_screen_main_createOtherScreens();
}

/**
 * Show main screen
 *
 */
void rec_screen_main_showScreen(void)
{
	struct MainScreen *MainScreen = AccessMainScreen();

	rec_screen_main_refreshInterfaces(MainScreen);

	rec_screen_main_enableNavigationViaKeys(MainScreen);

	rec_screen_clearNotification(&MainScreen->Screen);

	rec_screen_loadScreen(&MainScreen->Screen);
}
