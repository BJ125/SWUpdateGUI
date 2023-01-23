#!/bin/sh

#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2024 IFM Ecomatic GmbH
#


action=""
gateway_address=""

print_help()
{
	echo "Description:"
	echo "    This script can be used to set or clear the default gateway."
	echo
	echo "Usage:"
	echo "    $(basename "$0") [OPTION]"
	echo
	echo "Options:"
	echo "    --action=[set, delete]"
	echo "        'set'    - to set the default gateway. The address has to be"
	echo "                   passed to the '--ip' option"
	echo "        'delete' - to delete the default gateway"
	echo "    --ip"
	echo "        The address to set to the default gateway"
	echo
	echo "Return values:"
	echo "    0 when the function succeed"
	echo "    1 when the function fails"
	echo "    2 for invalid options passed to the script"
	echo
	echo "Examples:"
	echo "    $(basename "$0") --action=set --ip=1.2.3.4"
	echo "    $(basename "$0") --action=delete"
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
			echo "$key empty value"
			echo
			print_help
		fi

		case $key in
			--action)
				action=$value
				;;
			--ip)
				gateway_address=$value
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


# main program starts here

parse_and_verify_commandline_options "$@"

if [ "$action" =  "delete" ];
then

	# delete default gateway
	# It will fail if the gateway is not set, but the "ip" will not return 1 in that case
	ip route delete default || exit 1

elif [ "$action" =  "set" ];
then

	# set default gateway
	ip route replace default via "$gateway_address" || exit 1

else

	echo "Action not supported: $action"
	exit 2

fi

