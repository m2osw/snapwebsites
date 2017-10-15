# -- NO HASH BANG, WE ADD IT IN THE CODE --
#
# Do the necessary to attach this node to an existing cluster
#

# First we want to stop the existing Cassandra server if we detect that
# it is running
#
# Note: the PID variable in /etc/init.d/cassandra uses $NAME and could
#       change further in future versions so we hard code our path.
#
CASSANDRA_PID=/run/cassandra/cassandra.pid

# If the PID file exists, then we assume that Cassandra is running
#
if test -f $CASSANDRA_PID
then
    # Get the process PID
    #
    PID=`cat $CASSANDRA_PID`
    
    # Request a stop by systemctl
    #
    systemctl stop cassandra
    
    # Block until that process really quits (systemd may "lose its patience!")
    #
    while kill -0 $PID >/dev/null 2>&1
    do
        # sleep on it
        sleep 1
    done
fi

# Delete all the existing data because the default is never compatible with
# the other nodes data
#
rm -rf /var/lib/cassandra/*

# Apply the cluster_name and seeds parameters so we can restart Cassandra
#
sed -i.bak \
    -e "s/^cluster_name: .*/cluster_name: '$BUNDLE_UPDATE_CLUSTER_NAME'/" \
    -e "s/- seeds: \".*\"/- seeds: \"$BUNDLE_UPDATE_SEEDS\"/" \
        /etc/cassandra/cassandra.yaml

# Finally, restart this node, it will then be joined to the cluster
#
systemctl start cassandra

# vim: ts=4 sw=4 et
