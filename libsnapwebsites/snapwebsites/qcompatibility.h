// Snap Websites Servers -- helper functions used against the DOM
// Copyright (c) 2021  Made to Order Software Corp.  All Rights Reserved
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

#include    <QString>
#include    <QStringList>

namespace snap
{


inline QStringList split_string(QString const & str, int sep)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return str.split(QChar(sep), Qt::SkipEmptyParts);
#else
    return str.split(QChar(sep), QString::SkipEmptyParts);
#endif
}


inline QStringList split_string(QString const & str, QString const & sep)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return str.split(sep, Qt::SkipEmptyParts);
#else
    return str.split(sep, QString::SkipEmptyParts);
#endif
}


inline QStringList split_string(QString const & str, QRegExp const & sep)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return str.split(sep, Qt::SkipEmptyParts);
#else
    return str.split(sep, QString::SkipEmptyParts);
#endif
}


} // namespace snap
// vim: ts=4 sw=4 et
