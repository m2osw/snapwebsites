/*
 * Text:
 *      snapwebsites/snapdb/src/snapdb.cpp
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

#include "snapdb.h"

#include "version.h"

// snapwebsites library
//
#include <snapwebsites/dbutils.h>
#include <snapwebsites/qstring_stream.h>

// casswrapper library
//
#include <casswrapper/query.h>

// C++ lib
//
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>

// last include
//
#include <snapwebsites/poison.h>

using namespace casswrapper;
using namespace casswrapper::schema;

namespace
{
    const std::vector<std::string> g_configuration_files; // Empty

    const advgetopt::getopt::option g_snapdb_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "Usage: %p [-<opt>] [table [row [cell [value]]]]",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "where -<opt> is one or more of:",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            'h',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "help",
            nullptr,
            "show this help output",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "config",
            nullptr,
            "Configuration file to initialize snapdb.",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            '\0',
            0,
            "context",
            nullptr,
            "name of the context from which to read",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            '\0',
            0,
            "count",
            nullptr,
            "specify the number of rows to display",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            '\0',
            0,
            "create-row",
            nullptr,
            "allows the creation of a row when writing a value",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            "drop-cell",
            nullptr,
            "drop the specified cell (specify table, row, and cell)",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            "drop-row",
            nullptr,
            "drop the specified row (specify table and row)",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            "drop-table",
            nullptr,
            "drop the specified table (specify table)",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            "full-cell",
            nullptr,
            "show all the data from that cell, by default large binary cells get truncated for display",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            "yes-i-know-what-im-doing",
            nullptr,
            "Force the dropping of tables, without warning and stdin prompt. Only use this if you know what you're doing!",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "host",
            nullptr,
            "host IP address or name (defaults to localhost)",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "port",
            nullptr,
            "port on the host to connect to (defaults to 9042)",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "info",
            nullptr,
            "print out the cluster name and protocol version",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "no-types",
            nullptr,
            "supress the output of the column type",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "use-ssl",
            nullptr,
            "Force the use of SSL, only if the keys are present.",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            "timeout",
            nullptr,
            "Define the timeout in milliseconds (i.e. 60000 represents 1 minute).",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            0,
            "save-cell",
            nullptr,
            "save the specified cell (specify table, row, and cell)",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "version",
            nullptr,
            "show the version of %p and exit",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "[table [row] [cell] [value]]",
            advgetopt::getopt::argument_mode_t::default_multiple_argument
        },
        {
            '\0',
            0,
            nullptr,
            nullptr,
            nullptr,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };
}
//namespace


snapdb::snapdb(int argc, char * argv[])
    : f_session( casswrapper::Session::create() )
    //, f_host("localhost") --auto-init
    //, f_port(9042) -- auto-init -- default to connect to snapdbproxy
    //, f_count(100) -- auto-init
    //, f_context("snap_websites") -- auto-init
    //, f_table("") -- auto-init
    //, f_row("") -- auto-init
    , f_opt( new advgetopt::getopt( argc, argv, g_snapdb_options, g_configuration_files, nullptr ) )
    , f_config( "snapdb" )
{
    if(f_opt->is_defined("version"))
    {
        std::cout << SNAPDB_VERSION_STRING << std::endl;
        exit(0);
    }

    // Set up configuration file
    //
    if( f_opt->is_defined("config") )
    {
        f_config.set_configuration_filename( f_opt->get_string("config") );
    }

    // first check options
    if( f_opt->is_defined( "count" ) )
    {
        f_count = f_opt->get_long( "count" );
    }

    if( f_opt->is_defined( "host" ) )
    {
        f_host = f_opt->get_string( "host" ).c_str();
    }
    else if( f_config.configuration_file_exists()
             && f_config.has_parameter("host") )
    {
        f_host = f_config["host"];
    }

    if( f_opt->is_defined( "port" ) )
    {
        f_port = f_opt->get_long( "port" );
    }
    else if( f_config.configuration_file_exists() && f_config.has_parameter("port") )
    {
        std::size_t pos(0);
        std::string const port(f_config["port"]);
        f_port = std::stoi(port, &pos, 10);
        if(pos != port.length())
        {
            throw snap::snapwebsites_exception_invalid_parameters("port to connect to Cassandra must be defined between 0 and 65535.");
        }
    }
    //
    if(f_port < 0 || f_port > 65535)
    {
        throw snap::snapwebsites_exception_invalid_parameters("port to connect to Cassandra must be defined between 0 and 65535.");
    }

    if( f_opt->is_defined( "context" ) )
    {
        f_context = f_opt->get_string( "context" ).c_str();
    }
    else if( f_config.configuration_file_exists() && f_config.has_parameter("context") )
    {
        f_context = f_config["context"];
    }

    if( f_opt->is_defined( "use-ssl" ) )
    {
        f_use_ssl = true;
    }
    else if( f_config.configuration_file_exists() && f_config.has_parameter("use_ssl") )
    {
        f_use_ssl = (f_config["use_ssl"] == "true");
    }

    // then check commands
    if( f_opt->is_defined( "help" ) )
    {
        usage(advgetopt::getopt::status_t::no_error);
        snap::NOTREACHED();
    }

    try
    {
        if( f_opt->is_defined( "info" ) )
        {
            info();
            exit(0);
        }
    }
    catch( const std::exception& except )
    {
        std::cerr << "Error connecting to the cassandra server! Reason=[" << except.what() << "]" << std::endl;
        exit( 1 );
    }
    catch( ... )
    {
        std::cerr << "Unknown error connecting to the cassandra server!" << std::endl;
        exit( 1 );
    }

    if( f_opt->is_defined( "timeout" ) )
    {
        // by creating this object we allow changing the timeout without
        // having to do anything else, however, we must make sure that
        // the object remains around at least until the session gets
        // created so we allocate it
        //
        f_request_timeout = std::make_shared<request_timeout>(f_session, f_opt->get_long( "timeout" ));
    }

    // finally check for parameters
    if( f_opt->is_defined( "--" ) )
    {
        int const arg_count = f_opt->size( "--" );
        if( arg_count > 4 )
        {
            std::cerr << "error: only four parameters (table, row, cell and value) can be specified on the command line." << std::endl;
            usage(advgetopt::getopt::status_t::error);
        }
        for( int idx = 0; idx < arg_count; ++idx )
        {
            if( idx == 0 )
            {
                f_table = f_opt->get_string( "--", idx ).c_str();
            }
            else if( idx == 1 )
            {
                f_row = f_opt->get_string( "--", idx ).c_str();
            }
            else if( idx == 2 )
            {
                f_cell = f_opt->get_string( "--", idx ).c_str();
            }
            else if( idx == 3 )
            {
                f_value = f_opt->get_string( "--", idx ).c_str();
            }
        }
    }
}

void snapdb::usage(advgetopt::getopt::status_t status)
{
    f_opt->usage( status, "snapdb" );
    exit(1);
}

void snapdb::info()
{
    try
    {
        f_session->connect( f_host, f_port, f_use_ssl );
        if(f_session->isConnected())
        {
            // read and display the Cassandra information
            auto q = Query::create( f_session );
            q->query( "SELECT cluster_name,native_protocol_version,partitioner FROM system.local" );
            q->start();
            std::cout << "Working on Cassandra Cluster Named \""    << q->getStringColumn("cluster_name")            << "\"." << std::endl;
            std::cout << "Working on Cassandra Protocol Version \"" << q->getStringColumn("native_protocol_version") << "\"." << std::endl;
            std::cout << "Using Cassandra Partitioner \""           << q->getStringColumn("partitioner")             << "\"." << std::endl;
            q->end();

            // At this time the following does not work, we will need CQL support first
            //const QCassandraClusterInformation& cluster_info(f_cassandra->clusterInformation());
            //int max(cluster_info.size());
            //std::cout << "With " << max << " nodes running." << std::endl;
            //for(int idx(0); idx < max; ++idx)
            //{
            //    const QCassandraNode& node(cluster_info.node(idx));
            //    std::cout << "H:" << node.nodeHost() << " R:" << node.nodeRack() << " DC:" << node.nodeDataCenter() << std::endl;
            //}
            exit(0);
        }
        else
        {
            std::cerr << "error: The connection failed!" << std::endl;
            exit(1);
        }
    }
    catch( const std::exception& ex )
    {
        std::cerr << "error: The connection failed! what=" << ex.what() << std::endl;
        exit(1);
    }
}


void snapdb::drop_table() const
{
    if(!f_opt->is_defined("yes-i-know-what-im-doing"))
    {
        if(!isatty(STDERR_FILENO))
        {
            std::cerr << "error: --drop-table aborted, either do it on your command line or use the --yes-i-know-what-im-doing option." << std::endl;
            exit(1);
        }
        std::cout << "WARNING: You are about to delete a table." << std::endl;
        std::cout << "Are you absolutely sure you want to do that?" << std::endl;
        std::cout << "Type \"I know what I'm doing\" and then enter: " << std::flush;

        std::string answer;
        std::getline(std::cin, answer);

        if(answer != "I know what I'm doing")
        {
            std::cerr << "error: aborting as apparently you do not know what you are doing." << std::endl;
            exit(1);
        }
    }

    try
    {
        auto q( Query::create(f_session) );
        q->setConsistencyLevel(Query::consistency_level_t::level_quorum);
        q->query( QString("DROP TABLE %1.%2;")
                    .arg(f_context)
                    .arg(f_table)
                    );
        q->start();
        q->end();
    }
    catch( std::exception const & ex )
    {
        std::cerr << "Drop table exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
}


void snapdb::drop_row() const
{
    try
    {
        snap::dbutils du( f_table, f_row );
        const QByteArray row_key( du.get_row_key() );
        auto q( Query::create(f_session) );
        q->setConsistencyLevel(Query::consistency_level_t::level_quorum);
        q->query( QString("DELETE FROM %1.%2 WHERE key = ?;")
                    .arg(f_context)
                    .arg(f_table)
                    );
        int bind = 0;
        q->bindString( bind++, row_key );
        q->start();
        q->end();
    }
    catch( const std::exception& ex )
    {
        std::cerr << "Remove row QCassandraQuery exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
}


void snapdb::drop_cell() const
{
    try
    {
        snap::dbutils du( f_table, f_row );
        const QByteArray row_key( du.get_row_key() );
        QByteArray col_key;
        du.set_column_name( col_key, f_cell );
        auto q( Query::create(f_session) );
        q->setConsistencyLevel(Query::consistency_level_t::level_quorum);
        q->query( QString("DELETE FROM %1.%2 WHERE key = ? and column1 = ?;")
            .arg(f_context)
            .arg(f_table)
            );
        int bind = 0;
        q->bindByteArray( bind++, row_key );
        q->bindByteArray( bind++, col_key );
        q->start();
        q->end();
    }
    catch( const std::exception& ex )
    {
        std::cerr << "Remove cell QCassandraQuery exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
}


bool snapdb::row_exists() const
{
    try
    {
        snap::dbutils du( f_table, f_row );
        const QByteArray row_key( du.get_row_key() );
        auto q( Query::create(f_session) );
        q->setConsistencyLevel(Query::consistency_level_t::level_quorum);
        q->query( QString("SELECT column1 FROM %1.%2 WHERE key = ?")
            .arg(f_context)
            .arg(f_table)
            );
        int bind = 0;
        q->bindByteArray( bind++, row_key );
        q->start();
        return q->rowCount() > 0;
    }
    catch( const std::exception& ex )
    {
        std::cerr << "Row exists QCassandraQuery exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    return false;
}


void snapdb::display_tables() const
{
    try
    {
        SessionMeta::pointer_t sm( SessionMeta::create(f_session) );
        sm->loadSchema();
        const auto& keyspaces( sm->getKeyspaces() );
        auto snap_iter = keyspaces.find(f_context);
        if( snap_iter == keyspaces.end() )
        {
            throw std::runtime_error(
                    QString("Context '%1' does not exist! Aborting!")
                    .arg(f_context).toUtf8().data()
                    );
        }

        auto kys( snap_iter->second );
        for( auto table : kys->getTables() )
        {
            std::cout << table.first << std::endl;
        }
    }
    catch( const std::exception& ex )
    {
        std::cerr << "Display tables exception caught! what=" << ex.what() << std::endl;
        exit(1);
    }
}


void snapdb::display_rows() const
{
    if( f_opt->is_defined("drop-table") )
    {
        drop_table();
        return;
    }

    try
    {
        snap::dbutils du( f_table, f_row );
        auto q( Query::create(f_session) );
        q->setConsistencyLevel(Query::consistency_level_t::level_quorum);
        q->query( QString("SELECT DISTINCT key FROM %1.%2;")
                    .arg(f_context)
                    .arg(f_table)
                    );
        q->setPagingSize(f_count);
        q->start();
        do
        {
            while( q->nextRow() )
            {
                std::cout << du.get_row_name( q->getByteArrayColumn(0) ) << std::endl;
            }
        }
        while( q->nextPage() );
        q->end();
    }
    catch( const std::exception& ex )
    {
        std::cerr << "Display rows QCassandraQuery exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
}


void snapdb::display_rows_wildcard() const
{
    try
    {
        snap::dbutils du( f_table, f_row );
        QString const row_start(f_row.left(f_row.length() - 1));
        std::stringstream ss;

        auto q( Query::create(f_session) );
        q->setConsistencyLevel(Query::consistency_level_t::level_quorum);
        q->query( QString("SELECT DISTINCT key FROM %1.%2;")
                    .arg(f_context)
                    .arg(f_table)
                    );
        q->setPagingSize(f_count);
        q->start();
        do
        {
            while( q->nextRow() )
            {
                const QString name(du.get_row_name(q->getByteArrayColumn(0)));
                if( name.length() >= row_start.length()
                    && row_start == name.mid(0, row_start.length()))
                {
                    ss << name << std::endl;
                }
            }
        }
        while( q->nextPage() );
        q->end();

        std::cout << ss.str();
    }
    catch( const std::exception& ex )
    {
        std::cerr << "Display rows wildcard QCassandraQuery exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
}


void snapdb::display_columns() const
{
    if( f_opt->is_defined("drop-row") )
    {
        drop_row();
        return;
    }

    try
    {
        snap::dbutils du( f_table, f_row );
        du.set_display_len( 24 );   // len for the elipsis for hex entries

        auto q( Query::create(f_session) );
        q->setConsistencyLevel(Query::consistency_level_t::level_quorum);
        q->query( QString("SELECT column1, value FROM %1.%2 WHERE key = ?;")
                    .arg(f_context)
                    .arg(f_table)
                    );
        q->bindByteArray( 0, du.get_row_key() );
        q->setPagingSize(f_count);
        q->start();
        QStringList keys;
        QStringList types;
        QStringList values;
        do
        {
            while( q->nextRow() )
            {
                const auto column_key( q->getByteArrayColumn("column1") );
                const auto column_val( q->getByteArrayColumn("value") );
                keys   << du.get_column_name      ( column_key );
                types  << "[" + du.get_column_type_name ( column_key ) + "]";
                values << du.get_column_value     ( column_key, column_val, true /*display_only*/ );
            }
        }
        while( q->nextPage() );
        q->end();

        int max_key_len = 0;
        std::for_each( keys.begin(), keys.end(),
            [&max_key_len]( const QString& key )
            {
                max_key_len = std::max( key.size(), max_key_len );
            }
        );

        int max_value_len = 0;
        std::for_each( values.begin(), values.end(),
            [&max_value_len]( const QString& key )
            {
                max_value_len = std::max( key.size(), max_value_len );
            }
        );

        auto old_flags = std::cout.flags();
        std::cout.flags(std::ios::left);
        for( int idx = 0; idx < keys.size(); ++idx )
        {
            std::cout
                    << std::setw(max_key_len)   << keys[idx]   << " = "
                    << std::setw(max_value_len) << values[idx]
                    ;
            if( !f_opt->is_defined("no-types") )
            {
                std::cout << " " << types[idx];
            }
            std::cout << std::endl;
        }
        std::cout.flags(old_flags);
    }
    catch( std::exception const & e )
    {
        // in most cases we get here because of something invalid in
        // the database
        std::cerr   << "error: could not properly read row \""
                    << f_row
                    << "\" in table \""
                    << f_table
                    << "\". It may not exist or its key is not defined as expected (i.e. not a valid md5sum)"
                    << std::endl
                    << "what=" << e.what()
                    << std::endl;
    }
}


