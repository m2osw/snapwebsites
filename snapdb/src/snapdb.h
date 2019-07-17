/*
 * Text:
 *      snapdb/src/snapdb.h
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

// our lib
//
#include <snapwebsites/snapwebsites.h>

// 3rd party libs
//
#include <QtCore>
#include <casswrapper/session.h>
#include <advgetopt/advgetopt.h>

// system
//
#include <memory>

/** \brief A class for easy access to all resources.
 *
 * This class is just so we use resource in an object oriented
 * manner rather than having globals, but that is clearly very
 * similar here!
 */
class snapdb
{
public:
    snapdb(int argc, char * argv[]);

    void info();
    void exec();

private:
    bool confirm_drop_check()    const;
    void drop_context()          const;
    void drop_table()            const;
    void drop_row()              const;
    void drop_cell()             const;
    bool row_exists()            const;
    void display_tables()        const;
    void display_rows()          const;
    void display_rows_wildcard() const;
    void display_columns()       const;
    void display_cell()          const;
    void set_cell()              const;

    casswrapper::Session::pointer_t             f_session         = casswrapper::Session::pointer_t();
    casswrapper::request_timeout::pointer_t     f_request_timeout = casswrapper::request_timeout::pointer_t();
    QString                                     f_host            = QString("localhost");
    int32_t                                     f_port            = 9042;
    bool                                        f_use_ssl         = false;
    int32_t                                     f_count           = 100;
    QString                                     f_context         = QString("snap_websites");
    QString                                     f_table           = QString();
    QString                                     f_row             = QString();
    QString                                     f_cell            = QString();
    QString                                     f_value           = QString();
    advgetopt::getopt::pointer_t                f_opt             = advgetopt::getopt::pointer_t();
    snap::snap_config                           f_config          = snap::snap_config();
};

// vim: ts=4 sw=4 et
