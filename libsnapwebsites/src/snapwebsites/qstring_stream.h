// Snap Websites Servers -- allow for QString in ostream (i.e. "std::cout << qstring")
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#pragma once

// Qt lib
//
#include <QString>


// C++ lib
//
#include <iostream>



inline std::ostream & operator << ( std::ostream & strm, QByteArray const & qarray )
{
    return strm << qarray.data();
}


inline std::ostream & operator << ( std::ostream & strm, QString const & qstr )
{
    return strm << qstr.toUtf8();
}


inline std::ostream & operator << ( std::ostream & strm, QStringRef const & qstr )
{
    return strm << qstr.toString();
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


inline QString toQString(std::string const & s)
{
    return QString::fromUtf8(s.c_str());
}


inline std::string to_string(QString const & q)
{   
    return std::string(q.toUtf8().data());
}


// vim: ts=4 sw=4 et
