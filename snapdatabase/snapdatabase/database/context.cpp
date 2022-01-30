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


// C lib
//
#include    <sys/types.h>
#include    <sys/stat.h>
#include    <unistd.h>
#include    <fcntl.h>


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

    context_impl &                      operator = (context_impl const & rhs) = delete;

    void                                initialize();
    table::pointer_t                    get_table(std::string const & name) const;
    table::map_t                        list_tables() const;
    std::string                         get_path() const;
    size_t                              get_config_size(std::string const & name) const;
    std::string                         get_config_string(std::string const & name, int idx) const;
    long                                get_config_long(std::string const & name, int idx) const;

private:
    context *                           f_context = nullptr;
    advgetopt::getopt::pointer_t        f_opts = advgetopt::getopt::pointer_t();
    std::string                         f_path = std::string();
    int                                 f_lock = -1;        // TODO: lock the context so only one snapdatabasedaemon can run against it
    table::map_t                        f_tables = table::map_t();
    schema_complex_type::map_pointer_t  f_complex_types = schema_complex_type::map_pointer_t();
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

    // TODO: this is perfect for workers to distribute the load on many
    //       threads (and then the creation/loading of each table)
    //

    xml::map_t xml_files;

    // the first loop goes through all the files
    // it reads the XML and parses the complex types
    //
    for(size_t idx(0); idx < max; ++idx)
    {
        std::string const path(f_opts->get_string("table_schema_path", idx));

        // WARNING: we use an std::set<> for the list of filenames so that
        //          way they get sorted in a way which will not change
        //          between runs; we ignore some definitions, such as a
        //          second definition of a column, and by making sure we
        //          always load things in the same order, we limit the number
        //          of potential problems
        //
        //          note that if you add/remove columns with the same name
        //          then the order will change and the existing tables may
        //          not be 100% compatible with the new data (the system
        //          will automatically convert the data, but you may have
        //          surprises...)
        //
        snapdev::glob_to_list<std::set<std::string>> list;
        if(!list.read_path<
                  snapdev::glob_to_list_flag_t::GLOB_FLAG_ONLY_DIRECTORIES
                , snapdev::glob_to_list_flag_t::GLOB_FLAG_TILDE>(path + "/*.xml"))
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

        for(auto const & filename : list)
        {
            xml::pointer_t x(std::make_shared<xml>(filename));
            xml_files[filename] = x;

            xml_node::pointer_t root(x->root());
            if(root == nullptr)
            {
                SNAP_LOG_WARNING
                    << filename
                    << ": Problem reading table schema. The file will be ignored."
                    << SNAP_LOG_SEND;
                continue;
            }

            if(root->tag_name() != "keyspaces"
            && root->tag_name() != "context")
            {
                SNAP_LOG_WARNING
                    << filename
                    << ": XML table declarations must be a \"keyspaces\" or \"context\". \""
                    << root->tag_name()
                    << "\" is not acceptable."
                    << SNAP_LOG_SEND;
                continue;
            }

            // complex types are defined outside of tables which allows us
            // to use the same complex types in different tables
            //
            for(auto child(root->first_child()); child != nullptr; child = child->next())
            {
                if(child->tag_name() == "complex-type")
                {
                    std::string const name(child->attribute("name"));
                    if(name_to_struct_type(name) != INVALID_STRUCT_TYPE)
                    {
                        SNAP_LOG_WARNING
                            << filename
                            << ": The name of a complex type cannot be the name of a system type. \""
                            << name
                            << "\" is not acceptable."
                            << SNAP_LOG_SEND;
                        continue;
                    }
                    if(f_complex_types->find(name) != f_complex_types->end())
                    {
                        SNAP_LOG_WARNING
                            << filename
                            << ": The complex type named \""
                            << name
                            << "\" is defined twice. Only the very first instance is used."
                            << SNAP_LOG_SEND;
                        continue;
                    }
                    (*f_complex_types)[name] = std::make_shared<schema_complex_type>(child);
                }
            }
        }
    }

    for(auto const & x : xml_files)
    {
        for(auto child(x.second->root()->first_child()); child != nullptr; child = child->next())
        {
            if(child->tag_name() == "table")
            {
                table::pointer_t t(std::make_shared<table>(f_context, child, f_complex_types));
                f_tables[t->name()] = t;

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

    SNAP_LOG_NOTICE
        << "Adding "
        << table_extensions.size()
        << " XML schema extensions."
        << SNAP_LOG_SEND;

    for(auto const & e : table_extensions)
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
        << " table schema"
        << (f_tables.size() == 1 ? "" : "ta")
        << "."
        << SNAP_LOG_SEND;

    for(auto & t : f_tables)
    {
        t.second->get_schema();
    }

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


size_t context_impl::get_config_size(std::string const & name) const
{
    return f_opts->size(name);
}


/** \brief Retrieve a .conf file parameter.
 *
 * This function is used to access a parameter in the configuration file.
 * For example, we retrieve the murmur3 seed from that file. This way each
 * installation can make use of a different value.
 *
 * \param[in] name  The name of the parameter to retrieve.
 * \param[in] idx  The index to the data to retrieve.
 *
 * \return The parameter's value as a string.
 */
std::string context_impl::get_config_string(std::string const & name, int idx) const
{
    return f_opts->get_string(name, idx);
}


long context_impl::get_config_long(std::string const & name, int idx) const
{
    return f_opts->get_long(name, idx);
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


context::pointer_t context::create_context(advgetopt::getopt::pointer_t opts)
{
    pointer_t c(new context(opts));
    c->initialize();
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


size_t context::get_config_size(std::string const & name) const
{
    return f_impl->get_config_size(name);
}


std::string context::get_config_string(std::string const & name, int idx) const
{
    return f_impl->get_config_string(name, idx);
}


long context::get_config_long(std::string const & name, int idx) const
{
    return f_impl->get_config_long(name, idx);
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
