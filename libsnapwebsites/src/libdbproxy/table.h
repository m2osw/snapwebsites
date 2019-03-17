/*
 * Header:
 *      libsnapwebsites/src/libdbproxy/table.h
 *
 * Description:
 *      Handling of Cassandra Tables.
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

#include "libdbproxy/consistency_level.h"
#include "libdbproxy/predicate.h"
#include "libdbproxy/proxy.h"
#include "libdbproxy/row.h"

#include <casswrapper/schema.h>

#include <cstdint>
#include <memory>

namespace libdbproxy
{

class context;


// Cassandra Column Family
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class table
    : public QObject
    , public std::enable_shared_from_this<table>
{
public:
    typedef std::shared_ptr<table> pointer_t;

    virtual             ~table();

    proxy::pointer_t    getProxy() const { return f_proxy; }

    // context name
    const QString&      contextName() const;
    QString             tableName()   const;

    // fields
    //
    const casswrapper::schema::Value::map_t& fields() const;
    casswrapper::schema::Value::map_t&       fields();

    // handling
    void                create();
    //void                update();
    void                truncate();
    void                clearCache();

    // row handling
    uint32_t            readRows(row_predicate::pointer_t row_predicate );

    row::pointer_t      getRow(const char *      row_name);
    row::pointer_t      getRow(const QString&    row_name);
    row::pointer_t      getRow(const QByteArray& row_name);
    const rows&         getRows();

    row::pointer_t      findRow(const char * row_name) const;
    row::pointer_t      findRow(const QString& row_name) const;
    row::pointer_t      findRow(const QByteArray& row_name) const;
    bool                exists(const char* row_name) const;
    bool                exists(const QString& row_name) const;
    bool                exists(const QByteArray& row_name) const;
    row&                operator [] (const char * row_name);
    row&                operator [] (const QString& row_name);
    row&                operator [] (const QByteArray& row_name);
    const row&          operator [] (const char * row_name) const;
    const row&          operator [] (const QString& row_name) const;
    const row&          operator [] (const QByteArray& row_name) const;

    void                dropRow(const char *      row_name);
    void                dropRow(const QString&    row_name);
    void                dropRow(const QByteArray& row_name);

    std::shared_ptr<context> parentContext() const;

    void                startBatch();
    void                commitBatch();
    void                rollbackBatch();

private:
    friend class context;
    friend class row;

                        table(std::shared_ptr<context> context, const QString& table_name);

    void                setFromCassandra();
    void                parseTableDefinition( casswrapper::schema::TableMeta::pointer_t table_meta );
    void                insertValue(const QByteArray& row_key, const QByteArray& column_key, const value& value);
    bool                getValue(const QByteArray& row_key, const QByteArray& column_key, value& value);
    void                assignRow(const QByteArray& row_key, const QByteArray& column_key, const value& value);
    int32_t             getCellCount(const QByteArray& row_key, cell_predicate::pointer_t column_predicate);
    void                remove
                            ( const QByteArray& row_key
                            , const QByteArray& column_key
                            , consistency_level_t consistency_level = CONSISTENCY_LEVEL_DEFAULT
                            );
    void                remove( const QByteArray& row_key );
    void                closeCursor();

    bool                isCounterClass();

    void                loadTables();
    void                addRow( const QByteArray& row_key, const QByteArray& column_key, const QByteArray& data );

    QString             getTableOptions() const;

    casswrapper::schema::TableMeta::pointer_t
                            f_schema = casswrapper::schema::TableMeta::pointer_t();

    bool                    f_from_cassandra = false;
    std::weak_ptr<context>  f_context = std::weak_ptr<context>();
    QString                 f_context_name = QString();
    QString                 f_table_name = QString();
    rows                    f_rows = rows();

    proxy::pointer_t        f_proxy = proxy::pointer_t();
    int32_t                 f_cursor_index = -1;
    int32_t                 f_batch_index  = -1;
};
#pragma GCC diagnostic pop

// list of table definitions mapped against their name (see tableName())
typedef QMap<QString, table::pointer_t > tables;

} // namespace libdbproxy

// vim: ts=4 sw=4 et
