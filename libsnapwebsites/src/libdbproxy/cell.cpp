/*
 * Text:
 *      src/cell.cpp
 *
 * Description:
 *      Handling of cell. There is no class representing a row in Cassandra.
 *      A row is just a key. We have this object to allow for our C++ array
 *      syntax to access the Cassandra data.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2017 Made to Order Software Corp.
 * 
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "libdbproxy/cell.h"
#include "libdbproxy/exception.h"
#include "libdbproxy/row.h"

#include <stdexcept>
#include <iostream>

namespace libdbproxy
{


/** \class cell
 * \brief A cell holds a name and value pair.
 *
 * A cell represents the value of a column in a row. The name of a cell
 * is the name of the corresponding column. The value is the data saved
 * in the Cassandra database.
 *
 * The name of the cell is kept as a binary key (it can be binary.) It
 * is limited it length to a little under 64Kb.
 *
 * Cells are loaded from the Cassandra database whenever the user
 * reads its value. Actually, the value is marked as cached once read
 * the first time. Each further access is done using the memory value
 * thus avoiding accessing the Cassandra server each time. Note that
 * may have some side effects if your process runs for a long time.
 * Use the cell, row, table, or context clearCache() functions to
 * palliate to this problem.
 *
 * Cells are saved to the Cassandra database whenever the user overwrite
 * its current value. In this case the cache is updated but the data is
 * non the less written to Cassandra except if the value was not modified
 * and the cache was marked as active.
 */


/** \var cell::f_row
 * \brief A pointer back to the row onwer.
 *
 * This bare pointer back to the row owner is used whenever the value is
 * read (and not yet cached) or written. This way we can send the data
 * back to the Cassandra database.
 */


/** \var cell::f_key
 * \brief The column name of this cell.
 *
 * This cell has a name paired with its value. This is the name part.
 * The key is saved in binary form only.
 */


/** \var cell::f_cached
 * \brief Whether a cell is a cache.
 *
 * This flag mark the cell as being a cache for the value defined in it.
 * By default a cell is marked as not caching anything. It becomes a cached
 * value once the value was saved in the Cassandra database or read from
 * the Cassandra system.
 *
 * Note however that the cell is no aware of whether the table is a memory
 * or Cassandra table. As such, the cache flag may be lying.
 */


/** \var cell::f_value
 * \brief A cell value.
 *
 * This member represents the value of this cell.
 *
 * Note that by default when you copy a cell value the value buffer itself
 * is not copied, instead it is shared. This is quite useful to avoid many
 * memory copies.
 */


/** \brief Initialize a row object.
 *
 * This function initializes a cell object. You must specify the
 * key of the column.
 *
 * In this case, the key of the cell is a binary buffer of data. Remember
 * however that the column names that are not ASCII may cause problems
 * (i.e. with CQL and the Cassandra CLI.)
 *
 * A cell is set to the NULL value by default.
 *
 * \exception exception
 * The key of the column cannot be empty or more than 64Kb. If that happens,
 * this exception is raised.
 *
 * \param[in] row  The parent row of this cell.
 * \param[in] column_key  The binary key of this cell.
 */
cell::cell(row::pointer_t row, const QByteArray& column_key)
    : f_row(row)
    , f_key(column_key)
    //, f_value() -- auto-init to "NULL" (nullValue() == true)
{
    if(f_key.size() == 0) {
        throw exception("the cell binary column key cannot be empty");
    }
    if(f_key.size() > 65535) {
        throw exception("the cell binary column key is more than 64Kb");
    }
}


/** \brief Clean up the cell object.
 *
 * This function ensures that all resources allocated by the
 * cell are released.
 */
cell::~cell()
{
}


/** \brief Retrieve the name of the column.
 *
 * This function returns the name of the column as specified in the
 * constructor.
 *
 * The name cannot be changed.
 *
 * Note that if you created the cell with a binary key (i.e. a
 * QByteArray parameter) then you CANNOT retrieve the column name.
 * Instead, use the columnKey() function.
 *
 * \exception exception
 * This function raises an exception if the cell was created with
 * a binary key.
 *
 * \return A string with the column name.
 *
 * \sa rowKey()
 */
