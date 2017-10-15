#!/bin/sh
#
# Sample code to setup a bounce on a user account in order to test and see
# that the code using those fields works.
#
# You will have to tweak the date and email address.
#

DBIP=127.0.0.1
EMAIL=halk89@m2osw.com
FAILURE_DATE="2016-05-11 18:57:50.290553"

../../BUILD/snapwebsites/snapdb/src/snapdb --host $DBIP users "$EMAIL" sendmail::bounce_arrival_date0 "${FAILURE_DATE}"
../../BUILD/snapwebsites/snapdb/src/snapdb --host $DBIP users "$EMAIL" sendmail::bounce_diagnostic_code0 "5.1.0"
../../BUILD/snapwebsites/snapdb/src/snapdb --host $DBIP users "$EMAIL" sendmail::bounce_email0 "snap-17058"
../../BUILD/snapwebsites/snapdb/src/snapdb --host $DBIP users "$EMAIL" sendmail::bounce_notification0 "\n<test@example.com>: host smtp.example.net[1.2.3.4] said: 550 5.1.0\n    2.3.4.5 is not allowed to send from <example.com> per it's SPF\n    Record. Please inspect your SPF settings, and try again. \n    (in reply to MAIL FROM command)"