void snapdb::display_cell() const
{
    if(f_opt->is_defined("drop-cell"))
    {
        drop_cell();
    }
    else
    {
        snap::dbutils du( f_table, f_row );
        du.set_display_len( 24 );   // len for the elipsis for hex entries

        QByteArray value;
        try
        {
            const QByteArray row_key( du.get_row_key() );
            QByteArray col_key;
            du.set_column_name( col_key, f_cell );
            auto q( Query::create(f_session) );
            q->setConsistencyLevel(Query::consistency_level_t::level_quorum);
            q->query( QString("SELECT value FROM %1.%2 WHERE key = ? AND column1 = ?;")
                    .arg(f_context)
                    .arg(f_table)
                    );
            int num = 0;
            q->bindByteArray( num++, row_key );
            q->bindByteArray( num++, col_key );
            q->start();
            if( !q->nextRow() )
            {
                throw std::runtime_error( "Row/cell NOT FOUND!" );
            }
            value = q->getByteArrayColumn("value");
            q->end();
        }
        catch( const std::exception& ex )
        {
            std::cerr << "QCassandraQuery exception caught! what=" << ex.what() << std::endl;
            exit(1);
            snap::NOTREACHED();
        }

        if(f_opt->is_defined("save-cell"))
        {
            std::fstream out;
            out.open(f_opt->get_string( "save-cell" ), std::fstream::out );
            if( !out.is_open() )
            {
                std::cerr << "error:display_cell(): could not open \"" << f_opt->get_string( "save-cell" )
                    << "\" to output content of cell \"" << f_cell
                    << "\" in table \"" << f_table
                    << "\" and row \"" << f_row
                    << "\"." << std::endl;
                exit(1);
                snap::NOTREACHED();
            }
            out.write( value.data(), value.size() );
        }
        else
        {
            std::cout << du.get_column_value
                ( f_cell.toUtf8(), value
                , !f_opt->is_defined("full-cell") /*display_only*/
                );
            if( !f_opt->is_defined("no-types") )
            {
                std::cout << " [" << du.get_column_type_name( f_cell.toUtf8() ) << "]";
            }
            std::cout << std::endl;
        }
    }
}


