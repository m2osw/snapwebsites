#!/bin/sh
#
# To execute as www-data (the user that usually runs the snap.cgi binary)
# you may use the following command line:
#
#   sudo su - www-data -c 'sh /path/to/request.sh' -s /bin/sh
#
# You may want to consider using the -H command line option too.
#

# Prepare the request
#
export REQUEST_METHOD=GET
export REMOTE_ADDR=192.168.2.45
export HTTP_HOST=csnap.m2osw.com
export REQUEST_URI=/
export HTTP_USER_AGENT="Mozilla 5.1"
export SERVER_PORT=80

# Run the CGI under full control
#
# Please correct the path depending on your installation.
#
# For a developer after compiling in debug mode
../../BUILD/snapwebsites/snapcgi/src/snap.cgi
# For a normal installation
#/usr/lib/cgi-bin/snap.cgi
