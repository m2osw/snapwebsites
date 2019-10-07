
# File Format

We handle the data in an RDBM type of format using blocks to allocate space
for the data.

We use such a mechanism, rather than a write at the end because that way we
can very often avoid wasting more space (i.e. we can have a form of instant
compaction of the data).


## Files Per Table

### Schema

The schema is defined in a Table XML file.

See libsnapwebsites/src/snapwebsites/tables.xsd for the documentation of
that XML file format.

### Journal

The incoming data is immediately written to a Journal and also kept in
memory until the time when it gets written to the main Data table.

In memory, we keep the position (`tell()`) where the data gets written
to the journal.

#### Basic Implementation

Whenever we receive the data, it is kept in memory in one of our objects.
Until we can fully acknowledge that we received all the data, the objects
remains in a state such as "New Object".

Once all the data was received, the object gets saved at the end of the
journal in `sync` mode. Once that is complete, we have the data in a safe
place so we can reply to the sender that we are all good.

Note: We may use the `O_DSYNC` in this case since writing will be done in
one single block (open, exclusive lock, write, unlock, close).

#### Large Data

When the incoming data is larger than a given threshold, we save it
in its own file instead of keeping it in memory. This allows for no
delay from other connections and we do not need to keep that large
block in memory.

The writes to the secondary file can be done as normal so in other words
the O/S will probably keep it in its caches.

Once we receive the entire file, then we may want to run `fdatasync()`
to commit the data to disk. Once that command returns, then we can
acknowledge that the write occurred safely.

### Data

We can manage all the data in a single file. In this case, we have one
table with its indexes, list of free blocks, etc. all in one file. This
is practical if the file is not expected to grow very much (i.e. below
1 Mb of data, for example).

Note that the XML Schema is always separate.

We can also manage one file for the data and one file per index.

For smaller files, like a minimal journal table, everything in one file
is viable. For medium to large tables, one file for the data is probably
much better.

In order to allow for very large data, we have to either support files
of more than 4Tb or allow for multiple data files. On the other hand,
you can get really large databases by having many computers rather than
super large files. Having a single file is certainly much easier to
manage. That being said, with a 64 bit pointer, it's also very easy to
point to any file. We can remove the bottom N bits (31?) and the top
33 bits is a table number.

Also, if a table has one cell which is a really large blob, we may want
to conside having a separate table with those blobs and the main table
with the B+tree and basic key/data plus a pointer to the blob.

### Implementation

* Solution 1. Without `INDR` table/blocks:

    Note: a file represents a set of blocks. We can also put all the data
    in one single file. The advantage of multiple files is to somewhat
    increase the total size of the Data file. We can also support multiple
    data files.

        +------------------+     +------------------+
        |                  |     |                  |
        |   XML Schema     |     |      Journal     |
        |                  |     |                  |
        +------------------+     +-------------+----+
                 ^                             |
                 | Checks for Updates          |
                 |                             v
        +--------+---------+                   |
        |                  |                   |
        |      Data        |<--------+---<-----+
        |                  |         |
        +------------------+         |
                 ^                   ^
                 |                   | (no compaction use direct pointers)
                 |                   |
        +--------+---------+     +---+--------------+
        |                  |     |                  |
        |  Primary Index   |<----+ Secondary Index  |
        |                  |     |                  |
        +------------------+     +------------------+
    
    The Secondary Index may point to the Primary Index or directly to the
    Data. The advantage to use the Primary Index is to simplify the
    compaction quite a bit.
    
    If we do not have secondary indexes, then we can do compaction anyway.
    
    If we have secondary indexes, pointing to the Primary Index is best.

    TODO: make sure to include a counter (number of rows) in each one of
    our indexes. The Primary Index does not require it since all the
    rows are always present in that index. Secondary indexes, however,
    may have less.

* Solution 2. Use `INDR` to for Compaction Feature

    This solution makes it easier to allow for the compaction feature.
    All the tables, except the `INDR`, make use of a row number and the
    `INDR` is used to convert that number in a pointer in the data table.

        +------------------+     +------------------+
        |                  |     |                  |
        |   XML Schema     |     |      Journal     |
        |                  |     |                  |
        +------------------+     +-------------+----+
                 ^                             |
                 | Checks for Updates          |
                 |                             |
        +--------+---------+                   |
        |                  |                   |
        |      Data        |<------------------+
        |                  |
        +------------------+
                 ^
                 |
                 |
        +--------+---------+
        |                  |
        |     Indirect     |<--------+
        |                  |         |
        +------------------+         |
                 ^                   |
                 |                   |
                 |                   |
        +--------+---------+     +---+--------------+
        |                  |     |                  |
        |  Primary Index   |     | Secondary Index  |
        |                  |     |                  |
        +------------------+     +------------------+

    When using multiple files to implement the table, this is certainly
    very welcome. All the files except the data file would use row numbers.
    So in effect the data file has a super fast counter based index as a
    result.


## Schema Global Settings

