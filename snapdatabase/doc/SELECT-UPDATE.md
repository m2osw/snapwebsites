
# Atomically Retriving a Row

Especially for Journal Entries, it is very practical to read a row and
update it at the same time. This is very much what one does when one
performs a mutex or semaphore lock.

The idea is to do a GET of the existing data and then do an update with
a new status (i.e. update at least one column). The client will receive
the old status, but knows of the new one since they included that in
the request.

For example, the status of a journal has to be updated from `WAITING`
to `PROCESSING` just at the time we read the row. If the row is not
`WAITING` at the time we read it, then we're too late and have to
try with another row. If it is `WAITING` we want to change the status
to `PROCESSING` before any other process on any computer has a chance
of taking that same row (i.e. we want exactly one backend to work on
one journal entry at a time).

This is done by using a lock:

    LOCK <table> ROW <key>
    result = SELECT ROW FROM <table> WHERE KEY = <key>
    IF STATUS = `WAITING` THEN
        UPDATE <table> SET STATUS  = `PROCESSING`
    END IF
    UNLOCK ROW <key>
    RETURN result

If the function returns `result` with a status of `WAITING` then it was
a success.


vim: ts=4 sw=4 et
