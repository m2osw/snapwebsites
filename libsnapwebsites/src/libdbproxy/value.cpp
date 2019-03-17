/*
 * Text:
 *      libsnapwebsites/src/libdbproxy/value.cpp
 *
 * Description:
 *      Handling of a cell value.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
 * 
 *      https://snapwebsites.org/
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

#include "libdbproxy/value.h"
#include "libdbproxy/exception.h"
#include <stdexcept>

namespace libdbproxy
{

/** \class value
 * \brief Holds a cell value.
 *
 * This class defines a value that is saved in a cell in the Cassandra
 * database. The class is optimized to with a QByteArray as the main
 * data holder.
 *
 * You can set the value as an integer, a floating point, a string, or
 * directly as a binary buffer. Strings are converted to UTF-8. Integers
 * and floating points are saved in big endian format (i.e. can then
 * be compared with a simple memcmp() call and sorted without magic
 * when saved as a Cassandra BytesType.)
 *
 * At this point, the copy operator is not overloaded meaning that
 * everything is copied as is. This also means the QByteArray copy
 * on write feature is used (i.e. the data buffer itself doesn't
 * get copied until the source of the destination is written to.)
 *
 * On the other hand, this means we need to copy the QByteArray
 * buffer to the std::string of the Column structure defined in
 * the Cassandra thrift code before sending it to Cassandra. In other
 * words, you may end up with one copy that you could otherwise have
 * avoided. On the other hand, many times just the value
 * objects will be copied between each others.
 */

/** \var value::f_ttl
 * \brief The TTL of this value.
 *
 * The TTL represents the number of seconds this value will be kept in the
 * Cassandra database. For example, a log could be made to disappear
 * automatically after 3 months.
 *
 * The default value is TTL_PERMANENT which means that the value
 * is permanent.
 */

/** \var value::timestamp_mode_t
 * \brief A timestamp mode.
 *
 * The timestamp can be defined in multiple ways. This mode specifies which
 * way you want to use for this value. The same mode is used by the different
 * remove() functions.
 *
 * The available modes are:
 *
 * \li TIMESTAMP_MODE_CASSANDRA -- the Cassandra server defines the timestamp
 * \li TIMESTAMP_MODE_AUTO -- the libQtCassandra library defines the timestamp
 * \li TIMESTAMP_MODE_DEFINED -- the user defined the timestamp
 *
 * \sa value::f_timestamp_mode
 */

/** \var value::f_timestamp_mode
 * \brief The timestamp mode.
 *
 * This variable member defines how the timestamp value is used. It can
 * be set to any one of the timestamp_mode_t values.
 */

/** \var value::f_timestamp
 * \brief The timestamp for this value.
 *
 * This variable member holds the timestamp of this value, however it is
 * used only if the value::f_timestamp_mode is set to
 * CASSANDRA_VALUE_TIMESTAMP. In all other cases it is ignored.
 */

/** \brief Initialize a value object.
 *
 * This function initializes a row object to NULL. This is
 * an equivalent to a BINARY with a size of 0.
 */
value::value(                     )      : cassvalue::Value()      {}
value::value(bool              val)      : cassvalue::Value(val)   {}
value::value(char              val)      : cassvalue::Value(val)   {}
value::value(signed char       val)      : cassvalue::Value(val)   {}
value::value(unsigned char     val)      : cassvalue::Value(val)   {}
value::value(int16_t           val)      : cassvalue::Value(val)   {}
value::value(uint16_t          val)      : cassvalue::Value(val)   {}
value::value(int32_t           val)      : cassvalue::Value(val)   {}
value::value(uint32_t          val)      : cassvalue::Value(val)   {}
value::value(int64_t           val)      : cassvalue::Value(val)   {}
value::value(uint64_t          val)      : cassvalue::Value(val)   {}
value::value(float             val)      : cassvalue::Value(val)   {}
value::value(double            val)      : cassvalue::Value(val)   {}
value::value(const QString&    val)      : cassvalue::Value(val)   {}
value::value(const QByteArray& val)      : cassvalue::Value(val)   {}
value::value(const char *data, int size) : cassvalue::Value(data, size) {}


/** \brief Compare this and rhs values for equality.
 *
 * This function returns true if this value and the rhs value are considered
 * equal.
 *
 * The equality takes the value buffer content, the TTL and the consistency
 * level in account. All three must be equal for the function to return true.
 *
 * \param[in] rhs  The value to compare against this value.
 *
 * \return true if both values are considered equal, false otherwise.
 */
bool value::operator == (const value& rhs)
{
    if(f_ttl != rhs.f_ttl) {
        return false;
    }
    return static_cast<Value>(*this) == static_cast<Value>(rhs);
}

