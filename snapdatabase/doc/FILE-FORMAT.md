
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

#### Indexing

* Indirect Index (`INDR`/`TIND`)

    To allow for changes in the location of the data, we use a two layered
    index mechanism. Each row is assigned an identifier (OID). That identifier
    is incremented on each INSERT and starts at 1. Knowing the number of
    pointers per page and the current number of levels (the `TIND` includes
    that level; the `INDR` level is always 0 so we don't include a field for
    that one), we can immediately calculate where the pointer is located and
    load the exact page at once.

    Note: we can't safely expose this identifier because (1) when a row gets
    deleted its OID gets reassigned to another row; (2) the OIDs are specific
    to the table instantiation on a certain machine, so the same table on
    several different machines may have  different OIDs for the same rows.

* Primary Index (`PIDX`/`TIDX`/`EIDX`)

    We primarily use B+tree indexes, however, the Primary Index first uses
    the `PIDX` (the highest level), which is an index using the last bits
    of our key to very quickly go to the next level (a `TIDX`).

    This index uses the lower bits because the top bits are used to
    determine the computers in the cluster receiving the data (i.e.
    partitioning). So on a server in such a cluster the top bits are
    not going to be as random as the bottom bits.

    Cassandra uses LSMtree instead. LSMtree has the advantage of offering
    a very fast write mechanism. We want to look into using a level 0
    LSMtree which spans between the _commitlog_ (incoming data saved in our
    temporary journal) and the commited data.

        +--------------------+
        |                    |  For the Primary Index, here we include the
        |    Entry Index     |  whole key (128 bits); for Secondary Indexes
        |     (EIDX)         |  we use n bytes as defined by the user
        +--------------------+
                 ^
                 |
        +--------+-----------+
        |                    |  Include n bytes of key in an array so we can
        |     Top Index      |  do a simple binary search
        |     (TIDX)         |
        +--------------------+
                 ^
                 |
        +--------+-----------+
        |                    |  Use lower n bits of key as an index
        |   Primary Index    |
        |     (PIDX)         |
        +--------------------+

* Branch Index (`PIDX`/`TIDX`/`EIDX`)

    Similar to the Primary Index, except that the major version number gets
    added at the end of the key:

        row_key + "::" + version.major

    This is used to save branch specific data in a row. Here the version is
    the version of the row, not the version of the database, schema, etc.
    (since each row also gets assigned a schema version, watchout!)

    See talk about the `_version` field in
    snapdatabase/snapdatabase/data/schema.cpp

    IMPORTANT NOTE: If we decide to use a secondary index instead of the
    Murmur3 value, then we would not use the `PIDX` block. For branches
    it may not be too important, but if we want to find the list of the
    last five versions in a branch, that not going to be possible with
    a Murmur3 key.

* Revision Index (`PIDX`/`TIDX`/`EIDX`)

    Similar to the Primary Index, except that this includes the major and
    the minor version numbers in the key and the language.

        row_key + "::" + version.major + version.minor + language

    Note: internally, we use 0xFF as the separator.

    This is used to save the revision specific data in a row. This is the
    data which changes whenever a person edits a page (the title, body,
    tags, date of the modification, etc.)

    See talk about the `_version` field in
    snapdatabase/snapdatabase/data/schema.cpp

    IMPORTANT NOTE: If we decide to use a secondary index instead of the
    Murmur3 value, then we would not use the `PIDX` block. However, I think
    that we are very likely to want to read the versions in order because
    that way we can let the user choose the one he wants to display now.

* Expiration Index (`TIDX`/`EIDX`)

    This index is used to track when a row is going to expire. The system
    has to automatically delete rows that expire. This index is used to
    know of all the rows with an `expiration_date` column and delete it
    as required.

    The index uses the `expiration_date` in incrementing order. The type of
    the column much represent a date. The precision is left to the database
    administrator.

* Tree Index (`TIDX`/`EIDX`)

    We also have a special case of a Direct Tree to support searches on a path
    organized in a _perfect tree_ (perfect as in any child always has a parent
    page). This will allow us to access the data by path and to assign any page
    to any point in a path (to make it easy to move the page to a new location
    and even duplicate the same page under multiple paths).

    The primary index makes use of a Murmur3 hash to transform the key to a
    value which is exactly 128 bits. This is very useful to better partition
    the data (grow horizontally). However, it's not as useful for secondary
    indexes that require a specific sort order.

    Note: Cassandra's first solution was to use column names. These were sorted
    (in binary by default). However, that meant it wasn't distributed. Later
    Cassandra added Secondary Indexes which they implemented so they would be
    distributed.


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
    
    When we do not have secondary indexes, we can easily compact tables.
    
    If we have secondary indexes, pointing to the Primary Index is best.

    TODO: make sure to include a counter (number of rows) in each one of
    our indexes. The Primary Index does not require it since all the
    rows are always present in that index. Secondary indexes, however,
    may have less.

* Solution 2. Use `INDR` to for Compaction Feature

    (Note: my first implementation will forcibly include an `INDR`
    and `TIND` once the first `INDR` is full; but later versions should
    support not having the indirect indexes.)

    This solution makes it easier to allow for the compaction feature.
    All the tables, except the `INDR`, make use of a row number (OID) and
    the `INDR` is used to convert that number in a pointer in the data table.

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

### Block Header (all blocks)

Absolutely all blocks, including _file blocks_ (i.e. block with offset 0),
start with the exact same header defined here. The most important part for
us is to be capable of upgrading files whenever we update our table
structures. For that to happen easily, we use a version on a per block
basis. This helps in various ways, especially, it helps with the fact
that we do not upgrade an entire file if only a few block types were updated.
Also, the way we implemented it, we do not even have to do the update up
until the time you actually read that block for the first time since the
upgrade. This may have a small speed impact but in comparison to having
to upgrade humongous files all at once, it will be mostly transparent.

