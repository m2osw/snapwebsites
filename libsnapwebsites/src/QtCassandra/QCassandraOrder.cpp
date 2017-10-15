/*
 * Text:
 *      src/QCassandraOrder.cpp
 *
 * Description:
 *      Manager an order to be sent to the snapdbproxy daemon.
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

//#pragma GCC push
//#pragma GCC diagnostic ignored "-Wundef"
//#include <sys/time.h>
//#pragma GCC pop

#include "QtCassandra/QCassandraOrder.h"

#include "QtCassandra/QCassandraException.h"
#include "QtCassandra/QCassandraValue.h"

#include "snapwebsites/log.h"

#include <QtCore>

#include <iostream>
#include <sstream>

namespace QtCassandra
{



QCassandraOrder::type_of_result_t QCassandraOrder::get_type_of_result() const
{
    return f_type_of_result;
}



/** \brief Get the CQL command.
 *
 * This function returns the UTF-8 encoded CQL order in a QString.
 *
 * \return The CQL order converted to a QString.
 */
QString QCassandraOrder::cql() const
{
    return f_cql;
}


void QCassandraOrder::setCql(QString const & cql_string, type_of_result_t const result_type)
{
    f_cql = cql_string;
    f_type_of_result = result_type;
}


/** \brief Check whether the order is considered valid.
 *
 * By default, an order is considered valid. It may be marked as invalid
 * to avoid sending it or on receipt to know that the order could not be
 * properly parsed back in the structure.
 *
 * \return true if the order is considered valid, false otherwise.
 */
bool QCassandraOrder::validOrder() const
{
    return f_valid;
}


/** \brief Set whether the order is valid.
 *
 * Orders are considered valid by default. It is possible to change that
 * value to false to mark them as invalid.
 *
 * The decodeOrder() function makes use of this function mark the order
 * result as invalid up until the entire order was parsed from the
 * source.
 *
 * \return true if the order is considered valid, false otherwise.
 */
void QCassandraOrder::setValidOrder(bool const valid)
{
    f_valid = valid;
}


/** \brief Retrieve the consistency level for this order.
 *
 * This function returns the consistency level to be used with this
 * CQL order.
 *
 * \return The order consistency level.
 */
consistency_level_t QCassandraOrder::consistencyLevel() const
{
    return f_consistency_level;
}


/** \brief Change the consistency level for this order.
 *
 * This function sets the consistency level of this order.
 *
 * \param[in] consistency_level  The new consistency level.
 */
void QCassandraOrder::setConsistencyLevel(consistency_level_t consistency_level)
{
    f_consistency_level = consistency_level;
}


int64_t QCassandraOrder::timestamp() const
{
    return f_timestamp;
}


void QCassandraOrder::setTimestamp(int64_t const user_timestamp)
{
    f_timestamp = user_timestamp;
}


int32_t QCassandraOrder::timeout() const
{
    return f_timeout_ms;
}


void QCassandraOrder::setTimeout(int32_t const statement_timeout_ms)
{
    f_timeout_ms = statement_timeout_ms;
}


int8_t QCassandraOrder::columnCount() const
{
    return f_column_count;
}


void QCassandraOrder::setColumnCount(int8_t const column_count)
{
    f_column_count = column_count;
}


int32_t QCassandraOrder::pagingSize() const
{
    return f_paging_size;
}


void QCassandraOrder::setPagingSize(int32_t const paging_size)
{
    f_paging_size = paging_size;
}


int32_t QCassandraOrder::cursorIndex() const
{
    return f_cursor_index;
}


void QCassandraOrder::setCursorIndex(int32_t const cursor_index)
{
    f_cursor_index = cursor_index;
}


int32_t QCassandraOrder::batchIndex() const
{
    return f_batch_index;
}


void QCassandraOrder::setBatchIndex(int32_t const batch_index)
{
    f_batch_index = batch_index;
}


bool QCassandraOrder::clearClusterDescription() const
{
    return f_clear_cluster_description;
}


void QCassandraOrder::setClearClusterDescription(bool const clear)
{
    f_clear_cluster_description = clear;
}


bool QCassandraOrder::blocking() const
{
    return f_blocking;
}


void QCassandraOrder::setBlocking(bool const block)
{
    f_blocking = block;
}


size_t QCassandraOrder::parameterCount() const
{
    return f_parameter.size();
}


QByteArray const & QCassandraOrder::parameter(int index) const
{
    if(static_cast<size_t>(index) >= f_parameter.size())
    {
        throw QCassandraOverflowException("QCassandraOrderOrder::parameter() called with an index too large.");
    }
    return f_parameter[index];
}


void QCassandraOrder::addParameter(QByteArray const & data)
{
    f_parameter.push_back(data);
}


