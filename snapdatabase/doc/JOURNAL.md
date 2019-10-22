
# Requirements to Support the Journal Type of Table

Specific requirements to make the Journal a reality so we can eliminate
that from our backend processes which right now kill the Cassandra
database.

The type of table is JOURNAL. This allows us to have the system add a
set of predefined columns to our schema (see below) to handle all the
nitty gritty of the journaling capability.


## Special Settings

### Immutable Payload

The Journal has this special feature that the user data is never going
to change. That means a row once written will never have to move. Although
it may be _replaced_ later and _deleted_.

Here by _replaced_ we mean that we can get a new copy of the same payload
with a different Start Processing Date. (So you could say that it in fact
changes! But in reality it just doesn't really.)

The table will allow for updates to these few columns:

* Status
* Start Processing Date
* Timeout Counter

Then it can mark the row as available space when it gets deleted (the
work was done).

This feature also allows us to use the covering index feature, which is to
save all the data with the primary index instead of having separate data
blocks. This means the index blocks hold less entries but you have one
less pointer. This works well for journals because the files get deleted
(or at least reset/truncated) often and this no heavy cleaning will be
necessary. It should also make our cursor faster.


### Ignore Schema Changes

The Journal does not last long so if the user makes a change to the schema
we ignore it in existing tables. We will continue to use the old format.

This can be problematic for new additions which need to be don to a new
table if new rows were added. However, it very much simplifies the existing
table handling.



## Schema

Each row in a journal includes the following columns:

* Insertion Date (Predefined, Type: `time_ms_t`)

    Note: there is an Insert Date in all the rows of all the tables. This
    cell is mandatory.

    The Insertion Date is the date whenever the row was created. This date
    actually comes from the client (it is in the packet sent by the client)
    so we can make sure that we only keep the latest data (See ALGORITHMS.md).

* Expiration Date (Predefined, Type: `time_t`)

    Note: there is an Expiration Date in all the rows of all the tables,
    although this is not mandatory. If not defined, it does not expire.

    A date after which this row is considered _deleted_.

    If the `Expiration Date` is set to 0, then it does not expire which is
    the default if the column is not defined.

* Priority (Predefined, Type: `uint8_t`)

    The Priority is used to sort the jobs in a given order. Jobs with an
    equal priority are sorted by Process Date.

    If your journal does not require a priority, use the same value for
    all the insertions and the entries will automatically be sorted by
    Process Date.

    The items with the smallest Priority are worked on first.

    Here are the existing priorities as defined in the specialized journal
    implementation of the list plugin:

        static priority_t const     LIST_PRIORITY_NOW         =   0;      // first page on the list
        static priority_t const     LIST_PRIORITY_IMPORTANT   =  10;      // user / developer says this page is really important and should be worked on ASAP
        static priority_t const     LIST_PRIORITY_NEW_PAGE    =  20;      // a new page that was just created
        static priority_t const     LIST_PRIORITY_RESET       =  50;      // user asked for a manual reset of (many) pages
        static priority_t const     LIST_PRIORITY_UPDATES     = 180;      // updates from content.xml files
        static priority_t const     LIST_PRIORITY_SLOW        = 200;      // from this number up, do not process if any other pages were processed
        static priority_t const     LIST_PRIORITY_REVIEW      = 230;      // once in a while, review all the pages, just in case we missed something
        static priority_t const     LIST_PRIORITY_TIME_ALLOWS = 255;      // last page on the list

* Process Date (Predefined, Type: `time_ms_t`)

    The date when the entry has to be processed. It can be set to _now_
    in which case it will be processed as soon as possible.

    Process Date should always be larger or equal to Insertion Date.

* Status (Predefined, Type: `char`)

    Further, each row includes a status.

    Possible statuses are:

    - `W` (Waiting) -- the row is just sitting there at the moment
    - `P` (Processing) -- the row is being processed

    Once done, a journal entry gets deleted so there is no need for
    a `D` (Done) status.

* Start Processing Date (Predefined, Type: `time_t`)

    The date when we started processing this row.

    Depending on your table settings, you can specify how long processing
    is expected to take. For example, you could use 1h. If the `Status`
    says `PROCESSING` but `now - Start Process Date > 1h`, then the
    `Status` will be considered to be `WAITING`. i.e. we suppose that
    the process failed and the database was not updated accordingly.

    If your processing can be really slow or vary a lot, one method is to
    update the `Start Processing Date` every now and then to make sure
    that your _lock_ doesn't time out.

    **IMPORTANT:** If an entry is `PROCESSING` for too long and we view it
    as a `WAITING` entry, then we must add one to our `Timed Out Counter`.
    If that counter is too large, we want o mark the entry as done either
    way because the prceoss is likely failing. We should also raise a
    flag so that way the `snapwatcher` processes can inform the
    administrator.

* Timeout Counter (Predefined, Type: `uint8_t`)

    Counter incremented each time the processing does not end with a
    call to the `Done()` function. This means the process is either
    taking to long or it crashed.

    If another process checks on this journal row or our database timeout
    tick is received, then we increase this counter and mark the row as
    `WAITING` again, letting another backend start the processing again.

    By default the column is not defined. The default value is 0.

* Key (Required, Type: User Defined)

    Whenever the clinet adds an entry to the journal, we first search
    using this key (see the Secondary Index: Key). When the key already
    exists and the Status is still `WAITING` we replace that entry with
    the new data as follow:

    - Insertion Date -- do not modify
    - Expiration Date -- keep largest
    - Priority -- keep smallest
    - Process Date -- keep the largest
    - Status -- do not change, must be `WAITING` for an update to happen
    - Start Processing Date -- do not modify (not from user, updated by
      special cursor)
    - Timeout Counter -- reset to 0
    - Key -- equal since we searched using the key
    - User Data -- replace

    The reason for this is to avoid repeating work multiple times on the
    exact same key.

* User Data (Required, Type: User Defined)

    The `User Data` is the user payload for this journal. In most cases this
    is a key to the data that requires processing. The user can define
    whatever fields necessary for his data.


## Short Lived Tables

The journal can optimize the indexing it makes because we want to use
a table for 1 hour, 1 day, or 1 month. After that amount of time, it
only gets emptied and once empty, we delete the entire file (or at least
reset it to be reused as if it were brand new).

Here are several ideas on how to manage the file once empty:

* Delete the File

    This method makes it easy and really fast to know that it's empty.
    However, it may generate more fluctuation on the drive meaning
    that we end up with more fragmentation.

    A deletion may be necessary to avoid using too much space on the
    disk. At least the Truncate feature should be used.

    Note: on an SSD, fragmentation happens in a completely different manner.

* Reset the File

    The files are organized in blocks of data. The very first block
    (first 4Kb) defines what is allocated in the file. So it is easy
    to write the necessary to clear the allocations.

    One drawback: some of the data could be viewed as security data and
    it may not be good to keep it behind. At the same time, that would
    be true either way.

* Truncate the File

    Like the Reset, we could keep the first top blocks, but then truncate
    whatever we do not need. So a basic file would have 3 blocks: top block
    with magic, bloom filter, and pointers to top index. The top index
    and the first block of data.

    This may be very useful if once in a while the file grows really big.
    Without a truncate, we could end up using more space than necessary
    long term.


## Indexing

We want replication for the journals because a computer could go out of
commission and we do not want to lose any data when that happens.

However, since the data is always expected to be short lived and altered
very quickly, we want the journal tables to spread between N nodes and
avoid partitioning for both, the data and the index. In other words,
the journals are not expected to grow horizontally. However, each journal
table can sit on a different set of N nodes. So if you have N=3, 20 journals,
and 200 nodes, it is still horizontal because you only need 3 x 20 = 60
computers to to handle all the journals independently (on separate computers).

We need several indexes to allow for all the features to work as expected.

### Primary Key

The journal Primary Key needs to be the Priority and Process Date.

**WARNING:** Since it is a primary key, it also needs to be unique. One
simple solution is to also add the User Data column to the Primary Key
or at least a Murmur of that data.

    <Priority>:<Process Date>:<User Data Murmur>

We could include the status, but since the status is going to change
much, it may be a waste of time since it would change the index _all
the time_. That being said, that way we could easilly search for the
next entry which is `WAITING` to be picked up.

TBD: if the processing of all of our journals remains very slow altogether,
we could also _merge_ all of our journals in one table. All we need to do
is add a Journal Name column and use that in our Primary Key:

    <Journal Name>:<Priority>:<Process Date>:<Key Murmur>

I think that this would not add much overhead to this table in itself,
however, it would not grow horizontally at all. So there is a trade off.
That being said, with 3 dedicated servers, it would probably not be a
big deal at all.

### Secondary Index: Expiration Date

This timestemp is used in an index so we can find rows that need to be
deleted. Along with an Event Dispatcher timeout object, we can very easily
wake up our process and emit the DELETE as required.

Note that a GET needs to test the Expiration Date too. If the row has
expired, then the GET has to ignore it.

The FILE-FORMAT.md has information about this secondary index and it
actually knows how to handle it already. The Journal and other tables
do not have to replicate that work each and every time. Just the
existance of an expiration date somewhere is enough for the index to
automatically get generated.

### Secondary Index: Key

The Primary Key gives us access to the data by priority and processing date.
However, when the user adds a new entry, it may overwrite an existing entry.
We know that because the user has a Key in his User Data. There can be only
one entry with that Key. This index can use a Murmur of the key so that way
the length doesn't vary (16 bytes).

So when the user updates the same Key again, the dates are all updated.
In other words, the Processing Date is likely going to be pushed back a few
seconds or minutes. This allows us to avoid processing the same piece of
data twice in a row. (i.e. in most cases, it would be close to impossible
or very costly to check whether the last processing needs to be redone
without just redoing it directly.)


## Special Cursor Capability

Some processes add lines to the journal for work they want to have
performed in the backends.

When the backends detects that there is work to do (i.e. receives a `PING`
or wakes up and checks the database), it retrieves the line specifying the
work to be done and sets the row to `PROCESSING`.


## `LISTEN` on `Start Processing Date`

It would be really cool if the database would emit the `LISTEN` message
when the `Start Processing Date` is reached. Since we have a specialized
system, it is very much possible and can be dearly optimized since we
can have that information kept in memory and use a timer to emit the
messages. We already have the algorithm in our snapbackend only it
uses a **heavy** SELECT to find the smallest possible time at which
to wake up again.


## Snap Database Library

The library needs to support:

* Table Declaration

    The table declaration is an XML file with the list of fields in the
    table. This is important so we can actually have 




## Communication

The library makes use of the Event Dispatcher library for all communications.

It uses the TCP message interface for the controller and the UDP support
to broadcast the data. UDP is much lighter than TCP so it should be able
to send the data faster (although on a local network, it's probably about
the same).



vim: ts=4 sw=4 et