A table is assigned a certain number of settings that are used table wide.

### Read-Only Table

In some cases we want to mark a table as a read-only table. In that
case, we may have a fallback table where we can write. This happens in
the Journal when a schema change happens. The current table becomes
read-only and new additions are expected to be added to a new file.

This is very useful to avoid having to convert any data in the old
table, but it requires our queries to happen in both tables (i.e. if
a row in the old table gets replaced by a row in the new table, then
you have to ignore the row in the old table by deleting it there,
so it's not 100% read-only).

This technique may be used for any number of tables that we want to
compact or modify in some other way.

### Handling of Schema Changes

Whenever the schema of the rows is updated we have several mechanism to
get the data updated:

* Only Update On Write

    Any time we do a write, we use the newest schema for the new rows.
    This is the default and it will allow for the schema to be upgraded
    as time passes.

    Data which is immutable never gets updated.

* Also Update On Read

    Whenever a user reads a row, we have to do the necessary changes from
    the old row to the new format. At that point we have the data for the
    new schema and we can as well save it to disk so next time we can at
    least avoid the conversion step.

* Background Update

    Have a process which goes through the entire database and update each
    row to the newest format. This can be done in the background and
    _very slowly_ (assuming you do not make a ton of changes to your
    schema, which should always be the case, being slow on this one should
    not be a problem. The idea is to not impact the normal access to the
    database.)

    This background process can use the a cursor to read all the rows.
    Since it this mode implies the "Update on Read", just reading all
    the rows once will automatically update all the rows to the newest
    schema.

* Immediate Update

    This case means we want to have the table updated immediately. This
    process uses the same technique as the Background Update: it reads
    all the rows once and that has the side effect of writing the data
    in the new format.


## Type of Blocks

We want to use several types of blocks to maintain the database:

### Header (SDBT)

The header defines where the other blocks are and includes the bloom filter
data.

* Block Type (SDBT, `char[4]`)

    This block defines a Snap Database Table. Since it is at the very
    beginning, it is most often used as the Magic for the entire file.

* Version (`uint32_t`)

    The version of this table

* Block Size (`uint32_t`)

    The size of one block in bytes. By default we use 4Kb, but some tables
    may defined a larger size such as 64Kb. It will depend on the size of
    data expected in the table. Having a size larger than any blob that is
    going to be inserted is a good idea, but there are trade offs too (i.e.
    we do not want 1Mb blocks for indexes, for example).

* Table Definition (`uint64_t`)

    A pointer to the full schema definition which appears in another
    block. It is the parsed data from the XML file defining a table
    in binary form that we can load and use immediately.

    We have to have our own definition so if the user makes a modification to
    the XML file that was first used to declare this table, we can easily
    apply those changes. For example, a column which type changes requires us
    to go through all the data and update that column data to match the new
    type (i.e. `string` becomes `uint32_t`, we must make sure that all the
    cells with that column are convert to a `uint32_t` otherwise we won't be
    able to read the data properly). However, we can easily do that work
    incrementally. So changes of the schema will be instantaneous in our
    database system.

    To get the incremental change to work, we time stamp each schema and
    keep a copy of each one in our database.

    When updates occur in the Schema, we can handle them by keeping a copy of
    the old schema and then _know_ whether a row is schema version 1, version 2,
    etc.

    > _Note: to avoid having to add anything more to every single one of our rows
    > we actually timestamp the schema versions. Since we always have a date in
    > each one of our rows, we know which one to keep and also that date can be
    > used to find which schema was used to save that row._

    This means we do not have to do anything to the existing table at all.
    Instead, we view rows using the old schema as _read-only_. If a write
    occurs, the old row gets read for any field that does not appear in the
    new row and then the old row gets deleted as the new row gets saved.

    Once all the old rows were overwritten, the old schema can be removed from
    the table. To know whether that event occurs, we can have a counter in each
    schema. When adding a row for schema version 1, add 1 to its counter.
    When removing a row from schema version 1 and saving it using schema version 2,
    subtract 1 from the version 1 counter and add one to the version 2 counter.

    TBD: in order to do this work incrementally we would have to mark
    all the cells that were already transformed. One possibility is to
    include the type in the column, do the transformation, then remove
    the type once all done and it is not a dynamic type (i.e. `any`).
    Another way is to have a new column with a new name. If the column
    is NULL (which it would be in existing rows), then default the
    value to the old column after convertion with a default in case
    an error occurs while applying the conversion. Later you could then
    delete the old column and rename the new one. Also, we could leave
    the old column alone up until we discover that it exists and either
    a new column already exists (just delet the old one) or the new
    column is still missing (then copy the old one to the new one
    applying the type conversion first). Speaking of that, just
    changing the type would then be possible by renaming the old
    column to something like "old_<name>" and giving the new
    column the old name "<name>". Then it works like a charm just like
    that because the new column picks the old data when required and
    thus we do not need to do anything immediately. Only whenever the
    data gets accessed and we notice the discrepancy (and we consider
    that we have enough time to apply such changes--i.e. on a heavy
    `SELECT` we may want to do it only once or twice, not 10,000 times
    for each row being selected.).

    Note: see also the SCHEMA.md about the change of schema version. This
    is useful to avoid having to fix the data of millions of rows all at
    once when we can do it incrementally (or even not at all if the data
    is pretty much considered immutable).

* Indirect Index (`uint64_t`)

    A pointer to an `INDR` block or, once the first `INDR` is full, to
    a `TIND` block.

    The indirect index allows for many deletions to happen and still allow
    for compaction of blocks. This is particularly useful when the data is
    not clustered and the table uses many writes and deletions, such as a
    session table.

    These blocks add one indirection, though, but with about 500 pointers
    per block (with a 4Kb block) you get a 250,000 rows with just 2 levels
    and 125,000,000 with 3 levels. With much larger blocks (say 64Kb) we
    would get some 8,180 with just the first level and 66,912,400 with
    just two levels.

    Note: this imposes an OID on each row. This means we need a lock of
    the file first block and incrementing the OID each time a new row
    gets added to the database. If we also implement the free list, we
    get a similar lock to manage that list.

    The row itself does not need to save the OID although it is useful for
    the compaction process. If we accept a _slow_ compaction process, we
    can search for the OID using the row key and searching the primary index.

    Whenever a row is deleted, we want to add its OID in a free list. This
    is done by saving that OID in the first block and saving the OID in the
    first block in place of the pointer at that OID position. Thus creating
    a linked list. When allocating a new OID we first check whether the
    free list is empty or not. If not, use that OID, if it is empty allocate
    a new OID. This means the rows OID get reused.

* Last OID (`uint64_t`)

    The last OID that was used. If 0, then no rows were allocated yet. This
    value is used to allow for the `INDR` blocks.

    This feature doesn't get used if the file does not require an indirect
    index.

    WARNING: This OID is specific to this very table. On another computer
    with exactly the same data, the OID is likely going to be different
    because the data would have been inserted in a different order.

* First Free OID (`uint64_t`)

    Whenever we add a new row we assign an identifier to that row. That
    identifier is used everywhere that row gets referenced. The Indirect Index
    can then be used to find the data in the data file.

    The Free OID is a list. At the location in the `INDR` this Free OID points
    to is another OID (the previous one that was released). Up to the NULL
    which is represented by 0 (note that position 0 is valid and is used
    by OID 1; when we search for the row pointer, we use `OID - 1`).

    This feature doesn't get used if the file does not require an indirect
    index.

* First Compactable Block (`uint64_t`)

    Whenever a DELETE happens in a block, we are likely to have the ability
    to compact that block to save data. That block is therefore added here.
    That way we know we can process this row at some point and we avoid
    wasting time attempting to update any one block (i.e. in most cases, on
    a very large table, it is very unlikely that so many blocks would require
    compaction).

    Data blocks allocate data with a minimum of 8 bytes. This allows us to
    create a list of Compactable Block, so what we do is read the First
    Compactable Block. Load that block in memory. Read the first 8 bytes
    which represent the poitner to another block which can be compacted.
    Save these 8 bytes in the First Compactable Block.

    To have a double security about this, we should save the First
    Compactable Block in another field, copy the next block in the
    First Compactable Block field, then work on the block. Once the
    work is done, set the other field to 0. When we start the process
    over, we first look for that other field. If not 0, then work on
    that block first. This is much more likely to work properly in
    all cases even if we are killed before we are done.

    We still can have an audit process which verifies that a block is as
    expected and that could be one of the elements to verify.

* Process Compactable Block (`uint64_t`)

    The block being processed by the compaction service.

* Top Key Index Block (`uint64_t`)

    Pointer to the first B+tree Primary Index block.

* First Key Index Block with Free Space (`uint64_t`)

    Pointer to the first B+tree Primary Index block with space available.

    TODO: look into implementing a set of first key pointers depending on
    the amount of space available. If under 64 bytes, between 65 and 128,
    between 129 and 256, etc. That way we can directly go to a block with
    at least the amount of space we need for the row being written.

* Timeout Index Block (`uint64_t`)

    Pointer to the first B+tree index block with the rows having a
    Timeout Date are sorted by that date. Note that we only allow
    one entry per row. If a cell within a row has a timeout date
    then we still add this as if the row had that timeout and when
    it happens, we just remove the cell. That should be more than
    fast enough.

* Secondary Index Blocks (`uint64_t`)

    A pointer to a block representing a list of secondary indexes. These
    include the name of the indexes and a pointer to the actual index
    and the number of rows in that index (it is here because the `TIND`
    is also used by the `PRIMARY KEY` which dos not require that number
    and also when a new level is create the `TIND` can become a child
    of another `TIND`).

    Add support for a `reverse()` function so we can use the reversed
    of a value as the key to search by. This allows for '%blah' searches
    using 'halb%' instead. Yes, we could add a column with the reversed
    data. But having a reverse function is going to be much better
    than duplicating a column.

    Like with VIEWs, though, we could have calculated columns (these are
    often called virtual column). So query the `eman` column could run a
    function which returns `reverse(name)`. We already make use of such
    a feature for indexes to inverse `time_ms_t` fields and sort these
    columns in DESC order (i.e. `9223372036854775807 - created_on`).
    Actually, we have some "complex" computations that we repeat in many
    indexes when it could be one single virtual column in the Content
    table. This is exciting.

* Tree Index (`uint64_t`)

    For the Tree table, we want to be able to select the direct children
    of a page. So we want to have a table with a specialized index where
    a query could have a WHERE clause such as CHILDREN_OF('path').
    This table would allow us to travel to 'path' and then return all
    the children under 'path' in one quick swoop.

    The table could also be built from the Primary Index assuming that
    the primary index uses 'path' as its key, it could have a pointer to
    a list of children in a separate index table.

    For sure, when this feature is turned on, the key must be a string
    with a path. Then we can simply remove one segment from the path to
    find the parent and save this new row OID/pointer as a child.

* Deleted Rows (`uint32_t`)

    The number of rows that have been deleted in this file so far.
    This is useful to know whether the Bloom Filter should be
    regenerated and since it's a very expensive process, we do not
    want to do it too often.

    Note only that, this process needs to be incremental because really
    large tables are not likely to be done quickly enough for the new
    filter to be done in one go. In that case we allocate another block
    to save the new filter and once done copy that data here.

* New Bloom Filter (`uint64_t`)

    A pointer to the block holding the new bloom filter whenever it gets
    regenerated. This block can be released once the new filter was
    regenerated.

* Bloom Filter Size (`uint32_t`)

    The Bloom Filter is at the end of this block. This indicates its size.
    Large blocks can use larger bloom filters which decreases our chances
    of false positive.

    Some tables, such as Journals, do not require a bloom filter at
    all. In this case this size is set to 0.

    The one problem with the bloom filter algorithm is that it needs to
    be regenerated once in a while if you do a lot of deletion. This is
    why we have a counter so we know when that happens.


### Free Block (FREE)

Whenever a block is added to the file, we mark it as a `FREE` block and
add it to the linked list of `FREE` blocks. Whenever a block becomes
available again we clear it, mark it as a `FREE` block and then add it
to our linked list of `FREE` blocks.

We want to add 64Kb at a time so if your
blocks are 4Kb each, this is 16 blocks at once),

