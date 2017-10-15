#!/bin/sh
#
# I use this little script to merge all the separate buttons in one
# single file making it MUCH smaller (on my firsttest over 31x smaller!!!
# that's what I call compression!)
#
# Requirements:
#   . ImageMagick
#   . Shell
#
# See also    http://imagemagick.org/Usage/layers/
#

# group.png is a special case as it is only 4 pixels wide, put it first
# so the math for the position of the other items is made easy
# (we do not use *.png because we want group.png first)
IMAGES="group.png
        bold.png
        createLink.png
        formatBlock.png
        indent.png
        insertFieldset.png
        insertHorizontalRule.png
        insertOrderedList.png
        insertUnorderedList.png
        italic.png
        justifyCenter.png
        justifyFull.png
        justifyLeft.png
        justifyRight.png
        outdent.png
        removeFormat.png
        strikeThrough.png
        subscript.png
        superscript.png
        underline.png
        unlink.png"


# +append generates an horizontal bar
# -append generates a vertical bar
convert $IMAGES +append ../buttons.png

# vim: ts=4 sw=4 et
