/*
 * Header:
 *      src/libdbproxy/libdbproxy.h
 *
 * Description:
 *      Handling of the cassandra::CassandraClient and corresponding transports,
 *      protocols, sockets, etc.
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

#include "libdbproxy/context.h"
#include "libdbproxy/exception.h"
#include "libdbproxy/version.h"

#include <casswrapper/schema.h>

#include <memory>

namespace libdbproxy
{

class KsDef;
class CfDef;
class ColumnDef;

// Handling of the transport and CassandraClient objects
class libdbproxy
    : public QObject
    , public std::enable_shared_from_this<libdbproxy>
{
public:
    typedef std::shared_ptr<libdbproxy> pointer_t;

    static pointer_t create();
    virtual ~libdbproxy();

    static int versionMajor();
    static int versionMinor();
    static int versionPatch();
    static const char *version();

    // connection functions
    bool connect( const QString& host = "localhost", const int port = 4042 );
    void disconnect();
    bool isConnected() const;
    const QString& clusterName() const;
    const QString& protocolVersion() const;
    //const QCassandraClusterInformation& clusterInformation() const;
    const QString& partitioner() const;

    proxy::pointer_t      getProxy() const { return f_proxy; }

    // context functions (the database [Cassandra keyspace])
    context::pointer_t currentContext() const { return f_current_context; }
    context::pointer_t getContext(const QString& context_name);
    const contexts& getContexts() const;

    context::pointer_t findContext(const QString& context_name) const;
    context& operator[] (const QString& context_name);
    const context& operator[] (const QString& context_name) const;

    void dropContext(const QString& context_name);

    // default consistency level
    consistency_level_t defaultConsistencyLevel() const;
    void setDefaultConsistencyLevel(consistency_level_t default_consistency_level);

    // time stamp helper
    static int64_t timeofday();

private:
    libdbproxy();

    //context::pointer_t currentContext() const;
    void setCurrentContext(context::pointer_t c);
    void clearCurrentContextIf(const context& c);

    context::pointer_t getContext( casswrapper::schema::KeyspaceMeta::pointer_t keyspace_meta );
    void retrieveContextMeta( context::pointer_t c, const QString& context_name ) const;

    friend class context;

    proxy::pointer_t              f_proxy;
    context::pointer_t            f_current_context;
    mutable bool                  f_contexts_read = false;
    contexts                      f_contexts;
    QString                       f_cluster_name;
    QString                       f_protocol_version;
    QString                       f_partitioner;
    consistency_level_t           f_default_consistency_level = CONSISTENCY_LEVEL_ONE;
};

} // namespace libdbproxy

// vim: ts=4 sw=4 et
