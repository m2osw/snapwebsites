#!/bin/sh
#
# Run make with proper parameters for me...

PROCESSORS=`nproc`

case "$1" in
"-a"|"--all")
    # Rebuild all
    #
    shift
    make -j${PROCESSORS} -C ../../BUILD $*
    ;;

"-l"|"--less")
    # Rebuild and use less to ease search within long errors (C++...)
    #
    make -C ../../BUILD/snapwebsites install 2>&1 | less -R
    ;;

"-p"|"--packages")
    # Run a dput to generate all the packages on launchpad
    #
    #make -C ../../BUILD dput
    echo "Please use ../bin/send-to-launchpad.sh instead"
    ;;

"-v"|"--verbose")
    # Rebuild snapwebsites with verbosity ON
    #
    VERBOSE=1 make -C ../../BUILD/snapwebsites install
    ;;

"-r"|"--release")
    # Rebuild the release version
    #
    make -j${PROCESSORS} -C ../../RELEASE/snapwebsites install
    ;;

"")
    # Default, just rebuild snapwebsites
    #
    make -j${PROCESSORS} -C ../../BUILD/snapwebsites install | grep -v " Up-to-date: "

    # The following is a bit better as it does not print out all the
    # installation stuff, but it does the first part twice which is
    # taking too long
    #if make -C ../../BUILD/snapwebsites
    #then
    #    # It's important to install for various parts so we run that too
    #    # but we send the output to a log because it's just way too much
    #    # ("unfortunately" this goes through the "make sure things are
    #    # up to date" before it does the installation itself.)
    #    #
    #    echo "-- Installing Now (output saved in ../../tmp/install.log)"
    #    mkdir -p ../../tmp
    #    make -C ../../BUILD/snapwebsites install > ../../tmp/install.log
    #fi
    ;;

*)
    echo "error: unknown command line option $1"
    exit 1
    ;;

esac

# vim: ts=4 sw=4 et
