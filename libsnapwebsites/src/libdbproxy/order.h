/*
 * Header:
 *      src/libdbproxy/order.h
 *
 * Description:
 *      Manager an order to be sent to the snapdbproxy daemon.
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

class order
{
public:
    enum type_of_result_t
    {
        TYPE_OF_RESULT_CLOSE,           // i.e. close a cursor
        TYPE_OF_RESULT_BATCH_ADD,       // i.e. add to an open batch
        TYPE_OF_RESULT_BATCH_COMMIT,    // i.e. commit the accumulated batch to the database
        TYPE_OF_RESULT_BATCH_DECLARE,   // i.e. create a batch
        TYPE_OF_RESULT_BATCH_ROLLBACK,  // i.e. abort the current batch
        TYPE_OF_RESULT_DECLARE,         // i.e. create a cursor (SELECT)
        TYPE_OF_RESULT_DESCRIBE,        // i.e. just whether it worked or not
        TYPE_OF_RESULT_FETCH,           // i.e. read next page from cursor (nextPage)
        TYPE_OF_RESULT_ROWS,            // i.e. one SELECT
        TYPE_OF_RESULT_SUCCESS          // i.e. just whether it worked or not
    };

    type_of_result_t    get_type_of_result() const;
    QString             cql() const;
    void                setCql(QString const & cql_string, type_of_result_t const result_type = type_of_result_t::TYPE_OF_RESULT_SUCCESS);

    bool                validOrder() const;
    void                setValidOrder(bool const valid);

    consistency_level_t consistencyLevel() const;
    void                setConsistencyLevel(consistency_level_t consistency_level);

    int64_t             timestamp() const;
    void                setTimestamp(int64_t const user_timestamp);

    int32_t             timeout() const;
    void                setTimeout(int32_t const statement_timeout_ms);

    int8_t              columnCount() const;
    void                setColumnCount(int8_t const paging_size);

    int32_t             pagingSize() const;
    void                setPagingSize(int32_t const paging_size);

    int32_t             cursorIndex() const;
    void                setCursorIndex(int32_t const cursor_index);

    int32_t             batchIndex() const;
    void                setBatchIndex(int32_t const batch_index);

    bool                clearClusterDescription() const;
    void                setClearClusterDescription(bool const clear = true);

    bool                blocking() const;
    void                setBlocking(bool const block = true);

    size_t              parameterCount() const;
    QByteArray const &  parameter(int index) const;
    void                addParameter(QByteArray const & data);

    QByteArray          encodeOrder() const;
    bool                decodeOrder(unsigned char const * encoded_order, size_t size);

private:
    QString                         f_cql;
    bool                            f_valid = true;
    bool                            f_blocking = true;
    bool                            f_clear_cluster_description = false;
    type_of_result_t                f_type_of_result = type_of_result_t::TYPE_OF_RESULT_SUCCESS;
    consistency_level_t             f_consistency_level = CONSISTENCY_LEVEL_ONE; // TBD: can we get the libdbproxy default automatically?
    int64_t                         f_timestamp = 0;
    int32_t                         f_timeout_ms = 0;
    int8_t                          f_column_count = 1;
    int32_t                         f_paging_size = 0;
    int32_t                         f_cursor_index = -1;
    int32_t                         f_batch_index = -1;
    std::vector<QByteArray>         f_parameter;
};


} // namespace libdbproxy
// vim: ts=4 sw=4 et
