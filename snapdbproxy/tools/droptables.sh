#!/bin/sh
#
# WARNING: This script is VERY SLOW. Deleting all the tables is slower that
#          recreating the database from scratch (i.e. stop Cassandra, delete
#          all the data under /var/lib/cassandra/* and then resstarting
#          Cassandra); however, it keeps the "domains" and "websites" tables
#          around meaing that you do not need to use snapmanager to define
#          those entries again
#

# Make sure that snapdb was installed
if ! hash snapdb
then
    echo "error: could not find \"snapdb\" on this computer. Was it installed?"
    exit 1
fi

HOST="$1"

if test -z "$HOST"
then
    echo "error: you must specific the host that snapdb has to use to access the database (i.e. 127.0.0.1)"
    exit 1
fi

echo "Reset all the tables so you can try to reinstall a website."
echo
echo "WARNING: This command ***DESTROYS ALL YOUR DATA***"
echo "WARNING: Normally, only programmers want to run this command."
echo "WARNING: Please say that you understand by entering:"
echo "WARNING:     I know what I'm doing"
echo "WARNING: and then press enter."
echo
read -p "So? " answer

if test "$answer" != "I know what I'm doing"
then
    echo "error: the database destruction was canceled."
    exit 1
fi

echo "***"
echo "*** Note that this process is so slow that you are likely to get"
echo "*** various types of timeout errors."
echo "***"

# TODO: gather the list of tables using snapdb and remove "domains"
#       and "websites" from that list

for name in antihammering backend branch cache content emails epayment_paypal \
            epayment_stripe files firewall layout links list listref \
            mailinglist password processing revision secret serverstats \
            sessions shorturl sites test_results tracker users
do
    echo "Deleting $name..."
    snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing "$name"
done

# We do not delete domains and websites so we do not have to setup those
# again...
#snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing domains
#snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing websites

echo "Done."
echo
echo "WARNING: if you had \"snapdbproxy\" running before the deletion"
echo "         remember that it caches the Meta Data. That means it will"
echo "         believe the tables are still there until restarted."
echo
echo "Then later reinstall the websites with:"
echo "1. restart snapdbproxy to create the context and empty tables"
echo
echo "2. reinstall websites with:"
echo "         snapinstallwebsite --domain <your-[sub-]domain> --port <80|443>"
echo

# vim: ts=4 sw=4 et
