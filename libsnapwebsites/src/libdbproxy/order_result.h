/*
 * Header:
 *      src/libdbproxy/QCassandraResult.h
 *
 * Description:
 *      Handle receiving results from a CQL order sent to snapdbproxy.
 *
 * Documentation:
 *      See the corresponding .cpp file.
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
#pragma once

#include "libdbproxy/consistency_level.h"

#include <vector>


namespace libdbproxy
{



class order_result
{
public:
    bool                succeeded() const;
    void                setSucceeded(bool success);

    size_t              resultCount() const;
    const QByteArray &  result(int index) const;
    void                addResult(QByteArray const & data);

    QByteArray          encodeResult() const;
    bool                decodeResult(unsigned char const * encoded_result, size_t size);

    void                swap(order_result& rhs);

private:
    bool                        f_succeeded = false;
    std::vector<QByteArray>     f_result;
};


} // namespace libdbproxy
// vim: ts=4 sw=4 et
