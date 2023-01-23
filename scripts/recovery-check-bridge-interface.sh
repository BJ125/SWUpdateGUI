#!/bin/sh

#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2024 IFM Ecomatic GmbH
#


interface=""

print_help()
{
	echo "Description:"
	echo
	echo "This script checks provided interface bridge status."
	echo "It tells whether the network interface is part of a bridge,"
	echo "or whether it is a bridge."
	echo
	echo "Usage:"
	echo "    $(basename "$0") [OPTION]"
	echo
	echo "Options:"
	echo "    --interface     - interface to check"
	echo
	echo "Output:"
	echo "    bridge [interface]: when interface is itself a bridge"
	echo "    non-bridge [interface]: when interface is not part of a bridge"
	echo "    part-of-bridge [interface]: when interface is part of bridge"
	echo
	echo "Return values:"
	echo "    0 when interface is itself a bridge or not part of bridge"
	echo "    1 when interface is part of bridge"
	echo "    2 when the operation could not be performed (either invalid interface,"
	echo "      or invalid arguments passed to the script, or the operation failed)"
	echo

	exit 2
}

parse_and_verify_commandline_options()
{
	if [ $# -eq 0 ]
	then
		print_help
	fi

	while [ $# -gt 0 ]
	do
		key=${1%=*}
		value=${1#"$key"=}
		if [ -z "$value" ]
		then
			echo "Value must be provided for $key"
			echo
			print_help
		fi

		case $key in
			--interface)
				interface=$value
				;;
			*)
				echo "Unknown option $key"
				echo
				print_help
				;;
		esac

		shift;
	done
}

network_interface_is_invalid()
{
	echo "$interface is invalid network interface"

	exit 2
}

network_interface_is_bridge()
{
	echo "bridge $interface"

	exit 0	
}

network_interface_is_part_of_bridge()
{
	echo "part-of-bridge $interface"

	exit 1
}

network_interface_is_not_bridge()
{
	echo "non-bridge $interface"

	exit 0
}


# main program starts here

parse_and_verify_commandline_options "$@"


# check if the interface a network interface at all
cmd_output=$(ip addr show "$interface" 2> /dev/null) || network_interface_is_invalid


# check if the interface is a bridge
brctl show "$interface" > /dev/null 2>&1 && network_interface_is_bridge

# since it is not a bridge, decide if it is part of a bridge or if it is a non-bridge
if ( echo "$cmd_output" | grep -q " master " )
then
	network_interface_is_part_of_bridge
else
	network_interface_is_not_bridge
fi

