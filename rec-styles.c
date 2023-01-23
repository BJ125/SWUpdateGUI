/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains functions for creating and applying gui styles
 *
 */

#include "rec-styles.h"

#include "rec-util-config.h"
#include "rec-util-system.h"

#include <stdlib.h>

struct Styles {
	lv_palette_t ColorPrimary;

	lv_style_t HeaderStyle;
	lv_style_t ButtonStyle;
	lv_style_t ButtonPressedStyle;
	lv_style_t ButtonHighlightStyle;
	lv_style_t ContainerStyleDefault;
	lv_style_t ContainerStyleBorderless;
	lv_style_t ContainerStyleEditPanel;
	lv_style_t ContainerStyleNetworkPanel;
	lv_style_t NetworklistStyle;
	lv_style_t NetworklistEntriesStyle;
	lv_style_t ScrollbarStyle;
	lv_style_t CheckboxStyleDefault;
	lv_style_t CheckboxStyleChecked;
	lv_style_t CheckboxStyleSelected;
	lv_style_t CheckboxStyleDisabled;
	lv_style_t IpLabelStyle;
	lv_style_t NetworkScreenListStyle;
	lv_style_t NotifyInfoStyle;
	lv_style_t NotifyErrorStyle;
	lv_style_t NotifyWarningStyle;
	lv_style_t NotifySuccessStyle;
	lv_style_t NetworkEntryDefaultStyle;
	lv_style_t NetworkEntrySelectedStyle;
	lv_style_t TextAreaLogStyle;
};

static void rec_styles_createHeaderStyle(struct Styles *Styles)
{
	lv_style_init(&Styles->HeaderStyle);

	lv_style_set_border_opa(&Styles->HeaderStyle, LV_OPA_TRANSP);
	lv_style_set_radius(&Styles->HeaderStyle, 0);
	lv_style_set_text_align(&Styles->HeaderStyle, LV_TEXT_ALIGN_CENTER);
	lv_style_set_text_color(&Styles->HeaderStyle, lv_color_white());
	lv_style_set_bg_color(&Styles->HeaderStyle,
			      lv_palette_darken(LV_PALETTE_GREY, 3));
}

static void rec_styles_createButtonStyle(struct Styles *Styles)
{
	lv_style_init(&Styles->ButtonStyle);

	lv_style_set_radius(&Styles->ButtonStyle, 1);
	lv_style_set_bg_opa(&Styles->ButtonStyle, LV_OPA_100);
	lv_style_set_bg_color(&Styles->ButtonStyle,
			      lv_palette_main(Styles->ColorPrimary));
	lv_style_set_text_color(&Styles->ButtonStyle, lv_color_white());
	lv_style_set_pad_all(&Styles->ButtonStyle, 10);

	lv_style_init(&Styles->ButtonPressedStyle);

	lv_style_set_translate_y(&Styles->ButtonPressedStyle, 5);

	lv_style_init(&Styles->ButtonHighlightStyle);

	lv_style_set_outline_color(&Styles->ButtonHighlightStyle,
				   lv_color_white());
}

