/*
 * Header:
 *      src/QtCassandra/QCassandraContext.h
 *
 * Description:
 *      Handling of Cassandra contexts.
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

#include "QtCassandra/QCassandraTable.h"

#include <casswrapper/schema.h>

#include <memory>

namespace QtCassandra
{

class QCassandra;

class QCassandraContext
    : public QObject
    , public std::enable_shared_from_this<QCassandraContext>
{
public:
    typedef std::shared_ptr<QCassandraContext>  pointer_t;

    virtual ~QCassandraContext();

    const QString& contextName() const;

    // fields
    //
    const casswrapper::schema::Value::map_t& fields() const;
    casswrapper::schema::Value::map_t&       fields();

    // tables
    QCassandraTable::pointer_t table(const QString& table_name);
    const QCassandraTables& tables();

    QCassandraTable::pointer_t findTable(const QString& table_name);
    QCassandraTable& operator[] (const QString& table_name);
    const QCassandraTable& operator[] (const QString& table_name) const;

    // Context maintenance
    void create();
    void update();
    void drop();
    void dropTable(const QString& table_name);
    void clearCache();
    void loadTables();

    std::shared_ptr<QCassandra> parentCassandra() const;

private:
    void makeCurrent();
    QCassandraContext(std::shared_ptr<QCassandra> cassandra, const QString& context_name);
    QCassandraContext(QCassandraContext const &) = delete;
    QCassandraContext & operator = (QCassandraContext const &) = delete;

    void resetSchema();
    void parseContextDefinition( casswrapper::schema::SessionMeta::KeyspaceMeta::pointer_t keyspace );
    QString getKeyspaceOptions();

    friend class QCassandra;

    // f_cassandra is a parent that has a strong shared pointer over us so it
    // cannot disappear before we do, thus only a bare pointer is enough here
    // (there isn't a need to use a QWeakPointer or QPointer either)
    // Also, it cannot be a shared_ptr unless you make a restriction that
    // all instances must be allocated on the heap. Thus is the deficiency of
    // std::enabled_shared_from_this<>.
    casswrapper::schema::SessionMeta::KeyspaceMeta::pointer_t f_schema;
    //
    std::weak_ptr<QCassandra>                   f_cassandra;
    QString                                     f_context_name;
    QCassandraTables                            f_tables;
};

typedef QMap<QString, QCassandraContext::pointer_t> QCassandraContexts;


} // namespace QtCassandra

// vim: ts=4 sw=4 et
