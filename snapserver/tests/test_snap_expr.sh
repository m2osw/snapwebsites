#!/bin/sh
# Test the snap_expr tool

set -e

PATH=$PATH:`cd ..; pwd`/BUILD/snapwebsites/src

if test "$1" = "-v"
then
	verbose=true
else
	verbose=false
fi

# Tests to search bug I finally found in the expression parser: the expr_list
# would do an early optimization, the list has to be flatten once complete,
# otherwise you get an invalid list.
#snapexpr --host 127.0.0.1 -s '(1, 2, 3)' > ~/tmp/a.xml
#snapexpr --host 127.0.0.1 -s 'strlen("test") * 3 '
#snapexpr --host 127.0.0.1 -s '(branch := int32(cell("content", "foo", "content::revision_control::current_branch")),
#	key := int32(cell("content", "foo", "content::revision_control::current_revision_key::" + branch + "::en")),
#	cell("data", key, "content::title"))' > ~/tmp/a.xml
#exit 1


# WARNING: Do not use '@' in a string!
TESTS=`cat <<'EOF'
1 + 1@(int64) 2
1 + 1 + 1 + 1@(int64) 4
7 + 1 + 111 + 0xFF@(int64) 374
7 + 3 * 19@(int64) 64
(7 + 3) * 19@(int64) 190
(7 + 3, 19)@(int64) 19
(1,2,3,4,5,6,7,8,9,10)@(int64) 10
"test" * 3@(string) "testtesttest"
"test" + 3@(string) "test3"
strlen("test" * 3)@(int64) 12
"test" + " " + "me"@(string) "test me"
7 - 9@(int64) -2
3 * 3@(int64) 9
100 / 3@(int64) 33
1001 % 63@(int64) 56
123 << 3@(int64) 984
123 >> 3@(int64) 15
123 * -1 >> 3@(int64) -16
123 * (-1 >> 3)@(int64) -123
123 < 129@(bool) 1
123 <= 129@(bool) 1
123 > 129@(bool) 0
123 >= 129@(bool) 0
129 < 123@(bool) 0
129 <= 123@(bool) 0
129 > 123@(bool) 1
129 >= 123@(bool) 1
1 <? 100@(int64) 1
1 >? 100@(int64) 100
100 <? 1@(int64) 1
100 >? 1@(int64) 100
-1@(int64) -1
+1@(int64) 1
!1@(bool) 0
!6@(bool) 0
!!6@(bool) 1
!0@(bool) 1
!!0@(bool) 0
~1@(int64) -2
~0@(int64) -1
true@(bool) 1
false@(bool) 0
!true@(bool) 0
!false@(bool) 1
5 == 5@(bool) 1
5 != 5@(bool) 0
5 == 7@(bool) 0
5 != 3@(bool) 1
7 & -4@(int64) 4
7 ^ 4@(int64) 3
1 | 4@(int64) 5
4 | 1@(int64) 5
3 < 5 && 1 > -3@(bool) 1
3 > 5 && 1 > -3@(bool) 0
3 < 5 && 1 < -3@(bool) 0
3 > 5 && 1 < -3@(bool) 0
3 < 5 ^^ 1 > -3@(bool) 0
3 > 5 ^^ 1 > -3@(bool) 1
3 < 5 ^^ 1 < -3@(bool) 1
3 > 5 ^^ 1 < -3@(bool) 0
3 < 5 || 1 > -3@(bool) 1
3 > 5 || 1 > -3@(bool) 1
3 < 5 || 1 < -3@(bool) 1
3 > 5 || 1 < -3@(bool) 0
3 < 5 || 1 < -3 ? 123 : 555 * 2@(int64) 123
3 > 5 || 1 < -3 ? 123 : 555@(int64) 555
a := 7@(int64) 7
(a := 7, b := a * 3, b - 1)@(int64) 20
substr("carmichael", 3)@(string) "michael"
substr("carmichael", 0, 3)@(string) "car"
EOF
`
# At this time we forbid multiple assignments in a row
# (a := b := 7, b := a * 3, b - 1)@(int64) 20
# (a := b := 7, b := b * 3, b - 1)@(int64) 20

IFS="
"
for e in $TESTS
do
	expr=`echo $e | sed -e 's/@.*//'`
	result=`echo $e | sed -e 's/.*@//'`
	cmd="snapexpr --host 127.0.0.1 -q -- '$expr'"
	if $verbose
	then
		echo $cmd
	fi
	a=`bash -c $cmd`
	if test "$a" != "$result"
	then
		echo "error: '$expr' returned '$a' instead of '$result' as expected" >&2
	fi
done