static void rec_styles_createContainerStyles(struct Styles *Styles)
{
	lv_style_init(&Styles->ContainerStyleDefault);

	lv_style_set_border_opa(&Styles->ContainerStyleDefault, 100);
	lv_style_set_border_width(&Styles->ContainerStyleDefault, 2);
	lv_style_set_border_color(&Styles->ContainerStyleDefault,
				  lv_palette_darken(LV_PALETTE_GREY, 3));
	lv_style_set_bg_color(&Styles->ContainerStyleDefault,
			      lv_palette_main(LV_PALETTE_GREY));
	lv_style_set_radius(&Styles->ContainerStyleDefault, 0);

	lv_style_init(&Styles->ContainerStyleBorderless);

	lv_style_set_border_opa(&Styles->ContainerStyleBorderless,
				LV_OPA_TRANSP);
	lv_style_set_border_width(&Styles->ContainerStyleBorderless, 0);
	lv_style_set_radius(&Styles->ContainerStyleBorderless, 0);
	lv_style_set_bg_color(&Styles->ContainerStyleBorderless,
			      lv_palette_main(LV_PALETTE_GREY));

	lv_style_init(&Styles->ContainerStyleEditPanel);

	lv_style_set_border_opa(&Styles->ContainerStyleEditPanel,
				LV_OPA_TRANSP);
	lv_style_set_radius(&Styles->ContainerStyleEditPanel, 0);
	lv_style_set_pad_all(&Styles->ContainerStyleEditPanel, 0);

	lv_style_init(&Styles->ContainerStyleNetworkPanel);

	lv_style_set_border_opa(&Styles->ContainerStyleNetworkPanel,
				LV_OPA_TRANSP);
	lv_style_set_radius(&Styles->ContainerStyleNetworkPanel, 0);
	lv_style_set_pad_all(&Styles->ContainerStyleNetworkPanel, 0);
	lv_style_set_bg_color(&Styles->ContainerStyleNetworkPanel,
			      lv_palette_darken(LV_PALETTE_GREY, 3));
}

static void rec_styles_createScrollbarStyle(struct Styles *Styles)
{
	lv_style_init(&Styles->ScrollbarStyle);

	lv_style_set_width(&Styles->ScrollbarStyle, 15);
	lv_style_set_pad_right(&Styles->ScrollbarStyle, 5);
	lv_style_set_radius(&Styles->ScrollbarStyle, 2);
	lv_style_set_bg_opa(&Styles->ScrollbarStyle, LV_OPA_100);
	lv_style_set_bg_color(&Styles->ScrollbarStyle,
			      lv_palette_lighten(Styles->ColorPrimary, 2));
	lv_style_set_border_color(&Styles->ScrollbarStyle,
				  lv_palette_main(Styles->ColorPrimary));
	lv_style_set_border_width(&Styles->ScrollbarStyle, 2);
}

static void rec_styles_createCheckboxStyles(struct Styles *Styles)
{
	lv_style_init(&Styles->CheckboxStyleDefault);

	lv_style_set_pad_all(&Styles->CheckboxStyleDefault, 0);
	lv_style_set_bg_color(&Styles->CheckboxStyleDefault,
			      lv_palette_lighten(Styles->ColorPrimary, 4));
	lv_style_set_bg_opa(&Styles->CheckboxStyleDefault, LV_OPA_100);
	lv_style_set_radius(&Styles->CheckboxStyleDefault, 2);
	lv_style_set_border_color(&Styles->CheckboxStyleDefault,
				  lv_palette_darken(LV_PALETTE_GREY, 2));

	lv_style_init(&Styles->CheckboxStyleChecked);

	lv_style_set_bg_color(&Styles->CheckboxStyleChecked,
			      lv_palette_darken(Styles->ColorPrimary, 2));
	lv_style_set_bg_opa(&Styles->CheckboxStyleChecked, LV_OPA_100);
	lv_style_set_radius(&Styles->CheckboxStyleChecked, 2);

	lv_style_init(&Styles->CheckboxStyleDisabled);

	lv_style_set_bg_color(&Styles->CheckboxStyleDisabled,
			      lv_palette_main(LV_PALETTE_GREY));
	lv_style_set_bg_opa(&Styles->CheckboxStyleDisabled, LV_OPA_100);
	lv_style_set_radius(&Styles->CheckboxStyleDisabled, 2);

	lv_style_init(&Styles->CheckboxStyleSelected);

	lv_style_set_outline_color(&Styles->CheckboxStyleSelected,
				   lv_color_white());
}

static void rec_styles_createIPLabelStyle(struct Styles *Styles)
{
	lv_style_init(&Styles->IpLabelStyle);

	lv_style_set_text_font(&Styles->IpLabelStyle, &lv_font_montserrat_14);
}

