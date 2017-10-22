// Snap Websites Server -- links
// Copyright (C) 2012-2017  Made to Order Software Corp.
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
#pragma once

// other plugins
//
#include "../test_plugin_suite/test_plugin_suite.h"

// snapwebsites lib
//
#include <snapwebsites/snapwebsites.h>


namespace snap
{
namespace links
{

enum class name_t
{
    SNAP_NAME_LINKS_CLEANUPLINKS,
    SNAP_NAME_LINKS_CREATELINK,
    SNAP_NAME_LINKS_DELETELINK,
    SNAP_NAME_LINKS_NAMESPACE,
    SNAP_NAME_LINKS_SNAP547_FIX_LINK_BRANCHES,
    SNAP_NAME_LINKS_TABLE          // Cassandra Table used for links
};
char const * get_name(name_t name) __attribute__ ((const));


class links_exception : public snap_exception
{
public:
    explicit links_exception(char const *        what_msg) : snap_exception("links", what_msg) {}
    explicit links_exception(std::string const & what_msg) : snap_exception("links", what_msg) {}
    explicit links_exception(QString const &     what_msg) : snap_exception("links", what_msg) {}
};

class links_exception_missing_links_table : public links_exception
{
public:
    explicit links_exception_missing_links_table(char const *        what_msg) : links_exception(what_msg) {}
    explicit links_exception_missing_links_table(std::string const & what_msg) : links_exception(what_msg) {}
    explicit links_exception_missing_links_table(QString const &     what_msg) : links_exception(what_msg) {}
};

class links_exception_missing_branch_table : public links_exception
{
public:
    explicit links_exception_missing_branch_table(char const *        what_msg) : links_exception(what_msg) {}
    explicit links_exception_missing_branch_table(std::string const & what_msg) : links_exception(what_msg) {}
    explicit links_exception_missing_branch_table(QString const &     what_msg) : links_exception(what_msg) {}
};

class links_exception_invalid_name : public links_exception
{
public:
    explicit links_exception_invalid_name(char const *        what_msg) : links_exception(what_msg) {}
    explicit links_exception_invalid_name(std::string const & what_msg) : links_exception(what_msg) {}
    explicit links_exception_invalid_name(QString const &     what_msg) : links_exception(what_msg) {}
};

class links_exception_invalid_db_data : public links_exception
{
public:
    explicit links_exception_invalid_db_data(char const *        what_msg) : links_exception(what_msg) {}
    explicit links_exception_invalid_db_data(std::string const & what_msg) : links_exception(what_msg) {}
    explicit links_exception_invalid_db_data(QString const &     what_msg) : links_exception(what_msg) {}
};

class links_exception_missing_link : public links_exception
{
public:
    explicit links_exception_missing_link(char const *        what_msg) : links_exception(what_msg) {}
    explicit links_exception_missing_link(std::string const & what_msg) : links_exception(what_msg) {}
    explicit links_exception_missing_link(QString const  &    what_msg) : links_exception(what_msg) {}
};



class link_info
{
public:
    typedef std::vector<link_info>      vector_t;

    link_info()
        //: f_unique(false)
        //, f_name("")
        //, f_key("")
        //, f_branch(snap_version::SPECIAL_VERSION_UNDEFINED)
    {
    }

    link_info(QString const & new_name, bool unique, QString const & new_key, snap_version::version_number_t branch_number)
        : f_unique(unique)
        , f_name(new_name)
        , f_key(new_key)
        , f_branch(branch_number)
    {
        // empty is valid on construction
        if(!new_name.isEmpty())
        {
            verify_name(new_name);
        }
    }

    void set_name(QString const & new_name, bool unique = false)
    {
        verify_name(new_name);
        f_unique = unique;
        f_name = new_name;
    }
    void set_destination_cell_name(QString const & new_name)
    {
        f_destination_cell_name = new_name;
    }
    void set_key(QString const & new_key)
    {
        f_key = new_key;
    }
    void set_branch(snap_version::version_number_t branch_number)
    {
        f_branch = branch_number;
    }

    bool is_unique() const
    {
        return f_unique;
    }
    QString const & name() const
    {
        return f_name;
    }
    QString cell_name(link_info const & dst) const;
    QString cell_name(link_info const & dst, QString const & unique_number) const;
    QString destination_cell_name() const
    {
        return f_destination_cell_name;
    }
    void cell_predicate(libdbproxy::cell_range_predicate::pointer_t column_predicate, int const count);
    QString const & key() const
    {
        return f_key;
    }
    QString key_with_branch() const;
    QString row_key() const;
    QString link_key() const;
    snap_version::version_number_t branch() const
    {
        return f_branch;
    }

    QString data() const;
    void from_data(QString const & db_data);

    void verify_name(QString const & vname);

