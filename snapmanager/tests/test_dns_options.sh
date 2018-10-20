#!/bin/sh -e
#
# Try applying all the DNS options as the bundle does
# see conf/bundle-dns.xml

NAMED_CONF=named.conf.options
DNS_OPTIONS=../../BUILD/snapwebsites/snapmanager/dns/dns_options

if test ! -x $DNS_OPTIONS
then
	echo "$0:error: could not find $DNS_OPTIONS executable"
	exit 1
fi

# We expect to be running from right outside
# The script fails right away if not
#
cp snapmanager/tests/named.conf.options ${NAMED_CONF}

# Run the actual tests
#

# SNAP-522
$DNS_OPTIONS -e 'options.version = none'   ${NAMED_CONF}
$DNS_OPTIONS -e 'options.hostname = none'  ${NAMED_CONF}
$DNS_OPTIONS -e 'options.server-id = none' ${NAMED_CONF}

# SNAP-553
$DNS_OPTIONS -e 'acl[bogusnets]._ = 0.0.0.0/8;'      ${NAMED_CONF}
$DNS_OPTIONS -e 'acl[bogusnets]._ = 192.0.2.0/24;'   ${NAMED_CONF}
$DNS_OPTIONS -e 'acl[bogusnets]._ = 224.0.0.0/3;'    ${NAMED_CONF}

$DNS_OPTIONS -e 'options.blackhole._ = bogusnets;' ${NAMED_CONF}

# SNAP-483
$DNS_OPTIONS -e 'logging.category[security]._ = security_file' ${NAMED_CONF}
$DNS_OPTIONS -e 'logging.channel[security_file].file = "/var/log/named/security.log" versions 3 size 30m' ${NAMED_CONF}
$DNS_OPTIONS -e 'logging.channel[security_file].severity = dynamic' ${NAMED_CONF}
$DNS_OPTIONS -e 'logging.channel[security_file].print-time = yes' ${NAMED_CONF}

# You need bind9 to be installed for this one to work
# It verifies the syntax and availability of various entries
# (i.e. the "bogusnets" ACL entries referenced in the blackhole)
#
named-checkconf ${NAMED_CONF}

