#!/bin/bash

if [ "$#" -ne 3 ]
then
  echo "usage: $0 DOMAIN PUBLIC_IP NAMESERVER_2"
  exit 1
fi

ARG_DOMAIN=$1
ARG_PUBLIC_IP=$2
ARG_NAMESERVER_2=$3

################################################################################
# Set up the named.conf.local file
#
if ! grep "acl trusted-servers" /etc/bind/named.conf.local > /dev/null
then
    cat >> /etc/bind/named.conf.local <<EOF

// We only share records with these trusted servers.
//
acl trusted-servers  {
    ${ARG_NAMESERVER_2};  // ns2
};
EOF
fi

cat >> /etc/bind/named.conf.local <<EOF

zone "${ARG_DOMAIN}" {
    type master;
    file "/etc/bind/${ARG_DOMAIN}.zone";
    allow-transfer { trusted-servers; };
};
EOF

# TODO: make sure these values are sane
#
cat > /etc/bind/${ARG_DOMAIN}.zone <<EOF
\$ORIGIN .
\$TTL 86400
${ARG_DOMAIN} IN SOA ns1.${ARG_DOMAIN}. webmaster.${ARG_DOMAIN}. (
    `date +%Y%m%d01`
    3H
    180
    14D
    300
  )
  NS      ns1.${ARG_DOMAIN}
  NS      ns2.${ARG_DOMAIN}
  MX 10   mail.${ARG_DOMAIN}

  60 IN A ${ARG_PUBLIC_IP}
\$ORIGIN ${ARG_DOMAIN}.
ns1   60 IN A ${ARG_PUBLIC_IP}
ns2   60 IN A ${ARG_NAMESERVER_2}
mail  60 IN A ${ARG_PUBLIC_IP}
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
