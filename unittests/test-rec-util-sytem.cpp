/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 * */

#include "rec-util-system.h"

#include <vector>

#include <gtest/gtest.h>
#include <linux/input.h>

namespace test_rec_util_system {

using namespace testing;

class UtilSystemTest : public Test {};

using UtilSystemDeathTest = UtilSystemTest;

TEST(UtilSystemTest, test_util_system_GetEnvSettings_valid_values) {
  setenv("SCREEN_ORIENTATION_ANGLE", "90", 1);
  setenv("SCREEN_WIDTH", "1200", 1);
  setenv("SCREEN_HEIGHT", "800", 1);

  struct EnvSettings envSettings = util_system_getEnvSettings();
  EXPECT_EQ(envSettings.ScreenOrientationAngle, 90);
  EXPECT_EQ(envSettings.ScreenWidth, 1200);
  EXPECT_EQ(envSettings.ScreenHeight, 800);
}

TEST(UtilSystemTest, test_util_system_GetEnvSettings_nothing_set_in_env) {
  unsetenv("SCREEN_ORIENTATION_ANGLE");
  unsetenv("SCREEN_WIDTH");
  unsetenv("SCREEN_HEIGHT");

  struct EnvSettings envSettings = util_system_getEnvSettings();
  EXPECT_EQ(envSettings.ScreenOrientationAngle, 0);
  EXPECT_EQ(envSettings.ScreenWidth, 800);
  EXPECT_EQ(envSettings.ScreenHeight, 480);
}

TEST(UtilSystemTest, test_util_system_GetEnvSettings_empty_values) {
  setenv("SCREEN_ORIENTATION_ANGLE", "", 1);
  setenv("SCREEN_WIDTH", "", 1);
  setenv("SCREEN_HEIGHT", "", 1);

  struct EnvSettings envSettings = util_system_getEnvSettings();
  EXPECT_EQ(envSettings.ScreenOrientationAngle, 0);
  EXPECT_EQ(envSettings.ScreenWidth, 0);
  EXPECT_EQ(envSettings.ScreenHeight, 0);
}

TEST(UtilSystemTest, test_util_system_GetEnvSettings_invalid_resolution) {
  setenv("SCREEN_ORIENTATION_ANGLE", "180", 1);
  setenv("SCREEN_WIDTH", "ABC", 1);
  setenv("SCREEN_HEIGHT", "1200", 1);

  struct EnvSettings envSettings = util_system_getEnvSettings();
  EXPECT_EQ(envSettings.ScreenOrientationAngle, 180);
  EXPECT_EQ(envSettings.ScreenWidth, 0);
  EXPECT_EQ(envSettings.ScreenHeight, 1200);
}

TEST(UtilSystemTest, test_util_system_GetEnvSettings_0_in_resolution) {
  setenv("SCREEN_ORIENTATION_ANGLE", "90", 1);
  setenv("SCREEN_WIDTH", "900", 1);
  setenv("SCREEN_HEIGHT", "0", 1);

  struct EnvSettings envSettings = util_system_getEnvSettings();
  EXPECT_EQ(envSettings.ScreenOrientationAngle, 90);
  EXPECT_EQ(envSettings.ScreenWidth, 900);
  EXPECT_EQ(envSettings.ScreenHeight, 0);
}

TEST(UtilSystemTest, test_util_system_GetRotationEnum_valid) {
  lv_disp_rot_t value = util_system_getRotationEnum(0);
  EXPECT_EQ(value, LV_DISP_ROT_NONE);

  value = util_system_getRotationEnum(90);
  EXPECT_EQ(value, LV_DISP_ROT_90);

  value = util_system_getRotationEnum(180);
  EXPECT_EQ(value, LV_DISP_ROT_180);

  value = util_system_getRotationEnum(270);
  EXPECT_EQ(value, LV_DISP_ROT_270);
}

TEST(UtilSystemDeathTest, test_util_system_GetRotationEnum_invalid) {
  EXPECT_EXIT(util_system_getRotationEnum(3), ExitedWithCode(255), "");
}

TEST(UtilSystemTest, test_util_system_ConvertLinuxInputCodeToLvKey_valid) {
  lv_key_t key = util_system_convertLinuxInputCodeToLvKey(KEY_UP);

  EXPECT_EQ(key, LV_KEY_UP);

  key = util_system_convertLinuxInputCodeToLvKey(KEY_DOWN);

  EXPECT_EQ(key, LV_KEY_DOWN);

  key = util_system_convertLinuxInputCodeToLvKey(KEY_LEFT);

  EXPECT_EQ(key, LV_KEY_LEFT);

  key = util_system_convertLinuxInputCodeToLvKey(KEY_RIGHT);

  EXPECT_EQ(key, LV_KEY_RIGHT);

  key = util_system_convertLinuxInputCodeToLvKey(KEY_ENTER);

  EXPECT_EQ(key, LV_KEY_ENTER);
}

TEST(UtilSystemTest, test_util_system_ConvertLinuxInputCodeToLvKey_invalid) {
  const lv_key_t key = util_system_convertLinuxInputCodeToLvKey(KEY_A);

  EXPECT_EQ(key, 0);
}

TEST(UtilSystemTest, test_util_system_ConvertColorToLvPalette_valid) {
  lv_palette_t color = util_system_convertColorToLvPalette("orange");

  EXPECT_EQ(color, LV_PALETTE_ORANGE);
}

TEST(UtilSystemDeathTest, test_util_system_ConvertColorToLvPalette_invalid) {
  EXPECT_EXIT(util_system_convertColorToLvPalette(""), ExitedWithCode(255), "");

  EXPECT_EXIT(util_system_convertColorToLvPalette("dark-blue"),
              ExitedWithCode(255), "");
}

TEST(UtilSystemTest, test_util_system_isReturnValueInValidList) {
  const std::vector<int> validRetValues = {0, 1, 2, 5, 55, 12, 4};

  EXPECT_EQ(util_system_isReturnValueInValidList(0, &validRetValues[0],
                                                 validRetValues.size()),
            true);
  EXPECT_EQ(util_system_isReturnValueInValidList(1, &validRetValues[0],
                                                 validRetValues.size()),
            true);
  EXPECT_EQ(util_system_isReturnValueInValidList(2, &validRetValues[0],
                                                 validRetValues.size()),
            true);
  EXPECT_EQ(util_system_isReturnValueInValidList(5, &validRetValues[0],
                                                 validRetValues.size()),
            true);
  EXPECT_EQ(util_system_isReturnValueInValidList(55, &validRetValues[0],
                                                 validRetValues.size()),
            true);
  EXPECT_EQ(util_system_isReturnValueInValidList(12, &validRetValues[0],
                                                 validRetValues.size()),
            true);
  EXPECT_EQ(util_system_isReturnValueInValidList(4, &validRetValues[0],
                                                 validRetValues.size()),
            true);

  EXPECT_EQ(util_system_isReturnValueInValidList(3, &validRetValues[0],
                                                 validRetValues.size()),
            false);
  EXPECT_EQ(util_system_isReturnValueInValidList(6, &validRetValues[0],
                                                 validRetValues.size()),
            false);
  EXPECT_EQ(util_system_isReturnValueInValidList(20, &validRetValues[0],
                                                 validRetValues.size()),
            false);
  EXPECT_EQ(util_system_isReturnValueInValidList(111, &validRetValues[0],
                                                 validRetValues.size()),
            false);
}

}  // namespace test_rec_util_system
