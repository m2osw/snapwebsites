
ADD NODE TO CLUSTER
===================

One of the functions in the plugin is to add a node ("this" node) to a
Cassandra cluster.

At this time we assume that all nodes run on similar computers so no
special balance is done (all are born equal.)

The following is what we need to do to join the cluster in terms
of Cassandra generation:

Steps

1. Install Cassandra

2. Fix up the topology in:

    cassandra-topology.properties
    cassandra-rackdc.properties (we use this one by default)

3. Make sure the `auto_bootstrap` parameter is not defined in .yaml

4. Make sure the new node `cluster_name` is the same on both nodes

5. Make sure we have the same `endpoint_snitch` on both computers
(we should have `GossipingPropertyFileSnitch` and probably do
not need to worry about it)

6. We ignore `num_tokens` also it should be the same on all machines
(i.e. this is used to balance the amount of data used by a specific
node--i.e. a larger computer can handle more tokens.)

7. The node being cannot be a seed, what we want is the `seed_provider`
from the old nodes (more specifically the `seeds` parameter)

8. Anything else that we would modify needs to be changed in our
new node (more or less we could get the old node cassandra.yaml
file and make a few IP changes and call it a day.)

9. Stop Cassandra if running

10. Delete the existing data (`rm -rf /var/lib/cassandra/*`)

11. Start Cassandra

12. To make sure that the new node is attached to the ring, use:

    nodetool status

13. If attaching multiple new nodes, repeat the process until all the
nodes were added.

14. Once all the nodes were added, run the clean funtion so the data
gets balanced between nodes:

    nodetool cleanup

The cleanup has to run on each old node, but one at a time other you
will completely swamp the Cassandra cluster.

The cleanup can be done at any time, although the sooner the better,
if you have a high load at the moment, wait until that comes down first.
If you always have a high load, well... just do it.


What we really want to do
=========================

1. Grab the `cluster_name` parameter from the existing cluster and use
it in the new node (no need to know whether it's correct, just grab the
correct one.)

2. Get a copy of the `- seeds: ...`  parameter from the specified server
(i.e. we expect the user to give us a point of contact as the name of
a snapcommunicator, i.e. the same as the `server_name=...` parameter.)

3. Stop Cassandra if running

4. Delete the existing data (`rm -rf /var/lib/cassandra/*`)

5. Start Cassandra

The clean up process has to be a separate button because otherwise we
could not add multiple nodes at once. The clean up process has to
clearly tell us when the process is still running and once it is done
on all the Cassandra nodes since it can take a long time to move all
the necessary data out of the way, it could be a very long process...


Implementation Messages
=======================

* **CASSANDRAQUERY**

This message is sent to get information about another node. It can
either be broadcast to all computers or just sent to one specific
computer.

The broadcast is not yet implemented. We just are thinking that we
will want to gather the information from all computers to automatically
know whether your current computer can be attached to an existing
cluster instead of (1) create and (2) asking you the name of another
computer to join.

* **CASSANDRAFIELDS**

This message includes the `cluster_name` and `seeds` information that
we use to properly setup a node to join.

When we receive that message from a joinable node, we can start the
join process by stoping Cassandra, deleting the existing data,
change the fields we just received, and restart:

    systemctl stop cassandra
    rm -rf /var/lib/cassandra/*
    sed -i.bak \
        -e "s/^cluster_name: .*/cluster_name: '$BUNDLE_INSTALLATION_CLUSTER_NAME'/" \
        -e "s/- seeds: \".*\"/- seeds: \"$BUNDLE_INSTALLATION_SEEDS\"/" \
              /etc/cassandra/cassandra.yaml
    systemctl start cassandra

*WARNING: right now we do not first verify whether some cassandra nodes
are busy or not, which is problematic. If any one node is busy, we should
not process a join.*

* **CASSANDRABUSY**

At some point we will want to implement a CASSANDRABUSY so users cannot
attempt to join a cluster that has busy nodes.

A node may be considered busy if a command such as a clean up or repair
is running against it:

    nodetool cleanup
    nodetool repair


_This file is part of the [snapcpp project](http://snapwebsites.org/)._
