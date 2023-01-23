/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains utility functions for network settings
 *
 */

#include "rec-util-networking.h"

#include "rec-util-system.h"

#include <lvgl/lvgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>

static const char *SCRIPT_SET_GATEWAY =
	"/usr/bin/recovery-edit-default-gateway.sh";
static const char *SCRIPT_SET_INTERFACE_DHCP = "/usr/bin/recovery-set-dhcp.sh";
static const char *SCRIPT_SET_INTERFACE_STATIC =
	"/usr/bin/recovery-set-static.sh";
static const char *SCRIPT_GET_DHCP_STATUS =
	"/usr/bin/recovery-get-dhcp-status.sh";
static const char *SCRIPT_CHECK_BRIDGE_INTERFACE =
	"/usr/bin/recovery-check-bridge-interface.sh";

static const char *ROUTES_FILE = "/proc/net/route";

const char *DEFAULT_GATEWAY_STR = "default-gateway";

static bool util_networking_isDHCP(const char *Address)
{
	char Command[256];

	if (0 == strcmp(Address, "")) {
		return false;
	}

	snprintf(Command, sizeof(Command) - 1, "%s --address=%s",
		 SCRIPT_GET_DHCP_STATUS, Address);
	Command[sizeof(Command) - 1] = '\0';

	const int ret = util_system_executeScript(Command);

	util_system_checkIfReturnValueValid(Command, ret, 0, 1);

	const bool res = (0 == ret);

	return res;
}

static void util_networking_parseRoute(char *Line, char *Gateway)
{
	const char *Delim = "\t";
	int Index = 0;
	char *Token = NULL;
	char Destination[16];
	bool GatewayFound = false;

	Token = strtok(Line, Delim);

	while ((NULL != Token) && (!GatewayFound)) {
		if (0 != strcmp("", Token)) {
			if (1 == Index) {
				strncpy(Destination, Token,
					sizeof(Destination) - 1);
				Destination[sizeof(Destination) - 1] = '\0';
			} else if (2 == Index) {
				if (0 == strcmp("00000000", Destination)) {
					strncpy(Gateway, Token, IFACE_ADDR_MAX);
				}
				GatewayFound = true;
			}

			Index++;
		}
		Token = strtok(NULL, Delim);
	}
}

static bool util_networking_isInterfaceABridgePort(const char *Interface)
{
	char Command[256];

	if (0 == strcmp(Interface, "")) {
		LV_LOG_ERROR(
			"Error executing script to get interface bridge status; no interface provided");
		exit(-1);
	}

	snprintf(Command, sizeof(Command) - 1, "%s --interface=%s",
		 SCRIPT_CHECK_BRIDGE_INTERFACE, Interface);
	Command[sizeof(Command) - 1] = '\0';

	const int ret = util_system_executeScript(Command);

	util_system_checkIfReturnValueValid(Command, ret, 0, 1);

	const bool res = (1 == ret);

	return res;
}

/**
 * Create interface-info object with provided name, default address and netmask.
 *
 * @param[in] InterfaceName Name of interface
 *
 * @returns interface info object. When done with it, it has to be free()
 */
struct Ipv4Info *util_networking_createInterfaceInfo(const char *InterfaceName)
{
	struct Ipv4Info *Info = malloc(sizeof(struct Ipv4Info));
	if (NULL == Info) {
		LV_LOG_ERROR("Unable to allocate memory!");
		exit(-1);
	}

	memset(Info, 0, sizeof(struct Ipv4Info));

	strncpy(Info->Name, InterfaceName, sizeof(Info->Name) - 1);
	Info->Name[sizeof(Info->Name) - 1] = '\0';

	return Info;
}

/**
 * Set address in interface-info for IPV4 ifaddrs entry
 *
 * @param[in,out] Info Pointer to interface-info object
 * @param[in] CurrentIface ifaddrs object
 */
void util_networking_setAddressInInterfaceInfo(
	struct Ipv4Info *Info, const struct ifaddrs *CurrentIface)
{
	char AddressBuffer[INET_ADDRSTRLEN];
	void *TmpAddrPtr = NULL;

	if (AF_INET == CurrentIface->ifa_addr->sa_family) {
		TmpAddrPtr = &((struct sockaddr_in *)CurrentIface->ifa_addr)
				      ->sin_addr;
		const char *Output = inet_ntop(AF_INET, TmpAddrPtr,
					       AddressBuffer, INET_ADDRSTRLEN);

		if ((NULL != Output) &&
		    (0 != strcmp("0.0.0.0", AddressBuffer))) {
			strncpy(Info->Address, AddressBuffer,
				sizeof(Info->Address) - 1);
			Info->Address[sizeof(Info->Address) - 1] = '\0';
		}
	}
}

