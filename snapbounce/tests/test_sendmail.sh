#!/bin/sh
#
# Some example to test our sendmail tool
#
# This is not really a functional test at this point, but it would be nice
# to get such at some point; at least it gives me an idea of the command
# line to use to run tests

SENDMAIL=../../BUILD/snapwebsites/snapbounce/src/sendmail
EMAIL=snapbounce/tests/test.eml

# If necessary you need to make the tool root:root and suid
# (as a developer, it's not in your build, it is in the final package
# as the debian/rules take care of that)
#
sudo chown root:root ${SENDMAIL}
sudo chmod 4755 ${SENDMAIL}

# Two that are forced to fail (because of the --send-error)
#
${SENDMAIL} --debug --send-error -Am -Ac -Fq,d,t -Q"it's nice" '"Wilke, Alexis" <alexis_wilke@yahoo.com>' <${EMAIL}
${SENDMAIL} --debug --send-error -Am -Ac -Fq,d,t -Q"it's nice" '"Wilke, Alexis" <alexis_wilke@yahoo.com>' <${EMAIL}

# One that works
#
${SENDMAIL} --debug -Am -Ac -Fq,d,t -Q"it's nice" '"Wilke, Alexis" <alexis_wilke@yahoo.com>' <${EMAIL}

# here the /var/mail/root is gone

# vim: ts=4 sw=4 et