#### Sparse File

### Free Space Block (FSPC)

Whenever a block of data gets used we write some data to it and the rest of
the space is "free space". That "free space" is connected to a Free Space
Block so we can very quickly find the block with enough space to save the
next incoming blob of data.

* Block Type (`FSPC`, Type: `char[4]`)

    The type of this block.

* Free Space (Type: `uint64_t`)

    This is an array of pointers to a first block with free space. That
    block will include the next pointer (i.e. linked list).

    We always allocate a bare minimum of 16 bytes, although I will have
    to look closer because I think it is much more likely to be larger
    (64 bytes?) So this array is one pointer per possible size which is
    a multiple of the minimum size we'd allocate for a row.

    Assuming it is 16 bytes, the possible number of entries in a 4Kb
    block is 256 so this Free Space array is 256 `uint64_t` pointers.
    However, that would include the Block Type space, which is not
    possible. The fact is that any block in the BLOB


### Allocation Map Array (MAPA)

DO NOT IMPLEMENT: I think that a list of empty blocks is going to be
much more effective and for free space, we want a list of blocks with
N bytes still available, since a row is at least 16 bytes, we can put
all such entries in a single block making it easy to handle.

In order to quickly access any AMAP, we want to have an array of pointers
to all the AMAP. This is similar to the Top Index. Here we have an array
of pointers to AMAP blocks in order. So we can use very simple math to
go accross the blocks and download the correct data.

