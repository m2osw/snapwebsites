/*
 * Text:
 *      snapbackup/src/snapdb_table_list.h
 *
 * Description:
 *      Reads and describes a Snap database. This ease checking out the
 *      current content of the database as the cassandra-cli tends to
 *      show everything in hexadecimal number which is quite unpractical.
 *      Now we do it that way for runtime speed which is much more important
 *      than readability by humans, but we still want to see the data in an
 *      easy practical way which this tool offers.
 *
 * License:
 *      Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
 *
 *      https://snapwebsites.org/
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

// snapwebsites lib
//
#include <snapwebsites/snap_string_list.h>

// Qt lib
//
#include <QtCore>


class snapTableList
{
public:
    typedef QMap<QString,snapTableList>    name_to_list_t;

    snapTableList();

    static void initList();
    static void overrideTablesToDump( const QStringList& tables_to_dump );

    QStringList tablesToIgnore();

    bool    canDumpRow( const QString& table_name, const QString& row_name );
    QString tableName() const { return f_tableName; }

private:
    static void addEntry( const QString& name );

    static name_to_list_t           g_list;

    QString                         f_tableName = QString();    // Name of the table
    snap::snap_string_list          f_rowsToDump = snap::snap_string_list();   // If empty, backup all rows
};


// vim: ts=4 sw=4 et
