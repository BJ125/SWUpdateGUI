/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains utility functions for network settings
 *
 */

#ifndef REC_UTIL_NETWORKING_H
#define REC_UTIL_NETWORKING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rec-util-config.h"

#include <stdint.h>
#include <stdbool.h>

extern const char *DEFAULT_GATEWAY_STR;

struct ifaddrs;

struct Ipv4Info {
	char Name[IFACE_STR_LENGTH_MAX];
	char Address[IFACE_ADDR_MAX];
	char Netmask[IFACE_ADDR_MAX];
	bool IsDhcp;
	struct Ipv4Info *Next;
};

struct Ipv4Info *util_networking_createInterfaceInfo(const char *InterfaceName);

void util_networking_setAddressInInterfaceInfo(
	struct Ipv4Info *Info, const struct ifaddrs *CurrentIface);

void util_networking_setNetmaskInInterfaceInfo(
	struct Ipv4Info *Info, const struct ifaddrs *CurrentIface);

struct Ipv4Info util_networking_getDefaultGatewayInfo(void);

int util_networking_convertToCidr(const char *Address, const char *Netmask,
				  char *Output, const int OutputLenMax);

bool util_networking_setStaticConfiguration(const char *Interface,
					    const char *Address,
					    const char *Netmask);
bool util_networking_reconfigureAsDhcpClient(const char *Interface);

bool util_networking_deleteGateway(void);

bool util_networking_setGatewayAddress(const char *Address);

int util_networking_convertIPAddressToDotFormat(char *Buffer,
						unsigned int Size);

int util_networking_countContinuous1Bits(uint32_t Value);

struct Ipv4Info *util_networking_searchInterface(const char *Interface,
						 struct Ipv4Info *Ipv4Info);

void util_networking_deallocateInterfaceList(struct Ipv4Info **InterfacesPtr);

void util_networking_createInterfaceList(const char *ConfigInterfaces,
					 unsigned int *InterfaceCount,
					 struct Ipv4Info **InterfacesPtr);

const char *
util_networking_findInterfaceInList(const char *Interface,
				    char NetworkList[][IFACE_STR_LENGTH_MAX]);

bool util_networking_isDefaultGateway(const char *Interface);
const char *util_networking_getDhcpText(const struct Ipv4Info *IpInfo);

#ifdef __cplusplus
}
#endif

#endif /* REC_UTIL_NETWORKING_H */
