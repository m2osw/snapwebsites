#!/bin/bash

server=$1
client=$2

if [ -z "${server}" -o -z "${client}" ]
then
  echo "usage: $0 ip client-name"
  exit 1
fi

if ! [ -d "/etc/openvpn/easy-rsa" ]
then
  echo "server cert is not set up!"
  exit 1
fi

cd /etc/openvpn/easy-rsa
. ./vars

keys_dir=/etc/openvpn/easy-rsa/keys
client_file="${keys_dir}/${client}.conf"
./pkitool ${client}
sed \
  -e "s/^;user nobody/user nobody/" \
  -e "s/^;group nogroup/group nogroup/" \
  -e "s/^remote my-server-1 1194/remote ${server} 1194/" \
  -e "s/^ca ca.crt/;ca ca.crt/" \
  -e "s/^cert client.crt/;cert client.crt/" \
  -e "s/^key client.key/;key client.key/" \
  /usr/share/doc/openvpn/examples/sample-config-files/client.conf \
  > ${client_file}

append_file() {
  echo "<${1}>"           >> ${client_file}
  cat  "${keys_dir}/${2}" >> ${client_file}
  echo "</${1}>"          >> ${client_file}
}

append_file "ca"   "ca.crt"
append_file "key"  "${client}.key"
append_file "cert" "${client}.crt"

# vim: ts=2 sw=2 et
