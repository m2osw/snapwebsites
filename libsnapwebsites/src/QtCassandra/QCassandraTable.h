/*
 * Header:
 *      src/QtCassandra/QCassandraTable.h
 *
 * Description:
 *      Handling of Cassandra Tables.
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

#include "QtCassandra/QCassandraConsistencyLevel.h"
#include "QtCassandra/QCassandraPredicate.h"
#include "QtCassandra/QCassandraProxy.h"
#include "QtCassandra/QCassandraRow.h"

#include <casswrapper/schema.h>

#include <cstdint>
#include <memory>

namespace QtCassandra
{

class QCassandraContext;


// Cassandra Column Family
class QCassandraTable
    : public QObject
    , public std::enable_shared_from_this<QCassandraTable>
{
public:
    typedef std::shared_ptr<QCassandraTable> pointer_t;

    virtual ~QCassandraTable();

    QCassandraProxy::pointer_t proxy() const { return f_proxy; }

    // context name
    const QString&  contextName() const;
    QString         tableName()   const;

    // fields
    //
    const casswrapper::schema::Value::map_t& fields() const;
    casswrapper::schema::Value::map_t&       fields();

    // handling
    void create();
    //void update();
    void truncate();
    void clearCache();

    // row handling
    uint32_t readRows(QCassandraRowPredicate::pointer_t row_predicate );

    QCassandraRow::pointer_t    row(const char*       row_name);
    QCassandraRow::pointer_t    row(const QString&    row_name);
    QCassandraRow::pointer_t    row(const QByteArray& row_name);
    const QCassandraRows&       rows();

    QCassandraRow::pointer_t    findRow(const char* row_name) const;
    QCassandraRow::pointer_t    findRow(const QString& row_name) const;
    QCassandraRow::pointer_t    findRow(const QByteArray& row_name) const;
    bool                        exists(const char* row_name) const;
    bool                        exists(const QString& row_name) const;
    bool                        exists(const QByteArray& row_name) const;
    QCassandraRow&              operator[] (const char* row_name);
    QCassandraRow&              operator[] (const QString& row_name);
    QCassandraRow&              operator[] (const QByteArray& row_name);
    const QCassandraRow&        operator[] (const char* row_name) const;
    const QCassandraRow&        operator[] (const QString& row_name) const;
    const QCassandraRow&        operator[] (const QByteArray& row_name) const;

    void dropRow(const char *      row_name);
    void dropRow(const QString&    row_name);
    void dropRow(const QByteArray& row_name);

    std::shared_ptr<QCassandraContext> parentContext() const;

    void        startBatch();
    void        commitBatch();
    void        rollbackBatch();

private:
    QCassandraTable(std::shared_ptr<QCassandraContext> context, const QString& table_name);

    void        setFromCassandra();
    void        parseTableDefinition( casswrapper::schema::SessionMeta::KeyspaceMeta::TableMeta::pointer_t table_meta );
    void        insertValue(const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value);
    bool        getValue(const QByteArray& row_key, const QByteArray& column_key, QCassandraValue& value);
    void        assignRow(const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value);
    int32_t     getCellCount(const QByteArray& row_key, QCassandraCellPredicate::pointer_t column_predicate);
    void        remove
                    ( const QByteArray& row_key
                    , const QByteArray& column_key
                    , consistency_level_t consistency_level = CONSISTENCY_LEVEL_DEFAULT
                    );
    void        remove( const QByteArray& row_key );
    void        closeCursor();

    bool        isCounterClass();

    void        loadTables();
    void        addRow( const QByteArray& row_key, const QByteArray& column_key, const QByteArray& data );

    QString     getTableOptions() const;

    friend class QCassandraContext;
    friend class QCassandraRow;

    casswrapper::schema::SessionMeta::KeyspaceMeta::TableMeta::pointer_t   f_schema;

    bool                                f_from_cassandra = false;
    std::weak_ptr<QCassandraContext>    f_context;
    QString                             f_context_name;
    QString                             f_table_name;
    QCassandraRows                      f_rows;

    QCassandraProxy::pointer_t          f_proxy;
    int32_t                             f_cursor_index = -1;
    int32_t                             f_batch_index  = -1;
};

// list of table definitions mapped against their name (see tableName())
typedef QMap<QString, QCassandraTable::pointer_t > QCassandraTables;

} // namespace QtCassandra

// vim: ts=4 sw=4 et