The block includes the following

* Block Type (MAPA, Type: `char[4]`)

    The magic for this block.

* Level (Type: `uint8_t`)

    With the level we can calculate how many AMAP blocks (level 0) or
    how many MAPA blocks (level 1 or more) this block points to.

    Say one AMAP represents exactly 4000 blocks, that is the number of
    entries at Level 0. Say the MAPA represents 500 block pointers,
    then a full MAPA level 0 represents 500 x 4000 = 2,000,000 blocks
    in your file. At 4Kb per block, this is about 8gb of data. With
    yet another layer, you get a total of 1 billion blocks addressable
    which represents total of 4Tb of data.

    _Note: our pointers are 64 bits so we are limited in size to 2^64.
    (18,446,744,073,709,551,616 bytes, 18 exabyte in one table, however
    your File System probably has a lower limit.) This is one table file
    on one disk. If you have many nodes you can have much more data
    total._


### Block Allocation Map (AMAP)

IDEA: We could also have one byte per allocated page which allows us to
include data in the AMAP about the page such as its type on 4 bits and
possibly 4 bits as flags representing a form of status. Now in most cases
we do not need to check the type of a block because a pointer sending us
there is enough to determine that. Yet, the indexes have 3 different
types and we need to know what that is.

The blocks currently allocated are defined in block allocation maps.
If many `DELETE` occur, some blocks may become free again and thus
appear back in the table. A block which is full has its bit set to 1.

