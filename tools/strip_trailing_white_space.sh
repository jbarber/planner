#!/bin/sh

if [ "$1" == "--help" ] || [ "$2" != "" ]; then
	echo "Usage: $0 [options]"
  echo ""
	echo "This script searches the current directory and all directories below it"
	echo "for *.c and *.h files and will remove any trailing white space."
	echo ""
	echo "Options:"
	echo "  -c      Only check for and report trailing white space"
	echo "  --help  Show usage information"
	exit -1;
elif [ "$1" == "-c" ]; then
	echo "Searching for trailing white space..."
	find . -name \*.[ch] -exec grep -nEH "[ 	]+$" {} \;
	echo "done."
else
	echo "This script searches the current directory and all directories below it"
	echo "for *.c and *.h files and will remove any trailing white space."
	echo ""
	echo -n "Are you sure you want to remove trailing white space (y/n)? "
	read answer
	if [ "$answer" == "y" ]; then
		echo "Removing trailing white space..."
		find . -name \*.[ch] -exec sed -i 's/[ \t]\+$//' {} \;
		echo "done."
	else
		echo "Cancelled."
	fi
fi

