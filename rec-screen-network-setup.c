/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains functions to edit network settings
 *
 */

#include "rec-screen-network-setup.h"

#include "rec-screen-main.h"
#include "rec-screen-common.h"

#include "rec-styles.h"

#include "rec-util-networking.h"

#include <lvgl/lvgl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* This structure is for the area that displays list of network interfaces and their configuration.
   This is a Nx4 table, where N is number of network interfaces. Each row shows one network
   interface name, ip address, netmask and the information whether it is configured as a DHCP
   client or not. */
struct InterfaceListPanel {
	/* this widget is used as the parent for the whole area holding list of network interfaces and
       their configuration . */
	lv_obj_t *Container;

	/* this widget holds an array of elements, where each element is a row in the table, where
       elements are network interface name, it's ip address and it's netmask. */
	lv_obj_t *Table;

	/* this widget acts as a row in the table, where
       elements are network interface name, it's ip address and it's netmask.
       Each of these widget got 3 fields: interface name, ip address and netmask. */
	lv_obj_t *TableRows[IFACE_COUNT_MAX];

	/* this widget is a checkbox, which holds an information whether a network interface is
       selected or not
       */
	lv_obj_t *RowSelectionBox[IFACE_COUNT_MAX];

	/* number of elements in above arrays which are actually valid */
	unsigned int InterfaceCount;

	/* network interfaces and their configuration */
	struct Ipv4Info *IpInfo;
	/* default gateway configuration */
	struct Ipv4Info DefaultGatewayInfo;

	/* the selected item, or NULL if no item is selected */
	const struct Ipv4Info *SelectedItemIpInfo;
};

/*
 * This structure holds label and text edit area, and a parent widget to group them.
 * It is used in the edit area to display and edit ip address and netmask
 */
struct NetworkAddrPanel {
	/* this widget is used as a parent for the label and text edit area */
	lv_obj_t *Container;

	/* label containing description of the item */
	lv_obj_t *Label;

	/* edit area for the item */
	lv_obj_t *TextArea;
};

/*
 * This structure contains widgets needed for the edit area.
 */
struct EditAreaPanel {
	/* the parent for all widgets in the edit area */
	lv_obj_t *Container;

	/* label to show the name of the selected network interface at the top of the edit area */
	lv_obj_t *SelectedInterfaceLabel;

	/* this widget is used as the layout holding ip address and netmask edit areas */
	lv_obj_t *AddressRow;
	/* edit areas for the ip address and netmask */
	struct NetworkAddrPanel IpAddress;
	struct NetworkAddrPanel Netmask;

	/* this widget is the virtual keyboard */
	lv_obj_t *VirtualKeyboard;
};

/* The action panel contains all buttons on the network setup page. */
struct ActionsPanel {
	/* parent widget for all buttons. It also sets the horizontal layout for the buttons. */
	lv_obj_t *Container;

	/* The network setup page has:
           1. 2 buttons on the interface list page: OK and EDIT
           2. 3 buttons on the edit ip address page: SET STATIC, SET DHCP, and CANCEL
           3. 3 buttons on the edit gateway page: SET, DELETE, and CANCEL
       These buttons got different text and functionality. */
	lv_obj_t *Button[5];
	/* Each Button is parent to corresponding Label */
	lv_obj_t *Label[5];
};

/*
 * Object containing Screen, a contents-panel and action-buttons.
 * Contents panel contains interface-list and edit-area,
 * with either one of the two visible at once.
 *
 */
struct NetworkScreen {
	/* The screen object creates a page and acts as the parent for all other widgets on the page.
       It also contains the title and notification areas. */
	struct Screen Screen;

	/* top parent for all widgets on this page */
	lv_obj_t *Container;

	/* panels showing list of network interfaces */
	struct InterfaceListPanel InterfaceListPanel;
	/* panel to edit settings for the selected network interface or default gateway */
	struct EditAreaPanel EditAreaPanel;

	/* action area holding buttons */
	struct ActionsPanel ActionsPanel;
};

/*
 * Some function declarations are needed, because they are used in callbacks
 */
static void
rec_screen_network_InterfaceList_showPage(struct NetworkScreen *NetworkSetting);
static void
rec_screen_network_EditArea_showPage(struct NetworkScreen *NetworkSetting);
static void rec_screen_network_InterfaceList_repopulateTable(
	struct NetworkScreen *NetworkSetting);

static struct NetworkScreen *AccessNetworkScreen(void)
{
	static struct NetworkScreen *Obj = NULL;

	if (NULL == Obj) {
		Obj = (struct NetworkScreen *)malloc(
			sizeof(struct NetworkScreen));
		memset(Obj, 0, sizeof(struct NetworkScreen));

		Obj->InterfaceListPanel.SelectedItemIpInfo = NULL;
	}

	return Obj;
}

static void
rec_screen_network_ActionsPanel_resetButtons(struct ActionsPanel *ActionsPanel)
{
	const unsigned int n =
		sizeof(ActionsPanel->Button) / sizeof(ActionsPanel->Button[0]);
	lv_obj_t **it = ActionsPanel->Button;
	lv_obj_t **const end = it + n;

	while (end != it) {
		lv_obj_remove_event_cb(*it, NULL);
		lv_obj_add_flag(*it, LV_OBJ_FLAG_HIDDEN);

		++it;
	}
}

/* sets keyboard navigation for the ListInterfaces page */
static void rec_screen_network_InterfaceList_setListNavigation(
	struct NetworkScreen *NetworkSetting)
{
	lv_group_t *Group = lv_group_get_default();
	lv_group_remove_all_objs(Group);

	lv_group_add_obj(Group, NetworkSetting->InterfaceListPanel.Table);
	lv_gridnav_add(NetworkSetting->InterfaceListPanel.Table,
		       LV_GRIDNAV_CTRL_SCROLL_FIRST);

	lv_group_add_obj(Group, NetworkSetting->ActionsPanel.Container);
	lv_gridnav_add(NetworkSetting->ActionsPanel.Container,
		       LV_GRIDNAV_CTRL_NONE);
}

/* sets keyboard navigation for the EditArea page when editing an IP address */
static void rec_screen_network_EditArea_IpAddress_setNavigation(
	struct NetworkScreen *NetworkSetting)
{
	lv_group_t *Group = lv_group_get_default();