This block includes a small structure at the start with:

* Block Type (AMAP, `char[4]`)

    The magic for this block.

* Previous Block of Allocation Map (`uint64_t`)

    These blocks are linked between each others.

* Next Block of Allocation Map (`uint64_t`)

    These blocks are linked between each others.

* Next Block with Free Entries (`uint64_t`)

    In case a map is completely taken, we can skip it. This pointer allows
    us to directly go to a map with free data.

* Free Space (`uint64_t`)

    This represents the largest buffer we can allocate in this one block.

* Allocation Map

    The rest of the space is the allocation map. Each Block Allocation Map
    has this many bits that represent one block that can be allocated.
    When the bit is 0, there is still enough free space to consider checking
    out that block. When the bit is 1, the block is considered to be full.


### Top Index Block (`TIDX`)

A Top index block represents a B+tree index.

_Note: for the Key Index, the first B+tree level is actually a direct
map using the first N bits of the Murmur code. So that way we can
very quickly go to the second second level (avoid one binary search).
This is likely to be parsed and limited since it's already cut down
by partitioning those keys._

The fields of the Top Index Block are as follow:

* Header (`TIDX`, Type: `char[4]`)

    The header indicated a Primary Index.

* Count (Type: `uint32_t`)

    The number of Indexes currently defined in this block.

* Size (Type: `uint32_t`)

    The size of one index entry (so we can change the size of the key).

* Index (Type: `char[8]` [key], `uint64_t` [pointer])

    Each entry in the Top Index are a small part of the key (up to 8 bytes?)
    and then a pointer to the next level which is either another Top Index
    Block or an Entry Index Block.

    These 16 bytes are repeated up to the end of the block.

    _Note: the size of the key is no much of a problem for a Murmur hash,
    however, for a key which is user defined strings or random binary,
    it will certainly have unwanted side effects if too short._

Note that a top index block may end up filled up. This usually happens when
more of the data match a given number rather than being spread all over the
place. When that happens, a good B+tree implementation starts moving the
data between the various blocks in an attempt to having an additional level
in the B+tree, because an extra level means an extra read to reach the data.

Our implementation could use weights where level N knows of the weights (how
full) level N + 1 is. When it detects a relatively large discrepancy, it can
start moving data early on in an attempt to save ourselves from having to do
it later once some of the blocks are already full.

We never want to have a write blocked by a B+tree being full. So another
solution to this problem is to _temporarilly_ allow the creation of a new
level (so level N + 2) and at that time we request the weights of the blocks
to be adjusted. At some point we may then be able to remove that new level
which we used to make the write go really fast.

**Note: slow writes may not be much of a problem if the journal is not
growing very fast. I'd have to see what happens over time. Adding new
levels is fine if long term we can fix the issue anyway.**

Note that the system must be capable of removing levels anyway because we
want that to happen when enough deletions occurred and a level is not
required anymore. This is known since we have a total number of rows
in the table and we can calculate how many levels are required total
and see whether the level we're at could be removed (although if the
level before that is nearly full, it may not be useful to waste the
time to remove that extra level just yet).

### Secondary Index Block (`SIDX`)

**WARNING:** Secondary Indexes are probably required in separate files
because of the partitioning scheme. This also means that we end up
with a "broken scheme" when it requires to use the Primary Key as
the "pointer" because we just can't take the risk of using even the
row identifier. I have to review the concept of partitions for the
data vs secondary indexes, but I'm pretty sure I'm correct.

The Secondary Index blocks are used to give us a pointer to a Top Index
Block used to sort the elements present in that Secondary Index Block.

The definition of the secondary index is found in the XML file defining
this table.

* Header (`SIDX`, Type: `char[4]`)

* Name (Type: `uint32_t`)

    This secondary index identifier. As we parse the XML data, we
    assign an identifier to each secondary index. This is that
    identifier referencing the second index in the Table Schema.

* Number of Rows (`uint64_t`)

    The total number of rows indexed by this secondary index.

* Top Secondary Index Block (`uint64_t`)

    The pointer to the top index block for this secondary index.

* First Secondary Index Block with Free Space (`uint64_t`)

    Pointer to the first B+tree index block with space available.

These fields are repeated for each secondary index.

Note: to support our new "indexes" table, we would instead need a
special case where the `"start_key"` is used so we can have a
_separate_ index for each `"start_key"`. My main problem at the
moment is that I have to count the number of rows every time I
want to know how many items are in a _list_ (an index). I do
that _dynamically_ so it takes _forever_. With partitioned
indexes, having the total on each separate partition is enough
for us to get a proper total number of rows. (i.e. for one
specific index, one partition would be on 3 computers [where the
replication factor is 3], any other partition for that index
would **never** share those 3 computers.) So if you have 9
computers you could get:

