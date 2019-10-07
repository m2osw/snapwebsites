
# Locking Mechanims

We can have many layers of locks in an attempt to optimize our database locks.

However, the most important part is to avoid a lock on something which we are
not going to be modifying.

In most cases, for local files we just need to lock that local file. One
exception to this rule is the change of schema which must happen in all
copies the data we're managing.

Our existing snaplock still has a _small bug_ (when your cluster isn't up as
expected it may never wake up properly--but I may have fixed that already),
it still generally works really well. It also allows us to lock anything at
any depth. For example, we can go as low as a bit in a cell (not like we
are going to ever do that, though).

## Counters

We use locks for our counters so we can offer counters that are not just
increments and decrement but a real current number.

For example, we want to be able to give our users a clean incrementing
counter. This is really useful in this case.

This does not prevent us from using a local counter as we do often for
local unique numbers.