The _Block Header_ gets referenced as the very first field of all the other
blocks. In our structures, it is called `header`.

The header has the following fields at the moment:

* Magic (Type: `dbtype_t`)

    The _Magic_ field is a field of 4 bytes that represent the type of
    the block using letters (a la IFF, PNG, and some other such formats).
    This means it easy to look at a file with a binary editor and very
    easily see what the block is about

    All _Magic_ types use 4 bytes, although the last few could be set to
    '\0' it is rare if ever used that way.

* Structure Version (Type: `version_t`)

    The version of this block's structure on disk. This allows us to parse
    the block data using the correct structure.

    **WARNING:** This is not the data in the rows. The structures are the
    meta data we use to know what is in the various blocks. These structures
    will probably rarely change, but when they do they are very likely going
    to be incompatible with older versions. To ease upgrades (not downgrades)
    we want to have a version in each block allow us a very precise way to
    move the data forward to our newest version. Note that contrary to the
    number of schema (see `SCHM` and `SCHL`) which can grow and shrink over
    time, all the old structures will be kept so any older version can be
    upgraded (okay, after some time, we may force you to upgrade to an
    intermediate version or dump your data and re-upload it to a newer version
    of the database; after a long time, we would otherwise have too many
    structure descriptions in our files...)


### Header (`SDBT`)

The header defines where and how to find the other blocks and files.

* Block Header (Type: _complex_)

    The block header.

* Version (Type: `version_t`)

    The version of this table.

* Block Size (Type: `uint32_t`)

    The size of one block in bytes. By default we use 4Kb, but some tables
    may defined a larger size such as 64Kb. It will depend on the size of
    data expected in the table. Having a size larger than any blob that is
    going to be inserted is a good idea, but there are trade offs too (i.e.
    we do not want 1Mb blocks for indexes, for example).

* Table Definition (Type: `reference_t`)

    A pointer to the full schema definition which appears in another
    block. It is the parsed data from the XML file defining a table
    in binary form that we can load and use immediately.

    The reference may point to a `SCHM` (Schema) or a `SCHL` (Schema List)
    block. The `SCHL` is used when more than one schema is used in this
    file. This is important when you upgrade your table schema. It will
    upgrade all the rows in the background with a low priority so as to
    avoid preventing rows from being insert, selected, updated, deleted
    by clients. See the `SCHL` for additional details about this.

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
    the old schema (see `SCHL`) and then _know_ whether a row is schema
    version 1, version 2, etc.

    > _Note: to avoid having to add anything more to every single one of our rows
    > we actually timestamp the schema versions. Since we always have a date in
    > each one of our rows, we know which one to keep and also that date can be
    > used to find which schema was used to save that row._

    WRONG: We can't use a date. That's not enough to allow for a full upgrade
    on the spot which we probably want to do because otherwise we could end
    up with a ton of schemata and that's definitely not something we want in
    any table (i.e. with just a date, we have to hope that each row gets
    update _at some point_ to fix its schema).

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

* First Free Block (Type: `reference_t`)

    This field holds a reference to the first `FREE` block. This block
    is available to be used as any block. If all free blocks were used
    then this parameter is 0. For more details see the Free Block
    definition.

* Table Expiration Date (Type: `time_t`)

    **TBD:** do we really want to support temporary tables? It's often used
    in SQL, but here, maybe we could instead look into having a feature
    to create memory tables. That way it can be made non-block
    based. i.e. the whole set of the data can be kept in maps (using a
    boost multi-index map). Especially, **we do not have a way to create
    a table dynamically**. Only tables which have an XML definition in the
    context repository are created. So I'm not totally sure we want to
    support such here. At the same time, having a `memory_cache` table
    could be really cool too (possibly just on the front ends).

    A date when the table is to be dropped.

    This is generally used by temporary tables. Say you need a table for
    a few hours, you could set the expiration date to `now + 1 day`.
    If somehow your code doesn't come around dropping the table properly
    (i.e. it crashes along the way, your code is a tad bit flaky, etc.)
    then even though the table is marked as being temporary, it would
    stick around. This is because the cluster survives shutdowns (at
    least it is expected to survive and run forever--no down time ever).

    Just like the Expiration Date of a row or a cell, the table Expiration
    Date is recorded and an Event Dispatcher Timer object will wake us up
    when the table times out and gets deleted. Note that once a table
    has expired, any access to that table fail with an error.

* Indirect Index (Type: `reference_t`)

    A pointer to an `INDR` block or, once the first `INDR` is full, to
    a `TIND` block.

    The indirect index allows for many deletions to happen and still allow
    for compaction of blocks. This is particularly useful when the data is
    not clustered and the table uses many writes and deletions, such as a
    session table.

    These blocks add one indirection, but with about 500 pointers
    per block (with a 4Kb block) you get a 250,000 rows with just 2 levels
    and 125,000,000 with 3 levels. With much larger blocks (say 64Kb) we
    would get some 8,180 with just the first level and 66,912,400 with
    just two levels.

    Note: this imposes an OID on each row. This means we need a lock of
    the file first block and incrementing the OID each time a new row
    gets added to the database. If we also implement the free list, we
    get a similar lock to manage that list. Keep in mind also that the
    OID is specific to one database file. On a replicat, the same row
    is very likely to have a different OID.

    The row itself does not need to save the OID although it is useful for
    the compaction process. If we accept a _slow_ compaction process, we
    can search for the OID using the row key and searching the primary index.

    Whenever a row is deleted, we want to add its OID in a free list. This
    is done by saving that OID in the first block and saving the OID in the
    first block in place of the pointer at that OID position. Thus creating
    a linked list. When allocating a new OID we first check whether the
    free list is empty or not. If not, use that OID, if it is empty allocate
    a new OID. This means the row OIDs get reused.

