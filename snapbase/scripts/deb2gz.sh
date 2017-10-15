#!/bin/sh -e
#
# Some debian tools do not support the .xz compression scheme.

SOURCE=`pwd`
rm -rf /tmp/deb2gz /tmp/gzdebs
mkdir -p /tmp/deb2gz /tmp/gzdebs
cd /tmp/deb2gz
while test ! -z "$1"
do
	rm -f *
	if test -f "$SOURCE/$1"
	then
		ar x "$SOURCE/$1"
	else
		ar x "$1"
	fi

	if test -f data.tar.xz
	then
		unxz -c data.tar.xz | gzip -9 >data.tar.gz
		rm data.tar.xz
	fi

	ar rc /tmp/gzdebs/`basename $1` *

	shift
done

if test -x /home/alexis/m2osw/unigw/BUILD/wpkg/tools/deb2graph
then
	# remove the -doc files, not really useful here
	rm -f /tmp/gzdebs/*-doc*

	# output will be in current directory as deb2graph.dot and deb2graph.svg...
	/home/alexis/m2osw/unigw/BUILD/wpkg/tools/deb2graph /tmp/gzdebs/*.deb
fi
