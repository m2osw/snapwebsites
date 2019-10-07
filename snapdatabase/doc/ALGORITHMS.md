
# Row Date

Whenever we set, insert, update, delete data in a row, the packet requesting
the change includes a date.

If the packet's date is older than the current date in the row, then the
command is ignored and we return as if it had happened. Someone beat us to
it and that's fine.

Two ways to handle the situation:

* Do a Read and if `Our Date` < `Existing Data Date` return.
* Always do the data insertion, but include the Date in the key. When doing
  a `GET`, only returns the data with the largest date and ignore the rest.

A write is often less costly, however, the indexing can be very costly so
I do not think that this the second option is a good idea at all.

# Sorting Rows

We have several layers to sort the data:

* Computer Selection (Partition)

    First of all, we want to be able to use a cluster. That means saving
    the data on one specific set of computers. This is generally done
    using the Murmur hash of the `PRIMARY KEY`. That hash is then used
    to select the computers that will replicate this data.

    In a relational RMDB, this is called a _Hash Index_.

* B+Tree

    This method is used to sort rows in our tables. Contrary to Cassandra
    and Big Tables, we prefer to have separate indexes and not use one
    row to index everything (Actually, Cassandra 3.x fixed the problem
    with their secondary indexes which are _properly_ distributed).

    At this point all of our sorts will be using binary sorts.




vim: ts=4 sw=4 et