QString cell::columnName() const
{
    return QString::fromUtf8(f_key.data());
}


/** \brief Retrieve the column key.
 *
 * This function returns the column key of this cell. The key is a binary
 * buffer of data. This function works whether the cell was created with a
 * name or a key.
 *
 * Note that when creating a cell with a binary key, you cannot retrieve
 * it using the columnName() function.
 *
 * \return A buffer of data representing the column key.
 *
 * \sa columnName()
 */
const QByteArray& cell::columnKey() const
{
    return f_key;
}


/** \brief Retrieve the cell value.
 *
 * This function is used to retrieve the cell value.
 *
 * Note that the value gets cached. That means if you call the function
 * again, then the same value will be returned (although the setValue()
 * can be used to change the cached value.)
 *
 * To reset the cache, use the clearCache() function.
 *
 * \return The current value defined in the cell
 *
 * \sa clearCache()
 * \sa setValue()
 */
const value& cell::getValue() const
{
    if(!f_cached)
    {
        parentRow()->getValue( f_key, const_cast<value&>(f_value) );
        f_cached = true;
    }
    return f_value;
}


/** \brief Change the value.
 *
 * This function changes the value of this cell. If the cell is currently
 * attached to a table in the Cassandra server, then it is written to the
 * server except if the value does not change.
 *
 * In other words, we avoid sending the same value to the Cassandra server
 * over and over again. To force a write to the Cassandra server, call
 * the clearCache() function before the setValue() function.
 *
 * \todo
 * If setting a counter, "value" is save in the cache as is. This means
 * the value may be an integer of any size instead of being a 64 bit
 * integer.
 *
 * \note
 * When the values are compared, the timestamp is ignored.
 *
 * \param[in] value  The new value for this cell.
 */
void cell::setValue(const value& val)
{
    if(!f_cached || f_value != val)
    {
        // TODO: if the cell represents a counter, it should be resized
        //       to a 64 bit value to work in all places
        f_value = val;
        parentRow()->insertValue(f_key, f_value);
        f_cached = true;
    }
}


/** \brief Change the value as if read from Cassandra.
 *
 * This function assigns the specified value as if it had been read from
 * Cassandra. This way the Row can set a value it just read and avoid
 * another read() (or worse, a write!)
 *
 * The value is marked as cached meaning that it was read or written to
 * the Cassandra database.
 *
 * This generally happens when you call value(). There is a simplified
 * view of what happens (without the row, table,
 * libdbproxy, and Thrift shown):
 *
 * \msc
 * width=900;
 * cell,context,QCassandraPrivate,Cassandra;
 * cell:>context [label="getValue()"];
 * context=>QCassandraPrivate [label="getValue()"];
 * QCassandraPrivate->Cassandra [label="get()"];
 * ...;
 * Cassandra->QCassandraPrivate [label="return"];
 * |||;
 * cell abox QCassandraPrivate [label="if value returned from last get()"];
 * QCassandraPrivate=>cell [label="getValue()", linecolor="red"];
 * cell>>QCassandraPrivate [label="return"];
 * cell abox QCassandraPrivate [label="end if"];
 * |||;
 * QCassandraPrivate>>context [label="return"];
 * context:>cell [label="return"];
 * \endmsc
 *
 * Note that similar calls happen whenever you call row::readCells()
 * and table::readRows().
 *
 * \param[in] value  The new value to assign to this cell.
 */
void cell::assignValue(const value& val)
{
    f_value = val;
    f_cached = true;
}


/** \brief Set the cell value.
 *
 * This function is called whenever you write a value to the Cassandra
 * database using the array syntax such as:
 *
 * \code
 * cluster["context"]["table"]["row"]["column"] = value;
 * \endcode
 *
 * Note that the value gets cached. That means if you call a getValue()
 * function, you get a copy of the value you saved here.
 *
 * To reset the cache, use the clearCache() function.
 *
 * \param[in] value  The new cell value.
 *
 * \return A reference to this cell.
 *
 * \sa clearCache()
 * \sa setValue()
 */
