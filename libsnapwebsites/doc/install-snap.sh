#!/bin/sh
#
# Run with sudo as in:
#   sudo sh snap.sh
#
# Please check image snapwebsites/doc/snapwebsites-cluster-setup.png
# for an idea of a complete installation cluster; everything can be
# setup on a single computer, but the power of the Snap! CMS is to run
# on any number of computers to really make it FAST.
#

##
## Apache2 installation
##
# Apache should only be installed on front end computers
if ! test -f /etc/apache2
then
	apt-get install apache2
fi

##
## Cassandra sources (no PPA at this point)
##
# Cassandra should only be installed on Cassandra nodes
add-apt-repository 'deb http://www.apache.org/dist/cassandra/debian 20x main'
# Ad this point the source generates an error in 13.10
#add-apt-repository 'deb-src http://www.apache.org/dist/cassandra/debian 20x main'
gpg --keyserver pgp.mit.edu --recv-keys F758CE318D77295D
gpg --export --armor F758CE318D77295D | sudo apt-key add -
gpg --keyserver pgp.mit.edu --recv-keys 2B5C1B00
gpg --export --armor 2B5C1B00 | sudo apt-key add -

##
## Snap! CMS PPA
##
# snapcgi should be installed along Apache2 so on the front end computers
# snapserver should only be installed on Snap! Server nodes
add-apt-repository ppa:snapcpp/ppa

##
## Update apt-get database
##
# Since we added new sources we have to run a full update first
apt-get update


##
## Install Cassandra and Snap! CMS
##
# Now install all the elements we want to get
apt-get install cassandra snapserver snapcgi

# Automatically installed if not yet available:
#
# Standard libraries and tools
#
#  . liblog4cplus
#  . libqt4-bus
#  . libqt4-network
#  . libqt4-script
#  . libqt4-xml
#  . libqt4-xmlpatterns
#  . libqtcore4
#  . qdbus
#  . qtchooser
#
# Snap! CMS libraries and tools
#
#  . libqtcassandra
#  . libqtserialization
#  . libsnapwebsites
#  . libtld
#  . snapcgi
#  . snapwebserver
#  . snapserver-core-plugins


