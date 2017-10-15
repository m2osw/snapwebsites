#!/bin/sh
#
# Force a list to be reprocessed

# $1 is the protocol and domain name
# $2 is the URL of a page to add to a list
../BUILD/snapwebsites/src/snapbackend $1 --action processlist --param URL=$2

