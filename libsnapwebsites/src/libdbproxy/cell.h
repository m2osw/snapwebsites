/*
 * Header:
 *      src/libdbproxy/cell.h
 *
 * Description:
 *      Handling of a cell to access data in columns within the
 *      Cassandra database.
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

#include "value.h"
#include "consistency_level.h"

#include <QObject>
#include <QMap>

#include <memory>


namespace libdbproxy
{

class row;

// Cassandra Cell
class cell
    : public QObject
    , public std::enable_shared_from_this<cell>
{
public:
    typedef std::shared_ptr<cell> pointer_t;

    virtual ~cell();

    QString columnName() const;
    const QByteArray& columnKey() const;

    const value& getValue() const;
    void setValue(const value& value);

    cell& operator = (const value& value);
    operator value () const;

    // for counters handling
    void add(int64_t value);
    cell& operator += (int64_t value);
    cell& operator ++ ();
    cell& operator ++ (int);
    cell& operator -= (int64_t value);
    cell& operator -- ();
    cell& operator -- (int);

    void clearCache();

    consistency_level_t consistencyLevel() const;
    void setConsistencyLevel(consistency_level_t level);

    std::shared_ptr<row> parentRow() const;

private:
    cell(std::shared_ptr<row> row, const QByteArray& column_key);
    void assignValue(const value& value);

    friend class row;
    friend class table;

    std::weak_ptr<row>        f_row;
    QByteArray                          f_key;
    mutable bool                        f_cached = false;
    value                     f_value;
};

// array of rows
typedef QMap<QByteArray, cell::pointer_t> cells;



} // namespace libdbproxy

// vim: ts=4 sw=4 et
