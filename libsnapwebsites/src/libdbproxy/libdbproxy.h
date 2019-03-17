/*
 * Header:
 *      libsnapwebsites/src/libdbproxy/libdbproxy.h
 *
 * Description:
 *      Handling of the cassandra::CassandraClient and corresponding transports,
 *      protocols, sockets, etc.
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

// library used to send database commands to the dbproxy daemon
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class libdbproxy
    : public QObject
    , public std::enable_shared_from_this<libdbproxy>
{
public:
    typedef std::shared_ptr<libdbproxy> pointer_t;

    static pointer_t            create();
    virtual                     ~libdbproxy();

    static int                  versionMajor();
    static int                  versionMinor();
    static int                  versionPatch();
    static char const *         version();

    // connection functions
    bool                        connect( QString const & host = "localhost", int const port = 4042 );
    void                        disconnect();
    bool                        isConnected() const;
    QString const &             clusterName() const;
    QString const &             protocolVersion() const;
    //const QCassandraClusterInformation& clusterInformation() const;
    QString const &             partitioner() const;

    proxy::pointer_t            getProxy() const { return f_proxy; }

    // context functions (the database [Cassandra keyspace])
    context::pointer_t          currentContext() const { return f_current_context; }
    context::pointer_t          getContext(QString const & context_name);
    contexts const &            getContexts(bool reset = false) const;

    context::pointer_t          findContext(QString const & context_name) const;
    context &                   operator[] (QString const & context_name);
    context const &             operator[] (QString const & context_name) const;

    void                        dropContext(const QString& context_name);

    // default consistency level
    consistency_level_t         defaultConsistencyLevel() const;
    void                        setDefaultConsistencyLevel(consistency_level_t default_consistency_level);

    // time stamp helper
    static int64_t              timeofday();

private:
                                libdbproxy();

    void                        setCurrentContext(context::pointer_t c);
    void                        clearCurrentContextIf(const context& c);

    context::pointer_t          getContext( casswrapper::schema::KeyspaceMeta::pointer_t keyspace_meta );
    void                        retrieveContextMeta( context::pointer_t c, QString const & context_name ) const;

    friend class context;

    proxy::pointer_t            f_proxy = proxy::pointer_t();
    context::pointer_t          f_current_context = context::pointer_t();
    mutable bool                f_contexts_read = false;
    contexts                    f_contexts = contexts();
    QString                     f_cluster_name = QString();
    QString                     f_protocol_version = QString();
    QString                     f_partitioner = QString();
    consistency_level_t         f_default_consistency_level = CONSISTENCY_LEVEL_ONE;
};
#pragma GCC diagnostic pop

} // namespace libdbproxy

// vim: ts=4 sw=4 et
