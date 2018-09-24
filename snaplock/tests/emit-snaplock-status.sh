#!/bin/sh
#
# The snaplock daemon understands the SIGUSR1 signal. When it happens it
# prints out the current status of the daemon. Namely, the list of computers
# highlighting leaders and the list of tickets (a.k.a. locks.)
#
# You may send a signal to any snaplock at any time, although there is
# correct synchronization so it is not 100% guaranteed to work every time.
#
# This script is to gather the status of all the snaplock on a
# test system running many instances of snaplock. See the test_multi_snaplock
# test for additional information about this.
#

SIGNAL="USR1"

if test "$1" = "-2"
then
    SIGNAL="USR2"
fi


# Print each status, wait one second in between so that way we get them
# separated (because we could do `killall -USR1 snaplock` but then the
# output is all intermigled between all the snaplock instances...)
#
for p in `ps -ef | grep -v java | grep -v keybase | grep -v grep | grep 'snaplock --debug' | awk '{print $2}'`
do
    echo "Process $p"
    kill -s "$SIGNAL" $p
    sleep 1
done

echo "Done. Check your snaplock.log files for details."

# vim: ts=4 sw=4 et
