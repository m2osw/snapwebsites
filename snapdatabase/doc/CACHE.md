
# Cache

We use several types of caches to handle requests.

## Memory Management

The cache system is used on both sides.

### Cache on the Client

On the client side, the data first gets saved in a journal file.

The Service notices that happens and reads the data in memory and keeps it
here so any further request that may involve that data includes it.

Once in memory, the data is sent to the backend using the TCP to let the
backends know that the data is on its way and UDP to send the actual data.
Once at least one of the backends acknowledges that all was received, we
can mark that data as received here.

_Note: We want to keep really large blocks of data in the Journal and
stream it if needed. The size of "large blocks" is defined in a variable.
We still want to load enough to know what the row is about (i.e. we need to
at least have the key)_

### Cache on the Server

On the server side, the cache is used differently. There the idea is to
make use of the RAM as a way to very quickly access the data in the database.

I do not have any good point to make about this at this time. We want to
obviously keep hot data and get rid of data which is not been used/accessed.
There is also a maximum amount od memory that we want to limit ourselves to
read. We'll be using `mmap()` for each block.

## SET Cache

First of all, whenever we receive new data, we want to cache that data to
memory until it is fully received.

Remember that you receive data using UDP.

If we do not receive a UDP packet, we will ask the other nodes unless we
are a single node then we ask the client to repeat the UDP. That request
will be done using the TCP control channel.

We know how many UDP packets we'll receive and we give each one an index
so we can replace them properly in the buffer.

On a restart, the data in the Journals gets reloaded just as if the
`SET` just happened.

## GET Cache

Whenever we load data, we cache a lot of the tables blocks to find our way
to the actual data. Remember that for each key we may need quite a few
blocks (root, each level from the index, the data block, and if in use the
indirect index level).

TBD: this should be enough for us to keep the data in memory. After all,
that way the data does not need to be searched in the database again and
we have access to the other data entries in that block too.

