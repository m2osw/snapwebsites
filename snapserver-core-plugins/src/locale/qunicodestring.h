// Snap Websites Server -- QString and UnicodeString merge
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

// Qt lib
//
#include <QString>

// C lib
//
#include <unicode/unistr.h>

// C++ lib
//
#include <string>


/** \file
 * \brief Simplify the use of the UnicodeString with QString support.
 *
 * Define an "overload" class named QUnicodeString which we can use to
 * handle ICU UnicodeString along with QString objects without having
 * to think about how to convert from one to another each time.
 */


class QUnicodeString : public UnicodeString
{
public:
    QUnicodeString()
    {
    }

    QUnicodeString(QString const & qstr)
        : UnicodeString(qstr.utf16()) // both string formats use UTF-16 so we can copy as is
    {
    }

    operator QString ()
    {
        return QString::fromUtf16(getTerminatedBuffer()); // both string formats use UTF-16 so we can copy as is
    }
};


// vim: ts=4 sw=4 et
