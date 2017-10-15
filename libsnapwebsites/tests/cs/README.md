
Client-Server Tests
===================

The binaries generated in this directory are client and server executables.
The work in pair. In most cases you first start the server, verify that
the server is in place (i.e. there is a LISTEN in your netstat output),
and then start the client and see what happens.

These tests can be started by a Catch.hpp implementation, but they cannot
themselves be part of a catch test because they need to be executables
(well... the server could run by itself, but at the same time a clean
client-server test requires both to be standalone executables and not
part of some other binary.)


Shutdown Test
=============

This test was mainly to prove that the default `BIO_free_all()` function
is problematic because it calls `shutdown()` on the socket and that kills
a normal HTTP communication.


SSL Test
========

We offer the capability to create an SSL connection between various
objects when certificate and server files are specified and the system
sets the connection to `MODE_SECURE`.

The SSL Test checks that this actually works as expected. To get a pair
of certificate and private key, one uses a command as follow.

    openssl req \
       -newkey rsa:2048 -nodes -keyout ssl-test.key \
       -x509 -days 3650 -out ssl-test.crt

We include two files: `ssl-test.crt` and `ssl-test.key` which we created
with that command. These two files are here for test purposes and should
not ever be used in a secure manner since (obviously?) the private key
has been compromised.

To suppose more of the secure ciphers, you will want to use a dhparam.pem
file. This file is generated with:

    openssl dhparam -outform PEM -out dhparam.pem 1024

When the server is running, you can attempt a connection to the server
using openssl. This will give you information about what cipher is used
to connect, show you the certificate, etc.

    > openssl
    OpenSSL> s_client -cipher ALL -connect 127.0.0.1:4030 -tls1_2
    ...
    SSL-Session:
        Protocol  : TLSv1.2
        Cipher    : AES256-GCM-SHA384
    ...
    OpenSSL> quit

The name of the cipher should tell you how strong that connection
encryption is.


_This file is part of the [snapcpp project](http://snapwebsites.org/)._
