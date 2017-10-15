#!/bin/sh
#
# This script is intended for development use only, NOT TO BE
# USED ON A PRODUCTION SERVER! We recommend a full backup of your
# database BEFORE running this script! 
#
# To run the script, specify the DIST prefix (/usr), and the configuration
# file (optional, defaults to /etc/snapwebsites/snapserver.conf).
#
if [ -z "$1" ]
then
    echo "usage: $0 distdir [config-file]"
    exit 1
fi

CONFIGFILE=/etc/snapwebsites/snapserver.conf
if [ -n "$2" ]
then
    CONFIGFILE=$2
fi

echo
echo "Resetting your Snap! database..."
echo
DISTDIR=$1
BINDIR=${DISTDIR}/bin
LAYOUTDIR=${DISTDIR}/share/snapwebsites/layouts
${BINDIR}/snapdb --drop-tables
${BINDIR}/snaplayout ${LAYOUTDIR}/bare/bare_layout.zip
${BINDIR}/snaplayout ${LAYOUTDIR}/white/white_layout.zip
${BINDIR}/snapserver --no-log -c ${CONFIGFILE} --add-host

# TODO: add ability in snapserver to init the website from command-line
#       the library now has that capability, we need a command line
#       tool to run it, but it cannot really be here for websites, only
#       for the sites table... (not much help really!) and the server
#       needs to be running by itself (without backends)

# To setup the bare theme
# (but we cannot run it at this point yet because the database was not
# fully initialized to support such.)
#${BINDIR}/snaplayout --set-theme http://www.example.com layout '"bare";'
#${BINDIR}/snaplayout --set-theme http://www.example.com theme '"bare";'

echo
echo "Done!"
echo
echo "After a reset, remember that all your data is lost. So you will"
echo "have to register a new user and make him root again with a command"
echo "that looks like this (change the -c and -p parameters as required):"
echo ${BINDIR}/snapbackend -d -c ${CONFIGFILE} -a permissions::makeadministrator -p ROOT_USER_EMAIL=admin@example.com
echo ${BINDIR}/snapbackend -d -c ${CONFIGFILE} -a permissions::makeroot -p ROOT_USER_EMAIL=you@example.com

# vim: ts=4 sw=4 et nocindent