* Last OID (Type: `oid_t`)

    The last OID that was used. If 0, then no rows were allocated yet. This
    value is used to allow for the `INDR` blocks.

    This feature doesn't get used if the file does not require an indirect
    index.

    WARNING: This OID is specific to this very table. On another computer
    with exactly the same data, the OID is likely going to be different
    because the data would have been inserted in a different order.

* First Free OID (Type: `oid_t`)

    Whenever we add a new row we assign an identifier to that row. That
    identifier is used everywhere that row gets referenced. The Indirect Index
    can then be used to find the data in the data file.

    The Free OID is a list. At the location in the `INDR` this Free OID points
    to is another OID (the previous one that was released). Up to the NULL
    which is represented by 0 (note that position 0 is valid and is used
    by OID 1; when we search for the row pointer, we use `OID - 1`).

    This feature doesn't get used if the file does not require an indirect
    index.

* Update Last OID (Type: `oid_t`)

    The OID of the last row the background process has to read to have
    the entire database updated. The current OID to update is defined
    in the "Update OID" field. Once that field reaches (is equal to)
    the "Update Last OID" value, then the entire database was updated
    to the latest version of the schema.

* Update OID (Type: `oid_t`)

    The OID of the last row the background process used to update a table.

    Whenever you make changes to your table, it is required to go through
    the entire set of rows in that table to update them to the latest schema.
    The process runs in the background allowing for what looks like an instant
    change to the schema (i.e. the database runs as if nothing had changed).

    This OID is kept in the database so it can be interrupted and restarted
    any number of times.

* First Compactable Block (Type: `reference_t`)

    Whenever a DELETE happens in a block, we are likely to have the ability
    to _compact_ that block to save space. That block is therefore added here.
    That way we know we can process this block at some point and we avoid
    wasting time attempting to update any one block (i.e. in most cases, on
    a very large table, it is very unlikely that so many blocks would require
    compaction).

    Data blocks allocate data with a minimum of 8 bytes. This allows us to
    create a list of Compactable Block, so what we do is read the First
    Compactable Block. Load that block in memory. Read the first 8 bytes
    which represent the pointer to another block which can be compacted.
    Save these 8 bytes in the First Compactable Block.

    To have a double security about this, we save the First Compactable Block
    in the Process Compactable Block field and copy the next block in the
    First Compactable Block field, then work on the block. Once the
    work is done, set Process Compactable Block to 0. When we start the
    process over, we first look at the Process Compactable Block field. If
    not 0, then finish work on that block first. This is much more likely
    to work properly in all cases even if we are killed before we are done.

    We still can have an audit process which verifies that a block is as
    expected and that could be one of the elements to verify.

* Process Compactable Block (Type: `reference_t`)

    The block being processed by the compaction service. As mentioned in
    the First Compactable Block, it may take time for the process
    compacting a block to finish up and we do not want to lose that
    information if we quit (or worse, crash) which the compaction is
    going on.

* Top Primary Index Block (Type: `reference_t`)

    Pointer to the first B+tree Primary Index block (well, except that the
    `PIDX` is not a B+tree index, but well).

* Top Primary Index Reference Zero (Type: `oid_t`)

    The `PIDX` block uses all the space available and when the key ends with
    all zeroes, we end up with an invalid offset (because the block includes
    a header at that position) so instead we save that reference at here.

    This is a very special case indeed! However, it's much more effective
    since we can use one extra bit of data from the key in the `PIDX` and
    that means we save 50% of that space and we still have plenty of room
    in the header.

* Top Branch Index Block (Type: `reference_t`)

    Pointer to the first B+tree Branch Index Block.

    The branch index block is used to track branches. This may be a Murmur3
    or we may want to change that to a secondary type of index. The key is
    the same as the Top Key Index Block plus the major version number.

* Top Revision Index Block (Type: `reference_t`)

    Pointer to the first B+tree Revision Index Block.

    The revision index block is used to track revisions. This may be a Murmur3
    or we may want to change that to a secondary type of index. The key is
    the same as the Top Key Index Block plus the major & minor version numbers
    and the language.

* Blobs with Free Space (Type: `reference_t`)

    This reference points to a block of references used to define blobs
    with space still available where we can save rows (`FSPC`).

* Expiration Index Block (Type: `reference_t`)

    Pointer to the first B+tree index (TIDX/EIDX) block with rows having
    an `expiration_date` column. The index sorts the rows by that date.
    We do not support an expiration date on a per cell basis.

    The `expiration_date` column must be of a type representing time.
    Any precision of the time type is supported, whatever their precision.

* Secondary Index Blocks (Type: `reference_t`)

    A pointer to a block representing a list of secondary indexes. These
    include the name of the indexes and a pointer to the actual index
    and the number of rows in that index (it is here because the `TIND`
    is also used by the `PRIMARY KEY` which does not require that number
    and also when a new level is created the `TIND` can become a child
    of another `TIND`).

    Add support for a `reverse()` function so we can use the reversed
    of a value as the key to search by. This allows for '%blah' searches
    using 'halb%' instead. Yes, we could add a column with the reversed
    data. But having a reverse function is going to be much better
    than duplicating a column.

    Like with VIEWs, though, we could have calculated columns (these are
    often called virtual columns). So query the `eman` column could run a
    function which returns `reverse(name)`. We already make use of such
    a feature for indexes to inverse `time_us_t` fields and sort these
    columns in DESC order (i.e. `9223372036854775807 - created_on`).
    Actually, we have some "complex" computations that we repeat in many
    indexes when it could be one single virtual column in the Content
    table. This is exciting.