	lv_group_remove_all_objs(Group);

	lv_group_add_obj(Group,
			 NetworkSetting->EditAreaPanel.IpAddress.Container);
	lv_gridnav_add(NetworkSetting->EditAreaPanel.IpAddress.Container,
		       LV_GRIDNAV_CTRL_NONE);

	lv_group_add_obj(Group,
			 NetworkSetting->EditAreaPanel.Netmask.Container);
	lv_gridnav_add(NetworkSetting->EditAreaPanel.Netmask.Container,
		       LV_GRIDNAV_CTRL_NONE);

	lv_group_add_obj(Group, NetworkSetting->EditAreaPanel.VirtualKeyboard);
	lv_gridnav_add(NetworkSetting->EditAreaPanel.VirtualKeyboard,
		       LV_GRIDNAV_CTRL_NONE);

	lv_group_add_obj(Group, NetworkSetting->ActionsPanel.Container);
	lv_gridnav_add(NetworkSetting->ActionsPanel.Container,
		       LV_GRIDNAV_CTRL_NONE);
}

/* sets keyboard navigation for the EditArea page when editing the default gateway */
static void rec_screen_network_EditArea_Gateway_setNavigation(
	struct NetworkScreen *NetworkSetting)
{
	lv_group_t *Group = lv_group_get_default();

	lv_group_remove_all_objs(Group);

	lv_group_add_obj(Group,
			 NetworkSetting->EditAreaPanel.IpAddress.Container);
	lv_gridnav_add(NetworkSetting->EditAreaPanel.IpAddress.Container,
		       LV_GRIDNAV_CTRL_NONE);

	lv_group_add_obj(Group, NetworkSetting->EditAreaPanel.VirtualKeyboard);
	lv_gridnav_add(NetworkSetting->EditAreaPanel.VirtualKeyboard,
		       LV_GRIDNAV_CTRL_NONE);

	lv_group_add_obj(Group, NetworkSetting->ActionsPanel.Container);
	lv_gridnav_add(NetworkSetting->ActionsPanel.Container,
		       LV_GRIDNAV_CTRL_NONE);
}

static void rec_screen_network_EditArea_OnClickCancelButton(lv_event_t *E)
{
	struct NetworkScreen *NetworkSetting = lv_event_get_user_data(E);

	rec_screen_network_InterfaceList_showPage(NetworkSetting);
}

static void rec_screen_network_EditArea_applyStaticIpAddress(
	struct NetworkScreen *NetworkSetting, const char *selectedInterfaceName,
	const char *ipAddress, const char *netmask)
{
	const bool Status = util_networking_setStaticConfiguration(
		selectedInterfaceName, ipAddress, netmask);

	if (!Status) {
		char msg[MSG_LENGTH_MAX];
		snprintf(msg, sizeof(msg) - 1,
			 "Failed to apply static settings for '%s'.",
			 selectedInterfaceName);
		rec_screen_showNotification(&NetworkSetting->Screen, msg,
					    NOTIFY_ERROR);
	} else {
		char msg[MSG_LENGTH_MAX];
		snprintf(msg, sizeof(msg) - 1,
			 "Successfully applied static setting for '%s'.",
			 selectedInterfaceName);
		rec_screen_showNotification(&NetworkSetting->Screen, msg,
					    NOTIFY_SUCCESS);
	}

	rec_screen_network_InterfaceList_repopulateTable(NetworkSetting);
}

static void
rec_screen_network_EditArea_IpAddress_onClickSetStatic(lv_event_t *E)
{
	struct NetworkScreen *NetworkSetting = lv_event_get_user_data(E);

	const char *selectedInterfaceName =
		NetworkSetting->InterfaceListPanel.SelectedItemIpInfo->Name;
	const char *ipAddress = lv_textarea_get_text(
		NetworkSetting->EditAreaPanel.IpAddress.TextArea);
	const char *netmask = lv_textarea_get_text(
		NetworkSetting->EditAreaPanel.Netmask.TextArea);

	if (0 == strlen(ipAddress)) {
		rec_screen_showNotification(
			&NetworkSetting->Screen,
			"The ip address can not be an empty.", NOTIFY_ERROR);
	} else if (0 == strlen(netmask)) {
		rec_screen_showNotification(&NetworkSetting->Screen,
					    "The netmask can not be an empty.",
					    NOTIFY_ERROR);
	} else {
		rec_screen_network_EditArea_applyStaticIpAddress(
			NetworkSetting, selectedInterfaceName, ipAddress,
			netmask);

		rec_screen_network_InterfaceList_showPage(NetworkSetting);
	}
}

static void
rec_screen_network_EditArea_applyDhcp(struct NetworkScreen *NetworkSetting)
{
	const char *selectedInterfaceName =
		NetworkSetting->InterfaceListPanel.SelectedItemIpInfo->Name;

	const bool Status =
		util_networking_reconfigureAsDhcpClient(selectedInterfaceName);

	if (!Status) {
		char msg[MSG_LENGTH_MAX];
		snprintf(msg, sizeof(msg) - 1,
			 "Failed to reconfigure '%s' as a dhcp client.",
			 selectedInterfaceName);
		rec_screen_showNotification(&NetworkSetting->Screen, msg,
					    NOTIFY_ERROR);
	} else {
		char msg[MSG_LENGTH_MAX];
		snprintf(msg, sizeof(msg) - 1,
			 "Successfully reconfigured '%s' as a dhcp client.",
			 selectedInterfaceName);
		rec_screen_showNotification(&NetworkSetting->Screen, msg,
					    NOTIFY_SUCCESS);
	}

	rec_screen_network_InterfaceList_repopulateTable(NetworkSetting);
}

static void rec_screen_network_EditArea_IpAddress_onClickSetDhcp(lv_event_t *E)
{
	struct NetworkScreen *NetworkSetting = lv_event_get_user_data(E);

	rec_screen_network_EditArea_applyDhcp(NetworkSetting);

	rec_screen_network_InterfaceList_showPage(NetworkSetting);
}

static void rec_screen_network_EditArea_applyGatewayAddress(
	struct NetworkScreen *NetworkSetting, const char *GatewayAddress)
{
	const bool res = util_networking_setGatewayAddress(GatewayAddress);

