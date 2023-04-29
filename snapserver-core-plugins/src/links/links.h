// Snap Websites Server -- links
// Copyright (c) 2012-2021  Made to Order Software Corp.  All Rights Reserved
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



namespace snap
{

// TODO: remove dependency to content
//       (or as mentioned in SNAP-9 which is to move the content, links,
//       attachments, and probably one or two other things to the
//       libsnapwebsites instead of having them as plugins...)
//
namespace content
{
class path_info_t;
}


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


DECLARE_MAIN_EXCEPTION(links_exception);

DECLARE_EXCEPTION(links_exception, links_exception_missing_links_table);
DECLARE_EXCEPTION(links_exception, links_exception_missing_branch_table);
DECLARE_EXCEPTION(links_exception, links_exception_invalid_name);
DECLARE_EXCEPTION(links_exception, links_exception_invalid_db_data);
DECLARE_EXCEPTION(links_exception, links_exception_missing_link);



class link_info
{
public:
    typedef std::vector<link_info>      vector_t;

    link_info() noexcept
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
    void set_source_cell_name(QString const & new_name)
    {
        f_source_cell_name = new_name;
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
    QString source_cell_name() const // WARNING: not always defined! (for now, only in next_link() of multi-link supports it)
    {
        return f_source_cell_name;
    }
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
    QString                         f_name = QString();
    QString                         f_key = QString();
    snap_version::version_number_t  f_branch = snap_version::version_number_t();
    QString                         f_source_cell_name = QString();
    QString                         f_destination_cell_name = QString();
};




#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnoexcept"
class link_info_pair
{
public:
    typedef std::vector<link_info_pair>      vector_t;

                                link_info_pair(link_info const & src, link_info const & dst);

    link_info const &           source() const;
    link_info const &           destination() const;

private:
    link_info                   f_source = link_info();
    link_info                   f_destination = link_info();
};
#pragma GCC diagnostic pop





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

                    link_context(link_context const & rhs) = delete;

    link_context &  operator = (link_context const & rhs) = delete;

    bool            next_link(link_info & info);

private:
    friend class links;

    link_context(::snap::snap_child * snap, link_info const & info, mode_t const mode, const int count);

    snap_child *                                f_snap = nullptr;
    link_info                                   f_info = link_info();
    libdbproxy::row::pointer_t                  f_row = libdbproxy::row::pointer_t();
    libdbproxy::cell_range_predicate::pointer_t f_column_predicate = libdbproxy::cell_range_predicate::pointer_t();
    libdbproxy::cells                           f_cells = libdbproxy::cells();
    libdbproxy::cells::const_iterator           f_cell_iterator = libdbproxy::cells::const_iterator();
    link_info                                   f_link = link_info();
};





class links_cloned
{
public:
    virtual             ~links_cloned() {}

    virtual void        repair_link_of_cloned_page(
                                  QString const & clone
                                , snap_version::version_number_t branch_number
                                , link_info const & source
                                , link_info const & destination
                                , bool const cloning) = 0;
};





class links
    : public serverplugins::plugin
    , public server::backend_action
{
public:
    SERVERPLUGINS_DEFAULTS(links);

    typedef std::function<bool (content::path_info_t & ipath)> callback_func_t;

    static int const                READ_RECORD_COUNT = 1000;
    static int const                DELETE_RECORD_COUNT = 1000;

    // serverplugins::plugin implementation
    virtual void                    bootstrap() override;
    virtual time_t                  do_update(time_t last_updated, unsigned int phase) override;

    libdbproxy::table::pointer_t    get_links_table();

    // server signals
    void                            on_add_snap_expr_functions(snap_expr::functions_t & functions);
    void                            on_register_backend_action(server::backend_action_set & actions);

    // server::backend_action implementation
    virtual void                    on_backend_action(QString const & action);

    PLUGIN_SIGNAL(modified_link, (link_info const & link, bool const created), (link, created));

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
    bool                            enumerate_children(content::path_info_t & parent_ipath
                                                     , callback_func_t callback
                                                     , bool all_status = false);

    // links test suite
    PLUGIN_TEST_PLUGIN_SUITE_SIGNALS()

private:
    void                            init_tables();
    void                            on_backend_action_create_link();
    void                            on_backend_action_delete_link();
    void                            on_backend_action_snap547_fix_link_branches();
    void                            cleanup_links();

    // tests
    PLUGIN_TEST_PLUGIN_TEST_DECL(test_unique_unique_create_delete)
    PLUGIN_TEST_PLUGIN_TEST_DECL(test_unique_unique_create_replace_delete)
    PLUGIN_TEST_PLUGIN_TEST_DECL(test_unique_unique_create2_replace2_delete2)
    PLUGIN_TEST_PLUGIN_TEST_DECL(test_multiple_multiple_create_delete)

    snap_child *                    f_snap = nullptr;
    libdbproxy::table::pointer_t    f_links_table = libdbproxy::table::pointer_t();
    libdbproxy::table::pointer_t    f_branch_table = libdbproxy::table::pointer_t();
};

} // namespace links
} // namespace snap
// vim: ts=4 sw=4 et
