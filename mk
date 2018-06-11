#!/bin/sh
#
# Run make with proper parameters for me...

case "$1" in
"-a"|"--all")
    # Rebuild all
    #
    make -C ../../BUILD
    ;;

"-l"|"--less")
    # Rebuild and use less to ease search within long errors (C++...)
    #
    make -C ../../BUILD/snapwebsites install 2>&1 | less
    ;;

*)
    # Default, just rebuild snapwebsites
    #
    make -C ../../BUILD/snapwebsites install
    ;;

esac

# vim: ts=4 sw=4 et
