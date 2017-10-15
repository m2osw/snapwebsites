// Snap Websites Servers -- retrieve a list of nodes from a QDomDocument based on an XPath
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
#ifndef Q_CASE_INSENSITIVE_STRING_H
#define Q_CASE_INSENSITIVE_STRING_H

#include <QString>

/** \brief Case insensitive QString.
 *
 * This class is an overload of the QString class which allows you to
 * create case insensitive strings as far as the operators are concerned.
 * All the other functions still work the same way.
 *
 * This is particularly useful if you manage a QMap<> with a string as
 * the key, string which should not be case sensitive.
 *
 * Note that only operators are overloaded. All the other functions work
 * the same way at this point. Later we may decide to overload all the
 * functions that have a Qt::CaseSensitivity parameter to default to
 * Qt::CaseInsensitive (which would make sense to have a really complete
 * implementation...)
 */
class QCaseInsensitiveString : public QString
{
public:
    QCaseInsensitiveString()
        : QString()
    {
    }

    QCaseInsensitiveString(QChar const * uni, int sz)
        : QString(uni, sz)
    {
    }

    QCaseInsensitiveString(QChar const * uni)
        : QString(uni)
    {
    }

    QCaseInsensitiveString(QChar ch)
        : QString(ch)
    {
    }

    QCaseInsensitiveString(int sz, QChar ch)
        : QString(sz, ch)
    {
    }

    QCaseInsensitiveString(QLatin1String const & str)
        : QString(str)
    {
    }

    QCaseInsensitiveString(QString const & other)
        : QString(other)
    {
    }

    QCaseInsensitiveString(QCaseInsensitiveString const & other)
        : QString(other)
    {
    }

    QCaseInsensitiveString(char const * str)
        : QString(str)
    {
    }

    QCaseInsensitiveString(QByteArray const & ba)
        : QString(ba)
    {
    }

    bool operator == (QString const & rhs) const
    {
        return compare(rhs, Qt::CaseInsensitive) == 0;
    }
    bool operator == (QCaseInsensitiveString const & rhs) const
    {
        return compare(rhs, Qt::CaseInsensitive) == 0;
    }
    bool operator == (char const * rhs) const
    {
        return compare(rhs, Qt::CaseInsensitive) == 0;
    }

    bool operator != (QString const & rhs) const
    {
        return compare(rhs, Qt::CaseInsensitive) != 0;
    }
    bool operator != (QCaseInsensitiveString const & rhs) const
    {
        return compare(rhs, Qt::CaseInsensitive) != 0;
    }
    bool operator != (char const * rhs) const
    {
        return compare(rhs, Qt::CaseInsensitive) != 0;
    }

    bool operator < (QString const & rhs) const
    {
        return compare(rhs, Qt::CaseInsensitive) < 0;
    }
    bool operator < (QCaseInsensitiveString const & rhs) const
    {
        return compare(rhs, Qt::CaseInsensitive) < 0;
    }

    bool operator <= (QString const & rhs) const
    {
        return compare(rhs, Qt::CaseInsensitive) <= 0;
    }
    bool operator <= (QCaseInsensitiveString const & rhs) const
    {
        return compare(rhs, Qt::CaseInsensitive) <= 0;
    }

    bool operator > (QString const & rhs) const
    {
        return compare(rhs, Qt::CaseInsensitive) > 0;
    }
    bool operator > (QCaseInsensitiveString const & rhs) const
    {
        return compare(rhs, Qt::CaseInsensitive) > 0;
    }

    bool operator >= (QString const & rhs) const
    {
        return compare(rhs, Qt::CaseInsensitive) >= 0;
    }
    bool operator >= (QCaseInsensitiveString const & rhs) const
    {
        return compare(rhs, Qt::CaseInsensitive) >= 0;
    }
};

#endif
// #ifndef Q_CASE_INSENSITIVE_STRING_H
// vim: ts=4 sw=4 et