/**
 * Set netmask in interface-info
 *
 * @param[in,out] Info Pointer to interface-info
 * @param[in] CurrentIface ifaddrs object
 */
void util_networking_setNetmaskInInterfaceInfo(
	struct Ipv4Info *Info, const struct ifaddrs *CurrentIface)
{
	char NetmaskBuffer[INET_ADDRSTRLEN];
	void *TmpAddrPtr = NULL;

	if (NULL != CurrentIface->ifa_netmask) {
		TmpAddrPtr = &((struct sockaddr_in *)CurrentIface->ifa_netmask)
				      ->sin_addr;
		const char *Output = inet_ntop(AF_INET, TmpAddrPtr,
					       NetmaskBuffer, INET_ADDRSTRLEN);

		if ((NULL != Output) &&
		    (0 != strcmp("0.0.0.0", NetmaskBuffer))) {
			strncpy(Info->Netmask, NetmaskBuffer,
				sizeof(Info->Netmask) - 1);
			Info->Netmask[sizeof(Info->Netmask) - 1] = '\0';
		}
	}
}

/**
 * Convert a hex value to dotted IP address
 * Example: "FE01A8C0" > "192.168.1.254"
 *
 * @param[in,out] Buffer Input buffer containing hex value
 * @param[in] Size buffer size
 *
 * @return 0 for success, -1 for failure
 */
int util_networking_convertIPAddressToDotFormat(char *Buffer, unsigned int Size)
{
	unsigned int A = 0;
	unsigned int B = 0;
	unsigned int C = 0;
	unsigned int D = 0;

	if (sscanf(Buffer, "%2x%2x%2x%2x", &A, &B, &C, &D) != 4) {
		return -1;
	}

	snprintf(Buffer, Size, "%u.%u.%u.%u", D, C, B, A);
	return 0;
}

/**
 * Get default gateway information
 *
 * @return GatewayInfo Structure with default gateway IP address
 */
struct Ipv4Info util_networking_getDefaultGatewayInfo(void)
{
	struct Ipv4Info GatewayInfo;
	memset(&GatewayInfo, 0, sizeof(struct Ipv4Info));

	strncpy(GatewayInfo.Name, DEFAULT_GATEWAY_STR,
		sizeof(GatewayInfo.Name) - 1);
	GatewayInfo.Name[sizeof(GatewayInfo.Name) - 1] = '\0';

	FILE *RouteFile = fopen(ROUTES_FILE, "r");
	char Line[256];

	if (NULL == RouteFile) {
		LV_LOG_WARN("Unable to open %s; error code: %d\n", ROUTES_FILE,
			    errno);
	} else {
		while (fgets(Line, sizeof(Line), RouteFile) &&
		       (0 == strcmp("", GatewayInfo.Address))) {
			util_networking_parseRoute(Line, GatewayInfo.Address);
		}

		if ((0 != util_networking_convertIPAddressToDotFormat(
				  GatewayInfo.Address,
				  sizeof(GatewayInfo.Address))) ||
		    (0 == strcmp(GatewayInfo.Address, "0.0.0.0"))) {
			strncpy(GatewayInfo.Address, "",
				sizeof(GatewayInfo.Address));
		}
	}

	fclose(RouteFile);

	return GatewayInfo;
}

/**
 * Count continuous 1-bit from MSB in number
 *
 * @param[in] Value Input value
 *
 * @return Count of continuous bits from MSB; -1 if 1-bits are not continuous
 */
int util_networking_countContinuous1Bits(uint32_t Value)
{
	unsigned int Count = 0;

	while (0 < Value) {
		unsigned int Bit = Value & 0x80000000;

		if (0 == Bit) {
			return -1;
		}

		++Count;
		Value <<= 1;
	}
	return Count;
}

/**
 * Function to convert dotted IP-address & Netmask to cidr format
 *
 * @param[in] Address IP Address in aaa.aaa.aaa.aaa format
 * @param[in] Netmask Netmask in nnn.nnn.nnn.nnn format
 * @param[out] Output CIDR notation output in aaa.aaa.aaa.aaa/nnn format
 * @param[in] OutputLenMax Maximum length of output buffer
 *
 * @return -1 for error, 0 for success
 */
