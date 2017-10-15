
A few things that still need to be done in this project:

  1. Remove required dependency on Cassandra, we should have a version
     that works without any database because these servers will anyway
     be capable of sending their data to each others.

  2. Implement all the network shit... listener, PING reply, data sharing

  2.1  With the getifaddr() function we get all the information, per
       interface, of the amount of data transferred and received.
       We should look into a way to use that data (knowing that it
       does not get reset...)

       Note: the other method to gather information from the network
             is to use the ioctl() function with the netdevice structure
             (see "man 7 netdevide")

  3. Plugin for Snap! Websites to look at the data.

