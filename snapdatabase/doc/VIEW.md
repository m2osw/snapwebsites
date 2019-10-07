
# Implement a form of VIEW

A VIEW is generally a WHERE clause you save in your database. It acts like
a table. Most advance databases allow you to do UPDATE on VIEWs.

Most complex views have FROM clauses that reference multiple tables, which
makes it really complicated to allow writes.

In our case, I'm thinking that we could allow views by listening to changes
to the tables involved in your VIEW. Any time there is a change, check whether
that row would be included and not included in the view and update the view's
index.

In other words, the view is an index against one or more rows that come from
one or more tables.

This is much more of a dream at the moment especially because I do not have
any use for such but I was thinking about it as I was writing information
about `snap_expr` being needed for our WHERE clauses. It makes sense that
a secondary index could also be managed like a VIEW. Think about it this
way: a secondary index sorts rows using the _real data_. A VIEW would be
like another table. It would compute the key from a set of rows picked in
one or more table and generate the equivalent to a Primary Index which
is used to travel through a virtual table. So most of everything should
already be available to implement this feature.