int util_networking_convertToCidr(const char *Address, const char *Netmask,
				  char *Output, const int OutputLenMax)
{
	uint32_t Ipbytes[4] = { 0, 0, 0, 0 };

	sscanf(Netmask, "%u.%u.%u.%u", &Ipbytes[3], &Ipbytes[2], &Ipbytes[1],
	       &Ipbytes[0]);

	uint32_t Value = Ipbytes[0];
	Value += Ipbytes[1] << 8;
	Value += Ipbytes[2] << 16;
	Value += Ipbytes[3] << 24;

	const int Cidr = util_networking_countContinuous1Bits(Value);
	if (Cidr < 0) {
		LV_LOG_ERROR("Invalid netmask detected: %s", Netmask);
		return -1;
	}

	char NetmaskHalf[IFACE_ADDR_MAX];
	snprintf(NetmaskHalf, sizeof(NetmaskHalf) - 1, "/%d", Cidr);

	strncpy(Output, Address, OutputLenMax - 1);
	strcat(Output, NetmaskHalf);
	Output[OutputLenMax - 1] = '\0';

	return 0;
}

/**
 * Function to reconfigure a network interface with static configuration.
 *
 * @param[in] Interface Name of network interface
 * @param[in] Address IP Address(aaa.aaa.aaa.aaa)
 * @param[in] Netmask Netmask(nnn.nnn.nnn.nnn)
 *
 * @return true for success, false for failure
 */
bool util_networking_setStaticConfiguration(const char *Interface,
					    const char *Address,
					    const char *Netmask)
{
	char CidrAddress[SETTING_STR_LENGTH_MAX];
	if (0 != util_networking_convertToCidr(Address, Netmask, CidrAddress,
					       sizeof(CidrAddress))) {
		return false;
	}

	char Command[2048];
	snprintf(Command, sizeof(Command) - 1, "%s --interface=%s --ip=%s",
		 SCRIPT_SET_INTERFACE_STATIC, Interface, CidrAddress);
	Command[sizeof(Command) - 1] = '\0';

	const int ret = util_system_executeScript(Command);

	util_system_checkIfReturnValueValid(Command, ret, 0, 1);

	const bool res = (0 == ret);

	return res;
}

/**
 * Function to reconfigure a network interface as a dhcp client.
 *
 * @param[in] Interface Name of network interface or "default-gateway" for setting gateway
 *
 * @return true for success, false for failure
 */
bool util_networking_reconfigureAsDhcpClient(const char *Interface)
{
	char Command[2048];

	snprintf(Command, sizeof(Command) - 1, "%s --interface=%s",
		 SCRIPT_SET_INTERFACE_DHCP, Interface);
	Command[sizeof(Command) - 1] = '\0';

	const int ret = util_system_executeScript(Command);

	util_system_checkIfReturnValueValid(Command, ret, 0, 1);

	const bool res = (0 == ret);

	return res;
}

/**
 * Delete gateway
 *
 * @return true for success, false for failure
 */
bool util_networking_deleteGateway(void)
{
	char command[256];

	snprintf(command, sizeof(command) - 1, "%s --action=delete",
		 SCRIPT_SET_GATEWAY);
	command[sizeof(command) - 1] = '\0';

	const int ret = util_system_executeScript(command);

	util_system_checkIfReturnValueValid(command, ret, 0, 1);

	const bool res = (0 == ret);

	return res;
}

/**
 * Set gateway address
 *
 * @param[in] Address Gateway address
 *
 * @return true for success, false for failure
 */
bool util_networking_setGatewayAddress(const char *Address)
{
	char command[256];

	snprintf(command, sizeof(command) - 1, "%s --action=set --ip=%s",
		 SCRIPT_SET_GATEWAY, Address);
	command[sizeof(command) - 1] = '\0';

	const int ret = util_system_executeScript(command);

	util_system_checkIfReturnValueValid(command, ret, 0, 1);

	const bool res = (0 == ret);

	return res;
}

/**
 * Search for interface in ipv4Info array
 *
 * @param[in] Interface Name of interface to find
 * @param[in] InterfaceList Array of network-interfaces received from getifaddrs
 *
 * @return pointer to Ipv4Info object if found, NULL otherwise
 */
struct Ipv4Info *util_networking_searchInterface(const char *Interface,
						 struct Ipv4Info *InterfaceList)
{
	struct Ipv4Info *It = InterfaceList;

