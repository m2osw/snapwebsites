# Profile settings added by Snap! C++
# See: https://snapwebsites.org/

# Keep More History From All Terminals
#
HISTCONTROL=keepall
HISTSIZE=10000
HISTFILESIZE=400000000
PROMPT_COMMAND="${PROMPT_COMMAND:+$PROMPT_COMMAND$'\n'}history -a"
#PROMPT_COMMAND="${PROMPT_COMMAND:+$PROMPT_COMMAND$'\n'}history -a; history -c; history -r"
shopt -s histappend
history -n
shopt -s checkwinsize

# Show a message when entering /etc/snapwebsites asking users not to edit
# those files (it may work)
#
snap_please_do_not_edit() {
  if [ $PWD = "/etc/snapwebsites" ]
  then
    echo "*** WARNING ***"
    echo "Please do not edit files directly under \"/etc/snapwebsites\"."
    echo "Instead go to \"/etc/snapwebsites/snapwebsites.d\" and edit"
    echo "the files there. If the file you want to edit is not there"
    echo "yet create a new file which named \"50-<name>.conf\"."
    echo
  fi
}
PROMPT_COMMAND="${PROMPT_COMMAND:+$PROMPT_COMMAND$'\n'}snap_please_do_not_edit"
