
# Links

In Cassandra, there is absolutely no relational like scheme. You can save
keys in a row in one table allowing your to query another row. Good. But
we also have a heavy need in our data for a model where many parts are
linked together.

The main links we need are between our pages. A page as a parent and
children. These we want to create a tree to manage. We'll look into that
later.

Then in our Branch tables we use `links::...` fields to connect pages
together. For example, each page is given a `links::content::page_type`.
This is very important to have and implementing it in the front end is
extremely painful and wasteful too. On the backend we can use very super
simple pointers and we can have a many to many implemented using a simple
mechanism. In our frontend, we have to have an additional table and
many fields and it can easily break if any one WRITE fails.

What we want to be able to do is have fields of type "reference" which
allow us to connect things together. This is much more like an Object
Oriented Database than a Relational Database (i.e. we do not search for
the "reference", instead we use a pointer to it or at least the OID).

See the FILE-FORMAT.md for details about OID.

Our links use all for possibilities:

* One to One
* One to Many
* Many to One
* Many to Many

We really only need the first two because the Many to One or Many to Many
does not exist in a page. This is a side effect of multiple pages pointing
back to you.

Our existing links are also dual links. A points to B and B points back to A.
This is also important because when we delete A, we need to remove the link
to A from B.

See the links plugin for tons of details.


# Lists / Indexes

One place where we use links are lists and indexes. Both use the exact
same concept: they give us a way to filter rows using an expression
(WHERE clause) and to sort those in a given order.

Our Secondary Indexes will implement the Indexes which are the same as the
lists. So once we have the Indexes properly implemented in our database
the rest is just history. We can simple set the expression used to filter
the rows and _we're done_.

This means we will not be using Links anymore for lists and indexes.


# Tree (SNAP-381)

At some point we want to change the tree of pages using a tree table.

The tree table would be similar to our existing content table in terms
of key. That is, it would include the URL and that would give us a
reference to our content rows (an indirection).

The parent link would definitely not be required. We can just remove one
segment from the path to find the parent.

The child links should not be required. We should very easily be able to
do a _specialized_ SELECT to retrieve the direct children of a page by
using a tree index in this table.