/** \brief Encode the order so it can be sent to snapdbproxy.
 *
 * The function transforms the order in a blob that we can send over
 * the wire.
 *
 * The format is as follow:
 *
 * \li flags -- one byte with the type of result expected; whether this is
 *              a blocking call; and whether timestamp is included
 *
 * \li consistency level -- one byte, since these are still very small numbers
 *
 * \li CQL order -- size on 2 bytes (uint16_t) and then the string itself
 *
 * \li timestamp -- if bit 5 of the flags is set, 8 bytes with a timestamp
 *                  (at this time we use a timestamp only when deleting
 *                  something because it does not always delete otherwise...)
 *
 * \li number of parameters -- count on 2 bytes (uint16_t); this size may be
 *                             zero (i.e. no additional parameters)
 *
 * \li parameters -- a sequence of size (uint32_t) followed by parameter data                  
 *                   repeated for each parameter; if the number of parameters
 *                   is zero, then none of this exists
 *
 * \return A blob one can send to the snapdbproxy daemon.
 */
QByteArray QCassandraOrder::encodeOrder() const
{
    // Size of an order:
    //    4     tag (CQLP)
    //    4     size
    //    2     flags
    //    1     consistency level
    //    2     CQL length
    //  ...     length of CQL string
    //    8     timestamp
    //    4     timeout
    //    1     column count
    //    4     paging size
    //    2     cursor index
    //    2     number of parameters
    //    {
    //    4     parameter size
    //  ...     length of parameter
    //    }*
    //
    // Byte providing the expected size we get a single malloc()
    // instead of 1 malloc + X realloc()
    //
    uint32_t expected_size(4 + 4 + 2 + 1 + 2 + f_cql.length() + 8 + 4 + 1 + 4 + 2 + 2);
    for(auto param : f_parameter)
    {
        expected_size += 4 + param.size();
    }
    QCassandraEncoder encoder(expected_size);

    // Sending plain CQL (P for plain). We may later support CQLZ to send
    // a compressed QByteArray. (i.e. right now all snapdbproxies are
    // expected to be local so compression is not that useful, especially
    // for orders that are generally small except when uploading a file.)
    //
    encoder.appendSignedCharValue('C');
    encoder.appendSignedCharValue('Q');
    encoder.appendSignedCharValue('L');
    encoder.appendSignedCharValue('P');

    encoder.appendUInt32Value(expected_size - 8);

    // TBD: should we err if the f_valid flag is false?

    // flags
    //   f_type_of_result (bit 0 to 3)
    //   f_blocking (bit 4)
    //   f_timestamp included (bit 5)
    //   f_timeout_ms included (bit 6)
    //   f_column_count included (bit 7)
    //   f_paging_size included (bit 8)
    //   f_cursor_index included (bit 9)
    //   f_clear_cluster_description (bit 10)
    //
    uint16_t const flags(
                  (static_cast<uint16_t>(f_type_of_result) & 15)
                | (f_blocking                  ? 0x0010 : 0)
                | (f_timestamp != 0            ? 0x0020 : 0)
                | (f_timeout_ms != 0           ? 0x0040 : 0)
                | (f_column_count != 1         ? 0x0080 : 0)
                | (f_paging_size != 0          ? 0x0100 : 0)
                | (f_cursor_index != -1        ? 0x0200 : 0)
                | (f_clear_cluster_description ? 0x0400 : 0)
                | (f_batch_index != -1         ? 0x0800 : 0)
            );
    encoder.appendUInt16Value(flags);

    // consistency level (saved as one byte, signed)
    //
    signed char consistency_level(static_cast<signed char>(f_consistency_level));
    encoder.appendSignedCharValue(consistency_level);

    // CQL command as a PSTR (size is 2 bytes, max. 64Kb)
    //
    encoder.appendP16StringValue(f_cql);

    // the timestamp if not zero (save as 8 bytes, time in microseconds)
    //
    if(f_timestamp != 0)
    {
        encoder.appendInt64Value(f_timestamp);
    }

    // the timeout if not zero (save as 4 bytes, time in seconds)
    //
    if(f_timeout_ms != 0)
    {
        encoder.appendInt32Value(f_timeout_ms);
    }

    // the column count if not 1 (save as 1 bytes, 0 to 255 column in a select...)
    //
    if(f_column_count != 1)
    {
        encoder.appendSignedCharValue(f_column_count);
    }

    // the paging size if not zero (save as 4 bytes, 1 to 4 billion...)
    //
    if(f_paging_size != 0)
    {
        encoder.appendInt32Value(f_paging_size);
    }

    // the cursor index if not -1 (save as 2 bytes)
    //
    if(f_cursor_index != -1)
    {
        encoder.appendUInt16Value(f_cursor_index);
    }

    // the batch index if not -1 (save as 2 bytes)
    //
    if(f_batch_index != -1)
    {
        encoder.appendUInt16Value(f_batch_index);
    }

    // parameters, if any
    //
    // here we first save the number of parameters, possibly zero
    // (maximum of 64Kb); then we save the parameters as size
    // (up to 4Gb) and then the data
    //
    encoder.appendUInt16Value(f_parameter.size());
    for(auto param : f_parameter)
    {
        encoder.appendBinaryValue(param);
    }

    // adjust the size to match the buffer size exactly
    //
    encoder.replaceUInt32Value(encoder.size() - 8, 4);

    return encoder.result();
}


