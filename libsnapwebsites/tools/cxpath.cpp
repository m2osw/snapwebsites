/*
 * Text:
 *      libsnapwebsites/tools/cxpath.cpp
 *
 * Description:
 *      Compile an XPath to binary byte code.
 *
 * License:
 *      Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
 * 
 *      https://snapwebsites.org/
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


// snapwebsites lib
//
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/qdomxpath.h>


// snapdev lib
//
#include <snapdev/not_reached.h>


// advgetopt lib
//
#include <advgetopt/advgetopt.h>
#include <advgetopt/exception.h>


// C++ lib
//
#include <iostream>


// Qt lib
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <QFile>
#pragma GCC diagnostic pop


// last include
//
#include <snapdev/poison.h>



namespace
{


advgetopt::option const g_command_line_options[] =
{
    // COMMANDS
    advgetopt::define_option(
          advgetopt::Name("compile")
        , advgetopt::ShortName('c')
        , advgetopt::Flags(advgetopt::standalone_command_flags<advgetopt::GETOPT_FLAG_GROUP_COMMANDS>())
        , advgetopt::Help("compile the specified XPath and save it to a .xpath file and optionally print out the compiled code.")
    ),
    advgetopt::define_option(
          advgetopt::Name("disassemble")
        , advgetopt::ShortName('d')
        , advgetopt::Flags(advgetopt::standalone_command_flags<advgetopt::GETOPT_FLAG_GROUP_COMMANDS>())
        , advgetopt::Help("disassemble the specified .xpath file (if used with the -c, disassemble as we compile.)")
    ),
    advgetopt::define_option(
          advgetopt::Name("execute")
        , advgetopt::ShortName('x')
        , advgetopt::Flags(advgetopt::standalone_command_flags<advgetopt::GETOPT_FLAG_GROUP_COMMANDS>())
        , advgetopt::Help("execute an xpath (.xpath file or parsed on the fly XPath) against one or more .xml files.")
    ),
    // OPTIONS
    advgetopt::define_option(
          advgetopt::Name("namespace")
        , advgetopt::ShortName('n')
        , advgetopt::Flags(advgetopt::standalone_command_flags<advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("if specified, the namespaces are taken in account, otherwise the DOM ignores them.")
    ),
    advgetopt::define_option(
          advgetopt::Name("output")
        , advgetopt::ShortName('o')
        , advgetopt::Flags(advgetopt::command_flags<advgetopt::GETOPT_FLAG_GROUP_OPTIONS
                                                  , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("name of the output file (the .xpath filename.)")
    ),
    advgetopt::define_option(
          advgetopt::Name("xpath")
        , advgetopt::ShortName('p')
        , advgetopt::Flags(advgetopt::command_flags<advgetopt::GETOPT_FLAG_GROUP_OPTIONS
                                                  , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("an XPath to work on.")
    ),
    advgetopt::define_option(
          advgetopt::Name("results")
        , advgetopt::ShortName('r')
        , advgetopt::Flags(advgetopt::standalone_command_flags<advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("display the results of executing the XPath.")
    ),
    advgetopt::define_option(
          advgetopt::Name("verbose")
        , advgetopt::ShortName('v')
        , advgetopt::Flags(advgetopt::standalone_command_flags<advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("make the process verbose.")
    ),
    advgetopt::define_option(
          advgetopt::Name("filename")
        , advgetopt::Flags(advgetopt::command_flags<advgetopt::GETOPT_FLAG_GROUP_NONE
                                                  , advgetopt::GETOPT_FLAG_MULTIPLE
                                                  , advgetopt::GETOPT_FLAG_DEFAULT_OPTION>())
    ),
    advgetopt::end_options()
};



advgetopt::group_description const g_group_descriptions[] =
{
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_COMMANDS)
        , advgetopt::GroupName("command")
        , advgetopt::GroupDescription("Commands:")
    ),
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_OPTIONS)
        , advgetopt::GroupName("option")
        , advgetopt::GroupDescription("Options:")
    ),
    advgetopt::end_groups()
};




// until we have C++20, remove warnings this way
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "snapwebsites",
    .f_options = g_command_line_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p  [--<opt>] [-p '<xpath>'] | [-x <filename>.xpath <filename>.xml ...]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = SNAPWEBSITES_VERSION_STRING,
    .f_license = "GNU GPL v2",
    .f_copyright = "Copyright (c) 2013-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = g_group_descriptions
};
#pragma GCC diagnostic pop



advgetopt::getopt::pointer_t    g_opt = advgetopt::getopt::pointer_t();
bool                            g_verbose = false;
bool                            g_results = false;



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
        g_opt = std::make_shared<advgetopt::getopt>(g_options_environment, argc, argv);

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

        g_opt->reset();

        return 0;
    }
    catch( advgetopt::getopt_exit const & except )
    {
        return except.code();
    }
    catch(std::exception const& e)
    {
        std::cerr << "cxpath: exception: " << e.what() << std::endl;
        return 1;
    }
}


// vim: ts=4 sw=4 et
