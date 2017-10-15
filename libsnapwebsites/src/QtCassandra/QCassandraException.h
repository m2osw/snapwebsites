/*
 * Header:
 *      src/QtCassandra/QCassandraException.h
 *
 * Description:
 *      Declare QtCassandra exceptions.
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

#include <libexcept/exception.h>

#include <stdexcept>
#include <string>
#include <QString>
#include <QStringList>


namespace QtCassandra
{



class QCassandraException : public std::runtime_error, public libexcept::exception_base_t
{
public:
    QCassandraException( const QString&     what );
    QCassandraException( const std::string& what );
    QCassandraException( const char*        what );
};


class QCassandraLogicException : public QCassandraException
{
public:
    QCassandraLogicException( const QString&     what );
    QCassandraLogicException( const std::string& what );
    QCassandraLogicException( const char*        what );
};


class QCassandraOverflowException : public QCassandraException
{
public:
    QCassandraOverflowException( const QString&     what );
    QCassandraOverflowException( const std::string& what );
    QCassandraOverflowException( const char*        what );
};


}
// namespace casswrapper

// vim: ts=4 sw=4 et
