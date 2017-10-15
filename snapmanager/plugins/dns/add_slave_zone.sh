#!/bin/bash

if [ "$#" -ne 2 ]
then
  echo "usage: $0 DOMAIN NAMESERVER_1"
  exit 1
fi

ARG_DOMAIN=$1
ARG_NAMESERVER_1=$2

################################################################################
# Set up the named.conf.local file
#
cat >> /etc/bind/named.conf.local <<EOF
zone "${ARG_DOMAIN}" {
        type slave;
        file "/var/cache/bind/${ARG_DOMAIN}.zone";
        masters { ${ARG_NAMESERVER_1}; };
        allow-transfer { none; };
};
EOF

################################################################################
# (re)start service(s) with the correct parameters
#
# also the tripwire bundle may disable postfix so here we make sure
# it is enabled
#
if systemctl is-active bind9
then
  systemctl reload bind9
else
  if ! systemctl is-enabled bind9
  then
    systemctl enable bind9
  fi
  systemctl start bind9
fi

# vim: ts=4 sw=4 et
