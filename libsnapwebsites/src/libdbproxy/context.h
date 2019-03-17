/*
 * Header:
 *      libsnapwebsites/src/libdbproxy/context.h
 *
 * Description:
 *      Handling of Cassandra contexts.
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

#include "libdbproxy/table.h"

#include <casswrapper/schema.h>

#include <memory>

namespace libdbproxy
{

class libdbproxy;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class context
    : public QObject
    , public std::enable_shared_from_this<context>
{
public:
    typedef std::shared_ptr<context>  pointer_t;

    virtual             ~context();

    const QString&      contextName() const;

    // fields
    //
    const casswrapper::schema::Value::map_t& fields() const;
    casswrapper::schema::Value::map_t&       fields();

    // tables
    table::pointer_t    getTable(const QString& table_name);
    const tables&       getTables();
    table::pointer_t    createTable(const QString& table_name);

    table::pointer_t    findTable(const QString& table_name);
    table&              operator [] (const QString& table_name);
    const table&        operator [] (const QString& table_name) const;

    // Context maintenance
    void                create();
    void                update();
    void                drop();
    void                dropTable(const QString& table_name);
    void                clearCache();
    void                loadTables();

    std::shared_ptr<libdbproxy>     parentCassandra() const;

private:
    friend class libdbproxy;

    void                makeCurrent();
                        context(std::shared_ptr<libdbproxy> cassandra, const QString& context_name);
                        context(context const &) = delete;
                        context & operator = (context const &) = delete;

    void                resetSchema();
    void                parseContextDefinition( casswrapper::schema::KeyspaceMeta::pointer_t keyspace );
    QString             getKeyspaceOptions();

    // f_cassandra is a parent that has a strong shared pointer over us so it
    // cannot disappear before we do, thus only a bare pointer is enough here
    // (there isn't a need to use a QWeakPointer or QPointer either)
    // Also, it cannot be a shared_ptr unless you make a restriction that
    // all instances must be allocated on the heap. Thus is the deficiency of
    // std::enabled_shared_from_this<>.
    casswrapper::schema::KeyspaceMeta::pointer_t f_schema = casswrapper::schema::KeyspaceMeta::pointer_t();
    //
    std::weak_ptr<libdbproxy>                   f_cassandra = std::weak_ptr<libdbproxy>();
    QString                                     f_context_name = QString();
    tables                                      f_tables = tables();
};
#pragma GCC diagnostic pop

typedef QMap<QString, context::pointer_t> contexts;


} // namespace libdbproxy

// vim: ts=4 sw=4 et
