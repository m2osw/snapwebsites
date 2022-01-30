// Copyright (c) 2011-2020  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// self
//
#include "version.h"


// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/snap_initialize_website.h>
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/snap_config.h>
#include <snapwebsites/snap_cassandra.h>


// snapdev lib
//
#include <snapdev/not_used.h>


// advgetopt lib
//
#include <advgetopt/exception.h>


// last include
//
#include <snapdev/poison.h>



namespace
{



const advgetopt::option g_options[] =
{
    {
        'c',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "config",
        nullptr,
        "Configuration file to initialize snapdbproxy.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "domain",
        nullptr,
        "The domain and sub-domain for which a site is to be created (i.e. install.snap.website)."
            " You may also include parameters after a '?'. At this time we understand the 'install-latyouts'."
            " For example, you could use --domain install.snap.website?install-layouts=beautiful."
            " Note that if the domain and/or website were not yet described in the database, default"
            " definitions get added automatically.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "port",
        nullptr,
        "The port used to access this website (usually 80 or 443.)",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "protocol",
        nullptr,
        "The protocol used to access this website (HTTP or HTTPS),"
            " defaults to HTTP if port is 80 and to HTTPS if port is 443,"
            " required for any other port.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_END,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    }
};





// until we have C++20 remove warnings this way
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "snapwebsites",
    .f_group_name = nullptr,
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "SNAPINSTALLWEBSITE_OPTIONS",
    .f_section_variables_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    //.f_version = SNAPDBPROXY_VERSION_STRING,
    .f_help_footer = "%c",
    .f_version = SNAPWEBSITES_VERSION_STRING,
    .f_license = "GNU GPL v2",
    .f_copyright = "Copyright (c) 2013-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    //.f_build_date = UTC_BUILD_DATE,
    //.f_build_time = UTC_BUILD_TIME
};
#pragma GCC diagnostic pop








void create_domain_and_website( QString const & orig_domain, QString const & protocol, int const port )
{
    QString const domains_table_name      ( snap::get_name( snap::name_t::SNAP_NAME_DOMAINS));
    QString const websites_table_name     ( snap::get_name( snap::name_t::SNAP_NAME_WEBSITES));
    QString const core_rules_name         ( snap::get_name( snap::name_t::SNAP_NAME_CORE_RULES));
    QString const core_original_rules_name( snap::get_name( snap::name_t::SNAP_NAME_CORE_ORIGINAL_RULES));

    snap::snap_cassandra sc;
    sc.connect();

    snap::snap_uri uri( QString("%1://%2").arg(protocol.toLower()).arg(orig_domain) );
    QString const domain     ( uri.domain() + uri.top_level_domain() );
    QString const full_domain( uri.full_domain() );

    auto domains_table( sc.get_table(domains_table_name) );
    if( !domains_table->exists(domain) )
    {
        SNAP_LOG_INFO("Domain does not exist in domains table. Creating");
        QString const rules(
            QString( "main {\n"
                     "  required host = \"%1\\.\";\n"
                     "};\n"
                   ).arg(uri.sub_domains())
            );
        snap::snap_uri_rules domain_rules;
        QByteArray compiled_rules;
        if(!domain_rules.parse_domain_rules(rules, compiled_rules))
        {
            throw snap::snap_exception( QString("An error was detected in your domain rules: %1").arg(domain_rules.errmsg()) );
        }

        auto domain_row( domains_table->getRow(domain) );
        domain_row->getCell(core_original_rules_name)->setValue(rules.toUtf8());
        domain_row->getCell(core_rules_name)->setValue(compiled_rules);
    }

    auto websites_table( sc.get_table(websites_table_name) );
    if( !websites_table->exists(full_domain) )
    {
        SNAP_LOG_INFO("Website does not exist in websites table. Creating");
        QString const rules(
            QString( "main {\n"
                     "  protocol = \"%1\";\n"
                     "  port = \"%2\";\n"
                     "};\n")
            .arg(protocol.toLower())
            .arg(port)
            );
        snap::snap_uri_rules website_rules;
        QByteArray compiled_rules;
        if(!website_rules.parse_website_rules(rules, compiled_rules))
        {
            throw snap::snap_exception( QString("An error was detected in your website rules: %1").arg(website_rules.errmsg()) );
        }

        auto website_row( websites_table->getRow(full_domain) );
        website_row->getCell(core_original_rules_name)->setValue(rules.toUtf8());
        website_row->getCell(core_rules_name)->setValue(compiled_rules);
    }
}



}
//namespace



