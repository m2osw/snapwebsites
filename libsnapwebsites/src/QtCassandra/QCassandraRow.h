/*
 * Header:
 *      src/QtCassandra/QCassandraRow.h
 *
 * Description:
 *      Handling of a row to access colunms within that row.
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

#include "QtCassandra/QCassandraCell.h"
#include "QtCassandra/QCassandraPredicate.h"

#include <memory>

namespace QtCassandra
{

class QCassandraTable;

// Cassandra Row
class QCassandraRow
    : public QObject
    , public std::enable_shared_from_this<QCassandraRow>
{
public:
    typedef std::shared_ptr<QCassandraRow>  pointer_t;
    typedef QVector<QCassandraValue>        composite_column_names_t;

    virtual ~QCassandraRow();

    QString rowName() const;
    const QByteArray& rowKey() const;

    int cellCount(const QCassandraCellPredicate::pointer_t column_predicate = QCassandraCellPredicate::pointer_t());
    uint32_t readCells();
    uint32_t readCells(QCassandraCellPredicate::pointer_t column_predicate);

    QCassandraCell::pointer_t cell(const char* column_name);
    QCassandraCell::pointer_t cell(const QString& column_name);
    QCassandraCell::pointer_t cell(const QByteArray& column_key);
    const QCassandraCells& cells() const;

    QCassandraCell::pointer_t findCell(const QString& column_name) const;
    QCassandraCell::pointer_t findCell(const QByteArray& column_key) const;
    bool exists(const char* column_name) const;
    bool exists(const QString& column_name) const;
    bool exists(const QByteArray& column_key) const;
    QCassandraCell& operator [] (const char* column_name);
    QCassandraCell& operator [] (const QString& column_name);
    QCassandraCell& operator [] (const QByteArray& column_key);
    const QCassandraCell& operator [] (const char* column_name) const;
    const QCassandraCell& operator [] (const QString& column_name) const;
    const QCassandraCell& operator [] (const QByteArray& column_key) const;

    void clearCache();

    void dropCell(const char *      column_name);
    void dropCell(const QString&    column_name);
    void dropCell(const QByteArray& column_key);

    std::shared_ptr<QCassandraTable> parentTable() const;

private:
    QCassandraRow(std::shared_ptr<QCassandraTable> table, const QByteArray& row_key);

    void insertValue(const QByteArray& column_key, const QCassandraValue& value);
    bool getValue(const QByteArray& column_key, QCassandraValue& value);
    void addValue(const QByteArray& column_key, int64_t value);
    void closeCursor();

    friend class QCassandraTable;
    friend class QCassandraCell;

    // f_table is a parent that has a strong shared pointer over us so it
    // cannot disappear before we do, thus only a bare pointer is enough here
    // (there isn't a need to use a QWeakPointer or QPointer either)
    std::weak_ptr<QCassandraTable>      f_table;
    QByteArray                          f_key;
    QCassandraCells                     f_cells;
    int32_t                             f_cursor_index = -1;
};

// array of rows
typedef QMap<QByteArray, QCassandraRow::pointer_t > QCassandraRows;



} // namespace QtCassandra
// vim: ts=4 sw=4 et
