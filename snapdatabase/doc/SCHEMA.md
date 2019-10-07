
# The Schema

The database manages a schema which defines the environment where the data
lives. We have one schema per context.

We have several parts:

1. The _context_; which represents the equivalent of a Database in SQL.
2. The _table_; which represents a table in SQL 
3. The _blob_; which is the list of columns and their content

Our tables are a one to one equivalent to an SQL table in concept.
However, our database does not force you to define all the column in
each row. When you try to access a column it can be NULL or non-existant
and the distinction can be important in some cases.

Some columns are important because they are used to index the data, but
really only the columns used to define the primary index (the columns
used as the key to this row of data) are mandatory.

With the schema definition, the blob is limited to a specific set of
column names and their values to the specific type that column was
assigned. The database keeps a copy of that information. The local
library gets a copy through the local daemon which saves the information
in a file locally. So we can access it very quickly on further reload
(the `LISTEN` is used to know whether the schema changes.)

The schema is a list of tables.

## Table Types (i.e. the way the table gets used)

The database schema represents the entire set of tables.

Each table, though, has its own schema definition separate from the
database as a whole. When you query that information, you get one
table with the information from a table, not the database as a whole.

See libsnapwebsites/src/snapwebsites/tables.xsd for some detail
documentation about the XML format we use to define tables.

Note that we want to support many types. Those defined at the moment
may not include all the types we want to support in the end.

The existing types (as defined in the XSD file) are:

* content

    A standard content table, such tables are expected to be used to
    save data once and read it many times, also a content table is
    likely to have many updates too (i.e. branch table), those are
    limited once the data is good, it will settle.

    Tables with files should allow for the MD5 or Murmur to be used
    directly (i.e. with Cassandra, we calculate the MD5 of the file
    and then Cassandra calculates the Murmur of that MD5 and use
    that as the Primary Index key).

* data

    A content table which is written once and read many times and has
    nearly no updates (i.e. revision table). Like the content tables,
    the changes are likely to settle.

* queue

    The queue tables are used to save data once and read it once and
    then we are done with it and can drop the data.

    In truth, there is a bit more than that:
    
    - the data is written once, but it can be overwriting an existing entry;
      i.e. a page gets updated once, a request is made for a backend to do
      some additional work but before that happens, the page gets updated
      again; that second update request overwrites the first since the first
      was not yet handled the newest data is all that's of interest.

    - the row being worked on has its status changed from `WAITING` to
      `WORKING`.

    - the `WORKING` status comes along with a timestamp which marks when
      the work started. This allows us to automatically timeout a process.
      We will manage this timestamp (Start Processing Data) with a watchdog
      type of mechanism. So if your process is still running, you ping the
      database which updates the timestamp so the timeout in increased.

    - then the data is dropped once the backend is done

* log

    A table that is mainly used to write to in normal operation. Obviously
    we want to be able to read from those tables, but they will be optimized
    for writing.

    At this point we do not write logs to a database, only to flat files.
    However, it would be neat to have our own database and multiple indexes
    on the log fields (i.e. we can tweak our snaplogger to send data in a
    format such as JSON so it would be easy to save the JSON as seperate
    columns in our database and query the results on varioys fields.)

* session

    A table which is both written and read equally; also the rows are likely
    setup to timeout automatically; tables are not likely to grow forever,
    although they could still be really large.

    These tables should not be compacted if we can replace them after a
    little while (although we really keep user session for 1 year, so
    this may be tricky).

    The Murmur hash used to index these entries could be used directly by
    the database (instead of creating a Murmur of the session random
    number.)

## Blob

Whenever accessing the database, we access a row which is saved in the
file as a _blob_. Instead of distinguishing the columns within the table
we use a blob. It is important to distinguish the columns only when we
need to do a "SELECT" or have an "INDEX" against the data. In pretty much
all the other situations, it is not useful to have columns like in an RMDB.

A `SELECT ...` can either be a search or a cursor to go through much of
the data in a table.

An `INDEX ...` allows us to pre-select a set of rows based on various
fields having a given value. Indexes also allow us to get the data sorted
(since to create the index we have to sort the rows).

When reading the data from the row, we have a blob defined with
column name identifiers, the specificy type if the column is defined
as `any`, and then the data. If the data has a variable size, it
is preceeded by a `uint32_t` with the size.

* `<column identifier>` -- represents the column name
* `<specific type>` -- only if the column type in the XML definition is `any`
* `<size>` -- only if type is variable
* `<data>` -- the actual data

When the cell includes a composite value, the `<size>`/`<data>` is
repeated for each type. Again, the `<size>` is not used if the type
is not variable.

### Column Names

We limit the names of column to `[A-Za-z0-9_]` and it cannot start with
a digit. This way we can easily have such name appear in a script.

The length of a column name is limited to 128.

The names are defined in the Schema definition of the table file. Within
rows, we instead use an identifier. This makes it much more efficient.
The identifier is limited to 8 bits until we reach 256 columns then we
use 16 bits. We limit the number of columns to 65536. Note that we do
not have the "columns are sorted" feature as in Cassandra which is
why we can limit the columns this much. At this point, we never had
that many column except to support links, lists, etc. which we will
eliminate using our indexes.

### Column Types

We offer C++ types in our database, although for strings we limit the
type to only UTF-8. You can do conversion in your software if needed.
Since UTF-8 handles all possible characters, it is plenty sufficient
for strings. You can of course use a binary buffer for strings you
do not want to convert to UTF-8.

TODO: verify in the dbutils to see that we captured all the types we
want to support.

* `any`
* `void` (can only be NULL or empty)
* `bool`
* `int8_t`
* `uint8_t`
* `int16_t`
* `uint16_t`
* `int32_t`
* `uint32_t`
* `int64_t`
* `uint64_t`
* `float`
* `double`
* `long double`
* `char`
* `string`
* `time_t`
* `time_ms_t`
* `murmur`
* `composite`
* `reference`

The `any` type means any of the other types can be used for this data.
For this to work, we actually include the type along the data.
TBD: we may use this special type when converting a column from one type
to another.

#### Composite: Structure

A _Composite_ column is a column composed of other types. For example,
we have columns that include a date and a value (`time_t:int64_t`).
These are an equivalent to a C++ structure and it will be returned
as such (i.e. it should be properly aligned so you can use the data
in a structure after a reinterpret cast).

#### Composite: Array

The concept of arrays is also supported. So a definition such as:

    uint8_t[256]

Defines an array of 256 bytes. Do not put a size if the array size is
dynamic (note that we will not save the zeroes at the end of the array,
but once in memory, we'd otherwise allocate the full array if a specific
size is specified).

This works within the structure definition as shown above. Only a
dynamically sized array can only appear as the last field of the
structure.

    time_t:int64_t:uint8_t[]

This type has a `time_t` value, then a 64 bit integer (`int64_t`) and
finally a buffer of bytes of any size (0 or more).

### Other Column Settings

Columns can also be assigned the following settings:

* Mandatory -- this column must be defined or a `SET`/`INSERT` fails.
* Encrypt -- whether that data needs to be saved encrypted (i.e. user data)
* Default -- a default value of the type as defined above
* To Be Removed -- when attempting to change the type of a column, we actually
  want to create a new column which uses the old one as a default value.
  However, the old column should not be used for anything else that his
  default value. So by marking a column as "To Be Removed" we can enforce
  the "do not try to access this column" and especially, to avoid overwriting
  its existing contents. That being said, we could do these gently and
  automatically fallback to the new column when the old one is being
  accessed.









vim: ts=4 sw=4 et
