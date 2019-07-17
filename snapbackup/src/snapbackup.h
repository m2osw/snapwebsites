/*
 * Text:
 *      snapbackup/src/snapbackup.h
 *
 * Description:
 *      Dumps and restores the "snap_websites" context to/from a SQLite database.
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

// Snap libs
//
#include <advgetopt/advgetopt.h>
#include <casswrapper/query.h>

// Qt lib
//
#include <QSqlQuery>
#include <QString>

// C++ lib
//
#include <exception>
#include <memory>

typedef advgetopt::getopt::pointer_t    getopt_ptr_t;

class snapbackup
{
public:
    class cassandra_exception : public std::runtime_error
    {
    public:
        cassandra_exception( QString const & msg )
            : std::runtime_error(msg.toUtf8().data())
        {
        }
    };

    class sqlite_exception : public std::runtime_error
    {
    public:
        sqlite_exception( QString const & msg )
            : std::runtime_error(msg.toUtf8().data())
        {
        }
    };

    class schema_already_exists_exception : public std::runtime_error
    {
    public:
        schema_already_exists_exception( QString const & msg )
            : std::runtime_error(msg.toUtf8().data())
        {
        }
    };

                snapbackup( getopt_ptr_t opt );

    void        connectToCassandra();

    void        dropContext();
    void        dumpContext();
    void        restoreContext();

private:
    void        setSqliteDbFile( QString const & sqlDbFile );
    void        appendRowsToSqliteDb( int& id, casswrapper::Query::pointer_t cass_query, QString const & table_name );

    void        exec( QSqlQuery & q );

    void        storeSchemaEntry( QString const & description, QString const & name, QString const & schema_line );

    void        storeSchema( QString const & context_name );
    void        restoreSchema( QString const & context_name );

    void        dropContext( QString const & context_name );
    void        storeTables( int const count, QString const & context_name );
    void        restoreTables( QString const & context_name );

    casswrapper::Session::pointer_t     f_session;
    getopt_ptr_t                        f_opt;
    bool                                f_verbose = false;
};

// vim: ts=4 sw=4 et