	while (NULL != It) {
		if (0 == strcmp(It->Name, Interface)) {
			return It;
		}
		It = It->Next;
	}
	return NULL;
}

/**
 * Search for interface in list
 *
 * @param[in] Interface Interface to be searched
 * @param[in] List Interfaces list parsed from configuration-entry
 *
 * @return interface if found, else NULL
 */
const char *
util_networking_findInterfaceInList(const char *Interface,
				    char List[][IFACE_STR_LENGTH_MAX])
{
	char *It = &List[0][0];
	char *End = (It + IFACE_COUNT_MAX * IFACE_STR_LENGTH_MAX);

	while ((It != End) && (0 != strcmp(It, ""))) {
		if (0 == strcmp(It, Interface)) {
			return It;
		}
		It += IFACE_STR_LENGTH_MAX;
	}
	return NULL;
}

/**
 * Deallocate a network-info linked list
 *
 * @param[in|out] InterfacesPtr Address of linked-list start node
 */
void util_networking_deallocateInterfaceList(struct Ipv4Info **InterfacesPtr)
{
	struct Ipv4Info *Info = *InterfacesPtr;

	while (NULL != Info) {
		struct Ipv4Info *Tmp = Info;
		Info = Info->Next;

		free(Tmp);
	}
	*InterfacesPtr = NULL;
}

/**
 * Create interface list of applicable interfaces with detailed info.
 *
 * @param[in] ConfigInterfaces comma separated list of network-interfaces
 * @param[out] InterfaceCount Number of interfaces
 * @param[out] InterfaceList Address of interface-list linked list
 *
 * @note the list of interfaces has to be released with the util_networking_deallocateInterfaceList()
 */
void util_networking_createInterfaceList(const char *ConfigInterfaces,
					 unsigned int *InterfaceCount,
					 struct Ipv4Info **InterfacesPtr)
{
	struct ifaddrs *IfAddrs = NULL;
	struct Ipv4Info **It = InterfacesPtr;

	char NetworkList[IFACE_COUNT_MAX][IFACE_STR_LENGTH_MAX];
	memset(NetworkList, 0, sizeof(NetworkList));

	util_config_parseInterfaces(ConfigInterfaces, NetworkList);

	const int Status = getifaddrs(&IfAddrs);
	if (-1 == Status) {
		LV_LOG_WARN("Failed to get network information. Errorcode: %d",
			    errno);
		return;
	}

	util_networking_deallocateInterfaceList(InterfacesPtr);

	struct ifaddrs *CurrentIface = IfAddrs;
	*InterfaceCount = 0;
	while (NULL != CurrentIface) {
		const char *Interface = util_networking_findInterfaceInList(
			CurrentIface->ifa_name, NetworkList);

		if (NULL != Interface) {
			if (!util_networking_isInterfaceABridgePort(
				    Interface)) {
				struct Ipv4Info *Result =
					util_networking_searchInterface(
						Interface, *InterfacesPtr);
				if (NULL == Result) {
					Result =
						util_networking_createInterfaceInfo(
							CurrentIface->ifa_name);
					*It = Result;
					It = &Result->Next;
					++(*InterfaceCount);
				}
				util_networking_setAddressInInterfaceInfo(
					Result, CurrentIface);
				util_networking_setNetmaskInInterfaceInfo(
					Result, CurrentIface);
				Result->IsDhcp =
					util_networking_isDHCP(Result->Address);
			}
		}
		CurrentIface = CurrentIface->ifa_next;
	}

	freeifaddrs(IfAddrs);
}

/**
 * Check whether provided interface is the default gateway or not
 *
 * @param[in] Interface Name of interface
 *
 * @return true if the default gateway, otherwise false
 */
bool util_networking_isDefaultGateway(const char *Interface)
{
	if (0 == strcmp(Interface, DEFAULT_GATEWAY_STR)) {
		return true;
	}
	return false;
}

/**
 * This function returns the string for the dhcp field.
 *
 * @param[in] IpInfo pointer to the structure holding the information about the network interface
 *
 * @return empty string when it is the default gateway, "yes" when the network interface is a dhcp
 *         client, "no" if the network client is not a dhcp client
 * */
const char *util_networking_getDhcpText(const struct Ipv4Info *IpInfo)
{
	if (util_networking_isDefaultGateway(IpInfo->Name)) {
		return "";
	} else {
		if (IpInfo->IsDhcp) {
			return "yes";
		} else {
			return "no";
		}
	}
}
