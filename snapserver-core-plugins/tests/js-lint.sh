#!/bin/sh

#
# This script makes use of the Google JavaScript Linter.
#
# We may also want to run jslint with the following options:
#
#   jslint --browser --closure --nomen --plusplus --sloppy --white --maxerr 10000 filename.js
#
#       --browser    define browser globals
#       --closure    allow Google closure annotations
#	--nomen      ignore underscores
#       --plusplus   allow ++ and --
#       --sloppy     ignore the missing "user strict"
#       --white      ignore white space "requirements"
#	--maxerr     maximum number of errors before stopping
#


OUTPUT="$1/js-lint"
mkdir -p $OUTPUT

#
# Disable:
#   0002 -- Missing space before "("
#   0110 -- Line too long (over 80 characters)
#   0120 -- Binary operator should go on previous line "&&"
#   0131 -- Single-quoted string preferred over double-quoted string.
#

GOOGLE_OPTIONS="--disable 0002,0110,0120,0131 --jslint_error=blank_lines_at_top_level --jslint_error=unused_private_members"
NODEJS_OPTIONS="--browser --closure --maxerr 10000 --nomen --plusplus --sloppy --white"

for j in `find . -type f -name '*.js'`
do
	# Save the output in our analysis output directory
	echo "gjslint $GOOGLE_OPTIONS $j >$OUTPUT/`basename $j .js`-google.txt 2>&1"
	if    gjslint $GOOGLE_OPTIONS $j >$OUTPUT/`basename $j .js`-google.txt 2>&1
	then
		# if no errors, delete the file... (no need!)
		rm $OUTPUT/`basename $j .js`-google.txt
	fi

	echo "jslint $NODEJS_OPTIONS $j >$OUTPUT/`basename $j .js`-nodejs.txt 2>&1"
	if    jslint $NODEJS_OPTIONS $j >$OUTPUT/`basename $j .js`-nodejs.txt 2>&1
	then
		# if no errors, delete the file... (no need!)
		rm $OUTPUT/`basename $j .js`-nodejs.txt
	fi
done

echo JavaScript lint done.