static void rec_styles_createNetworkScreenListStyle(struct Styles *Styles)
{
	lv_style_init(&Styles->NetworkScreenListStyle);

	lv_style_set_border_opa(&Styles->NetworkScreenListStyle, LV_OPA_TRANSP);
	lv_style_set_radius(&Styles->NetworkScreenListStyle, 0);
	lv_style_set_text_align(&Styles->NetworkScreenListStyle,
				LV_TEXT_ALIGN_CENTER);
	lv_style_set_text_color(&Styles->NetworkScreenListStyle,
				lv_color_white());
	lv_style_set_bg_color(&Styles->NetworkScreenListStyle,
			      lv_palette_darken(LV_PALETTE_GREY, 1));
	lv_style_set_pad_all(&Styles->NetworkScreenListStyle, 0);

	lv_style_init(&Styles->NetworkEntryDefaultStyle);

	lv_style_set_bg_color(&Styles->NetworkEntryDefaultStyle,
			      lv_palette_main(LV_PALETTE_GREY));

	lv_style_init(&Styles->NetworkEntrySelectedStyle);

	lv_style_set_outline_color(&Styles->NetworkEntrySelectedStyle,
				   lv_color_white());
	lv_style_set_bg_color(&Styles->NetworkEntrySelectedStyle,
			      lv_palette_lighten(LV_PALETTE_GREY, 2));
	lv_style_set_text_color(&Styles->NetworkEntrySelectedStyle,
				lv_color_black());
}

static void rec_styles_createNetworkListStyle(struct Styles *Styles)
{
	lv_style_init(&Styles->NetworklistStyle);
	lv_style_set_bg_color(&Styles->NetworklistStyle,
			      lv_palette_main(LV_PALETTE_GREY));
	lv_style_set_radius(&Styles->NetworklistStyle, 0);
	lv_style_set_border_width(&Styles->NetworklistStyle, 0);

	lv_style_init(&Styles->NetworklistEntriesStyle);
	lv_style_set_bg_color(&Styles->NetworklistEntriesStyle,
			      lv_palette_darken(LV_PALETTE_GREY, 3));
	lv_style_set_radius(&Styles->NetworklistEntriesStyle, 0);
	lv_style_set_border_width(&Styles->NetworklistEntriesStyle, 0);
	lv_style_set_text_color(&Styles->NetworklistEntriesStyle,
				lv_color_white());
	lv_style_set_text_font(&Styles->NetworklistEntriesStyle,
			       &lv_font_montserrat_14);
}

static void rec_styles_createNotifyStyles(struct Styles *Styles)
{
	lv_style_init(&Styles->NotifyInfoStyle);

	lv_style_set_radius(&Styles->NotifyInfoStyle, 0);
	lv_style_set_bg_color(&Styles->NotifyInfoStyle,
			      lv_palette_lighten(LV_PALETTE_GREY, 1));
	lv_style_set_border_color(&Styles->NotifyInfoStyle,
				  lv_palette_main(LV_PALETTE_GREY));

	lv_style_init(&Styles->NotifySuccessStyle);

	lv_style_set_radius(&Styles->NotifySuccessStyle, 0);
	lv_style_set_bg_color(&Styles->NotifySuccessStyle,
			      lv_palette_lighten(LV_PALETTE_LIGHT_GREEN, 1));
	lv_style_set_border_color(&Styles->NotifySuccessStyle,
				  lv_palette_main(LV_PALETTE_GREY));

	lv_style_init(&Styles->NotifyWarningStyle);

	lv_style_set_radius(&Styles->NotifyWarningStyle, 0);
	lv_style_set_bg_color(&Styles->NotifyWarningStyle,
			      lv_palette_lighten(LV_PALETTE_YELLOW, 1));
	lv_style_set_border_color(&Styles->NotifyWarningStyle,
				  lv_palette_main(LV_PALETTE_GREY));

	lv_style_init(&Styles->NotifyErrorStyle);

	lv_style_set_radius(&Styles->NotifyErrorStyle, 0);
	lv_style_set_bg_color(&Styles->NotifyErrorStyle,
			      lv_palette_lighten(LV_PALETTE_RED, 1));
	lv_style_set_border_color(&Styles->NotifyErrorStyle,
				  lv_palette_main(LV_PALETTE_GREY));
}

