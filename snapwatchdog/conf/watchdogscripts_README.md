
# snapwatchdog Scripts Directory

This file is a placeholder in the default directory used by the
`watchdogscripts` plugin to read shell scripts that it executes
each time the `snapwatchdog` backend wakes up (once per minute by default.)


# Script Requirements

The scripts should be very small and execute quickly.

No script should start processes in the background as such could quickly
use up all the memory if these background processes cumulate.

In most cases we expect these files to be shell scripts but there is
no real restrictions on the files which could be binaries if better
adapt to your situation.


# Script Name Convention

At this time we expect scripts installed in that directory to be named
by following this convention:

* Start the name with `watch_`
* Follow with the name of your project, for example, the
firewall plugin uses `firewall_`
* Continue with the name of the object the script verifies for the
`snapwatchdog` daemon. For example, the firewall plugin test checks
the fail2ban client script that could derail (run using 100% of the CPU)
so this section of the name is `fail2ban_client`.

Although the name can really be anything, we suggest to follow this
convention to ensure unicity of each script name.


# Defaults

The `watchdogscripts` plugin starts your scripts using the
`watch_script_starter`. The starter script retrieves defaults
by sourcing the `/etc/default/snapwatchdog` shell script file.
This file defines various variables that the scripts are free
to use.

The default shell script is not mandatory, although it should not
be removed.


# Order of Execution

The `watchdogscripts` plugin reads the files in whatever order
they appear in this directory. It ignores this README.md file,
but no specific order is offered. All the scripts should work
independently and not require any specific order of execution.


# Execution Flags

The script execution flags are not required. If not set, the
script is forcibly started with `$SHELL` which is `/bin/sh` in
our case. Although you can change the `$SHELL` variable in
your default script, it is recommended that you restrain
from doing so and use the hash bang (`#!`) to instead change
shell script in your script.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/snapwebsites/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._