/** \brief Decode an order that was encoded with encodeOrder().
 *
 * snapdbproxy calls this function to get a QCassandraOrder from
 * data received from a client.
 *
 * \exception runtime_error
 * If the buffer is of the wrong size, the reading of the data will
 * fail raising this exception. We may later add a try/catch within
 * this function to return false instead. Yet, if the order is wrong
 * we are going to have a hard time reading the next buffer. Plus,
 * if things work as expected, synchronizing the input should never
 * be required.
 *
 * \param[in] encoded_order  The order received from a client.
 * \param[in] size  The number of bytes in the specified buffer.
 *
 * \return true if the buffer looked proper and can be processed further.
 */
bool QCassandraOrder::decodeOrder(unsigned char const * encoded_order, size_t size)
{
    // WARNING: Here I use the horrible fromRawData() function which does
    //          NOT copy the data to be checked here. However, that gives
    //          me full access to the QCassandraValue functions against
    //          QByteArray which is practical.
    //
    //          Just make sure to only use that 'encoded' buffer in this
    //          function. Do not pass it anywhere, or worst, return it!
    //
    QByteArray const encoded(QByteArray::fromRawData(reinterpret_cast<char const *>(encoded_order), size));
    QCassandraDecoder const decoder(encoded);

    try
    {
        // get the flags
        //
        int const flags(decoder.uint16Value());

        f_type_of_result = static_cast<type_of_result_t>(flags & 15);
        f_blocking = (flags & 0x10) != 0;
        f_clear_cluster_description = (flags & 0x400) != 0;

        // get the consistency level
        //
        f_consistency_level = static_cast<consistency_level_t>(decoder.signedCharValue());

        // get the CQL string (expected to be in UTF-8)
        //
        f_cql = decoder.p16StringValue();

        // if the timestamp was included, read it
        //
        if((flags & 0x20) != 0)
        {
            f_timestamp = decoder.int64Value();
        }
        else
        {
            // not included means we do not need it, i.e. zero
            f_timestamp = 0;
        }

        // if the timeout was included, read it
        //
        if((flags & 0x40) != 0)
        {
            f_timeout_ms = decoder.int32Value();
        }
        else
        {
            // not included means we do not need it, i.e. zero
            f_timeout_ms = 0;
        }

        // if the paging size was included, read it
        //
        if((flags & 0x80) != 0)
        {
            f_column_count = decoder.signedCharValue();
        }
        else
        {
            // not included means we do not need it, i.e. one
            f_column_count = 1;
        }

        // if the paging size was included, read it
        //
        if((flags & 0x100) != 0)
        {
            f_paging_size = decoder.int32Value();
        }
        else
        {
            // not included means we do not need it, i.e. zero
            f_paging_size = 0;
        }

        // if the cursor index was included, read it
        //
        if((flags & 0x200) != 0)
        {
            f_cursor_index = static_cast<int32_t>(decoder.uint16Value());
        }
        else
        {
            // not included means we do not need it, i.e. -1
            f_cursor_index = -1;
        }

        // if the batch index was included, read it
        //
        if((flags & 0x800) != 0)
        {
            f_batch_index = static_cast<int32_t>(decoder.uint16Value());
        }
        else
        {
            // not included means we do not need it, i.e. -1
            f_batch_index = -1;
        }

        // read the number of parameters that were included
        // this may be zero
        //
        size_t const param_count(decoder.uint16Value());
        for(size_t idx(0); idx < param_count; ++idx)
        {
            // read this parameter data and immediately push it in the
            // list of parameters; the binaryValue() function knows
            // to read the size first
            //
            f_parameter.push_back(decoder.binaryValue());
        }
    }
    catch( std::exception const& x )
    {
        SNAP_LOG_ERROR("decodeOrder(): exception! what=")(x.what());
        throw;
    }
    catch( ... )
    {
        SNAP_LOG_ERROR("unknown exception caught!");
        throw;
    }

    return true;
}



} // namespace QtCassandra
// vim: ts=4 sw=4 et
