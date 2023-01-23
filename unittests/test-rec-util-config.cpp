/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 * */

#include "rec-util-config.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace test_rec_util_config {

using namespace testing;

class TestConfigClass : public Test {
 public:
  TestConfigClass();
  ~TestConfigClass();

  void CreateTestConfigs();
  void SetEnvironmentVariables();

  std::string tmpDirName;
  std::string validConfig;
  std::string invalidConfig;
};

TestConfigClass::TestConfigClass()
    : tmpDirName(""), validConfig(""), invalidConfig("") {
  char dirTemplate[25] = "/tmp/fileXXXXXX";
  char* dirName = mkdtemp(dirTemplate);

  if (NULL == dirName) {
    throw std::string("Error creating local-temp folder errcode: " +
                      std::to_string(errno));
  }
  tmpDirName = dirName;
}

TestConfigClass::~TestConfigClass() {
  std::filesystem::remove_all(tmpDirName);
}

void TestConfigClass::CreateTestConfigs() {
  validConfig = tmpDirName + "/config_valid.txt";

  std::ofstream config1;
  config1.open(validConfig);
  config1 << "APP_ID = \"Recovery App\"\n";
  config1 << "NETWORK_INTERFACES = \"eth0, eth1\"\n";
  config1 << "VERSION = \"1.0\"\n";
  config1 << "MEDIAPATH =\"/run/media/sda\"\n";
  config1 << "KEYPAD_DEVICE =\"/dev/input/keypad0\"\n";

  invalidConfig = tmpDirName + "/config_invalid.txt";

  std::ofstream config2;
  config2.open(invalidConfig);
  config2 << "APP_ID =\n";
  config2 << "VERSION =\n\n\n";
  config2 << "MEDIAPATH =\n";
  config2 << "\n\n\n";

  sync();
}

void TestConfigClass::SetEnvironmentVariables() {
  setenv("SCREEN_ORIENTATION_ANGLE", "90", 1);
  setenv("SCREEN_WIDTH", "1200", 1);
  setenv("SCREEN_HEIGHT", "800", 1);
}

TEST_F(TestConfigClass, is_config_available_pass) {
  CreateTestConfigs();
  bool status = util_config_isAvailable(validConfig.c_str());
  ASSERT_TRUE(status);
}

TEST_F(TestConfigClass, is_config_available_fail) {
  std::string tmpPath = tmpDirName;
  tmpPath += "/abc.txt";
  bool status = util_config_isAvailable(tmpPath.c_str());
  ASSERT_FALSE(status);
}

TEST_F(TestConfigClass, get_config_valid) {
  CreateTestConfigs();
  ConfigRecovery config = util_config_get(validConfig.c_str());

  EXPECT_STREQ(config.Version, "1.0");
  EXPECT_STREQ(config.AppId, "Recovery App");
  EXPECT_STREQ(config.Logo, "");
  EXPECT_STREQ(config.Interfaces, "eth0, eth1");
  EXPECT_STREQ(config.Mediapath, "/run/media/sda");
  EXPECT_STREQ(config.KeypadDev, "/dev/input/keypad0");
}

TEST_F(TestConfigClass, get_config_invalid) {
  CreateTestConfigs();
  ConfigRecovery config = util_config_get(invalidConfig.c_str());

  EXPECT_STREQ(config.Version, "");
  EXPECT_STREQ(config.AppId, "");
  EXPECT_STREQ(config.Logo, "");
  EXPECT_STREQ(config.Interfaces, "");
  EXPECT_STREQ(config.Mediapath, "");
}

TEST_F(TestConfigClass, remove_whitespaces) {
  char input[SETTING_STR_LENGTH_MAX];

  strcpy(input, "this is a sky");
  util_config_removeChar(input, ' ');
  EXPECT_STREQ(input, "thisisasky");

  strcpy(input, "this-is a sky");
  util_config_removeChar(input, ' ');
  EXPECT_STREQ(input, "this-isasky");

  strcpy(input, "this is a sky? ");
  util_config_removeChar(input, ' ');
  EXPECT_STREQ(input, "thisisasky?");

  strcpy(input, "  this is_ a sky? ");
  util_config_removeChar(input, ' ');
  EXPECT_STREQ(input, "thisis_asky?");
}

