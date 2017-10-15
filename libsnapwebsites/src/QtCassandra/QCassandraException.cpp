/*
 * Text:
 *      src/QCassandraException.cpp
 *
 * Description:
 *      Declaration of QCassandra exceptions.
 *
 * Documentation:
 *      See each function below.
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
#include "QtCassandra/QCassandraException.h"

#include <sstream>

#include <execinfo.h>
#include <unistd.h>

namespace QtCassandra
{


QCassandraException::QCassandraException( const QString&     what ) : std::runtime_error(what.toUtf8().data()) {}
QCassandraException::QCassandraException( const std::string& what ) : std::runtime_error(what.c_str())         {}
QCassandraException::QCassandraException( const char*        what ) : std::runtime_error(what)                 {}


QCassandraLogicException::QCassandraLogicException( const QString&     what ) : QCassandraException(what.toUtf8().data()) {}
QCassandraLogicException::QCassandraLogicException( const std::string& what ) : QCassandraException(what.c_str())         {}
QCassandraLogicException::QCassandraLogicException( const char*        what ) : QCassandraException(what)                 {}


QCassandraOverflowException::QCassandraOverflowException( const QString&     what ) : QCassandraException(what.toUtf8().data()) {}
QCassandraOverflowException::QCassandraOverflowException( const std::string& what ) : QCassandraException(what.c_str())         {}
QCassandraOverflowException::QCassandraOverflowException( const char*        what ) : QCassandraException(what)                 {}


}
// namespace QtCassandra

// vim: ts=4 sw=4 et
