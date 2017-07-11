#!/bin/sh

case "$1" in
	"get")
		if [ "$#" -ne "4" ]; then
			echo "Usage: store get <user> <app> <tag>"
			exit 1
		fi
		curl -v "http://localhost:9001/api/ll-store/get?username=$2&appname=$3&tagname=$4&token="
		;;
	"set")
		if [ "$#" -ne "5" ]; then
			echo "Usage: store get <user> <app> <tag> <value>"
			exit 1
		fi
		curl -v "http://localhost:9001/api/ll-store/set?username=$2&appname=$3&tagname=$4&value=$5&token="
		;;
	*)
		echo "Usage: store <action> <user> <app> <tag> <value>"
		echo "    Action can be 'get' or 'set'."
		;;
esac