- computer 1, 2, 3 -- partition I
- computer 4, 5, 6 -- partition II
- computer 7, 8, 9 -- partition III

With a number of computers which is not a multiple of the replication
factor, you do not use all the available computers. For example, with
7 or 8 computers, our example above would only use 2 partitions.
Note that each index shuffles the computers so when we have 7 or 8
computers, computer 7 and 8 will still get used by some indexes
which will not use some of the other computers (maybe 1 and 2).

We want to at least have one partition and create others only if possible.


### Entry Index Block (`EIDX`)

An Entry Index records a longer key for the index so we have much less
false positive. If we still want to have indexes that are allocated
as an array, then we still need to limit the size of the data. That
being said, we should have enough to hit most of it if possible.
Especially for the primary index, we want to have the full key (which
is the Murmur hash). Otherwise we'd end up having to scan the data
one row at a time...

* Header (`EIDX`, Type: `char[4]`)

    The header indicates the type of index block. Note that this is very
    important in these blocks since you first navigate `TIDX` blocks
    until you reach an `EIDX` block and the algorithm changes at that
    point.

* Count (Type: `uint32_t`)

    The number of indexes defined here.

* Size (Type: `uint32_t`)

    The size of one index.

* Next (Type: `uint64_t`)

    A pointer to the next `EIDX`. This is used whenever we do a SELECT of
    may rows. We start from the first match and then go on to the last
    entry that still matches or the `LIMIT` is reached.

* Previous (Type: `uint64_t`)

    To easily maintain a `Next` field, we need a `Previous` field too.
    Also this is useful if we support the `DESC` feature in which case
    we first find the last match and then go backward to the first.
    Again, we either stop once we found the first match or when we
    reached `LIMIT` rows.

* Index

    This is the actual index which gives us a (more) complete key so we
    can be sure to read the correct data. Because of that, we may have an
    index of variable size. (TBD)

    The index includes the key and a pointer to the actual data.

### Data Block (`DATA`)

A data block is a block that includes the actual data of the row. If
the key in the index includes the entire key, we could avoid having
a copy here, however, when using secondary indexes, chances are we
need all the data here, which makes it easier.

The block includes the following fields:

* Header (`DATA`, Type: `char[4]`)

    The block of data starts with `DATA`.

* Size (Type: `uint32_t` or `uint16_t`)

    The Size of the next data buffer. Note that we have that size in the
    index, but it's really not that practical there. Also if we limit
    the size of a block to 64Kb then we could use a `uint16_t` and save
    much space. Also we could support a little bigger sizes if we consider
    that each block has to be aligned in some way. An 8 bytes alignment
    would give us 3 bits so 64Kb x 8 = 512Kb blocks would be supported.

* Data (Type: complex)

    A block of data is defined with a dynamic structure as defined in the
    SCHEMA.md file, see Blob.

    The first byte is used to define the status of this Data block:

    - Valid -- everything is fine, we can access this block as expected
    - Moved -- the user made an update and the new block was larger so
      it was saved somewhere else, the next 8 bytes represent the
      offset of the new location
    - Deleted -- the block was deleted (space is available for another
      block but really only if we properly removed the index too)

    If we want to be able to get rid of the `Moved` over time, we need
    to have a reference counter as well. So each index that points to
    this data can be updated whenever we pass through it and the counter
    decremented. Once the counter reaches 0, we can actually reclaim
    the space and mark the entry as Deleted and clear the bit in the
    AMAP as required.

    If the table is marked as "Secure" then the data of deleted rows
    get cleared immediately.

    Data with a blob larger than a Data Block requires special handling.
    We have multiple solutions at our disposal:

    - Save a minimum amount of data here (i.e. small fields and especially
      fields we may SELECT against) then save the rest at the end of a
      separate file and save the pointer to that data

    - Save the data in multiple block; this block would include a pointer
      to the first block where the block is saved and each such block can
      be linked to another block until the full size is saved. (See the
      Blob Block) -- to save space, we want to allow for pointers to
      go to the end of a previous blob. The code can easily use a modulo
      to know whether we're at the start or in the middle of a block.

    We want to have a special type that we use whenever a blob is not
    defined directly in the Data Block.


### Blob Block (`BLOB`)

Whenever a row is really large, instead of saving that data in a block
with other rows, we directly save the data in a Blob Block.

The block has still has a header:

* Block Type (`BLOB`, Type: `char[4]`)

    The type of block.

* Size (Type: `uint32_t`)

    The number of bytes saved in this block. It may be the entire block
    (outside of the header, of course).

* Next Block (Type: `uint64_t`)

    A pointer to the following block where the blob continues.