cell& cell::operator = (const value& val)
{
    setValue(val);
    return *this;
}


/** \brief Retrieve the cell value.
 *
 * This function is called whenever you read a value from the Cassandra
 * database using the array syntax such as:
 *
 * \code
 * value value = cluster["context"]["table"]["row"]["column"];
 * \endcode
 *
 * Note that the value gets cached. That means if you call the function
 * again, then the same value will be returned (although the setValue()
 * can be used to change the cached value.)
 *
 * To reset the cache, use the clearCache() function.
 *
 * \return The current value defined in the cell
 *
 * \sa clearCache()
 * \sa setValue()
 */
cell::operator value () const
{
    return value();
}


/** \brief Add a value to a counter.
 *
 * This function is used to add a value to a counter.
 *
 * The current cell value is expected to be 8 bytes, although we support
 * 1, 2, 4, and 8 byte integers. The result is saved back in this cell as
 * a 64 bits value (8 bytes).
 *
 * \param[in] value  The value to add to the counter. It may be negative.
 *
 * \return The expected value defined in the cell.
 */
void cell::add(int64_t val)
{
    // if cached, we update the value in memory as it is expected to be
    if(!f_value.nullValue())
    {
        // if the value is not defined, we'd have to read it before we can
        // increment it in memory; for this reason we don't do it at this point
        int64_t r(0);
        switch(f_value.size())
        {
        case 8:
            r = f_value.int64Value() + val;
            break;

        case 4:
            r = f_value.int32Value() + val;
            break;

        case 2:
            r = f_value.int16Value() + val;
            break;

        case 1:
            r = f_value.signedCharValue() + val;
            break;

        default:
            throw exception("a counter cell is expected to be an 8, 16, 32, or 64 bit value");

        }
        f_value.setInt64Value(r);
        f_cached = true;
    }

    parentRow()->insertValue( f_key, f_value );
}


/** \brief Add to a counter.
 *
 * This operator adds a value to a counter.
 *
 * \code
 * cluster["context"]["table"]["row"]["column"] += 5;
 * \endcode
 *
 * Note that the resulting value gets cached. That means if reading the value
 * after this call, the cached value is returned. To reset the cache, use
 * the clearCache() function.
 *
 * \warning
 * The value in the cell after this call is an approximation of the counter
 * value. The operator does not read the most current value.
 *
 * \param[in] value  The value to add to the existing value of this cell.
 *
 * \return A reference to this Cassandra cell.
 *
 * \sa clearCache()
 * \sa add()
 */
cell& cell::operator += (int64_t val)
{
    add(val);
    return *this;
}


/** \brief Increment a counter.
 *
 * This operator is used to add one to a counter.
 *
 * \code
 * ++cluster["context"]["table"]["row"]["column"];
 * \endcode
 *
 * \return A reference to this Cassandra cell.
 *
 * \sa add()
 * \sa operator + ()
 */
cell& cell::operator ++ ()
{
    add(1);
    return *this;
}


/** \brief Increment a counter.
 *
 * This operator is used to add one to a counter.
 *
 * \code
 * cluster["context"]["table"]["row"]["column"]++;
 * \endcode
 *
 * \warning
 * Note that this operator returns this cell and not
 * a copy because we cannot create a copy of the cell.
 *
 * \return A reference to this Cassandra cell.
 *
 * \sa add()
 * \sa operator + ()
 */
cell& cell::operator ++ (int)
{
    add(1);
    return *this;
}


/** \brief Subtract from a counter.
 *
 * This operator subtract a value from a counter.
 *
 * \code
 * cluster["context"]["table"]["row"]["column"] -= 9;
 * \endcode
 *
 * Note that the resulting value gets cached. That means if reading the value
 * after this call, the cached value is returned. To reset the cache, use
 * the clearCache() function.
 *
 * \warning
 * The value in the cell after this call is an approximation of the counter
 * value. The operator does not read the most current value.
 *
 * \param[in] value  The value to subtract from the current content of the cell.
 *
 * \return A reference to this Cassandra cell.
 *
 * \sa clearCache()
 * \sa add()
 */