* Tree Index Block (Type: `reference_t`)

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

* Deleted Rows (Type: `uint64_t`)

    Note: by default we use a Counting Bloom Filter which means that
    we can delete rows. However, if a counter reaches the maximum (which
    at this point we plan to be 255) then we will need to regenerate the
    bloom filter after a while because proper deletion of any row in link
    with that very entry will _fail_.

    The number of rows that have been deleted in this file so far.
    This is useful to know whether the Bloom Filter should be
    regenerated and since it's a very expensive process, we do not
    want to do it too often.

    Note only that, this process needs to be incremental because really
    large tables are not likely to be done quickly enough for the new
    filter to be done in one go. In that case we allocate another block
    to save the new filter and once done copy that data here.

    Note that when all the rows get deleted (equivalent to a TRUNCATE),
    we clear the bloom filter and copy this counter to the the Last 
    Bloom Filter Regeneration Counter.

* Bloom Filter Flags (Type: `uint32_t`)

    This bloom filter flags define whether and how the bloom filter is
    to be used. The flags are:

    - algorithm: 4 bits defining the algorithm to use to generate the
      bloom filter.

    - renewing: if set, the bloom filter is being regenerated; this means
      writes will send the data to two bloom filters and reads can use the
      old table, albeit may get more false positive than you'd really like
      to get (but the new table returns false negatives until complete)

    The currently defined algorithms are:

    - none: the bloom filter is not used

    - bits: just use bits; it makes the filter smaller; useful for
      tables which do not delete very many rows over time; note that
      for the primary index replacing and updating rows does not mean
      deleting because the key remains the same; however, for secondary
      indexes using other columns than the primary key columns, those
      will possibly have problems with updates

    - counters: use one byte per entry which gets increased when a match
      is found and decreased when we remove a row; useful for tables
      where many rows get deleted all the time (the decrement is not
      performed if the counter is at 255)


### Free Block (`FREE`)

Whenever a block is added to the file, we mark it as a `FREE` block and
add it to the linked list of `FREE` blocks.

* Magic (`FREE`, Type: `dbtype_t`)

    The magic word.

* Next Free Block (`reference_t`)

    A reference to the next free block or 0.

Whenever a block becomes available again we clear it, mark it as a `FREE`
block and then add it to our linked list of `FREE` blocks.

_Note: the "clear the block" is not necessary unless the block has its
"Secure" bit set. However, it should not make much of a difference if
our blocks are all written at once by the OS anyway._

When adding new `FREE` blocks, we can allocate N at once. For example,
if N=16 and our blocks are 4Kb, we would add 64Kb at once to our file
(16 x 4Kb = 64Kb).


#### Sparse Files

Having a few characters at the start of the block will prevent the OS from
creating a sparse file. Also, the only way to really create a sparse file
is by using the `lseek()` function beyond the end of the file and writin
eons later or using `truncate()` to enlarge the file (with `truncate()`
you do not even need the write). Writing zeroes will also create the
block because the OS doesn't check what you write, it only create sparse
files if you seek (at least so far).