Note that we could also consider allocating _large blocks_. In other words,
allocate a 200Mb block if the blob is 200Mb and save it in one go. However,
we may instead want to consider a separate table for the data if such large
blocks are necessary. The main data table would then reference those
separate files. With files of 2Mb or more, frankly, it's not a bad thing
to use the file system instead of attempting to save those biggies directly
in our database files (the overhead is rather monstrous). Just use a name
such as blob1.bin, blob2.bin, etc.

Those would be saved immediately when we receive the data from the user.
That way we make ONE copy instead of many copies (to the Journal, to memory,
to the mmap() memory, to another file...) We need a parameter in our
configuration file defining the threshold used to know when such happens.
The blob files can be saved in a sub-folder so it does not bother us too
much in our main folder.

TBD: Should the threshold be equal to the space available in a Blob block?
In other words, remove the "Next Block" and any time a user attempts to
save a blob larger than a block minus the header we switch to a blob file
or at least an sstable like file (but a delete is expensive in such a file
and compacting a file with Mb files in it... Outch)

_Note: there is actually a problem in Cassandra where incoming data,
even when really large, is just kept in memory. On smaller memory models
that's a real big problem because as a result a lot of the other useful
data is going to be removed from memory. Our solution is much better
and yes, we'll need to stream the data from disk, but overall that's
still a lot faster than losing a ton of data. A computer with 1Tb or
similar amount of RAM would not suffer as much from such a large amount
of memory being used, but in most cases clusters of databases like
Cassandra use much smaller computers (i.e. between 16Gb and 64Gb).


### Free Block (`FREE`)

Whenever a block is not used at all for what it was allocated for, it
can be marked as Free. This means it can be reused as any kind of block
and thus appears in the AMAP.

If the table is assigned the "Secure" bit, such blocks get cleared.


### Schema Block (`SCHM`)

A schema block allows us to have the definition of the schema which is
the global settings of the table and the list of columns.

The block is timestamped because rows created at a given time will
follow the latest schema. This gives us an easy way to know which schema
a row used when it was saved. This allows us to incrementally fix the
rows to a newer schema.



### Top Compacting Data Block (`TIND`)

When we have more rows in our database than one `INDR` can handle, we need
yet another indirection to allow use to find the correct `INDR`. With
time we may need even more levels of indirection. A `TIND` has to have a
level so we know how many indirect pointers each entry represents.

Say one `INDR` supports 500 blocks. A `TIND` at level 0 supports 500 x 500
= 250,000 rows. A `TIND` at level 1 supports one extra level so 500 ^ 3 =
125,000,000 rows and so on.

* Block Type (`TIND`, Type: `char[4]`)

    The magic word.

* Block Level (`uint8_t`)

    The level of this `TIND`. If 0, then each pointer represents one `INDR`
    table. If 1, then we have two levels, one more `TIND` level and then
    the `INDR`. That means at level 1, we support the number of pointers
    in one `INDR` square. Etc.

* Pointers

    This is an array of pointers to `INDR` or `TINDR` tables.

    Because of the way the level works, when level is zero, the pointers
    point to an `INDR`. When the level is larger than zero, the pointers
    point to a `TIND`.

### Compacting Data Rows (`INDR`)

The Data Rows are likely going to be a mess over time. One way to fix that
problem is to compact them. This is done by rewriting the rows within each
data block to avoid any gaps between rows. Gaps happen because we delete
or at least overwrite a row entry. As a result the data includes gaps.

Pointers and compaction are not compatible. To fix this problem we need
to add one indirection. This is done by assigning each row an identifier.
That identifier is used everywhere we want to reference that one row.
Then we make use of another set of blocks to convert the row identifier
in a pointer to the data. That way the compaction process can very easily
happen.

This is why we offer a "no-compaction" feature for tables because that
way we can complete eliminate that extra indirection. However, when a row
gets updated and as a result moves in the database, the old data location
must include a pointer to the new location. Anything pointing to the old
location ends up having to handle one extra redirection.

The block is pretty simple:

* Block Type (`INDR`, Type: `char[4]`)

    The magic defining this block's content.

* Block Size (Needed?)

    The number of pointers.

* Pointers (Type: `uint64_t`)

    An array of pointers to the data.

When the first `INDR` table is full, we need to create a tree of `INDR` where
to top table points to additional `INDR`. So such an indirection may add
multiple levels (i.e. required that many more blocks to be loaded to be
able to access the data).

#### Cassandra Solution

In Cassandra, the sstables are the data tables which are separate from the
indexes. The sstables have a similar problem, but Cassandra saves additional
meta data for each row allowing for the ability of only needing to read the
sstable to extract all the data.