TEST_F(TestConfigClass, remove_doublequotes) {
  char input[SETTING_STR_LENGTH_MAX];

  strcpy(input, "\"lorem ipset dolor asit\"");
  util_config_removeChar(input, '\"');
  EXPECT_STREQ(input, "lorem ipset dolor asit");

  strcpy(input, "He said, \"Hi!");
  util_config_removeChar(input, '\"');
  EXPECT_STREQ(input, "He said, Hi!");

  strcpy(input, "questions? Silence\"\" ");
  util_config_removeChar(input, '\"');
  EXPECT_STREQ(input, "questions? Silence ");

  strcpy(input, "  \"\"this is_\"\'\" a sky? ");
  util_config_removeChar(input, '\"');
  EXPECT_STREQ(input, "  this is_\' a sky? ");
}

TEST_F(TestConfigClass, trim_spaces) {
  char input[SETTING_STR_LENGTH_MAX];
  char* output = NULL;

  strcpy(input, " \" lorem ipset dolor asit\" ");
  output = util_config_trimWhitespaces(input);
  EXPECT_STREQ(output, "\" lorem ipset dolor asit\"");

  strcpy(input, "  _\"  pandora\" \'  ");
  output = util_config_trimWhitespaces(input);
  EXPECT_STREQ(output, "_\"  pandora\" \'");

  strcpy(input, "    ");
  output = util_config_trimWhitespaces(input);
  EXPECT_STREQ(output, "");

  strcpy(input, "");
  output = util_config_trimWhitespaces(input);
  EXPECT_STREQ(output, "");

  strcpy(input, "home           ");
  output = util_config_trimWhitespaces(input);
  EXPECT_STREQ(output, "home");

  strcpy(input, "eth0, eth1");
  output = util_config_trimWhitespaces(input);
  EXPECT_STREQ(output, "eth0, eth1");
}

TEST_F(TestConfigClass, parse_interfaces) {
  char list[100] = "eth0, br0, eth1";
  char output[IFACE_COUNT_MAX][IFACE_STR_LENGTH_MAX];

  util_config_parseInterfaces(list, output);

  EXPECT_STREQ("eth0", output[0]);
  EXPECT_STREQ("br0", output[1]);
  EXPECT_STREQ("eth1", output[2]);

  strcpy(list, "abc,pqr,_xyz");

  util_config_parseInterfaces(list, output);

  EXPECT_STREQ("abc", output[0]);
  EXPECT_STREQ("pqr", output[1]);
  EXPECT_STREQ("_xyz", output[2]);

  strcpy(list, " abc , pqr , \"_xyz\"");

  util_config_parseInterfaces(list, output);

  EXPECT_STREQ("abc", output[0]);
  EXPECT_STREQ("pqr", output[1]);
  EXPECT_STREQ("\"_xyz\"", output[2]);
}

TEST_F(TestConfigClass, set_defaults) {
  ConfigRecovery config;

  util_config_setDefaults(&config);

  EXPECT_STREQ(config.AppId, "Recovery App");
  EXPECT_STREQ(config.Version, "1.0");
  EXPECT_STREQ(config.Logo, "");
  EXPECT_STREQ(config.Interfaces, "");
  EXPECT_STREQ(config.Mediapath, "/media/usb/sda");
  EXPECT_STREQ(config.ThemeColor, "orange");
  EXPECT_STREQ(config.KeypadDev, "/dev/input/keyboard0");

  strcpy(config.AppId, "ABC");
  strcpy(config.Version, "4.3.2");
  strcpy(config.Logo, "Apple.png");
  strcpy(config.Interfaces, "eth0, eth1");
  strcpy(config.Mediapath, "/run");
  strcpy(config.ThemeColor, "yellow");
  strcpy(config.KeypadDev, "/dev/usb/keypad");

  util_config_setDefaults(&config);

  EXPECT_STREQ(config.AppId, "Recovery App");
  EXPECT_STREQ(config.Version, "1.0");
  EXPECT_STREQ(config.Logo, "");
  EXPECT_STREQ(config.Interfaces, "");
  EXPECT_STREQ(config.Mediapath, "/media/usb/sda");
  EXPECT_STREQ(config.ThemeColor, "orange");
  EXPECT_STREQ(config.KeypadDev, "/dev/input/keyboard0");
}

}  // namespace test_rec_util_config