So if we have the size of a block in the file, say 4Kb, and we are using
larger blocks, we could end up with a sparse file. To avoid that, we have
to write the block magic letters at the beginning of the block and then
at least 1 zero on each of the following pages (i.e. if our blocks are
64Kb, then we'd have to write at least an additional 15 zeroes to the file.


### Schema Block (`SCHM`)

A schema block allows us to have the definition of the schema which is
the global settings of the table and the list of columns.

The block is timestamped because rows created at a given time will
follow the latest schema. This gives us an easy way to know which schema
a row used when it was saved. This allows us to incrementally fix the
rows to a newer schema.

* Magic (`SCHM`, Type: `dbtype_t`)

    The magic word.

* Version (Type: `version_t`)

    The version of this structure.

* Size (Type: `uint32_t`)

    The size of this schema blob.

* Next Block (`reference_t`)

    If the schema is larger than one block, this is the next part. Before
    we transform the schema to C++ objects, we read all the parts.

* Schema (Type: _complex_--a.k.a. blob)

    This block is a structure describing the table and the columns
    as per the tables.xsd information. Only this is a binary version
    so we do not have to parse the XML each time.

    Parameters:

    - `version` (`uint16_t:uint16_t`)
    - `table_name` (`p8-string`)
    - `flags` (`uint64_t`)
        . secure (bit 0)
        . sparse (bit 1)
        . track create (bit 2)
        . track update (bit 3)
        . track delete (bit 4)
    - `block_size` (`uint32_t`)
    - `model` (`uint8_t`)
        . content (this is the default is not defined)
        . data
        . log
        . queue (a.k.a. journal)
        . session
        . sequential
        . tree (i.e. the parent/children tree as an index)
    - `row_key`
        . number of columns in row-key (`uint16_t`)
        . `column_id` -- vector of references to columns (`uint16_t`)
    - `secondary_indexes`
        . number of secondary indexes (`uint16_t`)
        . secondary index
            + secondary index `name` (`p8-string`)
            + secondary index `columns`
                > number of columns used in secondary index (`uint16_t`)
                > vector of references to columns (`uint16_t`)
    - columns
        . number of columns (`uint16_t`)
        . column
            + column name (`p8-string`)
            + column type (`uint16_t`)
            + column flags (`uint32_t`)
                > limited (0x0001)
                > required (0x0002)
                > blob (0x0004)
                > system (0x0008)
                > revision_type (0x0030)    0- global, 1- branch, 2- revision, 3- TBD
            + encrypt key name (`p16-string`)
            + default value (see column type)
            + minimum value (see column type)
            + maximum value (see column type)
            + minimum length (`uint32_t`)
            + maximum length (`uint32_t`)
            + validation (`"compiled script"`)

All strings are P-Strings with a 16 bit size. The following data is 8 bytes
aligned so we can access 64 bit numbers as is.

The column identifier is the column index in the vector of columns. When a
column gets removed, it stays in the vector and it gets marked as deleted.
When a new column gets added later, it can reuse that slot. However, we first
need to know whether that column was removed from all rows before we can
reuse it, One way would be to have counters, but that starts to be really
heavy (i.e. count all the columns added and removed over time so once the
old column's counter goes to 0 we can reuse that slot.

When the secure flag is set, the table is considered to include many
security related data which need to be erased on a delete. The idea is
to clear the data once not necessary anymore to reduce the potential for
data leakage as much as possible.

When the sparse flag is set, we allow sparse files. Instead of writing
full blocks when allocating the file we use lseek() which may end up
generating a sparse file (if your blocks are larger than the size of
one system block). This option is not recommended since it may generate
problems at the time a `FREE` block is required and it was not yet
allocated. (That being said, if the list of free blocks is empty, we
have a similar situation even in non-sparse files...)

When the track flags are set, the corresponding columns are managed by the
system. The global `_created_on` column, like the `_oid` column, is always
set. By not tracking all the events, you often avoid having to reallocate
a new location for the row as it is not unlikely to grow (i.e. adding new
columns means that the row becomes wider). We could allocate all of those
columns on creation, but in most tables the tracking information is never
used so we avoid having it and save a lot of space.

Note that the `_created_by`, `_last_updated_by`, and `_deleted_by` are
created only if you set those values. The low level database system has
no user concept.

The `revision_type` mini-field defines what part of the version affects the
field. In our content table, part of the data is versioned meaning that older
versions remain accessible as newer versions get added. The version is defined
as two numbers: `major` and `minor`. The `major` number is set to `0` when
default data is loaded in a column (i.e. it represents "system data"). The
minor revision is incremented each time new data gets added to a column
supporting the minor version. Here are the existing three modes:

 * `global` -- such fields ignore the version information; it gets defined
   once and overwritten on a write;
 * `branch` -- fields set as `branch` have distinct values for each `major`
   revision; so we get one value for revision `0`, one value for revision
   `1`, etc.; this is primarily used for links between pages;
 * `revision` -- fields set as `revision` have distinct values for each and
   every version, whether the `major` or the `minor` version changes; this
   is used for the actual user content such as the title, body, author, etc.

In terms of implementation, we may use the index (see Indexing) to implement
it. This is still TBD at this point. But for sure that way we can get all
the data in one place and as far as the client is concerned, it's reading
and writing data to one table, you are just required to include the version
information in your commands so it works with such a table.

See also libsnapwebsites/src/snapwebsites/tables.xsd


### Schema List Block (`SCHL`)

When updating the schema, the system does not immediately go through the
entire file to update it. Instead, it saves the different versions of the
schema and let a background process work on the schema update. This means
reading a row may require access to an older version of the schema. To
allow for such, the database uses one block listing the `SCHM` blocks with
all the old versions.

Once the background process updating the table went through the entire
database, the old schema gets removed since it would not be necessay any
more.

**IMPORTANT NOTE:** The process has to include all the incoming rows and
we now that we are 100% done only after all the computers in the cluster
all agree on the latest schema. Until then, we may still receive data from
other computers with the _wrong_ format (old schema).

**Note 2:** New rows always make use of the newest version of the schema even
if they were sent using an older version (i.e. a front end was not yet fully
updated and had to send us a few rows of data...)

**Note 3:** If this table gets full and the user requests for yet another
new schema, the request _fails_. More specifically, the front end of the
process will be blocked until the backend is done with with older schemata.
At this point, we do not foresee this to ever happen since the minimum size
of a page is 4Kb and that means some 2,000 changes in a row to reach the
limit.

**Note 4:** When you first create a table, no `SCHL` block is used. We
directly use an `SCHM` block. The `SCHL` gets used one an old schema needs
to be referenced. Once all old schemata were worked on and the list is just
and only the current schema, again the `SCHL` gets removed and the file
header points directly to the `SCHM`.


* Block Header (`SCHL`, Type: `dbtype_t`)

    The magic and version of this array.

* Schema Array (Type: `array16`)

    This field is repeated for each schema changes that happened so far
    and are still not fully updated by our background process. The array
    is used to very quickly find the schema by version (i.e. it is an
    index of the existing schemata sorted by version).

    * Schema Version (Type: `version_t`)

        The version of this schema reference.

        The entries are sorted by version in descending order so that way
        the very first entry is the newest schema.

    * Schema (Type: `reference_t`)

        A reference to the `SCHM` block with the actual schema.


### Free Space Block (`FSPC`)

Whenever a block of data gets used we write some data to it and the rest of
the space is "free space". That "free space" is added to a Free Space
Block so we can very quickly find the block with enough space to save the
next incoming blob of data.

* Magic (`FSPC`, Type: `dbtype_t`)

    The magic word.

* Free Space (Type: `reference_t`, Alignment: `sizeof(reference_t) * 2`)

    This block is an array of references to `DATA` blocks with free space.
    The index of the array represents a size available in that `DATA`
    block.

    The array has a single reference for a given size. That reference
    points to a `DATA` block where the first 8 bytes represent the next
    free space with that size (i.e. a linked list).

    Why does it work?
    
    (1) The size of a block, `DATA` or `FPSC`, is the same. The minimum
    size of a Blob of Data in the `DATA` block is at least going to be:

        version_t       (4 bytes)       _a.k.a. schema version_
        column_id_t     (2 bytes)
        int64_t         (8 bytes)       "_oid"
        column_id_t     (2 bytes)
        int64_t         (8 bytes)       "_created_on"
        column_id_t     (2 bytes)
        int8_t          (1+ bytes)      _user defined column_

                Total: 27+ bytes minimum

    in other words, we can very easily have a `reference_t` to another
    blob of free space.

    (2) The minimum size of a block also means that index 0 and 1 are not
    required in our array. This leaves space for our header (Magic field).


    We always allocate a bare minimum of 16 bytes which allows us to have
    an 8 bytes reference. So this array is one pointer per possible size
    which is a multiple of the minimum size we'd allocate for a row.

    The reference for case 0 an 1 are not defined in the Free Space array.
    Each reference represent a space with a multiple of 8 bytes. In other
    words, we divide the size available by 8 and then save it here. Since
    all blocks have a similar size, a reference to free space is going to
    be a multiple of 8 bytes, we get
    
        Block Size / 8 * 4

    as the number of entries in the Free Space block. So we use only 50%
    of this block.


### Top Direct Data Block (`TDIR`, for sequential data)

This is probably very rare, but for the few tables we have where the data
is sequential (all rows have exactly the same size) we can use the top
direct data block.

This type of block does not need to pinpoint each row one at a time.
Instead, it just has to pinpoint blocks. The number of rows in one block
is the size of a block minus its header divided by the size of one row.

For example:

    (4Kb blocks - 64 bytes of headers) / 32 bytes rows = 126 rows/row

That means one pointer to a `DATA` block found in a `TDIR` entry represents
126 rows. If one `TDIR` supports 500 pointers, it indexes a total of

    500 x 126 = 63,000 rows

This is very similar to managing an array only each 126 rows can appear
anywhere in the file. The `FREE` is handled _differently_ since only one
size is going to be used we could instead directly point to a free block
in the `SDBT` block and not have any `FREE` block.

* Magic (`TDIR`, Type: `dbtype_t`)

    The magic word.

* Block Level (`uint8_t`)

    The level of this `TDIR`. If 0, then each pointer represents one block
    of data. If 1, then we have two levels, one more `TDIR` level and then
    the `DATA`. That means at level 1, we support the number of pointers
    in one `TDIR` square. Etc.

* Pointers

    This is an array of pointers to `TDIR` or `DATA` tables.

    Because of the way the level works, when level is zero, the pointers
    point to a `DATA` block. When the level is larger than zero, the
    pointers point to a `TDIR`.


### Top Indirect Data Block (`TIND`, for easy compaction)

When we have more rows in our database than one `INDR` can handle, we need
yet another indirection to allow us to find the correct `INDR`. With
time we may need even more levels of indirection. A `TIND` has to have a
level so we know how many indirect pointers each entry represents.

Say one `INDR` supports 500 blocks. A `TIND` at level 0 supports 500 x 500
= 250,000 rows. A `TIND` at level 1 supports one extra level so 500 ^ 3 =
125,000,000 rows and so on.

* Magic (`TIND`, Type: `dbtype_t`)

    The magic word.

* Block Level (`uint8_t`)

    The level of this `TIND`. If 0, then each pointer represents one `INDR`
    table. If 1, then we have two levels, one more `TIND` level and then
    the `INDR`. That means at level 1, we support the number of pointers
    in one `INDR` square. Etc.

* Pointers

    This is an array of pointers to `TIND` or `INDR` tables.

    Because of the way the level works, when Block Level is zero, the
    pointers point to an `INDR` block. When the level is larger than zero,
    the pointers point to a `TIND`.


### Indirect Data Rows (`INDR`, for easy compaction)

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

* Magic (`INDR`, Type: `dbtype_t`)

    The magic word.

* Size (Needed?, Type: `uint32_t`)

    The number of pointers.

* Pointers (Type: `reference_t`)

    An array of pointers to the data.

When the first `INDR` table is full, we need to create a tree of `INDR` where
to top table points to additional `INDR`. So such an indirection may add
multiple levels (i.e. required that many more blocks to be loaded to be
able to access the data).

#### Cassandra's Solution

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

I'm not too sure how they handle the changes in the primary index, but I
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


### Primary Index Block (`PIDX`)

The very top Primary Index Block uses the last bits of the Murmur3 hash to
immediately jump to a `TIDX` or `EIDX` block.

The number of bits used from the Murmur3 hash depends on the size of your
blocks (page size). With a 4Kb block, we use 9 bits. The math goes something
like this:

    bits = log(page_size / sizeof(reference_t)) / log(2)

It is expected that the size of the `PIDX` block header will remain under or
equal to `sizeof(reference_t)` which means that the reference for offset 0
can't be saved here. Instead it gets saved in the header (see field
"Top Primary Index Reference Zero").

* Header (`PIDX`, Type: `dbtype_t`)

    The header indicating a Primary Index.

* Index (Type: `reference_t[bits]`)

    The index is `bits` references to `TIDX` or `EIDX` blocks. When we
    first add a new row, we use an `EIDX` block. Once that block is full,
    we add a `TIDX` and distribute the entries between the various blocks.

    Note: this is not an array because the number of indexes is not saved
    here. We save it in the header along the first `reference_t` field.


### Top Index Block (`TIDX`)

A Top index block represents a B+tree index.

_Note: for the Key Index, the first B+tree level is actually a direct
map using the first N bits of the Murmur code. So that way we can
very quickly go to the second level (avoid one binary search).
This is likely to be parsed and limited since it's already cut down
by partitioning those keys over the cluster._

The fields of the Top Index Block are as follow:

* Header (`TIDX`, Type: `dbtype_t`)

    The header indicated a Top Index.

* Count (Type: `uint32_t`)

    The number of Indexes currently defined in this block.

* Size (Type: `uint32_t`)

    The size of one index entry
    
    Having a size allows us to support keys of variable sizes and therefore
    we can reuse the same type of blocks for all our indexes that make use
    of a B+tree type of index.

* Index (Type: `char[8]` [key], `uint64_t` [pointer])

    Each entry in the Top Index are a small part of the key (up to 8 bytes?)
    and then a pointer to the next level which is either another Top Index
    Block or an Entry Index Block.

    These 16 bytes are repeated up to the end of the block.

    _Note: the size of the key is not much of a problem for a Murmur hash,
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
this table. That definition stays in the schema (`SCHM`).

* Header (`SIDX`, Type: `dbtype_t`)

    The block magic.

* Next Secondary Index (Type: `reference_t`)

    One 4Kb block supports about 170 secondary indexes so it is very unlikely
    that we would ever have to use this field. However, we do not want to
    limit the number of secondary indexes so we allow for a linked list.
    The indexes remain sorted so we can quickly know whether it is present
    in a page or not.

Each Secondary Index is defined by the following set of fields defined in an
array (16 bit size is enough):

* Secondary Index Identifier (Type: `uint16_t`)

    This secondary index identifier. As we parse the XML data, we
    assign an identifier to each secondary index. This is that
    identifier referencing the second index definition found in the
    Table Schema.

* Bloom Filter Flags (`uint32_t`)

    If set to 0, it is assumed that the primary bloom filter can be used
    (i.e. all or nearly all the rows will be found in this secondary index).

    When set to another value, this means this secondary index uses that
    other bloom filter because it is very likely to ignore many rows so we
    should have many less bits set for this index than the primary index.

* Number of Rows (`uint64_t`)

    The total number of rows indexed by this secondary index.

* Top Secondary Index Block (`reference_t`)

    The pointer to the top index block for this secondary index.

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
would **never** share data found on those 3 computers.) So if you
have 9 computers you could get:

