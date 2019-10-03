
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

The database schema represents the entire set of table.

Each type, though, has atype.

## Blob

Whenever accessing the database, we access a row which is saved in the
file as a _blob_. Instead of distinguishing the columns within the table
we use a blob. It is important to distinguish the columns only when we
need to do a "SELECT" or have an "INDEX" against the data. In pretty much
all the other situation, it is not useful to have columns like in an RMDB.

A `SELECT ...` can either be a search or a cursor to go through much of
the data in a table.

An `INDEX ...` allows us to pre-select a set of rows based on various
fields having a given value. Indexes also allow us to get the data sorted
(since to create the index we have to sort the rows).

### Column Names

We limit the names of column to `[A-Za-z0-9_]` and it cannot start with
a digit. This way we can easily have such name appear in a script.

The length of a column name is limited to 128.

### Column Types

We offer C++ types in our database, although for strings we limit the
type to only UTF-8. You can do conversion in your software if needed.
Since UTF-8 handles all possible characters, it is plenty sufficient
for strings. You can of course use a binary buffer for strings you
do not want to convert to UTF-8.

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
* `string`
* `time_t`
* `composite`
* `reference`

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
* Default -- a default value of the type as defined above


