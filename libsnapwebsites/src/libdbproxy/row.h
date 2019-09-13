/*
 * Header:
 *      libsnapwebsites/src/libdbproxy/row.h
 *
 * Description:
 *      Handling of a row to access colunms within that row.
 *
 * Documentation:
 *      See the corresponding .cpp file.
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
#pragma once

#include "libdbproxy/cell.h"
#include "libdbproxy/predicate.h"

#include <memory>

namespace libdbproxy
{

class table;

// Cassandra Row
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class row
    : public QObject
    , public std::enable_shared_from_this<row>
{
public:
    typedef std::shared_ptr<row>  pointer_t;
    typedef QVector<value>        composite_column_names_t;

    virtual             ~row();

    QString             rowName() const;
    const QByteArray&   rowKey() const;

    int32_t             timeout() const;
    void                setTimeout(int32_t const statement_timeout_ms);

    int                 cellCount(const cell_predicate::pointer_t column_predicate = cell_predicate::pointer_t());
    uint32_t            readCells();
    uint32_t            readCells(cell_predicate::pointer_t column_predicate);

    cell::pointer_t     getCell(const char * column_name);
    cell::pointer_t     getCell(const QString& column_name);
    cell::pointer_t     getCell(const QByteArray& column_key);
    const cells&        getCells() const;

    cell::pointer_t     findCell(const QString& column_name) const;
    cell::pointer_t     findCell(const QByteArray& column_key) const;
    bool                exists(const char * column_name) const;
    bool                exists(const QString& column_name) const;
    bool                exists(const QByteArray& column_key) const;
    cell&               operator [] (const char * column_name);
    cell&               operator [] (const QString& column_name);
    cell&               operator [] (const QByteArray& column_key);
    const cell&         operator [] (const char * column_name) const;
    const cell&         operator [] (const QString& column_name) const;
    const cell&         operator [] (const QByteArray& column_key) const;

    void                clearCache();

    void                dropCell(const char *      column_name);
    void                dropCell(const QString&    column_name);
    void                dropCell(const QByteArray& column_key);

    std::shared_ptr<table> parentTable() const;

private:
    row(std::shared_ptr<table> table, const QByteArray& row_key);

    friend class table;
    friend class cell;

    void                insertValue(const QByteArray& column_key, const value& value);
    bool                getValue(const QByteArray& column_key, value& value);
    void                addValue(const QByteArray& column_key, int64_t value);
    void                closeCursor();

    // f_table is a parent that has a strong shared pointer over us so it
    // cannot disappear before we do, thus only a bare pointer is enough here
    // (there isn't a need to use a QWeakPointer or QPointer either)
    std::weak_ptr<table>      f_table        = std::weak_ptr<table>();
    QByteArray                f_key          = QByteArray();
    cells                     f_cells        = cells();
    int32_t                   f_cursor_index = -1;
    int32_t                   f_timeout_ms   = 0;
};
#pragma GCC diagnostic pop

// array of rows
typedef QMap<QByteArray, row::pointer_t > rows;



} // namespace libdbproxy
// vim: ts=4 sw=4 et
