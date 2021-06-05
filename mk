#!/bin/sh
#
# See the snapcmakemodules project for details about this script
#     https://github.com/m2osw/snapcmakemodules

if test -x ../cmake/scripts/mk
then
	../cmake/scripts/mk $*
else
	echo "error: could not locate the cmake mk script"
	exit 1
fi

#"-a"|"--all")
#    # Rebuild all
#    #
#    shift
#    make -j${PROCESSORS} -C ../../BUILD $*
#    ;;
#
#"-p"|"--packages")
#    # Run a dput to generate all the packages on launchpad
#    #
#    #make -C ../../BUILD dput
#    echo "Please use ../bin/send-to-launchpad.sh instead"
#    ;;