- computer 1, 2, 3 -- partition I
- computer 4, 5, 6 -- partition II
- computer 7, 8, 9 -- partition III

With a number of computers which is not a multiple of the replication
factor, you do not use all the available computers. For example, with
7 or 8 computers, our example above would only use 2 partitions.
Note that each index shuffles the computers so when we have 7 or 8
computers, computer 7 and 8 will still get used by some indexes
which will not use some of the other computers (maybe 1 and 2).

We need to have at least have one partition and create others only if
possible.


### Entry Index Block (`EIDX`)

An Entry Index records a longer key for the index so we have many less
false positives. If we still want to have indexes that are allocated
as an array, then we still need to limit the size of the data. That
being said, we should have enough to hit most of it if possible.
Especially for the primary index, we want to have the full key (which
is the Murmur3 hash). Otherwise we'd end up having to scan the data
one row at a time...

* Header (`EIDX`, Type: `dbtype_t`)

    The block magic.

    The header indicates the type of index block. Note that this is very
    important in these blocks since you first navigate `TIDX` blocks
    until you reach an `EIDX` block and the algorithm changes at that
    point.

* Count (Type: `uint32_t`)

    The number of indexes defined here.

* Size (Type: `uint32_t`)

    The size of the key in one index.

* Next (Type: `reference_t`)

    A pointer to the next `EIDX`. This is used whenever we do a SELECT of
    may rows. We start from the first match and then go on to the last
    entry that still matches or the `LIMIT` is reached.

