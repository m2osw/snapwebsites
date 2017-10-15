#!/bin/sh

# This script is used partially as a debug opportunity of the Cassandra
# cluster you are running; if you load all the data from a database, it
# gives you the opportunity to find a table.row.cell that has problems.

set -e

HOST=127.0.0.1
COUNT=1000
SNAPDB="../BUILD/snapwebsites/src/snapdb --host $HOST --count $COUNT"

tables=`$SNAPDB`

for t in $tables
do
	echo "***"
	echo "*** Table: $t"
	echo "***"

	rows=`$SNAPDB $t`
	for r in $rows
	do
		echo "-- Row: $r"

		# in most cases problems will occur here
		$SNAPDB $t $r || exit 1
	done
done