	if (!res) {
		rec_screen_showNotification(
			&NetworkSetting->Screen,
			"Failed to set the gateway address.", NOTIFY_ERROR);
	} else {
		rec_screen_showNotification(&NetworkSetting->Screen,
					    "Gateway set successfully.",
					    NOTIFY_SUCCESS);

		rec_screen_network_InterfaceList_repopulateTable(
			NetworkSetting);
	}
}

static void rec_screen_network_EditArea_Gateway_onClickSet(lv_event_t *E)
{
	struct NetworkScreen *NetworkSetting = lv_event_get_user_data(E);

	const char *GatewayAddress = lv_textarea_get_text(
		NetworkSetting->EditAreaPanel.IpAddress.TextArea);

	if (0 == strlen(GatewayAddress)) {
		rec_screen_showNotification(
			&NetworkSetting->Screen,
			"The gateway address can not be an empty string.",
			NOTIFY_ERROR);
	} else {
		rec_screen_network_EditArea_applyGatewayAddress(NetworkSetting,
								GatewayAddress);

		rec_screen_network_InterfaceList_showPage(NetworkSetting);
	}
}

static void rec_screen_network_EditArea_Gateway_onClickDelete(lv_event_t *E)
{
	struct NetworkScreen *NetworkSetting = lv_event_get_user_data(E);

	const bool res = util_networking_deleteGateway();
	if (!res) {
		rec_screen_showNotification(&NetworkSetting->Screen,
					    "Failed to delete gateway!",
					    NOTIFY_ERROR);
	} else {
		rec_screen_showNotification(&NetworkSetting->Screen,
					    "Gateway IP address deleted!",
					    NOTIFY_SUCCESS);

		rec_screen_network_InterfaceList_repopulateTable(
			NetworkSetting);
	}

	rec_screen_network_InterfaceList_showPage(NetworkSetting);
}

static void rec_screen_network_EditArea_configureButtons_editIpAddress(
	struct NetworkScreen *NetworkSetting)
{
	rec_screen_network_ActionsPanel_resetButtons(
		&NetworkSetting->ActionsPanel);

	// configure button 1
	lv_obj_clear_flag(NetworkSetting->ActionsPanel.Button[2],
			  LV_OBJ_FLAG_HIDDEN);
	lv_label_set_text_static(NetworkSetting->ActionsPanel.Label[2],
				 "SET STATIC");
	lv_obj_add_event_cb(
		NetworkSetting->ActionsPanel.Button[2],
		rec_screen_network_EditArea_IpAddress_onClickSetStatic,
		LV_EVENT_CLICKED, NetworkSetting);

	// configure button 2
	lv_obj_clear_flag(NetworkSetting->ActionsPanel.Button[3],
			  LV_OBJ_FLAG_HIDDEN);
	lv_label_set_text_static(NetworkSetting->ActionsPanel.Label[3],
				 "SET DHCP");
	lv_obj_add_event_cb(
		NetworkSetting->ActionsPanel.Button[3],
		rec_screen_network_EditArea_IpAddress_onClickSetDhcp,
		LV_EVENT_CLICKED, NetworkSetting);

	// configure button 3
	lv_obj_clear_flag(NetworkSetting->ActionsPanel.Button[4],
			  LV_OBJ_FLAG_HIDDEN);
	lv_label_set_text_static(NetworkSetting->ActionsPanel.Label[4],
				 "CANCEL");
	lv_obj_add_event_cb(NetworkSetting->ActionsPanel.Button[4],
			    rec_screen_network_EditArea_OnClickCancelButton,
			    LV_EVENT_CLICKED, NetworkSetting);
}

static void rec_screen_network_EditArea_configureButtons_editGateway(
	struct NetworkScreen *NetworkSetting)
{
	rec_screen_network_ActionsPanel_resetButtons(
		&NetworkSetting->ActionsPanel);

	// configure button 1
	lv_obj_clear_flag(NetworkSetting->ActionsPanel.Button[2],
			  LV_OBJ_FLAG_HIDDEN);
	lv_label_set_text_static(NetworkSetting->ActionsPanel.Label[2], "SET");
	lv_obj_add_event_cb(NetworkSetting->ActionsPanel.Button[2],
			    rec_screen_network_EditArea_Gateway_onClickSet,
			    LV_EVENT_CLICKED, NetworkSetting);

	// configure button 2
	lv_obj_clear_flag(NetworkSetting->ActionsPanel.Button[3],
			  LV_OBJ_FLAG_HIDDEN);
	lv_label_set_text_static(NetworkSetting->ActionsPanel.Label[3],
				 "DELETE");
	lv_obj_add_event_cb(NetworkSetting->ActionsPanel.Button[3],
			    rec_screen_network_EditArea_Gateway_onClickDelete,
			    LV_EVENT_CLICKED, NetworkSetting);

	// configure button 3
	lv_obj_clear_flag(NetworkSetting->ActionsPanel.Button[4],
			  LV_OBJ_FLAG_HIDDEN);
	lv_label_set_text_static(NetworkSetting->ActionsPanel.Label[4],
				 "CANCEL");
	lv_obj_add_event_cb(NetworkSetting->ActionsPanel.Button[4],
			    rec_screen_network_EditArea_OnClickCancelButton,
			    LV_EVENT_CLICKED, NetworkSetting);
}

static void rec_screen_network_InterfaceList_OnClickOkButton(lv_event_t *E)
{
	// the event is not used
	(void)E;

	rec_screen_main_showScreen();
}

static void rec_screen_network_InterfaceList_OnClickEditButton(lv_event_t *E)
{
	struct NetworkScreen *NetworkSetting = lv_event_get_user_data(E);

	rec_screen_network_EditArea_showPage(NetworkSetting);
}

static void rec_screen_network_InterfaceList_configureButtons(
	struct NetworkScreen *NetworkSetting)
{
	rec_screen_network_ActionsPanel_resetButtons(
		&NetworkSetting->ActionsPanel);

	// configure button 1
	lv_obj_clear_flag(NetworkSetting->ActionsPanel.Button[0],
			  LV_OBJ_FLAG_HIDDEN);
	lv_label_set_text_static(NetworkSetting->ActionsPanel.Label[0], "OK");
	lv_obj_add_event_cb(NetworkSetting->ActionsPanel.Button[0],
			    rec_screen_network_InterfaceList_OnClickOkButton,
			    LV_EVENT_CLICKED, NetworkSetting);