    bool is_defined() const
    {
        return !f_name.isEmpty()
            && !f_key.isEmpty()
            && f_branch != snap_version::SPECIAL_VERSION_INVALID
            && f_branch != snap_version::SPECIAL_VERSION_UNDEFINED
            && f_branch != snap_version::SPECIAL_VERSION_EXTENDED;
    }

private:
    bool                            f_unique = false;
    QString                         f_name;
    QString                         f_key;
    snap_version::version_number_t  f_branch;
    QString                         f_destination_cell_name;
};





class link_info_pair
{
public:
    typedef std::vector<link_info_pair>      vector_t;

                                link_info_pair(link_info const & src, link_info const & dst);

    link_info const &           source() const;
    link_info const &           destination() const;

private:
    link_info                   f_source;
    link_info                   f_destination;
};





class link_context
{
public:
    enum class mode_t
    {
        LINK_CONTENT_MODE_CURRENT,
        LINK_CONTENT_MODE_CURRENT_OR_NEWEST,
        LINK_CONTENT_MODE_WORKING,
        LINK_CONTENT_MODE_WORKING_OR_NEWEST,
        LINK_CONTENT_MODE_NEWEST,
        LINK_CONTENT_MODE_OLDEST,
        LINK_CONTENT_MODE_ALL,

        LINK_CONTENT_MODE_DEFAULT = LINK_CONTENT_MODE_CURRENT_OR_NEWEST
    };

    bool next_link(link_info & info);

private:
    friend class links;

    link_context(::snap::snap_child * snap, link_info const & info, mode_t const mode, const int count);

    snap_child *                                    		f_snap = nullptr;
    link_info                                       		f_info;
    libdbproxy::row::pointer_t           		f_row;
    libdbproxy::cell_range_predicate::pointer_t    f_column_predicate;
    libdbproxy::cells                    		f_cells;
    libdbproxy::cells::const_iterator    		f_cell_iterator;
    link_info                                       		f_link;
};





class links_cloned
{
public:
    virtual void        repair_link_of_cloned_page(QString const & clone, snap_version::version_number_t branch_number, link_info const & source, link_info const & destination, bool const cloning) = 0;
};





class links : public plugins::plugin
            , public server::backend_action
{
public:
    static int const                READ_RECORD_COUNT = 1000;
    static int const                DELETE_RECORD_COUNT = 1000;

                                    links();
                                    ~links();

    // plugins::plugin implementation
    static links *                  instance();
    virtual QString                 icon() const;
    virtual QString                 description() const;
    virtual QString                 dependencies() const;
    virtual int64_t                 do_update(int64_t last_updated);
    virtual void                    bootstrap(snap_child * snap);

    libdbproxy::table::pointer_t get_links_table();

    // server signals
    void                            on_add_snap_expr_functions(snap_expr::functions_t & functions);
    void                            on_register_backend_action(server::backend_action_set & actions);

    // server::backend_action implementation
    virtual void                    on_backend_action(QString const & action);

    SNAP_SIGNAL(modified_link, (link_info const & link, bool const created), (link, created));

    // TBD should those be events? (they do trigger the modified_link() event already...)
    void                            create_link(link_info const & src, link_info const & dst);
    void                            delete_link(link_info const & info, int const delete_record_count = DELETE_RECORD_COUNT);
    void                            delete_this_link(link_info const & source, link_info const & destination);

    QSharedPointer<link_context>    new_link_context(link_info const & info
                                                   , link_context::mode_t const mode = link_context::mode_t::LINK_CONTENT_MODE_DEFAULT
                                                   , int const count = READ_RECORD_COUNT);
    link_info_pair::vector_t        list_of_links(QString const & path);
    void                            adjust_links_after_cloning(QString const & source_key, QString const & destination_key);
    void                            fix_branch_copy_link(libdbproxy::cell::pointer_t source_cell
                                                       , libdbproxy::row::pointer_t destination_row
                                                       , snap_version::version_number_t const destination_branch_number);

    // links test suite
    SNAP_TEST_PLUGIN_SUITE_SIGNALS()

private:
    void                            init_tables();
    void                            on_backend_action_create_link();
    void                            on_backend_action_delete_link();
    void                            on_backend_action_snap547_fix_link_branches();
    void                            cleanup_links();

    // tests
    SNAP_TEST_PLUGIN_TEST_DECL(test_unique_unique_create_delete)
    SNAP_TEST_PLUGIN_TEST_DECL(test_unique_unique_create_replace_delete)
    SNAP_TEST_PLUGIN_TEST_DECL(test_unique_unique_create2_replace2_delete2)
    SNAP_TEST_PLUGIN_TEST_DECL(test_multiple_multiple_create_delete)

    snap_child *                                    f_snap = nullptr;
    libdbproxy::table::pointer_t         f_links_table;
    libdbproxy::table::pointer_t         f_branch_table;
};

} // namespace links
} // namespace snap
// vim: ts=4 sw=4 et
