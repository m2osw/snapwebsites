/*
 * Text:
 *      src/QCassandraResult.cpp
 *
 * Description:
 *      Handle receiving results from a CQL order sent to snapdbproxy.
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

#include "QtCassandra/QCassandraException.h"
#include "QtCassandra/QCassandraProxy.h"
#include "QtCassandra/QCassandraValue.h"

#include <QtCore>

#include <iostream>
#include <sstream>

#include <unistd.h>


namespace QtCassandra
{


/** \brief Check whether the result represents a success or not.
 *
 * When you send an order to the snapdbproxy, the result may be a
 * failure. If this flag is false, then the result represents an
 * error and not the otherwise expected results from the order
 * sent.
 *
 * \return true if the order was successful.
 */
bool QCassandraOrderResult::succeeded() const
{
    return f_succeeded;
}


/** \brief Set whether the result was a success or not.
 *
 * This function can beused to set the succeeded flag to true
 * (i.e. the CQL order succeeded) or false (the CQL order
 * or something in between failed.)
 *
 * \param[in] success  The new value of the succeeded flag.
 */
void QCassandraOrderResult::setSucceeded(bool success)
{
    f_succeeded = success;
}


/** \brief Retrieve the number of results present in this object.
 *
 * In general you want to use this function to know how far your index
 * can go while calling the result() function.
 *
 * \return The number of results available in this object. It may be zero.
 *
 * \sa result()
 */
size_t QCassandraOrderResult::resultCount() const
{
    return f_result.size();
}


/** \brief Retrieve a result blob.
 *
 * This function is used to read back a resulting blob.
 *
 * \exception QCassandraOverflowException
 * The index must be between 0 and resultCount() - 1 or the
 * function raises this exception.
 *
 * \return A reference to a QByteArray representing this result data.
 *
 * \sa resultCount()
 */
QByteArray const & QCassandraOrderResult::result(int index) const
{
    if(static_cast<size_t>(index) >= f_result.size())
    {
        throw QCassandraOverflowException("QCassandraOrderResult::result() called with an index too large.");
    }
    return f_result[index];
}


/** \brief Add one result.
 *
 * This function adds one block of data representing a result.
 *
 * \param[in] data  A blob representing a result (i.e. maybe a cell).
 */
void QCassandraOrderResult::addResult(QByteArray const & data)
{
    f_result.push_back(data);
}


/** \brief Encode a set of results to be sent back to the client.
 *
 * This function is used by the snapdbproxy daemon to encode the results
 * and sent them to the client.
 *
 * \return A blob that the snapdbproxy can send to the client.
 */
QByteArray QCassandraOrderResult::encodeResult() const
{
    if(f_result.size() > 65535)
    {
        throw QCassandraException("result has too make values, limit is 64Kb - 1 value (a maximum of about 20,000 rows in one go)");
    }

    // the expected size of the final buffer, to avoid realloc() calls,
    // possibly many such calls, we use the final size in a reserve()
    // before adding the data
    //
    uint32_t expected_size(4 + 4 + 2);
    for(auto r : f_result)
    {
        expected_size += 4 + r.size();
    }
    QCassandraEncoder encoder(expected_size);

    // success or failure is encoded in the 4 letter we first send
    // when replying to the client
    //
    if(f_succeeded)
    {
        encoder.appendSignedCharValue('S');
        encoder.appendSignedCharValue('U');
        encoder.appendSignedCharValue('C');
        encoder.appendSignedCharValue('S');
    }
    else
    {
        encoder.appendSignedCharValue('E');
        encoder.appendSignedCharValue('R');
        encoder.appendSignedCharValue('O');
        encoder.appendSignedCharValue('R');
    }

    // we already have the size, contrary to the order, this size does
    // not vary depending on certain flags, so we can directly save the
    // correct value at once
    //
    encoder.appendUInt32Value(expected_size - 8);

    // save the number of result buffers
    //
    // then save each result with its size followed by its data
    //
    encoder.appendUInt16Value(f_result.size());
    for(auto r : f_result)
    {
        encoder.appendBinaryValue(r);
    }

#ifdef _DEBUG
    if(encoder.size() != expected_size)
    {
        throw QCassandraLogicException( "QCassandraOrderResult::encodeResult(): the expected and encoded sizes do not match..." );
    }
#endif

    return encoder.result();
}


/** \brief Decode a set of result buffers.
 *
 * This function is the opposite of the encodeResult() function. It is
 * used by the client to decode results sent to it by the snapdbproxy
 * daemon.
 *
 * \exception QCassandraException
 * If the buffer is of the wrong size, the reading of the data will
 * fail raising this exception. We may later add a try/catch within
 * this function to return false instead. Yet, if the order is wrong
 * we are going to have a hard time reading the next buffer. Plus,
 * if things work as expected, synchronizing the input should never
 * be required.
 *
 * \param[in] encoded_result  A pointer to the raw data just read from
 *                            the socket.
 * \param[in] size  The number of bytes in encoded_result.
 *
 * \return true if the function worked successfully.
 */
bool QCassandraOrderResult::decodeResult(unsigned char const * encoded_result, size_t size)
{
    // WARNING: Here I use the horrible fromRawData() function which does
    //          NOT copy the data to be checked here. However, that gives
    //          me full access to the QCassandraValue functions against
    //          QByteArray which is practical.
    //
    //          Just make sure to only use that 'decoder' buffer in this
    //          function. Do not pass it anywhere, or worst, return it!
    //
    QCassandraDecoder const decoder(QByteArray::fromRawData(reinterpret_cast<char const *>(encoded_result), size));

    // read the number of parameters that were included
    // this may be zero
    //
    size_t const result_count(decoder.uint16Value());
    for(size_t idx(0); idx < result_count; ++idx)
    {
        // read this parameter data and immediately push it in the
        // list of parameters
        //
        f_result.push_back(decoder.binaryValue());
    }

    return true;
}


void QCassandraOrderResult::swap(QCassandraOrderResult& rhs)
{
    std::swap(f_succeeded, rhs.f_succeeded);
    f_result.swap(rhs.f_result);
}



} // namespace QtCassandra
// vim: ts=4 sw=4 et
