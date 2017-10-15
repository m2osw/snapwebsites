/*
 * Text:
 *      cxpath.cpp
 *
 * Description:
 *      Compile an XPath to binary byte code.
 *
 * License:
 *      Copyright (c) 2013-2017 Made to Order Software Corp.
 * 
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/qdomxpath.h>

#include <advgetopt.h>

#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <QFile>
#pragma GCC diagnostic pop

#include <snapwebsites/poison.h>

namespace
{

const std::vector<std::string> g_configuration_files; // Empty

advgetopt::getopt::option const g_cxpath_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        nullptr,
        nullptr,
        "Usage: %p --<command> [--<opt>] ['<xpath>'] [<filename>.xml] ...",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    // COMMANDS
    {
        '\0',
        0,
        nullptr,
        nullptr,
        "commands:",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        'c',
        0,
        "compile",
        nullptr,
        "compile the specified XPath and save it to a .xpath file and optionally print out the compiled code",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'd',
        0,
        "disassemble",
        nullptr,
        "disassemble the specified .xpath file (if used with the -c, disassemble as we compile)",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'h',
        0,
        "help",
        nullptr,
        "display this help screen",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'x',
        0,
        "execute",
        nullptr,
        "execute an xpath (.xpath file or parsed on the fly XPath) against one or more .xml files",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    // OPTIONS
    {
        '\0',
        0,
        nullptr,
        nullptr,
        "options:",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        'n',
        0,
        "namespace",
        nullptr,
        "if specified, the namespaces are taken in account, otherwise the DOM ignores them",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'o',
        0,
        "output",
        nullptr,
        "name of the output file (the .xpath filename)",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        'p',
        0,
        "xpath",
        nullptr,
        "an XPath",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        'r',
        0,
        "results",
        nullptr,
        "display the results of executing the XPath",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'v',
        0,
        "verbose",
        nullptr,
        "make the process verbose",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        0,
        "version",
        nullptr,
        "print out the version",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        0,
        "filename",
        nullptr,
        nullptr, // hidden argument in --help screen
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



advgetopt::getopt * g_opt;
bool                g_verbose;
bool                g_results;



} // no name namespace


void display_node(int j, QDomNode node)
{
    // unfortunate, but QtDOM does not offer a toString() at the
    // QDomNode level, instead they implemented it at the document
    // level... to make use of it you have to create a new document
    // and import the node in there to generate the output
    if(node.isDocument())
    {
        // documents cannot be imported properly
        //node = node.toDocument().documentElement();
        std::cout << "Result[" << j << "] is the entire document." << std::endl;
        return;
    }
    QDomDocument document;
    QDomNode copy(document.importNode(node, true));
    document.appendChild(copy);
    std::cout << "Node[" << j << "] = \"" << document.toByteArray().data() << "\"" << std::endl;
}



void cxpath_compile()
{
    if(!g_opt->is_defined("xpath"))
    {
        std::cerr << "error: --xpath not defined, nothing to compile." << std::endl;
        exit(1);
    }

    std::string xpath(g_opt->get_string("xpath"));
    if(g_verbose)
    {
        std::cout << "compiling \"" << xpath.c_str() << "\" ... " << std::endl;
    }

    bool const disassemble(g_opt->is_defined("disassemble"));

    QDomXPath dom_xpath;
    dom_xpath.setXPath(QString::fromUtf8(xpath.c_str()), disassemble);

    if(g_opt->is_defined("output"))
    {
        QDomXPath::program_t program(dom_xpath.getProgram());
        QDomXPath::instruction_t const * inst(program.data());
        std::string filename(g_opt->get_string("output"));
        FILE * f(fopen(filename.c_str(), "w"));
        if(f == nullptr)
        {
            std::cerr << "error: cannot open output file \"" << filename.c_str() << "\" for writing." << std::endl;
            exit(1);
        }
        if(fwrite(inst, program.size(), 1, f) != 1)
        {
            std::cerr << "error: I/O error while writing to output file \"" << filename.c_str() << "\"." << std::endl;
            exit(1);
        }
        fclose(f);

        if(g_verbose)
        {
            std::cout << "saved compiled XPath in \"" << filename.c_str() << "\" ... " << std::endl;
        }
    }
}



void cxpath_execute()
{
    std::string program_filename(g_opt->get_string("execute"));
    FILE * f(fopen(program_filename.c_str(), "r"));
    if(f == nullptr)
    {
        std::cerr << "error: could not open program file \"" << program_filename.c_str() << "\" for reading." << std::endl;
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long const program_size(ftell(f));
    if(program_size < 0)
    {
        std::cerr << "error: could not tell program size for \"" << program_filename.c_str() << "\"." << std::endl;
        exit(1);
    }
    fseek(f, 0, SEEK_SET);
    QDomXPath::program_t program;
    program.resize(program_size);
    if(fread(&program[0], program_size, 1, f) != 1)
    {
        std::cerr << "error: an I/O error occured while reading the program file \"" << program_filename.c_str() << "\"." << std::endl;
        exit(1);
    }
    fclose(f);

    bool const keep_namespace(g_opt->is_defined("namespace"));
    bool const disassemble(g_opt->is_defined("disassemble"));

    QDomXPath dom_xpath;
    dom_xpath.setProgram(program, disassemble);

    if(g_verbose)
    {
        std::cout << "Original XPath: " << dom_xpath.getXPath().toUtf8().data() << std::endl;
    }

    int const size(g_opt->size("filename"));
    for(int i(0); i < size; ++i)
    {
        if(g_verbose)
        {
            std::cout << "Processing \"" << g_opt->get_string("filename", i) << "\" ... ";
        }
        QFile file(QString::fromUtf8(g_opt->get_string("filename", i).c_str()));
        if(!file.open(QIODevice::ReadOnly))
        {
            std::cerr << std::endl
                      << "error: could not open XML file \"" << g_opt->get_string("filename", i) << "\"." << std::endl;
            return;
        }
        QDomDocument document;
        if(!document.setContent(&file, keep_namespace))
        {
            std::cerr << std::endl
                      << "error: could not read XML file \"" << g_opt->get_string("filename", i) << "\"." << std::endl;
            return;
        }
        QDomXPath::node_vector_t result(dom_xpath.apply(document));

        if(g_results)
        {
            int const max_nodes(result.size());
            std::cout << "this XPath returned " << max_nodes << " nodes" << std::endl;
            for(int j(0); j < max_nodes; ++j)
            {
                display_node(j, result[j]);
            }
        }
    }
}


void cxpath_disassemble()
{
    std::string program_filename(g_opt->get_string("filename"));
    FILE * f(fopen(program_filename.c_str(), "r"));
    if(f == nullptr)
    {
        std::cerr << "error: could not open program file \"" << program_filename.c_str() << "\" for reading." << std::endl;
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    int const program_size(ftell(f));
    if(program_size < 0)
    {
        std::cerr << "error: ftell() failed to return file size of \"" << program_filename.c_str() << "\"." << std::endl;
        exit(1);
    }
    fseek(f, 0, SEEK_SET);
    QDomXPath::program_t program;
    program.resize(program_size);
    if(fread(&program[0], program_size, 1, f) != 1)
    {
        std::cerr << "error: an I/O error occured while reading the program file \"" << program_filename.c_str() << "\"." << std::endl;
        exit(1);
    }
    fclose(f);

    QDomXPath dom_xpath;
    dom_xpath.setProgram(program, true);

    std::cout << "Original XPath: " << dom_xpath.getXPath().toUtf8().data() << std::endl;

    dom_xpath.disassemble();
}


int main(int argc, char *argv[])
{
    try
    {
        g_opt = new advgetopt::getopt(argc, argv, g_cxpath_options, g_configuration_files, nullptr);
        if(g_opt->is_defined("version"))
        {
            std::cerr << SNAPWEBSITES_VERSION_STRING << std::endl;
            exit(1);
        }
        if(g_opt->is_defined("help"))
        {
            g_opt->usage(advgetopt::getopt::status_t::no_error, "Usage: cxpath [--<opt>] [-p '<xpath>'] | [-x <filename>.xpath <filename>.xml ...]");
            snap::NOTREACHED();
        }
        g_verbose = g_opt->is_defined("verbose");
        g_results = g_opt->is_defined("results");

        if(g_opt->is_defined("compile"))
        {
            cxpath_compile();
        }
        else if(g_opt->is_defined("execute"))
        {
            cxpath_execute();
        }
        else if(g_opt->is_defined("disassemble"))
        {
            cxpath_disassemble();
        }

        return 0;
    }
    catch(std::exception const& e)
    {
        std::cerr << "cxpath: exception: " << e.what() << std::endl;
        return 1;
    }
}


// vim: ts=4 sw=4 et
