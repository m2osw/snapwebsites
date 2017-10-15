
Steps to setup a server with Snap!

1. Create the server

2. Install the OS (Ubuntu 16.04 at time of writing)

3. Install the "snapmanagercgi" package; it will automatically install
   all the necessary dependencies; this require the step of adding our
   PPA or some other build server access...

4. Make sure Apache2 is properly installed (TODO)

5. Access snapmanager.cgi from your browser:

     http[s]://<domain name>/snapmanager.cgi

   Expect 's' so your connection is secure... but snapmanager.cgi can
   work on a non-excrypted connection (you'll get a warning, though.)


Note: with DigitalOcean (and others) they have an API that would allow
      us to create the computer automatically for our users, which can
      be cool because that way everything can be done nearly automatically!

      After such an installation you can connect with your SSH key
      (assuming you set it up as expected) specifying root as the
      user:

        ssh -l root <IP>



Everything else is to be setup with snapmanager.cgi

The following are part that snapmanager.cgi help you with:


a) setup the users as expected, especially, prevent root login from the
   outside (including ssh); add users with sudo and adm groups and your
   public key and a password (to make use of sudo you need a password.)
   Make sure the new users have ssh access and make use of bash as the
   default shell.

	useradd -m -s /bin/bash -G sudo,adm <name>
	passwd <name>
        mkdir /home/<name>/.ssh
	chown <name>:<name> /home/<name>/.ssh
	chmod 700 /home/<name>/.ssh
	vim /home/<name>/.ssh/authorized_keys
	(copy public key and save)
	chown <name>:<name> /home/<name>/.ssh/authorized_keys
	chmod 600 /home/<name>/.ssh/authorized_keys

   one we have at least one user who can sudo in the machine, we can
   prevent root SSH login:

	vim /etc/ssh/sshd_config
	(change PermitRootLogin to "no" and save)

   root will still be able to connect until you reboot the computer
   (which is safer than trying a 'sudo service ssh restart' because
   that could fail... we could also do the firewall installation in
   "parallel" and have one reboot that checks both at the same time.)


b) setup the firewall:

   b.1) include the list of rules to block most everything (by default)
        except SSH and Apache2; also make sure to install the firewall
	script and add the pre-up to at least one interface (both?)

   b.2) open local ports for local communication (i.e. so one can access
        Cassandra nodes, other front ends, etc.); this includes UDP as
	well as TCP since we use both (i.e. snapcommunicator and snapinit
	both accept one way UDP messages)

   b.3) Eventually allow DNS (53) and NTP (123) ports (TCP/UDP)

   b.4) Enable the port for emails (25)

	Just like for other parts, this should be us installing a package
	of some sort (or have that as part of the snapfirewall project?)
	that includes the necessary rules to open the necessary ports for
	sendmail and postmaster to work as expected.

   b.5) Allow ping from anyone or specific IPs (same IPs as for SSH)

   b.6) Include our usual list of bad_tcp_packets; although we have be
        able to manage the "legal" local IP addresses sich as the once
	used with our VPN to connect the various local computers

   b.7) Many of these rules will have an INPUT and an OUTPUT rule.


c) setup the swap memory; this just offers to add a swap if none is
   found; it makes use of a file on disk... (as done at DigitalOcean)
   for hardware owned by people, they will hopefully create a swap
   partition when they setup their disk, although with LVM we can always
   add it too (but right now, we won't work on that possibility!)

   https://www.digitalocean.com/community/tutorials/how-to-add-swap-space-on-ubuntu-16-04

   sudo fallocate -l 1G /var/cache/swapfile
   sudo chmod 600 /var/cache/swapfile
   sudo mkswap /var/cache/swapfile
   sudo vim /etc/fstab

   # swapfile
   /var/cache/swapfile none swap sw 0 0

   sudo vim /etc/sysctl.conf

   # Avoid swapping unless viewed as really useful or it is necessary
   vm.swappiness = 10
   vm.vfs_cache_pressure = 50

   


d) setup NTP; I'm not too sure whether we just want NTP or some other
   (more precise) time service; but snapmanager.cgi should be taking
   care of it;

   d.1) install ntp

   d.2) change servers to one of our servers (except on that one
        server) so... the user has to choose which one of our servers
	is the master, and all the other NTP servers will synchronize
	themselves against that master

	TBD: determine how that works when the master goes down!