* Previous (Type: `reference_t`)

    To easily maintain a `Next` field, we need a `Previous` field too.
    Also this is useful if we support the `DESC` feature in which case
    we first find the last match and then go backward to the first.
    Again, we either stop once we found the first match or when we
    reached `LIMIT` rows.

* Index (Type: _complex_, see below)

    This is the actual index which gives us a complete key (compared to a
    `TIDX` which may shorten the key) so we can be sure to read the correct
    data.

    The index is an array of equally sized entries when dealing with the
    primary key since keys are murmur3 codes which are always exactly 128
    bits (16 bytes).

    However, keys of secondary indexes may vary in size. TBD:

    Solution 1. we have a sequential array we can search using a binary search
    (fastest search) and the Key Data is an offset to the actual data.

    Solution 2. we have a non-sequential array (varying sizes) and we have
    to search linearly; if the number of entries is relatively small, it is
    likely going to be fast anyway but still much slower when hitting the
    latest keys of a page.

    To the minimum, one index entry includes flags, the key and a pointer
    to the actual data.

    - Flags (Type: `uint8_t`)
      - Flag: Key is Complete (0x01); which means we do not have to compare
        again once we access the row data
    - Row ID (`INDR`, Type: `oid_t`) or Index Pointer (`IDXP`, Type:
      `reference_t`)
    - Key Data (Actual data or an offset to the data because it varies in
      size; the size would appear in the destination as in an ARRAY16)

    The Index Identifier (`IDXP`) is used when one key matches multiple
    rows. This happens when dealing with secondary indexes where the
    index is not required to be unique. We use a flag to know whether the
    offset is to an `INDR` or an `IDXP` although just reading the block
    is enough to determine that, obviously.

    **NOTE:** There is a _right balance_ between functionality and efficiency.
    In most cases, I prefer functionality. However, having really long
    indexes is likely to very much impair your index. We want to support
    a hard coded limited in length at which point the key itself gets moved
    to a BLOB. At that point, the index is likely going to be _rather slow_.
    MySQL limits indexes to 1,000 bytes.

    **WARNING:** A Secondary Index may need support for multiple pointers.
    That is, one Index Key, to multiple Data Blocks. For example,
    in the Journal table, if we were to only index by Priority, then we
    would get a large number of Data Block that match the exact same
    priority. Without any other data than the key to sort a list of items,
    we do not have a choice. Although we could make the index fail in case
    there are too many entries under one key, we probably want to just use
    a block of pointers to extend this index when it goes over a certain number.


### Index Pointers Block (`IDXP`)

