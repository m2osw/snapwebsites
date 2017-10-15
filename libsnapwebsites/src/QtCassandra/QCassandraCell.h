/*
 * Header:
 *      src/QtCassandra/QCassandraCell.h
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

#include "QCassandraValue.h"
#include "QCassandraConsistencyLevel.h"

#include <QObject>
#include <QMap>

#include <memory>


namespace QtCassandra
{

class QCassandraRow;

// Cassandra Cell
class QCassandraCell
    : public QObject
    , public std::enable_shared_from_this<QCassandraCell>
{
public:
    typedef std::shared_ptr<QCassandraCell> pointer_t;

    virtual ~QCassandraCell();

    QString columnName() const;
    const QByteArray& columnKey() const;

    const QCassandraValue& value() const;
    void setValue(const QCassandraValue& value);

    QCassandraCell& operator = (const QCassandraValue& value);
    operator QCassandraValue () const;

    // for counters handling
    void add(int64_t value);
    QCassandraCell& operator += (int64_t value);
    QCassandraCell& operator ++ ();
    QCassandraCell& operator ++ (int);
    QCassandraCell& operator -= (int64_t value);
    QCassandraCell& operator -- ();
    QCassandraCell& operator -- (int);

    void clearCache();

    consistency_level_t consistencyLevel() const;
    void setConsistencyLevel(consistency_level_t level);

    std::shared_ptr<QCassandraRow> parentRow() const;

private:
    QCassandraCell(std::shared_ptr<QCassandraRow> row, const QByteArray& column_key);
    void assignValue(const QCassandraValue& value);

    friend class QCassandraRow;
    friend class QCassandraTable;

    std::weak_ptr<QCassandraRow>        f_row;
    QByteArray                          f_key;
    mutable bool                        f_cached = false;
    QCassandraValue                     f_value;
};

// array of rows
typedef QMap<QByteArray, QCassandraCell::pointer_t> QCassandraCells;



} // namespace QtCassandra

// vim: ts=4 sw=4 et
