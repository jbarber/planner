#!/bin/sh

PACKAGE="planner-libplanner"
PATH="$PATH:.."

echo "Calling intltool-update for you ..."
intltool-update --gettext-package $PACKAGE $*
