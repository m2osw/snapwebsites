#!/bin/sh

count=`snapdb --no-types users '*id_row*' users::identifier`

uid=1

while test $uid -lt $count
do
	if snapdb users $uid | grep sendmail
	then
		echo "User $uid"
		echo
	fi
	uid=`expr $uid + 1`
done