void snapdb::set_cell() const
{
    if( !f_opt->is_defined("create-row") )
    {
        if( !row_exists() )
        {
            std::cerr << "error:set_cell(): row \""
                      << f_row
                      << "\" not found in table \""
                      << f_table
                      << "\"."
                      << std::endl;
            exit(1);
            snap::NOTREACHED();
        }
    }

    try
    {
        snap::dbutils du( f_table, f_row );
        QByteArray const row_key( du.get_row_key() );
        QByteArray col_key;
        du.set_column_name( col_key, f_cell );
        QByteArray value;
        du.set_column_value( f_cell.toUtf8(), value, f_value );

        auto q( Query::create(f_session) );
        q->setConsistencyLevel(Query::consistency_level_t::level_quorum);
        q->query( QString("UPDATE %1.%2 SET value = ? WHERE key = ? AND column1 = ?;")
                .arg(f_context)
                .arg(f_table)
                );
        int bind_num = 0;
        q->bindByteArray( bind_num++, value   );
        q->bindByteArray( bind_num++, row_key );
        q->bindByteArray( bind_num++, col_key );
        q->start();
        q->end();
    }
    catch( const std::exception& ex )
    {
        std::cerr << "QCassandraQuery exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
}


void snapdb::exec()
{
    // the drop table is a "very slow" operation which times out every
    // time unless you allow for a very long timeout
    //
    if(!f_table.isEmpty()
    && f_row.isEmpty()
    && f_opt->is_defined("drop-table"))
    {
        // we put 5 minutes in this case... very slow!!!
        //
        f_session->setTimeout(5 * 60 * 60 * 1000);
    }

    f_session->connect( f_host, f_port, f_use_ssl );

    // TODO: the following is not very good in terms of checking whether
    //       the specified command line arguments are all proper; for
    //       example, you should not be allowed to use --save-cell and
    //       --drop-row together...
    //

    if(f_table.isEmpty())
    {
        display_tables();
    }
    else if(f_row.isEmpty())
    {
        display_rows();
    }
    else if(f_row.endsWith("%"))
    {
        display_rows_wildcard();
    }
    else if(f_cell.isEmpty())
    {
        display_columns();
    }
    else if(f_value.isEmpty())
    {
        display_cell();
    }
    else
    {
        set_cell();
    }
}

// vim: ts=4 sw=4 et
