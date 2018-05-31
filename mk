#!/bin/sh
#
# Run make with proper parameters for me...

case "$1" in
"-l")
    make -C ../../BUILD/snapwebsites install 2>&1 | less
    ;;

*)
    make -C ../../BUILD/snapwebsites install
    ;;

esac

# vim: ts=4 sw=4 et
