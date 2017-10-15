
# What is "snaphttpd"

The snaphttpd implementation will be a replacement to Apache2 and HAProxy
all in one block. There are several reasons in doing such as highlighted
below.

This code is part of the `snap_child` implementation. It requires us to
change the `snapserver` implementation to prefork children that listen
to incoming connections.

The `snap_child` answers the request, if the looks like the current
server is too heavily loaded already, then the `snap_child` acts as
a proxy (for load balancing) so it just receives the data, parses it
to make sure it is valid, and forwards it to another `snap_child` on
a not too heavily loaded server (in that second request, we know that
it comes from another `snap_child` so we do not have to use the blocking
mechanisms or have as heavy a security handling.


## Compatible with our various other daemons

It can understand the loadavg data as defined by snapcommunicator.

It can send BLOCK messages to the snapfirewall, which blocks IP
addresses on all the front ends immediately.

It knows how to communicate with other snapservers without the need for
any additional drivers.


## Understands various attacks upfront

Like `snapcgi` and various other things we already have implemented in
`snap_child`, our implementation can block various attacks very early on.
(See the Security chapter below.)

Actually, as soon as we get an HTTP request that starts with a protocol,
we can kill the connection. (i.e. a "GET + URI with a scheme" is a proxy
request which we do not support at all!)

Methods that we do not want to understand can also be ignored immediately.

It never has to stick tainted data in an environment variable, avoiding any
kind of attacks relating to such.


## Lightweight

`snap_child` already includes a way to determine whether a domain and
sub-domains are served by this system. Therefore, we avoid all the
Apache2 settings.

However, we are still missing the SSL private key. Although it would be
possible to have such in our Cassandra database, it is much safer to
stick most of that on a separate backend cluster of private keys. A form
of key store that we can access with encrypted connections and only from
the computers that need the information seeked. These separate cluster
can manager the keys as we need new ones for new websites.

As a result this means our servers directly talk with the client, no
intermediaries. That way we can really only answer the very requests
we know should be answered.


# Load Balancing

One of the points of directly answering client requests, is to handle
our own distributed load balancing mechanism.


## Why not use HAProxy?

Right now, many companies make use of HAProxy. There are two drawbacks
in using that tool with Snap!, instead of having our own HTTP implementation:

One, we would have to tweak the loadavg data so that HAProxy understands
it. That is not too complicated, though. However, HAProxy does not have
such advanced options that could be used to load balance many HAProxy
servers. It is a very thin frontend so they assume that it would not
need such. That being said, some companies receive millions of hits
through just 2 or 3 HAProxy systems. Yet, I have not seen the specs of
such systems and I am thinking they are not $5/mo VMs...

Two, you want to have frontend computers dedicated to run HAProxy. This
means additional costs and another set of computers that can go down.
Note that you would want at least 2 such computers if you do not want to
ever lose connectivity. Also if you are to have 10 `snapservers` frontends,
in our case you get 10 load balancers running. That's 10 computers that
need to go down before our system stops working. With HAProxy, you are
likely to have only two such front ends and that limits your losses two
just two computers...


## Distributed Load Balancing

With having many front end computers, you can better distribute the front
end load. Although those computers do not have to also run snapserver, they
can. In general, that means smaller costs.

The distribution allows you to have a smaller chance of a total breakage
of your cluster. Instead of 1 or 2 HAProxy computers that may break at
any time, now you may have as many front ends as you want.

All are communicating with snapcommunicator to get the loadavg information
from every computer. Then we use various algorithms to decide which computer
is best suited to handle the next request (a form of round robin helped with
the loadavg data of all the front end computers).


# Safety

Many attacks go right through Apache2. HAProxy, at least, has several
algorithms that recognize various types of attacks from clients (DoS, DDoS,
Slowloris, and a few more.)

We want to block many attacks, including many more that HAProxy
would not recognize and directly send a BLOCK message to `snapfirewall`.


## Slowloris

The debit must be timed. If we receive too few bytes per second, then we
must drop that connection altogether and put that IP address in our black
list. The debit should be relatively consistent, although we probably do
not want to penalize if the debit increases and slows down as long as it
does not go below a certain threshold.

The speed may be different while reading the header and reading the body.


## Method

The HTTP protocol gives you a way to reply to a request with an unknown
method by send a "405 Method Not Defined" error.

There are two problems with that concept:

One, we have to accept the entire incoming request when we know of the
method as soon as we receive the space after it. So we can immediately
block totally unwanted methods and avoid receiving more data.

Two, the request may include a field requesting the connection to stay up.
In that case, we would be wasting a connection for totally nothing.

On our end, we should probably accept a very first connection from a given
IP address and do the standard 405 reply, but still ignoring the keepalive
in case it was there. We could do that for a few methods that are common
and should be accepted. However, always refuse such a connection if the IP
address was blocked for any other reason (i.e. the IP is already in our
blacklist.)


### Method Length

One of the longest of the basic methods expected to be understood by an
HTTP server is 7 characters. We should limit the input that such a short
length and if the method is longer, then fail right away, whatever the
method might be.

Actually, since we have the list of methods in our `methods=...` variable,
we know of the maximum length we can accept. Anything longer, we can
immediately mark as invalid. Although we may have another variable, such
as `acceptable_methods=...`, which accepts other methods, but does not
handle them (i.e. generate the 405 when we receive such.)


### Method Characters

See: https://tools.ietf.org/html/rfc7230#section-3.1.1

Acceptable characters in the method are limited to the following set:

    # lex syntax

    token: [-A-Za-z0-9!#$%&'*+.^_`|~]+

    # HTTP 7230 definition:

    token          = 1*tchar

    tchar          = "!" / "#" / "$" / "%" / "&" / "'" / "*"
                   / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
                   / DIGIT / ALPHA
                   ; any VCHAR, except delimiters

If any other character is received, we can stop reading the input. It's bad.

Note that in our case, we can have a variable in our configuration file such
as "methods=..." and thus we can limit the characters to only those that we
have in the user defined methods. For example, if the only few methods
accepted are GET, HEAD, and POST, then the regex can be simplified to:

    [ADEGHOPST]

Absolutely any other characters can be refused immediately. We can easily
create a couple of vectors of bits that handles these characters very
efficiently.


## Path

After the method appears the path between a couple of spaces.


### First character in the path

As far as I know, all requests must have a path that start with a `/`
since all paths have to be absolute within an HTTP request (there is
no "base" concept in an HTTP request.)

So if the very first character is not a `/` then we can error
immediately.

Note: we do not accept Proxy requests (See below) and hence cannot
receive a scheme first. In effect, testing that very first character
should be enough to refuse proxy requests and invalid paths all at
once.


### Refuse Proxy Requests

Note: if the first character idea does not fly, then this has to be checked
to avoid proxy requests.

If that path includes a scheme then it is a proxy request and we automatically
refuse all of those. This can happen as soon as we detect that there is a
scheme definition:

    # lex syntax

    [A-Za-z][-A-Za-z0-9+.]*:

    # HTTP 3986 definition:

    scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )


### Length

Just like the method, a path has to be limited in length. However, a path
can be very long in HTTP. We want to have a way to define the length in
our configuration file. A path should be between 1 character (requesting
the root page) and 8kb or so. We could support longer length but should have
a hard limit of 64Kb, just because it is not sensible to have anything
much longer than 1Kb in the first place anyway (although it could be good
to see how many robots fail with long URIs...)


### Path Characters

Just like with the method, characters appearing in a path are somewhat
limited.

As we are at it, we automatically decode the `%XX` characters. If any
hexadecimal digit is invalid, we can immediately break with a BLOCK
message to `snapfirewall` and blacklist that IP address.

Not only that, we then can check that those are sensible characters
(i.e. not `'\0'`, actually, not a character below 0x20, and if it forms
a UTF-8 character, that the UTF-8 character is considered valid: a short
form that represents a real character.)

As we change the `%XX` characters, we want to change the `+` if it appears
(TBD).

The slash character should not be encoded using the `%XX` syntax and we can
consider that it is just plain invalid. For sure, the name of a segment can
never include a slash in Snap! so there is no need to support such.

Similarly, we should probably refuse some other special characters like
the `?` and `#` which have special meaning in a URI and should probably
never be used within a segment of the path.

Also, we want to immediately canonicalize the `'/'` characters to exactly
one and none at the end of the path unless it is the root path. (WARNING:
the number of characters received must count toward the Length as defined
previously, so if we receive `/////` it counts as 5 and not just one as
we reduce the input in our buffer.)

    # lex syntax

    /([-A-Za-z0-9._~!$&'()*+,;=:@/]|%[A-Fa-f0-9]{2})*

    # URI 3986 -- https://tools.ietf.org/html/rfc3986#section-3.3

    path-absolute = "/" [ segment-nz *( "/" segment ) ]
    segment       = *pchar
    segment-nz    = 1*pchar
    pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
    unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
    pct-encoded   = "%" HEXDIG HEXDIG
    sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
                  / "*" / "+" / "," / ";" / "="


## Protocol

The path is followed by the protocol name and version.


### Protocol Name

The only protocol understood is exactly `HTTP`. If we receive anything else,
we can immediately block that IP address.

Note that the protocol name is case sensitive, so `http` is also not
considered valid.


### Version

The version should accept the HTTP version definition: `DIGIT "." DIGIT`.
Meaning that the version cannot be more than one digit.

We should allow the user to decide which version we should support. There
are 4 in use: 0.9, 1.0, 1.1, and 2.0. We can refuse 0.9 altogether because
no one uses that version anymore.

Version 1.0 is still used by some servers. However, 1.0 has the `Host`
field optional and if we decided that this is not acceptable, then we should
not even accept 1.0. That means anything that hits us with that version,
we can block immediately. Note that there is a possible HTTP reply that
asks the client to switch to a different protocol. We could try that first.
I am thinking, though, that any client that says it knows 1.0 will not be
capable of switching to 1.1 or 2.0 _just like that_.

Version 1.1 is used all over the place and we should answer those to the
letter.

Version 2.0 offers the multi-file connection scheme. So if a request to
load page A finds out that it will also need files F, G, H, then all
of these can be sent within a "single reply" to the client request.
See https://tools.ietf.org/html/rfc7540 for additional details.
Note that requires us to support multiple connections in parallel. I
do not think that we are currently properly geared up for such, although
I suppose a `snap_child` that receives an HTTP/2.0 request can connect
to one or more `snapserver` and serve the various files by sending
HTTP/1.1 requests in the good old way. This may be optimizable later
as our servers become better aware of the various capabilities of HTTP/2.0.

See also [nghttp2](https://nghttp2.org/) and
[h2spec](https://github.com/summerwind/h2spec).


## Fields

After the first line defining the method, path, and protocol, we get
fields. Each line must to be separated by exactly one CR LF.

If we find an empty line, then we find the separator between the
header fields and the body. This empty line is required and we should
get it fairly quickly.


### Unknown Fields

These should be reported so one can mark such fields as unwanted (blacklisted
so we can block furhter connections with such a field.)

We can also count the number of unknown fields. If too many, block that user,
it most certainly crap.

See https://tools.ietf.org/html/rfc4229


### Fields too Long

Fields are composed of a name, a colon, and a value. The name of the fields
are not limited by the specification, but has to be very much limited
(probably 32 characters is enough?)

The value, however, is more complicated. Some fields have a very large
value. For example, if we support a 64Kb path, then the referrer can
end up being a little over 64Kb (path + domain).

What we can certainly do, though, is determine some form of maximum
size per field. That way we can prevent some bad robots from sending
overfully long fields when such are expected to be very short.


### Field Name Validity

A field name has a set of characters that is very limited. It is a token.

    # lex syntax

    token: [-A-Za-z0-9!#$%&'*+.^_`|~]+

    # HTTP 7230 definition:

    token          = 1*tchar

    tchar          = "!" / "#" / "$" / "%" / "&" / "'" / "*"
                   / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
                   / DIGIT / ALPHA
                   ; any VCHAR, except delimiters

While reading a field name, if we detect an invalid character, we can
immediately mark that request as invalid and block the client's IP address.


### Field Value Validity

Fields could be folded, although there is really no need for that. Yet,
many clients may still fold long fields (like in emails) so we want to
support that feature.)

    # lex syntax

    value: [ -~]*|(\r\n[ \t][!-~])*

    # HTTP 7230 definition:

    field-value    = *( field-content / obs-fold )
    field-content  = field-vchar [ 1*( SP / HTAB ) field-vchar ]
    field-vchar    = VCHAR / obs-text

    obs-fold       = CRLF 1*( SP / HTAB )
                    ; obsolete line folding
                    ; see Section 3.2.4

The VCHAR is any character that is "visible" (or printable). This is
code 0x21 to 0x7E.


## Body

After the list of fields may come a body. For most request, the body will
be empty. If the method is POST, we expect a non-empty body.


### Body Validity

For the body parsing, we already have code in `snap_child` that does the
work, including verifying the validity of the syntax, boundaries, etc.
We end up generating many different types of errors here too. We may want
to revisit that code and add some BLOCK since some of those errors should
just never happen from a sensible client.


### Body Size

One thing we can add easily, when in control of the HTTP request, is a
maximum size for the request.

Most forms expect really very small amount of data in their replies and
thus, receiving more than that small amount can immediately be stopped
and the IP blocked.

Forms include a session number which is included first or second in the
response, so we can read that, get the form, compute the size, then read
the rest with that maximum.

As an example, a form with 5 fields each of which accept a maximum of
12 characters would have a reply of 5 x 12 = 60 bytes plus the size
of the variable names, the equal signs, and the ampersands. So maybe
a total of 120 to 150 bytes at the most. If we start receiving 200+
bytes, this is wrong, kill that connection, block the IP to ignore
the brat.

If attachments are allowed, again we can have a limit on the size of an
attachment:

* as a top maximum for the entire system (defined in our .conf files)
* as a whole for the entire website (defined in the sites table?)
* as a type specific size, using the type of the page on which the form appears
* as a specific size for that category of files (defined by extension?)
* as a specific size for that specific form or field

The size limit for a form should be calculated once, any time the form
changes.


vim: ts=4 sw=4 et

_This file is part of the [snapcpp project](http://snapwebsites.org/)._
