#!/bin/sh
#
# Sample code to setup a bounce on a user account in order to test and see
# that the code using those fields works.
#
# You will have to tweak the date and user identifier.
#
# Note:
# To find your user identifier you can check the database for your emails
# in the '*row_index*' of the users table
#
#   snapdb --host $DBIP users '*index_row*' | grep $EMAIL
#
# the output will show you the email address and identifier
#

# Administrators do not get blocked even when their emails get bounced
# (this is just in case...)
#
# If you use --admin you mark that user as an admin so bounces will be
# ignored by that user.
#
if test "$1" = "--admin"
then
	ADMIN=1
else
	ADMIN=0
fi

UID=1
DBIP=127.0.0.1

# Use an old date to test that the 4 months test works as expected
# Keep in mind that dates in the database are all in UTC
#FAILURE_DATE="2016-05-11 18:57:50.290553"
FAILURE_DATE="`date -u +'%F %T.123456'`"

../../BUILD/snapwebsites/snapdb/src/snapdb --host $DBIP users "$UID" sendmail::bounce_arrival_date0 "${FAILURE_DATE}"
../../BUILD/snapwebsites/snapdb/src/snapdb --host $DBIP users "$UID" sendmail::bounce_diagnostic_code0 "5.1.0"
../../BUILD/snapwebsites/snapdb/src/snapdb --host $DBIP users "$UID" sendmail::bounce_email0 "snap-17058"
../../BUILD/snapwebsites/snapdb/src/snapdb --host $DBIP users "$UID" sendmail::bounce_notification0 "\n<test@example.com>: host smtp.example.net[1.2.3.4] said: 550 5.1.0\n    2.3.4.5 is not allowed to send from <example.com> per it's SPF\n    Record. Please inspect your SPF settings, and try again. \n    (in reply to MAIL FROM command)"

../../BUILD/snapwebsites/snapdb/src/snapdb --host $DBIP users "$UID" sendmail::is_admin ${ADMIN}

