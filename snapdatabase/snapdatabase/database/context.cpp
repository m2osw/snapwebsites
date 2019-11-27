// Copyright (c) 2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snapdatabase
// contact@m2osw.com
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
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


/** \file
 * \brief Database file implementation.
 *
 * Each table uses one or more files. Each file is handled by a dbfile
 * object and a corresponding set of blocks.
 */

#define NOQT

// self
//
#include    "snapdatabase/database/context.h"


// snapwebsites lib
//
#include    <snapwebsites/mkdir_p.h>


// snaplogger lib
//
#include    <snaplogger/message.h>


// snapdev lib
//
#include    <snapdev/glob_to_list.h>


// C++ lib
//
#include    <deque>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace detail
{



//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Weffc++"
class context_impl
{
public:
                                    context_impl(context *c, advgetopt::getopt::pointer_t opts);
                                    context_impl(context_impl const & rhs) = delete;
                                    ~context_impl();

    context_impl &                  operator = (context_impl const & rhs) = delete;

    void                            initialize();
    table::pointer_t                get_table(std::string const & name) const;
    table::map_t                    list_tables() const;
    std::string                     get_path() const;

private:
    context *                       f_context = nullptr;
    advgetopt::getopt::pointer_t    f_opts = advgetopt::getopt::pointer_t();
    std::string                     f_path = std::string();
    int                             f_lock = -1;        // TODO: lock the context so only one snapdatabasedaemon can run against it
    table::map_t                    f_tables = table::map_t();
};
//#pragma GCC diagnostic pop


context_impl::context_impl(context * c, advgetopt::getopt::pointer_t opts)
    : f_context(c)
    , f_opts(opts)
{
}


context_impl::~context_impl()
{
}


void context_impl::initialize()
{
std::cerr << "init context?\n";
    f_path = f_opts->get_string("context");

    SNAP_LOG_NOTICE
        << "Initialize context \""
        << f_path
        << "\"."
        << SNAP_LOG_SEND;

    if(f_path.empty())
    {
        f_path = "/var/lib/snapwebsites/database";
    }
    if(snap::mkdir_p(f_path, false, 0700, "snapwebsites", "snapwebsites") != 0)
    {
        throw io_error(
              "Could not create or access the context directory \""
            + f_path
            + "\".");
    }

    xml_node::deque_t table_extensions;

    size_t const max(f_opts->size("table_schema_path"));

    SNAP_LOG_NOTICE
        << "Reading context "
        << max
        << " XML schemata."
        << SNAP_LOG_SEND;

std::cerr << "schema dirs = " << max << "?\n";
    for(size_t idx(0); idx < max; ++idx)
    {
        std::string const path(f_opts->get_string("table_schema_path", idx));

        snap::glob_to_list<std::deque<std::string>> list;
        if(!list.read_path<
                  snap::glob_to_list_flag_t::GLOB_FLAG_ONLY_DIRECTORIES
                , snap::glob_to_list_flag_t::GLOB_FLAG_TILDE>(path + "/*.xml"))
        {
            SNAP_LOG_WARNING
                << "Could not read directory \""
                << path
                << "\" for XML table declarations."
                << SNAP_LOG_SEND;
            continue;
        }

        if(list.empty())
        {
            SNAP_LOG_DEBUG
                << "Directory \""
                << path
                << "\" is empty."
                << SNAP_LOG_SEND;
            continue;
        }

        // TODO: this is perfect for workers to distribute the load on many
        //       threads (and then the creation/loading of each table)
        //
        for(auto filename : list)
        {
            xml x(filename);
std::cerr << "parsed [" << filename << "]\n";

            xml_node::pointer_t root(x.root());
            if(root == nullptr)
            {
                SNAP_LOG_WARNING
                    << "Problem reading table schema \""
                    << filename
                    << "\" is not acceptable."
                    << SNAP_LOG_SEND;
                continue;
            }

            if(root->tag_name() != "keyspaces"
            && root->tag_name() != "context")
            {
                SNAP_LOG_WARNING
                    << "Table \""
                    << filename
                    << "\" schema must be a \"keyspaces\" or \"context\". \""
                    << root->tag_name()
                    << "\" is not acceptable."
                    << SNAP_LOG_SEND;
                continue;
            }

std::cerr << "complex types...\n";
            xml_node::map_t complex_types;
            for(auto child(root->first_child()); child != nullptr; child = child->next())
            {
                if(child->tag_name() == "complex-type")
                {
                    std::string const name(child->attribute("name"));
                    if(name_to_struct_type(name) != INVALID_STRUCT_TYPE)
                    {
                        SNAP_LOG_WARNING
                            << "The name of a complex type cannot be the name of a system type. \""
                            << name
                            << "\" is not acceptable."
                            << SNAP_LOG_SEND;
                        continue;
                    }
                    if(complex_types.find(name) != complex_types.end())
                    {
                        SNAP_LOG_WARNING
                            << "The complex type named \""
                            << name
                            << "\" is defined twice. Only the very first intance is used."
                            << SNAP_LOG_SEND;
                        continue;
                    }
                    complex_types[name] = child;
                }
            }

std::cerr << "tables...\n";
            for(auto child(root->first_child()); child != nullptr; child = child->next())
            {
                if(child->tag_name() == "table")
                {
std::cerr << "create table...\n";
                    table::pointer_t t(std::make_shared<table>(f_context, child, complex_types));
std::cerr << "got table [" << t->name() << "]...\n";
                    f_tables[t->name()] = t;

std::cerr << "attach dbfile to table [" << t->name() << "]...\n";
                    dbfile::pointer_t dbfile(t->get_dbfile());
                    dbfile->set_table(t);
                    dbfile->set_sparse(t->is_sparse());
                }
                else if(child->tag_name() == "table-extension")
                {
                    // we collect those and process them in a second
                    // pass after we loaded all the XML files because
                    // otherwise the table may still be missing
                    //
                    table_extensions.push_back(child);
                }
                else if(child->tag_name() == "complex-type")
                {
                    // already worked on; ignore in this loop
                }
                else
                {
                    // generate an error for unknown tags or ignore?
                    //
                    SNAP_LOG_WARNING
                        << "Unknown tag \""
                        << child->tag_name()
                        << "\" within a <context> tag ignored."
                        << SNAP_LOG_SEND;
                }
            }
        }
    }

std::cerr << "table extensions...\n";
    SNAP_LOG_NOTICE
        << "Adding "
        << table_extensions.size()
        << " XML schema extensions."
        << SNAP_LOG_SEND;

    for(auto e : table_extensions)
    {
        std::string const name(e->attribute("name"));
        table::pointer_t t(get_table(name));
        if(t == nullptr)
        {
            SNAP_LOG_WARNING
                << "Unknown table \""
                << e->tag_name()
                << "\" within a <table-extension>, tag ignored."
                << SNAP_LOG_SEND;
            continue;
        }
        t->load_extension(e);
    }

    SNAP_LOG_NOTICE
        << "Verify "
        << f_tables.size()
        << " table schemata."
        << SNAP_LOG_SEND;

std::cerr << "verify schemata...\n";
    for(auto t : f_tables)
    {
        t.second->verify_schema();
    }

std::cerr << "context ready...\n";

    SNAP_LOG_INFORMATION
        << "Context \""
        << f_path
        << "\" ready."
        << SNAP_LOG_SEND;
}


table::pointer_t context_impl::get_table(std::string const & name) const
{
    auto it(f_tables.find(name));
    if(it == f_tables.end())
    {
        return table::pointer_t();
    }

    return it->second;
}


table::map_t context_impl::list_tables() const
{
    return f_tables;
}


std::string context_impl::get_path() const
{
    return f_path;
}



} // namespace detail



context::context(advgetopt::getopt::pointer_t opts)
    : f_impl(std::make_unique<detail::context_impl>(this, opts))
{
}


// required for the std::unique_ptr<>() of the impl
context::~context()
{
}


context::pointer_t context::get_context(advgetopt::getopt::pointer_t opts)
{
std::cerr << "context: create\n";
    pointer_t c(new context(opts));
std::cerr << "context: initialize\n";
    c->initialize();
std::cerr << "context: ready to return\n";
    return c;
}


void context::initialize()
{
    f_impl->initialize();
}


table::pointer_t context::get_table(std::string const & name) const
{
    return f_impl->get_table(name);
}


table::map_t context::list_tables() const
{
    return f_impl->list_tables();
}


std::string context::get_path() const
{
    return f_impl->get_path();
}


/** \brief Signal that a new allocation was made.
 *
 * This function is just a signal sent to the memory manager so it knows
 * it should check and see whether too much memory is currently used and
 * attempt to release some memory.
 *
 * \note
 * The memory manager runs in a separate thread.
 */
void context::limit_allocated_memory()
{
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
