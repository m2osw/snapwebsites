#!/bin/sh
#
SNAPFLAGS="--detach --binary-path ./bin --logfile snapinit.log -c my_server.conf --all"

if [ -z "$1" ]
then
	echo "usage $0 [start|stop|restart]"
	exit 1
fi

./bin/snapinit ${SNAPFLAGS} $1

