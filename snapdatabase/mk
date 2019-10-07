#!/bin/sh
#
# Sample script to run make without having to retype the long path each time
# This will work if you built the environment using our ~/bin/build-snap script

PROCESSORS=`nproc`
PROJECT_PATH=`pwd`
PROJECT_NAME=`basename ${PROJECT_PATH}`

while test ! -z "$1"
do
	case "$1" in
	"-p")
		shift
		PROCESSORS="$1"
		shift
		continue
		;;

	"-v")
		shift
		export VERBOSE=1
		continue
		;;

	esac
	break
done

case $1 in
"-l")
	make -C ../../../BUILD/contrib/${PROJECT_NAME} 2>&1 | less -SR
	;;

"-d")
	if ! test -d doc
	then
		echo "error: no doc available."
		exit 1
	fi
	rm -rf ../../../BUILD/contrib/${PROJECT_NAME}/doc/${PROJECT_NAME}-doc-1.0.tar.gz
	make -j${PROCESSORS} -C ../../../BUILD/contrib/${PROJECT_NAME}
	;;

"-i")
	make -j${PROCESSORS} -C ../../../BUILD/contrib/${PROJECT_NAME} install
	;;

"-t")
	if ! test -d tests
	then
		echo "error: no tests available."
		exit 1
	fi
	(
		if make -j${PROCESSORS} -C ../../../BUILD/contrib/${PROJECT_NAME}
		then
			shift
			../../../BUILD/contrib/${PROJECT_NAME}/tests/unittest --progress $*
		fi
	) 2>&1 | less -SR
	;;

"")
	make -j${PROCESSORS} -C ../../../BUILD/contrib/${PROJECT_NAME}
	;;

*)
	echo "error: unknown command line option \"$1\""
	;;

esac