e) setup tincd on computers that have a shared private network like
   at DigitalOcean. There is a tutotial, from which we did not
   derive except for the name of the server:

   See https://www.digitalocean.com/community/tutorials/how-to-install-tinc-and-set-up-a-basic-vpn-on-ubuntu-14-04

   See also: https://www.digitalocean.com/community/tutorials/how-to-set-up-an-openvpn-server-on-ubuntu-16-04
   Because tinc is not as secure as OpenVPN...
   (see https://www.tinc-vpn.org/security/ entry here)


f) setup a node after you gave it a type (we may want to say that the
   type cannot be changed later, if you need a new type, create a new
   node and don't mess the type this time!)

   the type would be one of:

   f.0) snap base: This gets installed on all systems (right now we 
                   do not yet do that, but that's the long term plan)

	install snapinit, snapcommunicator, snapmanager.cgi, snapwatchdog,
	snapdbproxy, snapfirewall; auto-start all if the database is
	ready, otherwise avoid the snapdbproxy and snapfirewall (argh!)

	See also SNAP-355

   f.1) front end: install Apache2, snap.cgi, and snap base

	At this point, e.1 and e.2 are on the same computer; once we have
	our own computers and no need for encryption between each server
	we can have two layers, especially so we can load balance access.

	For Apache2 we need to do a few more things:

	f.1.1) allow for the installation of the domain name(s)

	f.1.2) setup SSL certificate(s)

	f.1.3) make sure the necessary modules are enabled:
	       * ssl
	       * rewrite (now done in snapcgi.postinst)
	       * cgi (now done in snapcgi.postinst)

	f.1.4) look into what we want to do in regard to the default sites
	       in Apache2; the 000-default.conf will give us a default page
	       says "It works!"--we could change that default page or have
	       a way to redirect people to the "correct page" (i.e. in our
	       case, we probably would want to redirect people to
	       www.m2osw.com or snap.website, however, a redirect in the
	       default site can be "dangerous" if it fails... because then
	       you get an infinite loop.)

	f.1.5) replace the default index.html with our own, make sure to
	       put a "NOINDEX" in the robots.txt header <meta> tag because
	       that specific entry should never be indexed (it even works
	       with a bare IP address if the user does not switch the default
	       to Snap!)

   f.2) snap server: install snapserver and plugins and snap base

     Also see comment in (e.1)

   f.3) cassandra node: install Java, Cassandra, and snap base

     The Java environment from Oracle:

	sudo add-apt-repository ppa:webupd8team/java
	sudo apt-get update
	sudo apt-get install oracle-java8-installer

     For Cassandra, add "/etc/apt/sources.list.d/cassandra.sources.list"
     and put the following in it:

	deb http://www.apache.org/dist/cassandra/debian 35x main

     Then add the keys:

	gpg --keyserver pgp.mit.edu --recv-keys F758CE318D77295D
	gpg --export --armor F758CE318D77295D | sudo apt-key add -
	gpg --keyserver pgp.mit.edu --recv-keys 2B5C1B00
	gpg --export --armor 2B5C1B00 | sudo apt-key add -
	gpg --keyserver pgp.mit.edu --recv-keys 0353B12C
	gpg --export --armor 0353B12C | sudo apt-key add -

     Until the package dependencies get fixed, we have to use a
     --force-depends to install the cassandra package; however,
     that means python won't get installed automatically, so you
     have to do that step. However, the error about python-support
     can safely be ignored. But we created a package that we use
     to simulate the python-support package which makes it simpler
     (i.e. we will continue to get updates from the Cassandra
     repository)

     See: http://stackoverflow.com/questions/36880886/error-while-installing-cassandra/37428029#37428029


     Actually the one defined below should work too, only the python
     dependency causes problems on 16.04 at this point as with the
     other one:

     http://docs.datastax.com/en/cassandra/3.x/cassandra/install/installDeb.html


     Then we need to change the cassandra.yaml file to support
     a cluster (Snitch & Seeds) and listen on the private address
     (i.e. 10.0.0.2)

     In the old setup we had tombstones setup at 1 million, but we
     upgraded to 3.x to avoid that problem...


   f.4) backend node: install snapbackend and snap base

     For the backends we need a way for the end user to define which
     backends run on that server and which on this server (it could be
     all on all backend servers...)


   f.5) other(?): install snap base, allow to install SMTP, NTP (see (d) and
	actually NTP is probably part of "base"--anyway Cassandra installs
	it for us!!!), etc.



   In all cases we should automatically adjust the firewall, that is
   each package can include a set of rules that our snapfirewall (?)
   tool can use to extend the firewall (or open additional ports)
   [so if uninstalled, we need to make sure that extension file gets
   removed since it would otherwise leave some ports open!]


