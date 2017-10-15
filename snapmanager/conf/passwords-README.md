
This folder is used to save passwords
=====================================

This directory is protected from praying eyes. It is only accessible by
the root user.

The files are handled by the libsnapwebsites library. See the password.cpp
and password.h for details.

The format is 4 fields per line:

* Username -- the name of the concerned user, it does not need to match a
              Linux user.
* Digest -- the name of the digest used to generate the hashed password.
* Salt -- a disturbance so hashing the same password changes randomly.
* Password -- the password hash.

The salt and password are hexadecimal strings (0-9a-f) in password files.
In memory, it is kept in binary form instead.


Username Limits
===============

At this time, it is not checked properly yet, but the username is currently
limited to any byte, except the colon (:) and null character ('\0').

The expected limit, once fully implemented will be any valid UTF-8 string
with characters between 0x20 (space) and the maximum character number
(0x1EFFFF) except the colon (:). This is pretty similar to what you would
get if using the password in the URI used to access your snapmanager.cgi
pages.


Digest Limits
=============

The Digest is the name of the function used to hash the password and its
salt. We use "sha512" by default.

I am not 100% positive, but to get a list of the available hashing
functions from openssl you can use the following command line:

    openssl dgst --help

The --help is not understood, but it will generate the list of known
options. Look at the options that say _"to use the ... message digest
altgorithm"_.


Password Salt
=============

At this time you have no control over the salt. It is exactly 32 bytes
and used as two 16 bytes numbers to randomly modify the password hashing.
This is very useful to make sure that passwords remain safe even when
more than one user decide to use the exact same password as another.
That way, both hashing will will not look alike.


Password Hashing
================

The password hashing is a SHA512, so 64 bytes. When saved in the file as
hexadecimal characters, it takes 128 characters.


Security
========

Beside using the best available hashing at this time and the password salt,
the security also comes from the fact that the passwords directory is
protected by praying eyes as it is only accessible by the root user.

The `snappassword` tool requires the user to be root to manage the password
files.

The `snapmanager.cgi` has to send a message to `snapmanagerdaemon` in order
to know whether a user gave us the correct password and can be logged in.
This means only the password of a login in user will eventually exist in
`snapmanager.cgi`.


_This file is part of the [snapcpp project](http://snapwebsites.org/)._
