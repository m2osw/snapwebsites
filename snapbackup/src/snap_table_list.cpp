/*
 * Text:
 *      snap_table_list.cpp
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
 *      Copyright (c) 2012-2017 Made to Order Software Corp.
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

// our lib
//
#include "snap_table_list.h"

// 3rd party libs
//
#include <QtCore>

snapTableList::snapTableList()
    //: f_tableName  -- auto-init
    //, f_rowsToDump -- auto-init
{
}

void snapTableList::initList()
{
    if( f_list.isEmpty() )
    {
        // Ignore these tables by default
        //
        addEntry( "antihammering" );
        addEntry( "backend"       );
        addEntry( "cache"         );
        addEntry( "firewall"      );
        addEntry( "list"          );
        addEntry( "serverstats"   );
        addEntry( "sessions"      );
        addEntry( "test_results"  );
        addEntry( "tracker"       );
    }
}

void snapTableList::overrideTablesToDump( const QStringList& tables_to_dump )
{
    for( auto const & table_name : tables_to_dump )
    {
        auto iter(f_list.find(table_name));
        if( iter != f_list.end() )
        {
            f_list.erase(iter);
        }
    }
}

QStringList snapTableList::snapTableList::tablesToIgnore()
{
    QStringList the_list;
    for( auto const & entry : f_list )
    {
        the_list << entry.f_tableName;
    }
    return the_list;
}

bool snapTableList::canDumpRow( const QString& table_name, const QString& row_name )
{
    const auto& entry(f_list[table_name]);
    if( entry.f_rowsToDump.isEmpty() )  return true;
    return entry.f_rowsToDump.contains( row_name );
}

void snapTableList::addEntry( const QString& name )
{
    snapTableList entry;
    entry.f_tableName = name;
    f_list[name] = entry;
}

snapTableList::name_to_list_t  snapTableList::f_list;

// vim: ts=4 sw=4 et
