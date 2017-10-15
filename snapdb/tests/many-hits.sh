#!/bin/sh

for g in a b c d e f g h i j k l m n o p q r s t u v w x y z
do
	for f in 1 2 3 4 5 6 7 8 9 10
	do
		wget -nv -O s$g-$f.html 'http://alexis.m2osw.com/cgi-bin/snap.cgi?q=/char-chart/1'
		if [ `stat -c %s s$g-$f.html` -ne 41812 ]
		then
			exit 1
		fi
	done
done

