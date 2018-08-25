// Snap Websites Server -- command line tool to raise a watchdog flag
// Copyright (c) 2018  Made to Order Software Corp.  All Rights Reserved
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

// snapwatchdog lib
//
#include "snapwatchdog/flags.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/snap_exception.h>
#include <snapwebsites/snap_child.h>
#include <snapwebsites/tokenize_string.h>
#include <snapwebsites/version.h>

// advgetopt lib
//
#include <advgetopt/advgetopt.h>

// boost lib
//
#include <boost/algorithm/string.hpp>

// Qt lib
//
#include <QDomDocument>
#include <QDomElement>
#include <QDomText>

// C++ lib
//
#include <iomanip>
#include <regex>


// last entry
//
#include <snapwebsites/poison.h>

namespace
{
    const std::vector<std::string> g_configuration_files;

    const advgetopt::getopt::option g_command_line_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "Usage: %p [-<opt>]",
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
            'd',
            0,
            "down",
            nullptr,
            "remove flag (Down)",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            "function",
            nullptr,
            "name of the function in your script calling %p",
            advgetopt::getopt::argument_mode_t::required_argument
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
            0,
            "line",
            nullptr,
            "line of your script calling %p",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            'l',
            0,
            "list",
            nullptr,
            "list raised flags",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'm',
            0,
            "manual",
            nullptr,
            "mark the flag as a manual flag, it has to manually be turned off by the administrator",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'p',
            0,
            "priority",
            nullptr,
            "a number from 0 to 100, 50+ forces an email to be sent to the administrator",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            0,
            "source-file",
            nullptr,
            "name of your script",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            't',
            0,
            "tags",
            nullptr,
            "list of tags",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            'u',
            0,
            "up",
            nullptr,
            "raise flag (Up), this is the default",
            advgetopt::getopt::argument_mode_t::no_argument
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
            0,
            "xml",
            nullptr,
            "list raised flags in an XML format",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "<unit> <section> <flag> [<message>]",
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


void list_in_plain_text()
{
    snap::watchdog_flag::vector_t flags(snap::watchdog_flag::load_flags());

    std::map<std::string, std::string::size_type> widths;

    // note: the date width is static (Fri Aug 24, 2018  12:29:23)

    widths["unit"]        = strlen("unit");
    widths["section"]     = strlen("section");
    widths["name"]        = strlen("name");
    widths["source-file"] = strlen("source_file");
    widths["function"]    = strlen("function");
    widths["line"]        = strlen("line");
    widths["message"]     = strlen("message");
    widths["priority"]    = strlen("priority");
    widths["manual"]      = strlen("manual");
    widths["date"]        = std::max(strlen("date"), 31UL);
    widths["modified"]    = std::max(strlen("modified"), 31UL);
    widths["tags"]        = strlen("tags");

    // run a first time through the list so we can gather the width of each field
    //
    for(auto const & f : flags)
    {
        // TODO: std::string().length() is wrong for UTF8
        //
        widths["unit"]        = std::max(widths["unit"],        f->get_unit()                    .length());
        widths["section"]     = std::max(widths["section"],     f->get_section()                 .length());
        widths["name"]        = std::max(widths["name"],        f->get_name()                    .length());
        widths["source-file"] = std::max(widths["source-file"], f->get_source_file()             .length());
        widths["function"]    = std::max(widths["function"],    f->get_function()                .length());
        widths["line"]        = std::max(widths["line"],        std::to_string(f->get_line())    .length());
        widths["message"]     = std::max(widths["message"],     f->get_message()                 .length());
        widths["priority"]    = std::max(widths["priority"],    std::to_string(f->get_priority()).length());
        widths["manual"]      = std::max(widths["manual"],      f->get_manual_down() ? 3UL : 2UL); // "yes" : "no"
        //widths["date"]        = std::max(widths["date"],        ...);
        //widths["modified"]    = std::max(widths["modified"],    ...);

        snap::watchdog_flag::tag_list_t const & tags(f->get_tags());
        std::string const tags_string(boost::algorithm::join(tags, ", "));
        widths["tags"] = std::max(widths["tags"], tags_string.length());
    }

    // now we have the widths so we can write the output mapped properly
    //
    std::cout << std::left
              << std::setw(widths["unit"]        + 2) << "unit"
              << std::setw(widths["section"]     + 2) << "section"
              << std::setw(widths["name"]        + 2) << "name"
              << std::setw(widths["source-file"] + 2) << "source_file"
              << std::setw(widths["function"]    + 2) << "function"
              << std::setw(widths["line"]        + 2) << "line"
              << std::setw(widths["message"]     + 2) << "message"
              << std::setw(widths["priority"]    + 2) << "priority"
              << std::setw(widths["manual"]      + 2) << "manual"
              << std::setw(widths["date"]        + 2) << "date"
              << std::setw(widths["modified"]    + 2) << "modified"
              << std::setw(widths["tags"]           ) << "tags"
              << std::endl;

    std::cout << std::left
              << std::setw(widths["unit"]        + 2) << "----"
              << std::setw(widths["section"]     + 2) << "-------"
              << std::setw(widths["name"]        + 2) << "----"
              << std::setw(widths["source-file"] + 2) << "-----------"
              << std::setw(widths["function"]    + 2) << "--------"
              << std::setw(widths["line"]        + 2) << "----"
              << std::setw(widths["message"]     + 2) << "-------"
              << std::setw(widths["priority"]    + 2) << "--------"
              << std::setw(widths["manual"]      + 2) << "------"
              << std::setw(widths["date"]        + 2) << "----"
              << std::setw(widths["modified"]    + 2) << "--------"
              << std::setw(widths["tags"]           ) << "----"
              << std::endl;

    for(auto const & f : flags)
    {
        std::cout << std::left  << std::setw(widths["unit"]        + 2) << f->get_unit()
                  << std::left  << std::setw(widths["section"]     + 2) << f->get_section()
                  << std::left  << std::setw(widths["name"]        + 2) << f->get_name()
                  << std::left  << std::setw(widths["source-file"] + 2) << f->get_source_file()
                  << std::left  << std::setw(widths["function"]    + 2) << f->get_function()
                  << std::right << std::setw(widths["line"]           ) << f->get_line() << "  "
                  << std::left  << std::setw(widths["message"]     + 2) << f->get_message()
                  << std::right << std::setw(widths["priority"]       ) << f->get_priority() << "  "
                  << std::left  << std::setw(widths["manual"]      + 2) << (f->get_manual_down() ? "yes" : "no")
                  << std::left  << std::setw(widths["date"]        + 2) << snap::snap_child::date_to_string(f->get_date()     * 1000000, snap::snap_child::date_format_t::DATE_FORMAT_HTTP)
                  << std::left  << std::setw(widths["modified"]    + 2) << snap::snap_child::date_to_string(f->get_modified() * 1000000, snap::snap_child::date_format_t::DATE_FORMAT_HTTP)
                  << std::left  << std::setw(widths["tags"]           ) << boost::algorithm::join(f->get_tags(), ", ")
                  << std::endl;
    }

    std::cout << "----------------------" << std::endl
              << "Found " << flags.size() << " raised flag" << (flags.size() == 1 ? "" : "s") << std::endl;
}



void list_in_xml()
{
    snap::watchdog_flag::vector_t flags(snap::watchdog_flag::load_flags());

    QDomDocument doc("watchdog-flags");

    // create the root element
    //
    QDomElement root(doc.createElement("watchdog-flags"));
    doc.appendChild(root);

    //std::cout << "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    //             "<watchdog-flags>";

    QDomElement flag_element;

    auto add_element = [&](QString const & name, std::string const & value)
        {
            QDomElement e(doc.createElement(name));
            flag_element.appendChild(e);

            QDomText t(doc.createTextNode(QString::fromUtf8(value.c_str())));
            e.appendChild(t);
        };

    for(auto const & f : flags)
    {
        flag_element = doc.createElement("flag");
        root.appendChild(flag_element);

        add_element("unit",        f->get_unit());
        add_element("section",     f->get_section());
        add_element("name",        f->get_name());
        add_element("source-file", f->get_source_file());
        add_element("function",    f->get_function());
        add_element("line",        std::to_string(f->get_line()));
        add_element("message",     f->get_message());
        add_element("priority",    std::to_string(f->get_priority()));
        add_element("manual",      f->get_manual_down() ? "yes" : "no");

        snap::watchdog_flag::tag_list_t const & tags(f->get_tags());
        if(!tags.empty())
        {
            QDomElement tags_element(doc.createElement("tags"));
            flag_element.appendChild(tags_element);

            for(auto const & t : tags)
            {
                QDomElement tag_element(doc.createElement("tag"));
                tags_element.appendChild(tag_element);

                QDomText text(doc.createTextNode(QString::fromUtf8(t.c_str())));
                tag_element.appendChild(text);
            }
        }
    }

    std::cout << doc.toString(-1) << std::endl;
}



int main(int argc, char * argv[])
{
    int exitval(1);

    try
    {
        advgetopt::getopt opt(argc, argv, g_command_line_options, g_configuration_files, nullptr);

        if(opt.is_defined("version"))
        {
            std::cout << SNAPWEBSITES_VERSION_STRING << std::endl;
            exit(0);
            snap::NOTREACHED();
        }

        if(opt.is_defined("help"))
        {
            opt.usage(advgetopt::getopt::status_t::no_error, "raise_flag");
            exit(1);
            snap::NOTREACHED();
        }

        if(opt.is_defined("list"))
        {
            list_in_plain_text();
            return 0;
        }

        if(opt.is_defined("xml"))
        {
            list_in_xml();
            return 0;
        }

        bool const down(opt.is_defined("down"));
        bool const up(!down || opt.is_defined("up"));

        if(up && down)
        {
            std::cerr << "raise-flag:error: you can't specify --up and --down at the same time." << std::endl;
            return 1;
        }

        if(down)
        {
            if(opt.is_defined("manual"))
            {
                std::cerr << "raise-flag:error: you can't define --manual with --down." << std::endl;
                return 1;
            }
            if(opt.is_defined("priority"))
            {
                std::cerr << "raise-flag:error: you can't define --priority with --down." << std::endl;
                return 1;
            }
            if(opt.is_defined("tags"))
            {
                std::cerr << "raise-flag:error: you can't define --tags with --down." << std::endl;
                return 1;
            }
        }

        int const name_count(opt.size("--"));
        if(up)
        {
            if(name_count != 4)
            {
                std::cerr << "raise-flag:error: --up expected 4 parameters: unit section flag message." << std::endl;
                return 1;
            }
        }
        else
        {
            if(name_count != 3 && name_count != 4)
            {
                std::cerr << "raise-flag:error: --down expected 3 or 4 parameters: unit section flag [message]." << std::endl;
                return 1;
            }
        }

        std::string const unit(opt.get_string("--", 0));
        std::string const section(opt.get_string("--", 1));
        std::string const flag_name(opt.get_string("--", 2));
        std::string message;
        if(name_count == 4)
        {
            message = opt.get_string("--", 3);
        }

        snap::watchdog_flag::pointer_t flag;
        if(up)
        {
            flag = SNAPWATHCDOG_FLAG_UP(unit, section, flag_name, message);

            if(opt.is_defined("manual"))
            {
                flag->set_manual_down(true);
            }

            if(opt.is_defined("priority"))
            {
                flag->set_priority(opt.get_long("priority", 0, 0, 100));
            }

            if(opt.is_defined("tags"))
            {
                std::string const tags(opt.get_string("tags"));
                std::vector<std::string> list_of_tags;
                snap::tokenize_string(list_of_tags, tags, ",", true, " \r\n\t");
                for(auto const & t : list_of_tags)
                {
                    flag->add_tag(t);
                }
            }
        }
        else
        {
            flag = SNAPWATHCDOG_FLAG_DOWN(unit, section, flag_name);
            if(!message.empty())
            {
                flag->set_message(message);
            }
        }

        if(opt.is_defined("source-file"))
        {
            flag->set_source_file(opt.get_string("source-file"));
        }

        if(opt.is_defined("function"))
        {
            flag->set_function(opt.get_string("function"));
        }

        if(opt.is_defined("line"))
        {
            flag->set_line(opt.get_long("line", 0, 1));
        }

        if(!flag->save())
        {
            std::cerr << "raise-flag:error: an error occurred while saving flag to disk." << std::endl;
        }
    }
    catch( snap::snap_exception const & except )
    {
        SNAP_LOG_FATAL("raise-flag:fatal error: snap_exception caught: ")(except.what());
    }
    catch( std::exception const & std_except )
    {
        SNAP_LOG_FATAL("raise-flag:fatal error: std::exception caught: ")(std_except.what());
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("raise-flag:fatal error: unknown exception caught!");
    }

    // exit via the server so the server can clean itself up properly
    //
    exit( exitval );
    snap::NOTREACHED();
    return 0;
}

// vim: ts=4 sw=4 et