static void rec_styles_createTextAreaLogStyle(struct Styles *Styles)
{
	lv_style_init(&Styles->TextAreaLogStyle);

	lv_style_set_radius(&Styles->TextAreaLogStyle, 0);
	lv_style_set_bg_color(&Styles->TextAreaLogStyle,
			      lv_palette_lighten(LV_PALETTE_GREY, 1));
	lv_style_set_pad_all(&Styles->TextAreaLogStyle, 5);
}

static struct Styles *rec_styles_accessStyles(void)
{
	static struct Styles *Styles = NULL;
	if (NULL == Styles) {
		Styles = malloc(sizeof(struct Styles));

		const struct RecoveryParameters *RecoveryParameters =
			util_config_getRecoveryParameters();
		Styles->ColorPrimary = util_system_convertColorToLvPalette(
			RecoveryParameters->Config.ThemeColor);

		rec_styles_createHeaderStyle(Styles);
		rec_styles_createButtonStyle(Styles);
		rec_styles_createContainerStyles(Styles);
		rec_styles_createScrollbarStyle(Styles);
		rec_styles_createCheckboxStyles(Styles);
		rec_styles_createIPLabelStyle(Styles);
		rec_styles_createNetworkScreenListStyle(Styles);
		rec_styles_createNetworkListStyle(Styles);
		rec_styles_createNotifyStyles(Styles);
		rec_styles_createTextAreaLogStyle(Styles);
	}
	return Styles;
}

/**
 * Apply style to header
 *
 * @param[in] Header Header widget
 */
void rec_styles_applyHeaderStyle(lv_obj_t *Header)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(Header, &Styles->HeaderStyle, LV_PART_MAIN);
}

/**
 * Apply style to button
 *
 * @param[in] Button Button widget
 */
void rec_styles_applyButtonStyle(lv_obj_t *Button)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(Button, &Styles->ButtonStyle, LV_PART_MAIN);
	lv_obj_add_style(Button, &Styles->ButtonPressedStyle, LV_STATE_PRESSED);
	lv_obj_add_style(Button, &Styles->ButtonHighlightStyle,
			 LV_PART_MAIN | LV_STATE_FOCUS_KEY);
}

/**
 * Apply default style to panels
 *
 * @param[in] Container Panel widget
 */
void rec_styles_applyContainerStyle(lv_obj_t *Container)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(Container, &Styles->ContainerStyleDefault,
			 LV_PART_MAIN);
}

/**
 * Apply borderless style to panels
 *
 * @param[in] Container Panel widget
 */
void rec_styles_applyContainerStyleBorderless(lv_obj_t *Container)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(Container, &Styles->ContainerStyleBorderless,
			 LV_PART_MAIN);
}

/**
 * Apply special edit-panel style to panels
 *
 * @param[in] Container Panel widget
 */
void rec_styles_applyContainerStyleEditPanel(lv_obj_t *Container)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(Container, &Styles->ContainerStyleEditPanel,
			 LV_PART_MAIN);
}

/**
 * Apply network-list style to container
 *
 * @paramp[in] Container Panel widget containing network-list entry
 */
void rec_styles_applyContainerStyleNetworkPanel(lv_obj_t *Container)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(Container, &Styles->ContainerStyleNetworkPanel,
			 LV_PART_MAIN);
}

/**
 * Apply network-list style
 *
 * @param[in] List Network-interfaces list
 */
void rec_styles_applyNetworkListStyle(lv_obj_t *List)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(List, &Styles->NetworklistStyle, LV_PART_MAIN);
}