int main(int argc, char * argv[])
{
    try
    {
        advgetopt::getopt opt(g_options_environment, argc, argv);

        snap::logging::set_progname(opt.get_program_name());
        snap::logging::configure_console();

        if(opt.is_defined("help"))
        {
            std::cout << opt.usage();
            exit(0);
            snapdev::NOT_REACHED();
        }

        if(opt.is_defined("version"))
        {
            std::cout << SNAPDBPROXY_VERSION_STRING << std::endl;
            exit(0);
            snapdev::NOT_REACHED();
        }

        if(!opt.is_defined("domain")
        || !opt.is_defined("port"))
        {
            std::cerr << "error: --domain and --port are both required." << std::endl;
            std::cerr << opt.usage(advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR);
            exit(1);
            snapdev::NOT_REACHED();
        }

        SNAP_LOG_INFO("Get snapserver info.");

        // read the snapserver IP:port information directly from the "snapserver"
        // configuration file
        //
        // TODO: we may want to use snapcgi.conf instead of snapserver.conf?
        //       or maybe support both/either in case the user starts this
        //       tool "on the wrong machine".
        //
        snap::snap_config config("snapserver");
        if(opt.is_defined("config"))
        {
            config.set_configuration_path(opt.get_string("config"));
        }
        config.set_parameter_default("listen", "127.0.0.1:4004");

        QString snap_host("127.0.0.1");
        int snap_port(4004);
        tcp_client_server::get_addr_port(config["listen"], snap_host, snap_port, "tcp");

        std::string const certificate(config["ssl_certificate"]);
        std::string const private_key(config["ssl_private_key"]);

        bool secure(!certificate.empty() || !private_key.empty());
        if(secure
        && snap_host == "127.0.0.1")
        {
            secure = false;
        }

        SNAP_LOG_INFO("snapserver is at ")(snap_host)(":")(snap_port)(secure ? " using SSL" : "")(".");

        // we need the URL:port to initialize the new website
        //
        QString url(QString::fromUtf8(opt.get_string("domain").c_str()));
        if(url.isEmpty())
        {
            std::cerr << "error: domain cannot be empty and must be specified." << std::endl;
            exit(1);
            snapdev::NOT_REACHED();
        }

        int const site_port(opt.get_long("port", 0, 0, 65535));

        QString protocol(site_port == 443 ? "HTTPS" : "HTTP");
        if(opt.is_defined("protocol"))
        {
            protocol = QString::fromUtf8(opt.get_string("protocol").c_str());
        }
        else if(site_port != 80 && site_port != 443)
        {
            std::cerr << "error: --protocol is required if the port is not 80 or 443." << std::endl;
            exit(1);
            snapdev::NOT_REACHED();
        }

        // extract the query string if there is one
        //
        QString query_string;
        int const pos(url.indexOf('?'));
        if(pos > 0)
        {
            query_string = url.mid(pos + 1);
            url = url.mid(0, pos);
        }

        SNAP_LOG_INFO("website is at \"")(protocol.toLower())("://")(url)(":")(site_port)("/\".");

        // Create domain/website if non-existent
        //
        create_domain_and_website( url, protocol, site_port );

        // create a snap_initialize_website object and listen for messages
        // up until is_done() returns true
        //
        snap::snap_initialize_website::pointer_t initialize_website(
                std::make_shared<snap::snap_initialize_website>(
                              snap_host
                            , snap_port
                            , secure
                            , url
                            , site_port
                            , query_string
                            , protocol));

        SNAP_LOG_INFO("start website initializer.");

        if(!initialize_website->start_process())
        {
            SNAP_LOG_INFO("start_process() failed. Existing immediately.");
            exit(1);
            snapdev::NOT_REACHED();
        }

        for(;;)
        {
            for(;;)
            {
                QString const status(initialize_website->get_status());
                if(status.isEmpty())
                {
                    break;
                }
                SNAP_LOG_INFO(status);
            }

            if(initialize_website->is_done())
            {
                break;
            }

            // unfortunately, I do not have a non-polling version for this
            // one yet...
            //
            sleep(1);
        }

        return 0;
    }
    catch( advgetopt::getopt_exit const & except )
    {
        return except.code();
    }
    catch(advgetopt::getopt_exception const & e)
    {
        std::cerr << "error: an advgetopt exception was raised while handling the command line. " << e.what() << std::endl;
    }
    catch(snap::snap_exception const & e)
    {
        std::cerr << "error: a snap exception was raised. " << e.what() << std::endl;
    }
    catch(tcp_client_server::tcp_client_server_logic_error const & e)
    {
        std::cerr << "error: a logic (programmer) error TCP client/server exception was raised. " << e.what() << std::endl;
    }
    catch(tcp_client_server::tcp_client_server_runtime_error const & e)
    {
        std::cerr << "error: a runtime error TCP client/server exception was raised. " << e.what() << std::endl;
    }
    catch(std::exception const & e)
    {
        std::cerr << "error: a standard exception was caught. " << e.what() << std::endl;
    }

    return 1;
}


// vim: ts=4 sw=4 et nowrap
