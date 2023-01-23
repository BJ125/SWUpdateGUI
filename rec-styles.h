/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains functions for creating and applying gui styles
 *
 */

#ifndef REC_STYLES_H
#define REC_STYLES_H

#include <lvgl/lvgl.h>

void rec_styles_applyHeaderStyle(lv_obj_t *Header);

void rec_styles_applyButtonStyle(lv_obj_t *Button);

void rec_styles_applyContainerStyle(lv_obj_t *Container);

void rec_styles_applyContainerStyleBorderless(lv_obj_t *Container);

void rec_styles_applyContainerStyleEditPanel(lv_obj_t *Container);

void rec_styles_applyContainerStyleNetworkPanel(lv_obj_t *Container);

void rec_styles_applyNetworkListStyle(lv_obj_t *List);

void rec_styles_applyNetworkListEntriesStyle(lv_obj_t *ListEntry);

void rec_styles_applyScrollbarStyle(lv_obj_t *Panel);

void rec_styles_applyCheckboxStyle(lv_obj_t *Checkbox);

void rec_styles_applyIPLabelStyle(lv_obj_t *Label);

void rec_styles_applyNetworkScreenListStyle(lv_obj_t *List);

void rec_styles_applyNotifyInfoStyle(lv_obj_t *Obj);
void rec_styles_applyNotifySuccessStyle(lv_obj_t *Obj);
void rec_styles_applyNotifyErrorStyle(lv_obj_t *Obj);
void rec_styles_applyNotifyWarningStyle(lv_obj_t *Obj);

void rec_styles_applyNetworkEntryStyle(lv_obj_t *Entry);

void rec_styles_applyTextLogStyle(lv_obj_t *Entry);

#endif /* REC_STYLES_H */
