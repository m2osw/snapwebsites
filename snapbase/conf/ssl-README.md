
INTRODUCTION
============

The SSL directory is expected to be used to place your SSL keys,
certificate and alike.

The directory is automatically protected from reading and writing
from anyone other than root.


CREATING CERTIFICATES
=====================

When you want to create a new certificate, we strongly suggest that
you create a new directory named after the domain name for which you
are creating that certificate. Then cd into it and run the necessary
command to create the certificate (`openssl gendsa ...`)

The Apache2 settings can then be tweaked to include another
VirtualHost for this certificate. Now that you can bind more
than one certificate per IP address, it has become pretty simple.


SSL CONFIGURATION IN APACHE2
============================

The "snap-apache2-ssl.conf" file is expected to be included in your
`<VirtualHost>` entries after you included the certificates. Somehow,
it cannot work without first having the certificates in place.

So, you will have something like this:

    # Note: you may want to use your public IP address instead of "*"
    <VirtualHost *:443>
        ...snip...

        # The CRT is the signed certificate
        #
        SSLCertificateFile /etc/apache2/ssl-snap/cert-01/example.com.crt

        # The key is the private key you generated
        #
        SSLCertificateKeyFile /etc/apache2/ssl-snap/cert-01/example.com.key

        # Some systems may come with a CRT chain, in which case you include
        # it this way (i.e. godaddy offers certificates with such)
        #
        SSLCertificateChainFile /etc/apache2/ssl-snap/cert-01/bundle.crt

        # Now we can include our SSL options, that includes turning ON
        # the SSL engine
        #
        Include /etc/apache2/ssl-snap/snap-apache2-ssl.conf
    </VirtualHost>

Note that we include these parameters in our sample file (snap-apache2.conf)
so you should be ready to go. Just make sure to endit the filenames.


_This file is part of the [snapcpp project](http://snapwebsites.org/)._