Whenever a row is "deleted" (keep in mind that a delete happens when the user
use INSERT or UPDATE or DELETE, because a new INSERT replaces the existing
row by writing a new copy at the end of the table) the first byte becomes 'D'.
This allows us to skip the row at once. It's called a tombstone. What happens
when compacting a table is that tombstones are ignored on read, but
overwritten on write. So the process goes something like this:

    WHILE !EOF
        pos_before_last_read = TELL
        row = READ
        IF row.deleted THEN
            IF write_pos == -1 THEN
                write_pos = pos_before_last_read
            END IF
        ELSE
            IF write_pos != -1 THEN
                SEEK write_pos
                WRITE row
                write_pos += row.size
            END IF
        END IF
    END WHILE

I'm not too sure how the handle the changes in the primary index, but I
suspect that it works as in my Other Solution below. In other words, any
secondary index includes the key of the row you found and then that key
is used to find the data. In other words, the only place where we have
a pointer to the data is the primary index and that's where a row that
gets moved in the sstable has its pointer updated.

#### Other Solution

Note that there is one other solution, but it is much slower on secondary
indexes.

The idea is to have a pointer in the primary index only. Everywhere else
you only include the key. This assumes that the key is a murmur key.

Whenever you find an entry in a pointer, you find the key, not a direct
pointer to the data.

Similarly, the data itself includes the key so we can also find the
primary index from there. This is useful when compacting since we are
not searching the data, instead we directly access the row.

So whenever a row changes and that change requires the row to be moved,
the only location where the pointer to the data has to be updated is the
primary index and we already have that in memory on a user UPDATE.

This solution is possibly much slower when you handle a secondary index
since we have to search the secondary index first, then the primary
index. With an `INDR` block, we instead of "one" small impact on each
access.


### Shrinking Database

In most cases we probably do not want to care too much about shrinking the
database files. However, that's something that happens a lot with Cassandra
(albeit rather slowly...)

The idea is pretty simple: whenever all the rows in a block were deleted,
we can just reuse that block as a brand new empty block. We can also take
the very last block in the file and copy it in that new available space.
The _only_ problem here is that we do not currently have pointers back to
our indexes (too expensive!) so it would be difficult to do that without
going through all the indexes (well, not just that, it would depend on the
type of block we are dealing with.)


# Accessing the Data

In order to access the data efficiently, we want to use the `mmap()`
instruction. Also, we may be using multiple threads so we need to use
locks to make sure that we do not walk on each others toes. However,
there is no need to lock between computers since our files are specific
to one computer (we assume people are not going to use NTFS!?)

## Mapping vs Blocks

Our implementation will have two separate sets of objects:

1. mappings -- which use `mmap()` to give us access to the files; it takes
   care of the `open()`, `msync()`, etc.

2. blocks -- which are used to access the data in each block; we can
   specialize each block to its type

_Note: the blocks could be managing their own `mmap()` calls. However,
doing so would mean many more calls to `mmap()` and certainly a lot more
work by the kernel. This is why we want the mapping separate and working
with many blocks in one go even if not the entire file. That being said
once we have an implementaton that works, we should test with various
sizes to see what works best on various computers._

The blocks make use of the mapping objects (through a file object) to find
the address to use to access the data. This is constantly done because the
mapping may change "at any time" (whenever the file grows or shrinks).

The mapping objects are managed by one file object.

The files within one table are managed by a table object.

The tables in a context are managed by a context object.

The contexts are managed by the cluster object. There is only one cluster.

## File Management

The table manages the files. It knows how the data is spread among files.
For example, it will know whether the primary and second indexes are to
be saved in separate files or are found clusted in the data file.

When we need to access some data, we want to make it as simple as possible
for the functions that need access.

A user request is usually just "GET" with a key. We want to make the key
to the available indexes. Once we find the index, we use that info to
get access to the file and start our search. Once we have an answer, we
either have a pointer directly to the data or have to use the INDR index.
Once we finally have the pointer to the data, we can read the corresponding
data and return it to the client.

Since `mmap()` is managed by the kernel, the data gets cached automatically
and we do not have to do anything more than that on that end. However, we
may need to reduce the number of mmap() currently in action to control
how much RAM gets used; i.e. if we want to use about 50% of the RAM so the
kernel still has plenty of buffer space, then we have to make sure that the
mapping does not go beyond that point.

The database object will be in charge of control the amount of cache
currently in use.

## Locking

We expect to be the only process directly accessing the database files.
(i.e. if you want to run multiple instances of the database process on
a single computer, then they each need to make use of different folders
with the database).

The locking can therefore make use of very simple mutexes to make sure
that threads do not attempt to access the same part of the files
simultaneously when a write is about to occur.

At a later date we can look in to making sure that the locks for writes
happen in the correct order and all, but at first we can simplify that
process quite a bit by just locking all the blocks we are about to modify,
do the write, then unlock.

So a lock can happen on a per block basis.

The write process should do a `trylock()`. If any one of those locks fail,
assume that we are likely to have a deadlock. Unlock everything and try
again until the whole lot works in one go. This may seems slow, but it
will let reads happen properly. Remember that at first the data is written
to a Journal so it won't matter too much if the write to the full database
does not happen immediately. We still have the data accessible as expected.




vim: ts=4 sw=4 et
