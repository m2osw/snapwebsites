/*
 * Header:
 *      src/QtCassandra/QStringStream.h
 *
 * Description:
 *      Handling of QString with standard C++ streams.
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

#include <iostream>

#include <QString>

inline std::ostream & operator << ( std::ostream & str, QByteArray const & qarray )
{
    str << qarray.data();
    return str;
}


inline std::ostream & operator << ( std::ostream & str, QString const & qstr )
{
    str << qstr.toUtf8();
    return str;
}


inline std::ostream & operator << ( std::ostream & str, QStringRef const & qstr )
{
    str << qstr.toString();
    return str;
}


inline std::string operator + ( std::string const & str, QByteArray const & qarray )
{
    return str + qarray.data();
}


inline std::string operator + ( std::string const & str, QString const & qstr )
{
    return str + qstr.toUtf8();
}


inline std::string operator + ( std::string const & str, QStringRef const & qstr )
{
    return str + qstr.toString();
}


inline std::string & operator += ( std::string & str, QByteArray const & qarray )
{
    str = str + qarray;
    return str;
}


inline std::string & operator += ( std::string & str, QString const & qstr )
{
    str = str + qstr;
    return str;
}


inline std::string & operator += ( std::string & str, QStringRef const & qstr )
{
    str = str + qstr;
    return str;
}


// vim: ts=4 sw=4 et