	// configure button 2
	lv_obj_clear_flag(NetworkSetting->ActionsPanel.Button[1],
			  LV_OBJ_FLAG_HIDDEN);
	lv_label_set_text_static(NetworkSetting->ActionsPanel.Label[1], "EDIT");
	lv_obj_add_event_cb(NetworkSetting->ActionsPanel.Button[1],
			    rec_screen_network_InterfaceList_OnClickEditButton,
			    LV_EVENT_CLICKED, NetworkSetting);
}

static void rec_screen_network_InterfaceList_clearSelectedInterface(
	struct NetworkScreen *NetworkSetting)
{
	NetworkSetting->InterfaceListPanel.SelectedItemIpInfo = NULL;
}

static void rec_screen_network_InterfaceList_clearAllCheckboxes(
	struct NetworkScreen *NetworkSetting)
{
	/* number of elements in the table is number of interfaces plus one gateway */
	lv_obj_t **It = NetworkSetting->InterfaceListPanel.RowSelectionBox;
	lv_obj_t **End =
		It + NetworkSetting->InterfaceListPanel.InterfaceCount + 1;

	while (It != End) {
		lv_obj_clear_state(*It, LV_STATE_CHECKED);
		++It;
	}
}

static void rec_screen_network_InterfaceList_handleTableRowSelection(
	lv_obj_t *Checkbox, const struct Ipv4Info *IpInfo)
{
	struct NetworkScreen *NetworkSetting = AccessNetworkScreen();

	const bool checkboxSelected =
		(0 != (LV_STATE_CHECKED & lv_obj_get_state(Checkbox)));

	if (checkboxSelected) {
		rec_screen_network_InterfaceList_clearSelectedInterface(
			NetworkSetting);

		lv_obj_clear_state(Checkbox, LV_STATE_CHECKED);
	} else {
		rec_screen_network_InterfaceList_clearAllCheckboxes(
			NetworkSetting);

		lv_obj_add_state(Checkbox, LV_STATE_CHECKED);

		// select the interface
		NetworkSetting->InterfaceListPanel.SelectedItemIpInfo = IpInfo;
	}
}

static void rec_screen_network_InterfaceList_onClickTableRow(lv_event_t *E)
{
	const struct Ipv4Info *IpInfo = (struct Ipv4Info *)E->user_data;

	lv_obj_t *Button = E->current_target;

	lv_obj_t *Checkbox = lv_obj_get_child(Button, 1);

	if (NULL != Checkbox) {
		rec_screen_network_InterfaceList_handleTableRowSelection(
			Checkbox, IpInfo);
	}
}

/* This function updates list of interfaces and their configuration.
   The "InterfaceListPanel.IpInfo" points to the array holding this information */
static void rec_screen_network_InterfaceList_updateListOfNetworkInterfaces(
	struct NetworkScreen *NetworkSetting)
{
	rec_screen_network_InterfaceList_clearSelectedInterface(NetworkSetting);

	const struct RecoveryParameters *RecoveryParameters =
		util_config_getRecoveryParameters();

	util_networking_createInterfaceList(
		RecoveryParameters->Config.Interfaces,
		&NetworkSetting->InterfaceListPanel.InterfaceCount,
		&NetworkSetting->InterfaceListPanel.IpInfo);

	NetworkSetting->InterfaceListPanel.DefaultGatewayInfo =
		util_networking_getDefaultGatewayInfo();
}

static void rec_screen_network_InterfaceList_createTableRowEntry_Checkbox(
	lv_obj_t **CheckboxPtr, lv_obj_t *Button, struct Ipv4Info *IpInfo)
{
	*CheckboxPtr = lv_checkbox_create(Button);

	lv_obj_set_width(*CheckboxPtr, LV_PCT(25));
	lv_checkbox_set_text(*CheckboxPtr, IpInfo->Name);

	/* Forwarding all events to the parent because the checkbox got it's own logic
     * related to when it is selected
     */
	lv_obj_add_flag(*CheckboxPtr, LV_OBJ_FLAG_EVENT_BUBBLE);
	lv_obj_clear_flag(*CheckboxPtr, LV_OBJ_FLAG_CLICKABLE);
}

static void rec_screen_network_InterfaceList_createTableRowEntry_IpAddress(
	lv_obj_t *Button, const struct Ipv4Info *IpInfo)
{
	lv_obj_t *IpAddrLabel = lv_label_create(Button);

	lv_obj_set_width(IpAddrLabel, LV_PCT(25));
	lv_obj_set_style_text_align(IpAddrLabel, LV_ALIGN_LEFT_MID,
				    LV_PART_MAIN);
	lv_label_set_text(IpAddrLabel, IpInfo->Address);

	lv_obj_add_flag(IpAddrLabel,
			LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_EVENT_BUBBLE);
}

static void rec_screen_network_InterfaceList_createTableRowEntry_Netmask(
	lv_obj_t *Button, const struct Ipv4Info *IpInfo)
{
	lv_obj_t *NetmaskLabel = lv_label_create(Button);

	lv_obj_set_width(NetmaskLabel, LV_PCT(25));
	lv_obj_set_style_text_align(NetmaskLabel, LV_ALIGN_LEFT_MID,
				    LV_PART_MAIN);
	lv_label_set_text(NetmaskLabel, IpInfo->Netmask);

	lv_obj_add_flag(NetmaskLabel,
			LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_EVENT_BUBBLE);
}

static void rec_screen_network_InterfaceList_createTableRowEntry_Dhcp(
	lv_obj_t *Button, const struct Ipv4Info *IpInfo)
{
	lv_obj_t *DhcpLabel = lv_label_create(Button);

	lv_obj_set_width(DhcpLabel, LV_PCT(25));
	lv_obj_set_style_text_align(DhcpLabel, LV_ALIGN_LEFT_MID, LV_PART_MAIN);

	const char *dhcpFieldText = util_networking_getDhcpText(IpInfo);
	lv_label_set_text(DhcpLabel, dhcpFieldText);

	lv_obj_add_flag(DhcpLabel,
			LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_EVENT_BUBBLE);
}

