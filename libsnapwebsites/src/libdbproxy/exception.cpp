/*
 * Text:
 *      src/exception.cpp
 *
 * Description:
 *      Declaration of libdbproxy exceptions.
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
#include "libdbproxy/exception.h"

#include <sstream>

#include <execinfo.h>
#include <unistd.h>

namespace libdbproxy
{


exception::exception( const QString&     what ) : std::runtime_error(what.toUtf8().data()) {}
exception::exception( const std::string& what ) : std::runtime_error(what.c_str())         {}
exception::exception( const char*        what ) : std::runtime_error(what)                 {}


logic_exception::logic_exception( const QString&     what ) : exception(what.toUtf8().data()) {}
logic_exception::logic_exception( const std::string& what ) : exception(what.c_str())         {}
logic_exception::logic_exception( const char*        what ) : exception(what)                 {}


overflow_exception::overflow_exception( const QString&     what ) : exception(what.toUtf8().data()) {}
overflow_exception::overflow_exception( const std::string& what ) : exception(what.c_str())         {}
overflow_exception::overflow_exception( const char*        what ) : exception(what)                 {}


}
// namespace libdbproxy

// vim: ts=4 sw=4 et
