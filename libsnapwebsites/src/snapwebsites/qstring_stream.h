// Snap Websites Servers -- snap websites child process hanlding
// Copyright (C) 2011-2017  Made to Order Software Corp.
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
