/*
 * Text:
 *      snapexpr.cpp
 *
 * Description:
 *      Process a C-like expression. This tool is mainly a test to check
 *      that the C-like parser and execution environment work.
 *
 * License:
 *      Copyright (c) 2014-2017 Made to Order Software Corp.
 * 
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <snapwebsites/snap_expr.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/snapwebsites.h>

#include <advgetopt.h>

#include <QtCassandra/QCassandra.h>
#include <QtCassandra/QCassandraContext.h>

#include <iostream>

#include <QFile>
#include <QTextCodec>

#include <snapwebsites/poison.h>


advgetopt::getopt::option const g_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        NULL,
        NULL,
        "Usage: %p -<opt> ...",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        NULL,
        NULL,
        "where -<opt> is one or more of:",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        'h',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        NULL,
        "Show usage and exit.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "host",
        "localhost",
        "Specify the IP address to the Cassandra node.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "no-cassandra",
        NULL,
        "Prevent Cassandra's initialization. This allows for testing Cassandra related functions in the event the database was not setup.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'p',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "port",
        "4042",
        "Define the port used by the Cassandra node.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        'q',
        0,
        "quiet",
        NULL,
        "Print out the result quietly (without introducer)",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        's',
        0,
        "serialize",
        NULL,
        "compile and then serialize the expressions and print out the result",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'v',
        0,
        "verbose",
        NULL,
        "information about the task being performed",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'e',
        0,
        "expression",
        NULL,
        "one or more C-like expressions to compile and execute",
        advgetopt::getopt::argument_mode_t::default_multiple_argument
    },
    {
        '\0',
        0,
        NULL,
        NULL,
        NULL,
        advgetopt::getopt::argument_mode_t::end_of_options
    }
};
advgetopt::getopt *g_opt = NULL;

int g_errcnt = 0;
bool g_verbose = 0;

QtCassandra::QCassandra::pointer_t          g_cassandra;
QtCassandra::QCassandraContext::pointer_t   g_context;


void connect_cassandra()
{
    // Cassandra already exists?
    if(g_cassandra)
    {
        return;
    }

    // connect to Cassandra
    g_cassandra = QtCassandra::QCassandra::create();
    g_cassandra->setDefaultConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
    QString const host(QString::fromUtf8(g_opt->get_string("host").c_str()));
    int const port(QString(g_opt->get_string("port").c_str()).toInt());
    if(!g_cassandra->connect(host, port))
    {
        std::cerr << "error: could not connect to Cassandra." << std::endl;
        exit(1);
    }

    // select the Snap! context
    g_cassandra->contexts();
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
    g_context = g_cassandra->findContext(context_name);
    if(!g_context)
    {
        // we connected to the database, but it is not properly initialized!?
        std::cerr << "error: the process connected to Cassandra but it could not find the \"" + context_name + "\" context." << std::endl;
        exit(1);
    }
    // The host name is important only if we need a lock which at this
    // point we do not provide in the C-like expression feature
    //f_context->setHostName(f_server->get_parameter("server_name"));

    snap::snap_expr::expr::set_cassandra_context(g_context);
}


void expr(std::string const & expr)
{
    if(g_verbose)
    {
        std::cout << "compiling [" << expr << "]" << std::endl;
    }

    snap::snap_expr::expr e;
    if(!e.compile(expr.c_str()))
    {
        std::cerr << "expression \"" << expr << "\" failed compilation." << std::endl;
        return;
    }

    if(g_opt->is_defined("serialize"))
    {
        if(g_verbose)
        {
            std::cout << "serializing..." << std::endl;
        }

        QByteArray s(e.serialize());
        std::cout << s << std::endl;
        return;
    }

    if(g_verbose)
    {
        std::cout << "execute the expression..." << std::endl;
    }

    snap::snap_expr::variable_t result;
    snap::snap_expr::variable_t::variable_map_t variables;
    snap::snap_expr::functions_t functions;
    e.execute(result, variables, functions);

    if(!g_opt->is_defined("quiet"))
    {
        std::cout << "result of type " << static_cast<int>(result.get_type()) << " is " << result.get_value().size() << " bytes = ";
    }
    switch(result.get_type())
    {
    case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_NULL:
        std::cout << "(null)";
        break;

    case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
        std::cout << "(bool) " << result.get_value().boolValue();
        break;

    case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
        std::cout << "(int8) " << result.get_value().signedCharValue();
        break;

    case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
        std::cout << "(uint8) " << result.get_value().unsignedCharValue();
        break;

    case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
        std::cout << "(int16) " << result.get_value().int16Value();
        break;

    case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
        std::cout << "(uint16) " << result.get_value().uint16Value();
        break;

    case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
        std::cout << "(int32) " << result.get_value().int32Value();
        break;

    case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
        std::cout << "(uint32) " << result.get_value().uint32Value();
        break;

    case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
        std::cout << "(int64) " << result.get_value().int64Value();
        break;

    case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
        std::cout << "(uint64) " << result.get_value().uint64Value();
        break;

    case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
        std::cout << "(float) " << result.get_value().floatValue();
        break;

    case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
        std::cout << "(double) " << result.get_value().doubleValue();
        break;

    case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
        {
            std::cout << "(string) \"";
            QByteArray const str(result.get_value().stringValue().toUtf8());
            int const len(str.size());
            for(int idx(0); idx < len; ++idx)
            {
                int const c(static_cast<unsigned char>(str[idx]));
                switch(c)
                {
                case 0x07:
                    std::cout << "\\a";
                    break;

                case 0x08:
                    std::cout << "\\b";
                    break;

                case 0x09:
                    std::cout << "\\t";
                    break;

                case 0x0A:
                    std::cout << "\\n";
                    break;

                case 0x0B:
                    std::cout << "\\v";
                    break;

                case 0x0C:
                    std::cout << "\\f";
                    break;

                case 0x0D:
                    std::cout << "\\r";
                    break;

                case 0x22:
                    std::cout << "\\\"";
                    break;

                case 0x27:
                    std::cout << "'";
                    break;

                case 0x5C:
                    std::cout << "\\\\";
                    break;

                default:
                    // TODO: the following is incorrect (i.e. we are dealing
                    //       with UTF-8 and not just ISO-8859-1 or such)
                    //
                    if(c < 0x20
                    || (c >= 0x80 && c <= 0x9F))
                    {
                        std::cout << "\\" << std::oct << c << std::dec;
                    }
                    else
                    {
                        std::cout << static_cast<unsigned char>(c);
                    }
                    break;

                }
            }
            std::cout << "\"";
        }
        break;

    case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BINARY:
        std::cout << "(binary) ...";
        break;

    }
    std::cout << std::endl;
}




int main(int argc, char * argv[])
{
    try
    {
        std::vector<std::string> const no_config;
        g_opt = new advgetopt::getopt(argc, argv, g_options, no_config, NULL);
        if(g_opt->is_defined("help"))
        {
            g_opt->usage(advgetopt::getopt::status_t::no_error, "Usage: %s [--<opts>] <expressions> ...\n", argv[0]);
            exit(1);
        }
        g_verbose = g_opt->is_defined("verbose");

        if(!g_opt->is_defined("no-cassandra"))
        {
            connect_cassandra();
        }

        // XXX -- the expression may actually make use of signals that
        //        different plugins may want to answer; this tool does
        //        not load the plugins (yet); should we not? for instance
        //        the secure fields are returned because the code does
        //        not know whether the cell is considered secure
        //
        int const max_expressions(g_opt->size("expression"));
        for(int i(0); i < max_expressions; ++i)
        {
            expr(g_opt->get_string("expression", i));
        }

        return g_errcnt == 0 ? 0 : 1;
    }
    catch(std::exception const & e)
    {
        std::cerr << "snapexpr: exception: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et