cell& cell::operator -= (int64_t val)
{
    add(-val);
    return *this;
}


/** \brief Decrement a counter.
 *
 * This operator is used to subtract one from a counter.
 *
 * \code
 * --cluster["context"]["table"]["row"]["column"];
 * \endcode
 *
 * \return A reference to this Cassandra cell.
 *
 * \sa add()
 * \sa operator - ()
 */
cell& cell::operator -- ()
{
    add(-1);
    return *this;
}


/** \brief Decrement a counter.
 *
 * This operator is used to subtract one from a counter.
 *
 * \code
 * cluster["context"]["table"]["row"]["column"]--;
 * \endcode
 *
 * \warning
 * Note that this operator returns this cell and not
 * a copy because we cannot create a copy of the cell.
 *
 * \return A reference to this Cassandra cell.
 *
 * \sa add()
 * \sa operator - ()
 */
cell& cell::operator -- (int)
{
    add(-1);
    return *this;
}


/** \brief Retrieve the current consistency level of this value.
 *
 * This function returns the consistency level of this value. By default
 * it is set to one (CONSISTENCY_LEVEL_ONE.)
 *
 * The consistency level can be set using the setConsistencyLevel() function.
 *
 * \return The consistency level of this value.
 *
 * \sa setConsistencyLevel()
 * \sa value::consistencyLevel()
 */
consistency_level_t cell::consistencyLevel() const
{
    return f_value.consistencyLevel();
}


/** \brief Define the consistency level of this cell.
 *
 * This function sets the consistency of the f_value field of this cell.
 * This can be used to ensure the proper consistency on a read. In case
 * of a write, the consistency is always taken from the input value
 * parameter. For a read this is the only way to specify the consistency.
 *
 * By default, the consistency level is set to CONSISTENCY_LEVEL_DEFAULT
 * which means: use the consistency level defined in the libdbproxy object
 * linked with this cell. It is possible to set the consistency level
 * back to CONSISTENCY_LEVEL_DEFAULT.
 *
 * \param[in] level  The new consistency level for this cell next read.
 *
 * \sa consistencyLevel()
 * \sa value::setConsistencyLevel()
 */
void cell::setConsistencyLevel(consistency_level_t level)
{
    f_value.setConsistencyLevel(level);
}


/** \brief The value of a cell is automatically cached in memory.
 *
 * This function can be used to mark that the currently cached
 * value need to be reset on the next call to the value
 * casting operator.
 *
 * However, note that the data of the cell is NOT released by this
 * call. To release the data, look into clearing the row cache
 * instead.
 *
 * \note
 * Setting a cell to the null value (i.e. value.setNullValue()) will
 * clear the data in the Cassandra database too. So don't use that
 * function to clear the data from memory!
 *
 * \sa row::clearCache()
 */
void cell::clearCache()
{
    f_cached = false;
    f_value.setNullValue();
}


#if 0
/** \brief Retrieve the current timestamp of this cell value.
 *
 * This function returns the timestamp of the value variable member defined
 * in the cell. This value may be incorrect if the value wasn't read from
 * the Cassandra database or was never set with setTimestamp().
 *
 * \return The timestamp 64bit value.
 *
 * \sa setTimestamp()
 * \sa value::timestamp()
 */
int64_t cell::timestamp() const
{
    return f_value.timestamp();
}

/** \brief Define your own timestamp for this cell value.
 *
 * Set the timestamp of the value variable member of this cell.
 *
 * \param[in] timestamp  The time used to mark this value.
 *
 * \sa timestamp()
 * \sa value::setTimestamp()
 */
void cell::setTimestamp(int64_t val)
{
    f_value.setTimestamp(val);
}
#endif


/** \brief Get the pointer to the parent object.
 *
 * \return Shared pointer to the cassandra object.
 */
row::pointer_t cell::parentRow() const
{
    row::pointer_t row(f_row.lock());
    if(row == nullptr)
    {
        throw exception("this cell was dropped and is not attached to a row anymore");
    }

    return row;
}


} // namespace libdbproxy
// vim: ts=4 sw=4 et
