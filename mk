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

"-p"|"--packages")
    # Run a dput to generate all the packages on launchpad
    #
    make -C ../../BUILD dput
    ;;

"-v"|"--verbose")
    # Rebuild snapwebsites with verbosity ON
    #
    VERBOSE=1 make -C ../../BUILD/snapwebsites install
    ;;

"-r"|"--release")
    # Rebuild the release version
    #
    make -C ../../RELEASE/snapwebsites install
    ;;

"")
    # Default, just rebuild snapwebsites
    #
    make -C ../../BUILD/snapwebsites install
    ;;

*)
    echo "error: unknown command line option $1"
    exit 1
    ;;

esac

# vim: ts=4 sw=4 et