/** \brief Compare this and rhs values for inequality.
 *
 * This function returns true if this value and the rhs value are considered
 * inequal.
 *
 * The inequality takes the value buffer content, the TTL and the consistency
 * level in account. Any of the three must be inequal for the function to
 * return true.
 *
 * \param[in] rhs  The value to compare against this value.
 *
 * \return true if both values are considered inequal, false otherwise.
 */
bool value::operator != (const value& rhs)
{
    if(f_ttl != rhs.f_ttl) {
        return true;
    }
    return static_cast<Value>(*this) != static_cast<Value>(rhs);
}

/** \brief Retrieve the current time to live value.
 *
 * This function returns the number of seconds defined as the lifetime of this
 * cell. The time to live is useful to create some temporary data. For example,
 * if you create an index of recent posts, you may want the older posts to
 * automatically be dropped after a given amount of time (i.e. 2 weeks.)
 *
 * This value can be set using the setTtl() function.
 *
 * \warning
 * The value is NOT read from an existing cell in the database. This is
 * because it slows down the SELECT quite a bit to read this value each
 * time even though 99.9% of the time it is not defined. If you really
 * need to have access, you can directly access the QCassandraQuery
 * system and send your own "SELECT TTL(value) FROM ...". Chances are,
 * you do not need to know how much longer a cell has to live. However,
 * if you read a cell to modify it and then save it back and that cell
 * may have a TTL, then it would be crusial to get that value. So far,
 * though, we only had to update with the standard TTL (i.e. if we update
 * a cell with a TTL, the TTL is reset back to the original, so something
 * that gets modified will last another full cycle instead of whatever
 * is left on it.)
 *
 * \return The number of seconds the cell will live.
 */
int32_t value::ttl() const
{
    return f_ttl;
}

/** \brief Set the time to live of this cell.
 *
 * Each cell can be defined as permanent (i.e. TTL not defined, or
 * set to TTL_PERMANENT) or can be defined as temporary.
 *
 * This value represents the number of seconds you want this value
 * to remain in the database.
 *
 * Note that if you want to keep values while running and then
 * lose them, you may want to consider creating a context in
 * memory only (i.e. a context on which you never call the
 * create() function.) Then the TTL is completely ignored, but
 * when you quit your application, the data is gone.
 *
 * \param[in] ttl  The new time to live of this value.
 */
void value::setTtl(int32_t ttl_val)
{
    if(ttl_val < 0) {
        throw exception("the TTL value cannot be negative");
    }

    f_ttl = ttl_val;
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
 * \sa cell::consistencyLevel()
 */
consistency_level_t value::consistencyLevel() const
{
    return f_consistency_level;
}

/** \brief Define the consistency level of this value.
 *
 * This function defines the consistency level of this value. The level is
 * defined as a static value in the value.
 *
 * Note that this value is mandatory so defining the right value is probably
 * often a good idea. The default is set to one which means the data is only
 * saved on that one cluster you are connected to. One of the best value is
 * QUORUM. The default can be changed in your libdbproxy object, set it with
 * your libdbproxy::setDefaultConsistencyLevel() function.
 *
 * The available values are:
 *
 * \li CONSISTENCY_LEVEL_ONE
 * \li CONSISTENCY_LEVEL_QUORUM
 * \li CONSISTENCY_LEVEL_LOCAL_QUORUM
 * \li CONSISTENCY_LEVEL_EACH_QUORUM
 * \li CONSISTENCY_LEVEL_ALL
 * \li CONSISTENCY_LEVEL_ANY
 * \li CONSISTENCY_LEVEL_TWO
 * \li CONSISTENCY_LEVEL_THREE
 *
 * The consistency level is probably better explained in the Cassandra
 * documentations that here.
 *
 * \param[in] level  The new consistency level for this cell.
 *
 * \sa consistencyLevel()
 * \sa libdbproxy::setDefaultConsistencyLevel()
 * \sa cell::setConsistencyLevel()
 */
void value::setConsistencyLevel(consistency_level_t level)
{
    // we cannot use a switch because these are not really
    // constants (i.e. these are pointers to values); although
    // we could cast to the Cassandra definition and switch on
    // those...
    if(level != CONSISTENCY_LEVEL_DEFAULT
    && level != CONSISTENCY_LEVEL_ONE
    && level != CONSISTENCY_LEVEL_QUORUM
    && level != CONSISTENCY_LEVEL_LOCAL_QUORUM
    && level != CONSISTENCY_LEVEL_EACH_QUORUM
    && level != CONSISTENCY_LEVEL_ALL
    && level != CONSISTENCY_LEVEL_ANY
    && level != CONSISTENCY_LEVEL_TWO
    && level != CONSISTENCY_LEVEL_THREE) {
        throw exception("invalid consistency level");
    }

    f_consistency_level = level;
}


} // namespace libdbproxy
// vim: ts=4 sw=4 et