g) setup postfix -- I have a specific entry because this is a biggy, also
   we may want to look into automating "everything" we know how to do in
   postfix:
   
	. add users accounts
	. create aliases / virtual entries
	. add white lists
	. add black lists
	. offer to install postgrey (should be optional because some people
	  do not like such at all)
	. etc.

   g.1) sudo apt-get install snapbounce

   g.2) sudo apt-get install postfix

   g.3) make sure the installation includes the proper domain name
        (mydestinations = ... and myhostname = ...)

   g.4) add the necessary elements to support bounces:

	# Sat Oct  4 16:04:04 PDT 2014
	# Adding this in preparation for SNAP-67
	notify_classes = bounce
	bounce_notice_recipient = bounces@snapwebsites.com
	transport_maps = hash:/etc/postfix/transport.maps

   g.5) add a file named transport.maps with one line and run the postmap
	command to create the .db counterpart

	# SNAP-67
	# After changes, run postmap:
	#   postmap /etc/postfix/transport.maps
	#
	#localhost bounces:
	#mail.snapwebsites.com bounces:
	bounces@snapwebsites.com snapbounce:

   g.6) add one line of code to the master.cf so the snapbounce tool actually
	gets executed whenever a bounced email is sent back to postfix.

	# Service to save bounced emails in Cassandra
	snapbounce unix -       n       n       -       -       pipe
	  flags=FRq user=bounce argv=/usr/bin/snapbounce --sender ${sender} --recipient ${recipient}

   g.7) offer to install postgrey (optional)


h) setup a Cassandra node

   Setting up a node is relatively easy, however, by default, after the
   "apt-get install cassandra", you get a standalone node with a default
   setup in the "cassandra.yaml". So we need snapmanager.cgi to tweak
   the .yaml file for us, including the seeds and whatever else we want
   to change in there (i.e. the IP addresses need to be changed to work
   in a cluster.)

   As the cluster is growing, with a certain limit (maybe 5?) we want to
   update the replication factor of the system_auth keyspace because that
   keyspace need replication (although at this point we do not ourselves
   make use of authentication, if some of our users want to do that...)

   See: http://docs.datastax.com/en/cql/3.0/cql/cql_using/update_ks_rf_t.html

   Finally, we need to be able to change the replication factor of our
   cluster for the end users. You run the ALTER command once and then
   need to repair each node:

      cqlsh -e "ALTER KEYSPACE snap_websites WITH replication = { 'class' : 'SimpleStrategy', 'replication_factor' : 3 };" <ip of this node>
      nodetool repair
      nodetool repair
      ...

   The replication factor should be selectable by end users. The default
   should be the number of nodes minus one with a minimum of 2 unless
   there is only one Cassandra node, then 1 is used.

