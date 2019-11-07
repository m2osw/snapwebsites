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

// self
//
#include    "snapdatabase/context.h"


// snapwebsites lib
//
#include    <snapwebsites/mkdir_p.h>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace detail
{



class context_impl
{
public:
                                    context_impl(advgetopt::getopt::pointer_t opts);

    table::pointer_t                get_table(std::string const & name) const;
    table::map_t                    list_tables() const;
    std::string                     get_path() const;

private:
    context::pointer_t              f_context = context::pointer_t();
    advgetopt::getopt::pointer_t    f_opts = advgetopt::getopt::pointer_t();
    std::string                     f_path = std::string();
    int                             f_lock = -1;        // TODO: lock the context so only one snapdatabasedaemon can run against it
    table::map_t                    f_tables = table::map_t();
};


context_impl::context_impl(context::pointer_t context, advgetopt::getopt::pointer_t opts)
    : f_context(context)
    , f_opts(opts)
{
    pointer_t me(shared_from_this());

    f_path = f_opts->string("context");
    if(f_path.empty())
    {
        f_path = "/var/lib/snapwebsites/database";
    }
    if(mkdir_p(f_path, false, 0700, "snapwebsites", "snapwebsites") != 0)
    {
        throw io_error(
              "Could not create or access the context directory \""
            + f_path
            + "\".");
    }

    xml_node::deque_t table_extensions;

    size_t const max(opts->size("table_schema_path"));
    for(size_t idx(0); idx < max; ++idx)
    {
        std::string const path(opts->get_string("table_schema_path", idx));

        snap::glob_to_list<std::deque> list;
        if(!list.read_path<
                  snap::glob_to_list_flag_t::GLOB_FLAG_ONLY_DIRECTORIES
                , snap::glob_to_list_flag_t::GLOB_FLAG_TILDE>(path + "/*.xml"))
        {
            SNAP_LOG_WARNING
                << "Could not read directory \""
                << path
                << "\" for XML table declarations."
                << SNAP_LOG_END;
            continue;
        }

        if(list.empty())
        {
            SNAP_LOG_DEBUG
                << "Directory \""
                << path
                << "\" is empty."
                << SNAP_LOG_END;
            continue;
        }

        // TODO: this is perfect for workers to distribute the load on many
        //       threads (and then the creation/loading of each table)
        //
        for(auto filename : list)
        {
            xml x(filename);

            xml_node::pointer_t root(x.root());
            if(root->tag_name() != "keyspaces"
            && root->tag_name() != "context")
            {
                SNAP_LOG_WARNING
                    << "A table schema must be a \"keyspaces\" or \"context\". \""
                    << root->tag_name()
                    << "\" is not acceptable."
                    << SNAP_LOG_END;
                continue;
            }

            xml_node::map_t complex_types;
            for(auto child(x->first_child()); child != nullptr; child = child->next())
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
                            << SNAP_LOG_END;
                        continue;
                    }
                    if(complex_types.find(name) != complex_types.end())
                    {
                        SNAP_LOG_WARNING
                            << "The complex type named \""
                            << name
                            << "\" is defined twice. Only the very first intance is used."
                            << SNAP_LOG_END;
                        continue;
                    }
                    complex_types[name].push_back(child);
                }
            }

            for(auto child(x->first_child()); child != nullptr; child = child->next())
            {
                if(child->tag_name() == "table")
                {
                    table::pointer_t t(std::make_shared<table>(me, child, complex_types));
                    f_tables[t->name()] = t;
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
                        << SNAP_LOG_END;
                }
            }
        }
    }

    for(auto e : table_extensions)
    {
        std::string const name(e->attribute("name"));
        table::pointer_t t(get_table(name));
        if(t == nullptr)
        {
            SNAP_LOG_WARNING
                << "Unknown table \""
                << child->tag_name()
                << "\" within a <table-extension>, tag ignored."
                << SNAP_LOG_END;
            continue;
        }
        t->load_extension(e);
    }
}


table::pointer_t context_impl::get_table(std::string const & name) const
{
    auto it(f_tables.find(name));
    if(it == f_tables.end())
    {
        return table::pointer_t();
    }

    return it.second;
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
    : f_impl(std::shared_from_this(), new context_impl(opt))
{
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



} // namespace snapdatabase
// vim: ts=4 sw=4 et