static void rec_screen_network_InterfaceList_createTableRow(
	struct NetworkScreen *NetworkSetting, struct Ipv4Info *IpInfo,
	lv_obj_t **tableRow, lv_obj_t **rowSelectionCheckbox)
{
	lv_obj_t *Button = lv_list_add_btn(
		NetworkSetting->InterfaceListPanel.Table, NULL, "");

	*tableRow = Button;

	lv_obj_set_width(Button, LV_PCT(100));
	lv_obj_set_flex_flow(Button, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(Button, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_SPACE_EVENLY);
	lv_obj_add_event_cb(Button,
			    rec_screen_network_InterfaceList_onClickTableRow,
			    LV_EVENT_CLICKED, IpInfo);

	rec_styles_applyNetworkEntryStyle(Button);

	rec_screen_network_InterfaceList_createTableRowEntry_Checkbox(
		rowSelectionCheckbox, Button, IpInfo);
	rec_screen_network_InterfaceList_createTableRowEntry_IpAddress(Button,
								       IpInfo);
	rec_screen_network_InterfaceList_createTableRowEntry_Netmask(Button,
								     IpInfo);
	rec_screen_network_InterfaceList_createTableRowEntry_Dhcp(Button,
								  IpInfo);
}

static void rec_screen_network_InterfaceList_createTableEntries(
	struct NetworkScreen *NetworkSetting)
{
	lv_obj_t **tableRowIt = NetworkSetting->InterfaceListPanel.TableRows;
	lv_obj_t **rowSelectionBoxIt =
		NetworkSetting->InterfaceListPanel.RowSelectionBox;

	// add entries for all network interfaces
	struct Ipv4Info *InterfaceInfoIt =
		NetworkSetting->InterfaceListPanel.IpInfo;
	while (NULL != InterfaceInfoIt) {
		rec_screen_network_InterfaceList_createTableRow(
			NetworkSetting, InterfaceInfoIt, tableRowIt,
			rowSelectionBoxIt);

		InterfaceInfoIt = InterfaceInfoIt->Next;
		++tableRowIt;
		++rowSelectionBoxIt;
	}

	// add entry for the default gateway
	rec_screen_network_InterfaceList_createTableRow(
		NetworkSetting,
		&NetworkSetting->InterfaceListPanel.DefaultGatewayInfo,
		tableRowIt, rowSelectionBoxIt);
}

static void
rec_screen_network_InterfaceList_createTableInterfaceNameColumnHeader(
	lv_obj_t *Header)
{
	lv_obj_t *NetIfHeader = lv_label_create(Header);

	lv_obj_set_width(NetIfHeader, LV_PCT(25));
	lv_label_set_text(NetIfHeader, "NETIF");
	lv_obj_set_style_text_align(NetIfHeader, LV_ALIGN_LEFT_MID,
				    LV_PART_MAIN);
}

static void
rec_screen_network_InterfaceList_createTableIpAddrColumnHeader(lv_obj_t *Header)
{
	lv_obj_t *IpAddressHeader = lv_label_create(Header);

	lv_obj_set_width(IpAddressHeader, LV_PCT(25));
	lv_label_set_text(IpAddressHeader, "IP ADDRESS");
	lv_obj_set_style_text_align(IpAddressHeader, LV_ALIGN_LEFT_MID,
				    LV_PART_MAIN);
}

static void rec_screen_network_InterfaceList_createTableNetmaskColumnHeader(
	lv_obj_t *Header)
{
	lv_obj_t *NetmaskHeader = lv_label_create(Header);

	lv_obj_set_width(NetmaskHeader, LV_PCT(25));
	lv_label_set_text(NetmaskHeader, "NETMASK");
	lv_obj_set_style_text_align(NetmaskHeader, LV_ALIGN_LEFT_MID,
				    LV_PART_MAIN);
}

static void
rec_screen_network_InterfaceList_createTableDhcpColumnHeader(lv_obj_t *Header)
{
	lv_obj_t *DhcpHeader = lv_label_create(Header);

	lv_obj_set_width(DhcpHeader, LV_PCT(25));
	lv_label_set_text(DhcpHeader, "DHCP");
	lv_obj_set_style_text_align(DhcpHeader, LV_ALIGN_LEFT_MID,
				    LV_PART_MAIN);
}

/* creates the header of the interface list table */
static void rec_screen_network_InterfaceList_createTableHeader(
	struct NetworkScreen *NetworkSetting)
{
	// this widgets represents a horizontal layout for the header row
	lv_obj_t *Header = lv_list_add_btn(
		NetworkSetting->InterfaceListPanel.Table, NULL, "");

	lv_obj_set_width(Header, LV_PCT(100));
	lv_obj_clear_flag(Header, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(Header, LV_OBJ_FLAG_CLICK_FOCUSABLE);
	lv_obj_add_state(Header, LV_STATE_DISABLED);
	lv_obj_set_style_bg_color(Header, lv_palette_main(LV_PALETTE_GREY),
				  LV_PART_MAIN);

	lv_obj_set_flex_flow(Header, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(Header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_SPACE_EVENLY);

	rec_screen_network_InterfaceList_createTableInterfaceNameColumnHeader(
		Header);
	rec_screen_network_InterfaceList_createTableIpAddrColumnHeader(Header);
	rec_screen_network_InterfaceList_createTableNetmaskColumnHeader(Header);
	rec_screen_network_InterfaceList_createTableDhcpColumnHeader(Header);
}

static void rec_screen_network_InterfaceList_createTable(
	struct NetworkScreen *NetworkSetting)
{
	NetworkSetting->InterfaceListPanel.Table =
		lv_list_create(NetworkSetting->InterfaceListPanel.Container);

	rec_styles_applyNetworkScreenListStyle(
		NetworkSetting->InterfaceListPanel.Table);
	rec_styles_applyScrollbarStyle(
		NetworkSetting->InterfaceListPanel.Table);

	lv_obj_set_width(NetworkSetting->InterfaceListPanel.Table, LV_PCT(100));
	lv_obj_set_flex_grow(NetworkSetting->InterfaceListPanel.Table, 5);

	rec_screen_network_InterfaceList_createTableHeader(NetworkSetting);
}

static void rec_screen_network_InterfaceList_createPanel(
	struct NetworkScreen *NetworkSetting)
{
	NetworkSetting->InterfaceListPanel.Container =
		lv_obj_create(NetworkSetting->Container);

	rec_styles_applyContainerStyleBorderless(
		NetworkSetting->InterfaceListPanel.Container);
	lv_obj_set_size(NetworkSetting->InterfaceListPanel.Container,
			LV_PCT(100), LV_PCT(100));
	rec_screen_disableScrolling(
		NetworkSetting->InterfaceListPanel.Container);
	lv_obj_set_flex_flow(NetworkSetting->InterfaceListPanel.Container,
			     LV_FLEX_FLOW_COLUMN);

	rec_screen_network_InterfaceList_createTable(NetworkSetting);
}

/* this function will update list of network interfaces, and then create the table */
static void rec_screen_network_InterfaceList_repopulateTable(
	struct NetworkScreen *NetworkSetting)
{
	rec_screen_network_InterfaceList_updateListOfNetworkInterfaces(
		NetworkSetting);

	lv_obj_del(NetworkSetting->InterfaceListPanel.Table);

	rec_screen_network_InterfaceList_createTable(NetworkSetting);
	rec_screen_network_InterfaceList_createTableEntries(NetworkSetting);
}

static void rec_screen_network_EditArea_createSelectedInterfaceLabel(
	struct NetworkScreen *NetworkSetting)
{
	NetworkSetting->EditAreaPanel.SelectedInterfaceLabel =
		lv_label_create(NetworkSetting->EditAreaPanel.Container);

	lv_obj_set_size(NetworkSetting->EditAreaPanel.SelectedInterfaceLabel,
			LV_PCT(100), 30);
}

static void rec_screen_network_EditArea_createAddressRow(
	struct NetworkScreen *NetworkSetting)
{
	NetworkSetting->EditAreaPanel.AddressRow =
		lv_obj_create(NetworkSetting->EditAreaPanel.Container);

	rec_styles_applyContainerStyleBorderless(
		NetworkSetting->EditAreaPanel.AddressRow);

	lv_obj_set_size(NetworkSetting->EditAreaPanel.AddressRow, LV_PCT(100),
			80);
	lv_obj_set_flex_flow(NetworkSetting->EditAreaPanel.AddressRow,
			     LV_FLEX_FLOW_ROW);
	lv_obj_set_style_pad_all(NetworkSetting->EditAreaPanel.AddressRow, 0,
				 LV_PART_MAIN);

	rec_screen_disableScrolling(NetworkSetting->EditAreaPanel.AddressRow);
}

static void rec_screen_network_EditArea_AddressPanel_createPanel(
	struct EditAreaPanel *EditAreaPanel,
	struct NetworkAddrPanel *AddressPanel)
{
	AddressPanel->Container = lv_obj_create(EditAreaPanel->AddressRow);

	lv_obj_set_width(AddressPanel->Container, LV_PCT(50));

	rec_styles_applyContainerStyleEditPanel(AddressPanel->Container);

	lv_obj_set_height(AddressPanel->Container, LV_PCT(100));
	lv_obj_set_flex_flow(AddressPanel->Container, LV_FLEX_FLOW_COLUMN);

	rec_screen_disableScrolling(AddressPanel->Container);
	lv_obj_clear_flag(AddressPanel->Container, LV_OBJ_FLAG_CLICK_FOCUSABLE);
}

static void rec_screen_network_EditArea_AddressPanel_createLabel(
	struct NetworkAddrPanel *AddressPanel, const char *labelText)
{
	AddressPanel->Label = lv_label_create(AddressPanel->Container);

	rec_styles_applyIPLabelStyle(AddressPanel->Label);

	lv_obj_set_width(AddressPanel->Label, LV_PCT(100));
	lv_label_set_text(AddressPanel->Label, labelText);
}

static void
rec_screen_network_EditArea_AddressPanel_onAllTextareaEvent(lv_event_t *E)
{
	struct NetworkScreen *NetworkSetting =
		(struct NetworkScreen *)E->user_data;
	lv_event_code_t Code = lv_event_get_code(E);
	lv_obj_t *TextArea = E->target;

	if (LV_EVENT_CLICKED == Code) {
		lv_keyboard_set_textarea(
			NetworkSetting->EditAreaPanel.VirtualKeyboard,
			TextArea);
		//rec_screen_network_setEditingNavigation(NetworkSetting);

		lv_obj_set_style_bg_color(
			NetworkSetting->EditAreaPanel.IpAddress.TextArea,
			lv_color_white(), LV_PART_MAIN);
		lv_obj_set_style_bg_color(
			NetworkSetting->EditAreaPanel.Netmask.TextArea,
			lv_color_white(), LV_PART_MAIN);
		lv_obj_set_style_bg_color(TextArea,
					  lv_palette_main(LV_PALETTE_YELLOW),
					  LV_PART_MAIN);

		lv_group_focus_obj(TextArea);
	}
}

static void rec_screen_network_EditArea_AddressPanel_createTextArea(
	struct NetworkScreen *NetworkSetting,
	struct NetworkAddrPanel *AddressPanel, const char *TextAreaInitial)
{
	AddressPanel->TextArea = lv_textarea_create(AddressPanel->Container);

	lv_obj_set_width(AddressPanel->TextArea, LV_PCT(75));
	lv_obj_set_height(AddressPanel->TextArea, 40);
	lv_textarea_set_one_line(AddressPanel->TextArea, true);
	lv_textarea_set_cursor_click_pos(AddressPanel->TextArea, false);
	lv_textarea_set_placeholder_text(AddressPanel->TextArea,
					 TextAreaInitial);

	lv_obj_add_event_cb(
		AddressPanel->TextArea,
		rec_screen_network_EditArea_AddressPanel_onAllTextareaEvent,
		LV_EVENT_ALL, NetworkSetting);
}

static void rec_screen_network_EditArea_createEditIpAddressArea(
	struct NetworkScreen *NetworkSetting)
{
	struct NetworkAddrPanel *IpAddressPanel =
		&NetworkSetting->EditAreaPanel.IpAddress;

	rec_screen_network_EditArea_AddressPanel_createPanel(
		&NetworkSetting->EditAreaPanel, IpAddressPanel);

	rec_screen_network_EditArea_AddressPanel_createLabel(IpAddressPanel,
							     "IP Address");

	rec_screen_network_EditArea_AddressPanel_createTextArea(
		NetworkSetting, IpAddressPanel, "aaa.aaa.aaa.aaa");
}

static void rec_screen_network_EditArea_createEditNetmaskArea(
	struct NetworkScreen *NetworkSetting)
{
	struct NetworkAddrPanel *NetmaskPanel =
		&NetworkSetting->EditAreaPanel.Netmask;

	rec_screen_network_EditArea_AddressPanel_createPanel(
		&NetworkSetting->EditAreaPanel, NetmaskPanel);

	rec_screen_network_EditArea_AddressPanel_createLabel(NetmaskPanel,
							     "Netmask");

	rec_screen_network_EditArea_AddressPanel_createTextArea(
		NetworkSetting, NetmaskPanel, "nnn.nnn.nnn.nnn");
}

static void rec_screen_network_EditArea_createVirtualKeyboard(
	struct NetworkScreen *NetworkSetting)
{
	NetworkSetting->EditAreaPanel.VirtualKeyboard =
		lv_keyboard_create(NetworkSetting->EditAreaPanel.Container);

	lv_obj_set_flex_grow(NetworkSetting->EditAreaPanel.VirtualKeyboard, 1);
	lv_keyboard_set_mode(NetworkSetting->EditAreaPanel.VirtualKeyboard,
			     LV_KEYBOARD_MODE_NUMBER);
}

static void
rec_screen_network_EditArea_createPanel(struct NetworkScreen *NetworkSetting)
{
	NetworkSetting->EditAreaPanel.Container =
		lv_obj_create(NetworkSetting->Container);

	rec_styles_applyContainerStyle(NetworkSetting->EditAreaPanel.Container);
	lv_obj_set_size(NetworkSetting->EditAreaPanel.Container, LV_PCT(100),
			LV_PCT(100));
	rec_screen_disableScrolling(NetworkSetting->EditAreaPanel.Container);
	lv_obj_set_flex_flow(NetworkSetting->EditAreaPanel.Container,
			     LV_FLEX_FLOW_COLUMN);

	rec_screen_network_EditArea_createSelectedInterfaceLabel(
		NetworkSetting);
	rec_screen_network_EditArea_createAddressRow(NetworkSetting);
	rec_screen_network_EditArea_createEditIpAddressArea(NetworkSetting);
	rec_screen_network_EditArea_createEditNetmaskArea(NetworkSetting);
	rec_screen_network_EditArea_createVirtualKeyboard(NetworkSetting);
}

static void rec_screen_network_EditArea_updateFieldsFromSelectedInterface(
	struct NetworkScreen *NetworkSetting)
{
	/* set the selected interface label */
	lv_label_set_text(
		NetworkSetting->EditAreaPanel.SelectedInterfaceLabel,
		NetworkSetting->InterfaceListPanel.SelectedItemIpInfo->Name);

	/* set the ip address text area */
	lv_obj_set_style_bg_color(
		NetworkSetting->EditAreaPanel.IpAddress.TextArea,
		lv_color_white(), LV_PART_MAIN);
	lv_textarea_set_text(
		NetworkSetting->EditAreaPanel.IpAddress.TextArea,
		NetworkSetting->InterfaceListPanel.SelectedItemIpInfo->Address);

	/* set the netmask text area */
	lv_obj_set_style_bg_color(
		NetworkSetting->EditAreaPanel.Netmask.TextArea,
		lv_color_white(), LV_PART_MAIN);
	lv_textarea_set_text(
		NetworkSetting->EditAreaPanel.Netmask.TextArea,
		NetworkSetting->InterfaceListPanel.SelectedItemIpInfo->Netmask);

	/* reset the virtual keyboard */
	lv_keyboard_set_textarea(NetworkSetting->EditAreaPanel.VirtualKeyboard,
				 NULL);
}

static void
rec_screen_network_createContentsPanel(struct NetworkScreen *NetworkSetting)
{
	NetworkSetting->Container = lv_obj_create(NetworkSetting->Screen.Obj);

	lv_obj_set_width(NetworkSetting->Container, LV_PCT(100));

	rec_styles_applyContainerStyleBorderless(NetworkSetting->Container);
	rec_screen_disableScrolling(NetworkSetting->Container);
}

static void rec_screen_network_ActionsPanel_createContainer(
	struct NetworkScreen *NetworkSetting)
{
	NetworkSetting->ActionsPanel.Container =
		lv_obj_create(NetworkSetting->Screen.ActionsPanel);

	rec_styles_applyContainerStyleBorderless(
		NetworkSetting->ActionsPanel.Container);

	lv_obj_set_size(NetworkSetting->ActionsPanel.Container, LV_PCT(100),
			LV_PCT(100));

	lv_obj_set_flex_flow(NetworkSetting->ActionsPanel.Container,
			     LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(NetworkSetting->ActionsPanel.Container,
			      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_CENTER);

	rec_screen_disableScrolling(NetworkSetting->ActionsPanel.Container);

	lv_obj_set_flex_grow(NetworkSetting->Container, 2);
	lv_obj_set_flex_flow(NetworkSetting->Container, LV_FLEX_FLOW_COLUMN);
}

static void rec_screen_network_ActionsPanel_createButtons(
	struct NetworkScreen *NetworkSetting)
{
	const unsigned int n = sizeof(NetworkSetting->ActionsPanel.Button) /
			       sizeof(NetworkSetting->ActionsPanel.Button[0]);

	lv_obj_t **itButton = NetworkSetting->ActionsPanel.Button;
	lv_obj_t **itLabel = NetworkSetting->ActionsPanel.Label;

	for (unsigned int i = n; 0 != i; --i, itButton++, itLabel++) {
		/* create and configure button */
		*itButton =
			lv_btn_create(NetworkSetting->ActionsPanel.Container);

		rec_styles_applyButtonStyle(*itButton);

		lv_obj_set_height(*itButton, 40);
		lv_obj_set_width(*itButton, LV_PCT(33));

		/* create and configure label */
		*itLabel = lv_label_create(*itButton);

		lv_obj_align(*itLabel, LV_ALIGN_CENTER, 0, 0);
	}
}

static void rec_screen_network_ActionsPanel_createPanel(
	struct NetworkScreen *NetworkSetting)
{
	NetworkSetting->Screen.ActionsPanel =
		lv_obj_create(NetworkSetting->Screen.Obj);

	rec_styles_applyContainerStyleBorderless(
		NetworkSetting->Screen.ActionsPanel);

	lv_obj_set_size(NetworkSetting->Screen.ActionsPanel, LV_PCT(100),
			LV_PCT(10));
	lv_obj_set_style_pad_all(NetworkSetting->Screen.ActionsPanel, 0,
				 LV_PART_MAIN);

	rec_screen_disableScrolling(NetworkSetting->Screen.ActionsPanel);

	rec_screen_network_ActionsPanel_createContainer(NetworkSetting);
	rec_screen_network_ActionsPanel_createButtons(NetworkSetting);
}

static void
rec_screen_network_InterfaceList_showPage(struct NetworkScreen *NetworkSetting)
{
	lv_obj_clear_flag(NetworkSetting->InterfaceListPanel.Container,
			  LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(NetworkSetting->EditAreaPanel.Container,
			LV_OBJ_FLAG_HIDDEN);

	rec_screen_network_InterfaceList_configureButtons(NetworkSetting);
	rec_screen_network_InterfaceList_setListNavigation(NetworkSetting);

	rec_screen_network_InterfaceList_clearAllCheckboxes(NetworkSetting);
	rec_screen_network_InterfaceList_clearSelectedInterface(NetworkSetting);
}

static void rec_screen_network_EditArea_showPage_EditIpAddress(
	struct NetworkScreen *NetworkSetting)
{
	lv_obj_add_flag(NetworkSetting->InterfaceListPanel.Container,
			LV_OBJ_FLAG_HIDDEN);

	lv_obj_clear_flag(NetworkSetting->EditAreaPanel.Container,
			  LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(NetworkSetting->EditAreaPanel.Netmask.Container,
			  LV_OBJ_FLAG_HIDDEN);

	rec_screen_network_EditArea_configureButtons_editIpAddress(
		NetworkSetting);
	rec_screen_network_EditArea_IpAddress_setNavigation(NetworkSetting);
}

static void rec_screen_network_EditArea_showPage_EditGateway(
	struct NetworkScreen *NetworkSetting)
{
	lv_obj_add_flag(NetworkSetting->InterfaceListPanel.Container,
			LV_OBJ_FLAG_HIDDEN);

	lv_obj_clear_flag(NetworkSetting->EditAreaPanel.Container,
			  LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(NetworkSetting->EditAreaPanel.Netmask.Container,
			LV_OBJ_FLAG_HIDDEN);

	rec_screen_network_EditArea_configureButtons_editGateway(
		NetworkSetting);
	rec_screen_network_EditArea_Gateway_setNavigation(NetworkSetting);
}

static void
rec_screen_network_EditArea_showPage(struct NetworkScreen *NetworkSetting)
{
	if (NULL == NetworkSetting->InterfaceListPanel.SelectedItemIpInfo) {
		rec_screen_showNotification(
			&NetworkSetting->Screen,
			"Please select an interface for editing first!",
			NOTIFY_WARNING);
	} else {
		rec_screen_network_EditArea_updateFieldsFromSelectedInterface(
			NetworkSetting);

		if (util_networking_isDefaultGateway(
			    NetworkSetting->InterfaceListPanel
				    .SelectedItemIpInfo->Name)) {
			rec_screen_network_EditArea_showPage_EditGateway(
				NetworkSetting);
		} else {
			rec_screen_network_EditArea_showPage_EditIpAddress(
				NetworkSetting);
		}
	}
}

/*
 * Set the screen layout:
 *      - header at the top
 *      - interface list and edit area in the middle bellow the header
 *      - action area with buttons above notification area
 *      - notification area at the bottom
 * */
static void
rec_screen_network_setScreenLayout(struct NetworkScreen *NetworkSetting)
{
	lv_obj_set_flex_flow(NetworkSetting->Screen.Obj, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(NetworkSetting->Screen.Obj, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND);

	lv_obj_align(NetworkSetting->Screen.Header, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_align_to(NetworkSetting->InterfaceListPanel.Container,
			NetworkSetting->Screen.Header, LV_ALIGN_OUT_BOTTOM_MID,
			0, 0);
	lv_obj_align_to(NetworkSetting->EditAreaPanel.Container,
			NetworkSetting->Screen.Header, LV_ALIGN_OUT_BOTTOM_MID,
			0, 0);
	lv_obj_align(NetworkSetting->Screen.Notification, LV_ALIGN_BOTTOM_MID,
		     0, 0);
	lv_obj_align_to(NetworkSetting->ActionsPanel.Container,
			NetworkSetting->Screen.Notification,
			LV_ALIGN_OUT_TOP_MID, 0, 0);
	lv_obj_align_to(NetworkSetting->Screen.Notification,
			NetworkSetting->ActionsPanel.Container,
			LV_ALIGN_OUT_TOP_MID, 0, 0);
}

/**
 * Create network-settings screen
 */
void rec_screen_network_createNetworkScreen(void)
{
	struct NetworkScreen *NetworkSetting = AccessNetworkScreen();
	const struct RecoveryParameters *RecoveryParameters =
		util_config_getRecoveryParameters();

	rec_screen_createHeader(&NetworkSetting->Screen,
				&RecoveryParameters->Config);

	rec_screen_network_createContentsPanel(NetworkSetting);
	rec_screen_network_InterfaceList_createPanel(NetworkSetting);
	rec_screen_network_EditArea_createPanel(NetworkSetting);

	rec_screen_network_ActionsPanel_createPanel(NetworkSetting);

	rec_screen_createNotification(&NetworkSetting->Screen);

	rec_screen_network_setScreenLayout(NetworkSetting);

	rec_screen_network_InterfaceList_repopulateTable(NetworkSetting);
}

/**
 * Show network-settings screen
 */
void rec_screen_network_showNetworkScreen(void)
{
	struct NetworkScreen *NetworkSetting = AccessNetworkScreen();

	rec_screen_clearNotification(&NetworkSetting->Screen);

	rec_screen_network_InterfaceList_showPage(NetworkSetting);

	lv_scr_load(NetworkSetting->Screen.Obj);
}
