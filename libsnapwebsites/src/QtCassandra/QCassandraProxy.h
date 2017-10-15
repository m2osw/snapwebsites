/*
 * Header:
 *      src/QtCassandra/QCassandraProxy.h
 *
 * Description:
 *      Handling of a data between libQtCassandra and our snapdbproxy
 *      daemon which is used to send CQL to Cassandra and receive the
 *      results.
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

// ourselves
//
#include "QtCassandra/QCassandraOrder.h"
#include "QtCassandra/QCassandraOrderResult.h"

// our lib
//
#include "QtCassandra/QCassandraConsistencyLevel.h"

//#include <QString>
//#include <QByteArray>
//#include <stdint.h>

// C++ lib
//
#include <memory>

// OS lib
//
#include <openssl/bio.h>



namespace QtCassandra
{

class QCassandraProxyIO
{
public:
    virtual                 ~QCassandraProxyIO() {}

    virtual ssize_t         read(void * buf, size_t count) = 0;
    virtual ssize_t         write(void const * buf, size_t count) = 0;
};


class QCassandraProxy
{
public:
    typedef std::shared_ptr<QCassandraProxy>    pointer_t;

                            QCassandraProxy();  // server
                            QCassandraProxy(QString const & host, int port);  // client

    QCassandraOrderResult   sendOrder(QCassandraOrder const & order);
    QCassandraOrder         receiveOrder(QCassandraProxyIO & reader);
    bool                    sendResult(QCassandraProxyIO & reader, QCassandraOrderResult const & result);

    bool                    isConnected() const;

private:
    void                    bio_get();
    void                    bio_reset();
    int                     bio_read(void * buf, size_t size);
    int                     bio_write(void const * buf, size_t size);

    std::shared_ptr<BIO>    f_bio;

    QString const           f_host;
    int const               f_port = 0;
};


} // namespace QtCassandra
// vim: ts=4 sw=4 et
