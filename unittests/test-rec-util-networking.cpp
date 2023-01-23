/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 * */

#include "rec-util-networking.h"

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <cstring>

namespace test_rec_util_networking {

using namespace testing;

class UtilNetworkingTest : public Test {};

using UtilNetworkingDeathTest = UtilNetworkingTest;

TEST(UtilNetworkingTest, check_binary_to_IPV4_conversion) {
  char buffer[IFACE_ADDR_MAX];
  int status = -1;

  strncpy(buffer, "FE01A8C0", IFACE_ADDR_MAX - 1);
  buffer[IFACE_ADDR_MAX - 1] = '\0';

  status = util_networking_convertIPAddressToDotFormat(buffer, sizeof(buffer));
  EXPECT_EQ(status, 0);
  EXPECT_STREQ("192.168.1.254", buffer);

  strncpy(buffer, "AXELOT", IFACE_ADDR_MAX - 1);
  buffer[IFACE_ADDR_MAX - 1] = '\0';

  status = util_networking_convertIPAddressToDotFormat(buffer, sizeof(buffer));
  EXPECT_EQ(status, -1);
  EXPECT_STREQ("AXELOT", buffer);

  strncpy(buffer, "0000001", IFACE_ADDR_MAX - 1);
  buffer[IFACE_ADDR_MAX - 1] = '\0';

  status = util_networking_convertIPAddressToDotFormat(buffer, sizeof(buffer));
  EXPECT_EQ(status, 0);
  EXPECT_STREQ("1.0.0.0", buffer);

  strncpy(buffer, "+-0039403", IFACE_ADDR_MAX - 1);
  buffer[IFACE_ADDR_MAX - 1] = '\0';

  status = util_networking_convertIPAddressToDotFormat(buffer, sizeof(buffer));
  EXPECT_EQ(status, -1);
  EXPECT_STREQ("+-0039403", buffer);
}

TEST(UtilNetworkingTest, convert_to_cidr) {
  char output[2 * SETTING_STR_LENGTH_MAX];

  int status = util_networking_convertToCidr("192.168.1.14", "255.255.255.0",
                                             output, sizeof(output));
  EXPECT_EQ(status, 0);
  EXPECT_STREQ("192.168.1.14/24", output);

  status = util_networking_convertToCidr("ABCD", "255.0.0.0", output,
                                         sizeof(output));
  EXPECT_EQ(status, 0);
  EXPECT_STREQ("ABCD/8", output);

  status = util_networking_convertToCidr("1.1.1.1", "220.128.0.0", output,
                                         sizeof(output));
  EXPECT_EQ(status, -1);

  status = util_networking_convertToCidr("192.168.1.14", "255.254.0.0", output,
                                         sizeof(output));
  EXPECT_EQ(status, 0);
  EXPECT_STREQ("192.168.1.14/15", output);

  status = util_networking_convertToCidr("192.168.1.14", "128.0.0.0", output,
                                         sizeof(output));
  EXPECT_EQ(status, 0);
  EXPECT_STREQ("192.168.1.14/1", output);

  status = util_networking_convertToCidr("192.168.1.14", "255.255.224.0",
                                         output, sizeof(output));
  EXPECT_EQ(status, 0);
  EXPECT_STREQ("192.168.1.14/19", output);

  status = util_networking_convertToCidr("192.168.1.14", "255.255.255.252",
                                         output, sizeof(output));
  EXPECT_EQ(status, 0);
  EXPECT_STREQ("192.168.1.14/30", output);

  status = util_networking_convertToCidr("192.168.1.14", "255.255.255.255",
                                         output, sizeof(output));
  EXPECT_EQ(status, 0);
  EXPECT_STREQ("192.168.1.14/32", output);

  status = util_networking_convertToCidr("192.168.1.14", "255.255.248.0",
                                         output, sizeof(output));
  EXPECT_EQ(status, 0);
  EXPECT_STREQ("192.168.1.14/21", output);
}

TEST(UtilNetworkingTest, count_continuous_bits_from_LSB) {
  int count = util_networking_countContinuous1Bits(0x00);
  EXPECT_EQ(count, 0);

  count = util_networking_countContinuous1Bits(0x80000000);
  EXPECT_EQ(count, 1);

  count = util_networking_countContinuous1Bits(0xC0000000);
  EXPECT_EQ(count, 2);

  count = util_networking_countContinuous1Bits(0xE0000000);
  EXPECT_EQ(count, 3);

  count = util_networking_countContinuous1Bits(0xF0000000);
  EXPECT_EQ(count, 4);

  count = util_networking_countContinuous1Bits(0xF8000000);
  EXPECT_EQ(count, 5);

  count = util_networking_countContinuous1Bits(0xFC000000);
  EXPECT_EQ(count, 6);

  count = util_networking_countContinuous1Bits(0xFFFF0000);
  EXPECT_EQ(count, 16);

  count = util_networking_countContinuous1Bits(0xFFFFFFFF);
  EXPECT_EQ(count, 32);

  count = util_networking_countContinuous1Bits(0xFFF0FFFF);
  EXPECT_EQ(count, -1);
}

TEST(UtilNetworkingTest, test_util_networking_SearchInterface_success) {
  Ipv4Info ipInfoList2[3] = {
      {"lo", "127.0.0.1", "255.255.255.0", false, NULL},
      {"enp1s0", "192.168.1.40", "255.255.255.0", false, NULL},
      {"wlps02", "", "", false, NULL}};
  ipInfoList2[0].Next = &ipInfoList2[1];
  ipInfoList2[1].Next = &ipInfoList2[2];

  Ipv4Info* info = util_networking_searchInterface("lo", ipInfoList2);
  EXPECT_EQ(info, &ipInfoList2[0]);

  info = util_networking_searchInterface("enp1s0", ipInfoList2);
  EXPECT_EQ(info, &ipInfoList2[1]);

  info = util_networking_searchInterface("wlps02", ipInfoList2);
  EXPECT_EQ(info, &ipInfoList2[2]);
}

TEST(UtilNetworkingTest, test_util_networking_SearchInterface_failure) {
  Ipv4Info ipInfoList2[3] = {
      {"lo", "127.0.0.1", "255.255.255.0", false, NULL},
      {"enp1s0", "192.168.1.40", "255.255.255.0", false, NULL},
      {"wlps02", "", "", false, NULL}};
  ipInfoList2[0].Next = &ipInfoList2[1];
  ipInfoList2[1].Next = &ipInfoList2[2];

  Ipv4Info* info = util_networking_searchInterface("eth0", ipInfoList2);
  EXPECT_EQ(NULL, info);

  info = util_networking_searchInterface("", ipInfoList2);
  EXPECT_EQ(NULL, info);
}

TEST(UtilNetworkingTest, test_utils_networking_CreateInterfaceInfo) {
  Ipv4Info* info = util_networking_createInterfaceInfo("eth0");

  EXPECT_NE(info, nullptr);
  EXPECT_STREQ(info->Name, "eth0");
  EXPECT_STREQ(info->Address, "");
  EXPECT_STREQ(info->Netmask, "");
  EXPECT_EQ(info->Next, nullptr);
}

TEST(UtilNetworkingTest, test_utils_networking_CreateInterfaceInfo_empty) {
  Ipv4Info* info = util_networking_createInterfaceInfo("");

  EXPECT_NE(info, nullptr);
  EXPECT_STREQ(info->Name, "");
  EXPECT_STREQ(info->Address, "");
  EXPECT_STREQ(info->Netmask, "");
  EXPECT_EQ(info->Next, nullptr);
}

TEST(UtilNetworkingTest,
     test_utils_networking_SetAddressInInterfaceInfo_success) {
  Ipv4Info info;
  memset(&info, 0, sizeof(info));

  EXPECT_STREQ(info.Name, "");

  struct ifaddrs addr;
  struct sockaddr_in sockAddr;

  sockAddr.sin_family = AF_INET;
  inet_pton(AF_INET, "127.0.0.1", &sockAddr.sin_addr);
  addr.ifa_addr = (struct sockaddr*)&sockAddr;

  util_networking_setAddressInInterfaceInfo(&info, &addr);

  EXPECT_STREQ(info.Address, "127.0.0.1");
}

TEST(UtilNetworkingTest,
     test_utils_networking_SetAddressInInterfaceInfo_no_ipv4) {
  Ipv4Info info;
  memset(&info, 0, sizeof(info));

  EXPECT_STREQ(info.Name, "");

  struct ifaddrs addr;
  struct sockaddr_in sockAddr;

  sockAddr.sin_family = AF_INET6;
  inet_pton(AF_INET, "127.0.0.1", &sockAddr.sin_addr);
  addr.ifa_addr = (struct sockaddr*)&sockAddr;

  util_networking_setAddressInInterfaceInfo(&info, &addr);

  EXPECT_STREQ(info.Address, "");
}

TEST(UtilNetworkingTest,
     test_utils_networking_SetAddressInInterfaceInfo_all_zero) {
  Ipv4Info info;
  memset(&info, 0, sizeof(info));

  struct ifaddrs addr;
  struct sockaddr_in sockAddr;

  sockAddr.sin_family = AF_INET;
  inet_pton(AF_INET, "0.0.0.0", &sockAddr.sin_addr);
  addr.ifa_addr = (struct sockaddr*)&sockAddr;

  util_networking_setAddressInInterfaceInfo(&info, &addr);

  EXPECT_STREQ(info.Address, "");
}

TEST(UtilNetworkingTest,
     test_utils_networking_SetAddressInInterfaceInfo_no_addr) {
  Ipv4Info info;
  memset(&info, 0, sizeof(info));

  struct ifaddrs addr;
  struct sockaddr_in sockAddr;
  memset(&sockAddr, 0, sizeof(sockAddr));

  sockAddr.sin_family = AF_INET;
  addr.ifa_addr = (struct sockaddr*)&sockAddr;

  util_networking_setAddressInInterfaceInfo(&info, &addr);

  EXPECT_STREQ(info.Address, "");
}

TEST(UtilNetworkingTest,
     test_utils_networking_SetNetmaskInInterfaceInfo_success) {
  Ipv4Info info;
  memset(&info, 0, sizeof(info));

  struct ifaddrs addr;
  struct sockaddr_in sockAddr;
  memset(&sockAddr, 0, sizeof(sockAddr));

  sockAddr.sin_family = AF_INET;
  inet_pton(AF_INET, "255.255.255.0", &sockAddr.sin_addr);
  addr.ifa_netmask = (struct sockaddr*)&sockAddr;

  util_networking_setNetmaskInInterfaceInfo(&info, &addr);

  EXPECT_STREQ(info.Netmask, "255.255.255.0");
}

TEST(UtilNetworkingTest, test_utils_networking_SetNetmaskInInterfaceInfo_null) {
  Ipv4Info info;
  memset(&info, 0, sizeof(info));

  struct ifaddrs addr;
  memset(&addr, 0, sizeof(addr));

  util_networking_setNetmaskInInterfaceInfo(&info, &addr);

  EXPECT_STREQ(info.Netmask, "");
}

TEST(UtilNetworkingTest,
     test_utils_networking_SetNetmaskInInterfaceInfo_all_zero) {
  Ipv4Info info;
  memset(&info, 0, sizeof(info));

  struct ifaddrs addr;
  struct sockaddr_in sockAddr;
  memset(&sockAddr, 0, sizeof(sockAddr));

  sockAddr.sin_family = AF_INET;
  inet_pton(AF_INET, "0.0.0.0", &sockAddr.sin_addr);
  addr.ifa_netmask = (struct sockaddr*)&sockAddr;

  util_networking_setNetmaskInInterfaceInfo(&info, &addr);

  EXPECT_STREQ(info.Netmask, "");
}

TEST(UtilNetworkingTest,
     test_utils_networking_SetNetmaskInInterfaceInfo_no_addr) {
  Ipv4Info info;
  memset(&info, 0, sizeof(info));

  struct ifaddrs addr;
  struct sockaddr_in sockAddr;
  memset(&sockAddr, 0, sizeof(sockAddr));

  sockAddr.sin_family = AF_INET;
  addr.ifa_netmask = (struct sockaddr*)&sockAddr;

  util_networking_setNetmaskInInterfaceInfo(&info, &addr);

  EXPECT_STREQ(info.Netmask, "");
}

TEST(UtilNetworkingTest, test_util_FindInterfaceInList_success) {
  char networkList[5][IFACE_STR_LENGTH_MAX] = {"eth0", "eth1", "wifi1"};
  const char* result = util_networking_findInterfaceInList("eth0", networkList);
  EXPECT_STREQ(result, "eth0");

  result = util_networking_findInterfaceInList("eth1", networkList);
  EXPECT_STREQ(result, "eth1");

  result = util_networking_findInterfaceInList("wifi1", networkList);
  EXPECT_STREQ(result, "wifi1");
}

TEST(UtilNetworkingTest, test_util_FindInterfaceInList_fail) {
  char networkList[5][IFACE_STR_LENGTH_MAX] = {"eth0", "eth1", "wifi1"};
  const char* result = util_networking_findInterfaceInList("eth5", networkList);
  EXPECT_EQ(result, nullptr);

  result = util_networking_findInterfaceInList("Wifi1", networkList);
  EXPECT_EQ(result, nullptr);

  result = util_networking_findInterfaceInList("", networkList);
  EXPECT_EQ(result, nullptr);
}

TEST(UtilNetworkingTest, test_util_networking_DeallocateInterfaceList_empty) {
  Ipv4Info* list = nullptr;

  util_networking_deallocateInterfaceList(&list);

  EXPECT_EQ(list, nullptr);
}

TEST(UtilNetworkingTest, test_util_networking_DeallocateInterfaceList_success) {
  Ipv4Info* info = util_networking_createInterfaceInfo("eth0");

  Ipv4Info* info1 = util_networking_createInterfaceInfo("eth1");
  info->Next = info1;

  Ipv4Info* info2 = util_networking_createInterfaceInfo("usb0");
  info1->Next = info2;

  util_networking_deallocateInterfaceList(&info);

  EXPECT_EQ(info, nullptr);
}

TEST(UtilNetworkingText, test_util_networking_isDefaultGateway) {
  EXPECT_TRUE(util_networking_isDefaultGateway(DEFAULT_GATEWAY_STR));
  EXPECT_FALSE(util_networking_isDefaultGateway("eth0"));
  EXPECT_FALSE(util_networking_isDefaultGateway("abcdefgh"));
}

TEST(UtilNetworkingText, test_util_networking_getDhcpText) {
  Ipv4Info ipInfoDefaultGateway = {"", "177.3.2.1", "", true, NULL};
  std::strcpy(ipInfoDefaultGateway.Name, DEFAULT_GATEWAY_STR);
  const Ipv4Info ipInfoDhcpClient = {"eth1", "13.1.2.4", "255.255.255.0", true,
                                     NULL};
  const Ipv4Info ipInfoNotDhcpClient = {"wl1", "22.5.6.7", "255.255.255.0",
                                        false, NULL};

  EXPECT_STREQ(util_networking_getDhcpText(&ipInfoDefaultGateway), "");
  EXPECT_STREQ(util_networking_getDhcpText(&ipInfoDhcpClient), "yes");
  EXPECT_STREQ(util_networking_getDhcpText(&ipInfoNotDhcpClient), "no");
}

}  // namespace test_rec_util_networking
