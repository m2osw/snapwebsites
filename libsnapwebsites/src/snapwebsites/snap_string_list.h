// Snap Websites Server -- wrapper of popen()/pclose() with iostream like functions
// Copyright (C) 2013-2017  Made to Order Software Corp.
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

#include <QStringList>

#include "snapwebsites/snap_exception.h"

namespace snap
{

class snap_string_list_exception : public snap_exception
{
public:
    explicit snap_string_list_exception(const char *        whatmsg) : snap_exception("snap_string_list", whatmsg) {}
    explicit snap_string_list_exception(const std::string & whatmsg) : snap_exception("snap_string_list", whatmsg) {}
    explicit snap_string_list_exception(const QString &     whatmsg) : snap_exception("snap_string_list", whatmsg) {}
};


class snap_string_list_exception_out_of_range : public snap_string_list_exception
{
public:
    explicit snap_string_list_exception_out_of_range(const char *        whatmsg) : snap_string_list_exception(whatmsg) {}
    explicit snap_string_list_exception_out_of_range(const std::string & whatmsg) : snap_string_list_exception(whatmsg) {}
    explicit snap_string_list_exception_out_of_range(const QString &     whatmsg) : snap_string_list_exception(whatmsg) {}
};


class snap_string_list : public QStringList
{
public:
    inline snap_string_list() : QStringList() { }
    inline explicit snap_string_list(const QString &i) : QStringList(i) { }
    inline snap_string_list(const QStringList &l) : QStringList(l) { }
    inline snap_string_list(const QList<QString> &l) : QStringList(l) { }
#ifdef Q_COMPILER_INITIALIZER_LISTS
    inline snap_string_list(std::initializer_list<QString> args) : QStringList(args) { }
#endif

    const QString & at(int i) const
    {
        if(i < 0 || i >= size())
        {
            throw snap_string_list_exception_out_of_range("index is out of range for the at() function");
        }
        return QStringList::at(i);
    }

    const QString & operator [] (int i) const
    {
        if(i < 0 || i >= size())
        {
            throw snap_string_list_exception_out_of_range("index is out of range for the at() function");
        }
        return QStringList::operator[](i);
    }

    QString & operator [] (int i)
    {
        if(i < 0 || i >= size())
        {
            throw snap_string_list_exception_out_of_range("index is out of range for the at() function");
        }
        return QStringList::operator[](i);
    }

    // we should add all the functions that deal with a bare index...
};


} // namespace snap
// vim: ts=4 sw=4 et