The Index Pointer Block is an extension for secondary indexes that match
many entries. One such block is used as an offload of the Index field of
a Secondary Index (see `EIDX`).

* Magic (`IDXP`, Type: `dbtype_t`)

    The magic word.

Since it is very likely that such blocks would just have one or two offloaded
pointers for a given index, we manage vectors with these items:

* Index Identifier (`reference_t`)

    A number that clearly defines which index entry this data is for.

* Next Index Pointers Block (`reference_t`)

    A pointer to another Index Pointers Block in case this one was not enough
    to manage all the pointers for this index.

    Note that before using this feature the system should allocate a new
    `INDR` and fill that block up with just this one Index offloaded pointers.
    (i.e. avoid one block access if we can instead have the entire list
    in a single block).

* Array of Pointers (`array32` of `reference_t`)

    The actual offloaded references.

This way the system can manage multiple arrays in the same block.


### Data Block (`DATA`)

A data block is a block that includes the actual data of the row. If
the key in the index includes the entire key, we could avoid having
a copy here, however, when using secondary indexes, chances are we
need all the data here, which makes it easier.

The block includes the following fields:

* Magic (`DATA`, Type: `dbtype_t`)

    The magic word.

* Data (Type: _complex_)

    A block of data is defined with a dynamic structure as defined in the
    SCHEMA.md file, see Blob as well.

    The row buffers are allocated using the block_free_space class. The
    result is a pointer to a block of data which is giving us access to
    a set of 24 flags. We use two flags to define the status of a row:

    - Valid -- the row is considered valid; when the Moved and Deleted
      flags are both 0, the status is considered to be Valid
    - Moved -- the user made an update and the new block was larger so
      it was saved somewhere else; the first 8 bytes of data represent
      the offset of the new location (note: this is used only if the
      `INDP` is not used in this table)
    - Deleted -- the block was deleted; space is available for another
      block only it still appears in the index(es) so we cannot just
      release the space (note: this is used only if the `INDP` is not
      used since with `INDP` we only have to update the `INDP` pages)

    If we want to be able to get rid of the `Moved` over time, we need
    to have a reference counter as well. So each index that points to
    this data can be updated whenever we pass through it and the counter
    decremented. Once the counter reaches 0, we can actually reclaim
    the space and mark the entry as Deleted and release the data to the
    `FSPC` as expected.

    If the table is marked as "Secure" then the data of deleted rows
    gets cleared immediately.

    Data with a blob larger than one Data Block requires special handling.
    We have multiple solutions at our disposal:

    - Save a minimum amount of data here (i.e. small fields and especially
      fields we may SELECT against) then save the rest at the end of a
      separate file and save the pointer to that data

    - Save the data in multiple blocks; this block would include a pointer
      to the first block where the excess is saved and each such block can
      be linked to another block until the full size is saved. (See the
      `BLOB` Block) -- to save space, we want to allow for pointers to
      go to the end of a previous blob. The code can easily use a modulo
      to know whether we're at the start or in the middle of a block.

    We want to have a special type that we use whenever a blob is not
    defined directly in the Data Block. The table defines the size at
    which the system decides to use an external file.


### Blob Block (`BLOB`)

Whenever a row is really large, instead of saving that data in a block
with other rows, we directly save the data in a Blob Block.

The block still has a header:

* Magic (`BLOB`, Type: `dbtype_t`)

    The magic word.

* Size (Type: `uint32_t`)

    The number of bytes saved in this block. It may be the entire block
    (outside of the header, of course).

* Next Blob (Type: `reference_t`)

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
Cassandra use much smaller computers (i.e. between 16Gb and 64Gb)._

TBD: I think we can give the user ways to specify:

* Use blobs or files;
* Specify the size at which a blob is saved in an external file.


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

## Direct Access

In order to make it fast we use `mmap()`. This assumes our tables are fine,
of course, but well... you have to put your trust at some level.

### Direct Access & Alignment

One concern with using a `reinterpret_cast<name *>(ptr)` is the alignment
of the data. If we need to access a 64 bit offset, then these 64 bits
have to be aligned.

One way to test this is to use and not use the `pack` keyword and compare
the size of the structures in both cases. If any packing happens, then the
packed structures will be smaller.

    struct test
    {
        uint8_t         f_mode;
        uint16_t        f_size;
    };

In this example, the comipler will insert one byte after `f_mode`. In packed
mode, though, the structure will come out as being 3 bytes.

Another method is to use our own functions which are forced compiled to
an instruction which we know will work unaligned. AMD64, ARM, MIPS have
native CPU instructions that work with unaligned pointers and load 16, 32,
or 64 bits of data. With a small function such as:

    uin32_t load(void * ptr)
    {
        TODO:
        mov [ptr], eax      // correct once we have the code working (i.e. extend to 64 bits, the _asm(), etc.
        return;
    }

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
For example, it will know whether the primary and secondary indexes are to
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



### Allocation Map Array (`MAPA`)

DO NOT IMPLEMENT: I think that a list of empty blocks is going to be
much more efficient and for free space in _data blocks_, we want a list
of blocks with N bytes still available, since a row is at least 16 bytes,
we can put all such entries in a single block making it very easy to handle.

In order to quickly access any `AMAP`, we want to have an array of pointers
to all the `AMAP`. This is similar to the Top Index. Here we have an array
of pointers to AMAP blocks in order. So we can use very simple math to
go accross the blocks and download the correct data.

The block includes the following

* Magic (`MAPA`, Type: `dbtype_t`)

    The magic word.

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

DO NOT IMPLEMENT: See MAPA, I do not think we need those two.

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

* Magic (`AMAP`, Type: `dbtype_t`)

    The magic word.

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




vim: ts=4 sw=4 et