/**
 * Apply network-list items style
 *
 * @param[in] ListEntry Individual entry in Network-interfaces list
 */
void rec_styles_applyNetworkListEntriesStyle(lv_obj_t *ListEntry)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(ListEntry, &Styles->NetworklistEntriesStyle,
			 LV_PART_MAIN);
}

/**
 * Apply scrollbar style to panel
 *
 * @param[in] Panel Panel widget
 */
void rec_styles_applyScrollbarStyle(lv_obj_t *Panel)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_remove_style(Panel, NULL, LV_PART_SCROLLBAR | LV_STATE_ANY);

	lv_obj_add_style(Panel, &Styles->ScrollbarStyle, LV_PART_SCROLLBAR);
}

/**
 * Apply checkbox styles to checkbox
 *
 * @param[in] Checkbox Checkbox widget
 */
void rec_styles_applyCheckboxStyle(lv_obj_t *Checkbox)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(Checkbox, &Styles->CheckboxStyleDefault,
			 LV_PART_INDICATOR | LV_STATE_DEFAULT);
	lv_obj_add_style(Checkbox, &Styles->CheckboxStyleChecked,
			 LV_PART_INDICATOR | LV_STATE_CHECKED);
	lv_obj_add_style(Checkbox, &Styles->CheckboxStyleDisabled,
			 LV_PART_INDICATOR | LV_STATE_DISABLED);
	lv_obj_add_style(Checkbox, &Styles->CheckboxStyleSelected,
			 LV_PART_MAIN | LV_STATE_FOCUS_KEY);
}

/**
 * Apply IP label style
 *
 * @param[in] Label Label object
 */
void rec_styles_applyIPLabelStyle(lv_obj_t *Label)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(Label, &Styles->IpLabelStyle, 0);
}

/**
 * Apply list style in network-settings page
 *
 * @param[in] List List object
 */
void rec_styles_applyNetworkScreenListStyle(lv_obj_t *List)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(List, &Styles->NetworkScreenListStyle, LV_PART_MAIN);
}

/**
 * Apply Information notification style
 *
 * @param[in] Obj Notification object
 */
void rec_styles_applyNotifyInfoStyle(lv_obj_t *Obj)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(Obj, &Styles->NotifyInfoStyle, LV_PART_MAIN);
}

/**
 * Apply Success notification style to object
 *
 * @param[in] Obj Notification object
 */
void rec_styles_applyNotifySuccessStyle(lv_obj_t *Obj)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(Obj, &Styles->NotifySuccessStyle, LV_PART_MAIN);
}

/**
 * Apply Error notification style to object
 *
 * @param[in] Obj Notification object
 */
void rec_styles_applyNotifyErrorStyle(lv_obj_t *Obj)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(Obj, &Styles->NotifyErrorStyle, LV_PART_MAIN);
}

/**
 * Apply Warning notification style to object
 *
 * @param[in] Obj Notification object
 */
void rec_styles_applyNotifyWarningStyle(lv_obj_t *Obj)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(Obj, &Styles->NotifyWarningStyle, LV_PART_MAIN);
}

/**
 * Apply style to entries on network-settings page
 *
 * @param[in] Entry Network entry
 */
void rec_styles_applyNetworkEntryStyle(lv_obj_t *Entry)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(Entry, &Styles->NetworkEntryDefaultStyle,
			 LV_PART_MAIN);
	lv_obj_add_style(Entry, &Styles->NetworkEntrySelectedStyle,
			 LV_PART_MAIN | LV_STATE_FOCUS_KEY);
}

/**
 * Apply style to textarea in extended-notification
 *
 * @param[in] TextArea Textarea widget
 */
void rec_styles_applyTextLogStyle(lv_obj_t *TextArea)
{
	struct Styles *Styles = rec_styles_accessStyles();

	lv_obj_add_style(TextArea, &Styles->TextAreaLogStyle, LV_PART_MAIN);
}
