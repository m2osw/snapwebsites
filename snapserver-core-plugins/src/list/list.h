// Snap Websites Server -- list management (sort criteria)
// Copyright (C) 2014-2017  Made to Order Software Corp.
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
#include "../layout/layout.h"
#include "../filter/filter.h"

//#include "../test_plugin_suite/test_plugin_suite.h"

// snapwebsites lib
//
#include <snapwebsites/snap_backend.h>
#include <snapwebsites/snap_expr.h>


namespace snap
{
namespace list
{


enum class name_t
{
    SNAP_NAME_LIST_ITEM_KEY_SCRIPT,
    SNAP_NAME_LIST_KEY,
    SNAP_NAME_LIST_LAST_UPDATED,
    SNAP_NAME_LIST_LISTJOURNAL,
    SNAP_NAME_LIST_LINK,
    SNAP_NAME_LIST_NAME,
    SNAP_NAME_LIST_NAMESPACE,
    SNAP_NAME_LIST_NUMBER_OF_ITEMS,
    SNAP_NAME_LIST_ORDERED_PAGES,
    SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT,
    SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT,
    SNAP_NAME_LIST_PAGE,
    SNAP_NAME_LIST_PAGELIST,
    SNAP_NAME_LIST_PAGE_SIZE,
    SNAP_NAME_LIST_PROCESSALLLISTS,
    SNAP_NAME_LIST_PROCESSLIST,
    SNAP_NAME_LIST_RESETLISTS,
    SNAP_NAME_LIST_SELECTOR,
    SNAP_NAME_LIST_STANDALONE,
    SNAP_NAME_LIST_STANDALONELIST,
    SNAP_NAME_LIST_TABLE,
    SNAP_NAME_LIST_TABLE_REF,
    SNAP_NAME_LIST_TAXONOMY_PATH,
    SNAP_NAME_LIST_TEST_SCRIPT,
    SNAP_NAME_LIST_THEME,
    SNAP_NAME_LIST_TYPE
};
char const * get_name(name_t name) __attribute__ ((const));


class list_exception : public snap_exception
{
public:
    explicit list_exception(char const *        what_msg) : snap_exception("list", what_msg) {}
    explicit list_exception(std::string const & what_msg) : snap_exception("list", what_msg) {}
    explicit list_exception(QString const &     what_msg) : snap_exception("list", what_msg) {}
};

class list_exception_no_backend : public list_exception
{
public:
    explicit list_exception_no_backend(char const *        what_msg) : list_exception(what_msg) {}
    explicit list_exception_no_backend(std::string const & what_msg) : list_exception(what_msg) {}
    explicit list_exception_no_backend(QString const &     what_msg) : list_exception(what_msg) {}
};

class list_exception_invalid_number_of_parameters : public list_exception
{
public:
    explicit list_exception_invalid_number_of_parameters(char const *        what_msg) : list_exception(what_msg) {}
    explicit list_exception_invalid_number_of_parameters(std::string const & what_msg) : list_exception(what_msg) {}
    explicit list_exception_invalid_number_of_parameters(QString const &     what_msg) : list_exception(what_msg) {}
};

class list_exception_invalid_parameter_type : public list_exception
{
public:
    explicit list_exception_invalid_parameter_type(char const *        what_msg) : list_exception(what_msg) {}
    explicit list_exception_invalid_parameter_type(std::string const & what_msg) : list_exception(what_msg) {}
    explicit list_exception_invalid_parameter_type(QString const &     what_msg) : list_exception(what_msg) {}
};

class list_exception_mysql : public list_exception
{
public:
    explicit list_exception_mysql(char const *        what_msg) : list_exception(what_msg) {}
    explicit list_exception_mysql(std::string const & what_msg) : list_exception(what_msg) {}
    explicit list_exception_mysql(QString const &     what_msg) : list_exception(what_msg) {}
};




class list_item_t
{
public:
    void                set_sort_key(QByteArray const & sort_key) { f_sort_key = sort_key; }
    void                set_uri(QString const & uri) { f_uri = uri; }

    QByteArray const &  get_sort_key() const { return f_sort_key; }
    QString const &     get_uri() const { return f_uri; }

private:
    QByteArray          f_sort_key;
    QString             f_uri;
};
typedef QVector<list_item_t> list_item_vector_t;



class paging_t
{
public:
    static int32_t const    DEFAULT_PAGE_SIZE = 20;

                        paging_t(snap_child * snap, content::path_info_t & ipath);

    list_item_vector_t  read_list();

    QString             get_list_name() const;

    void                set_maximum_number_of_items(int32_t maximum_items);
    int32_t             get_maximum_number_of_items() const;

    int32_t             get_number_of_items() const;

    void                set_start_offset(int32_t start_offset);
    int32_t             get_start_offset() const;

    void                process_query_string_info();
    QString             generate_query_string_info(int32_t page_offset) const;
    QString             generate_query_string_info_for_first_page() const;
    QString             generate_query_string_info_for_last_page() const;
    void                generate_list_navigation(QDomElement element, snap_uri uri, int32_t next_previous_count, bool const next_previous, bool const first_last, bool const next_previous_page) const;

    void                set_page(int32_t page);
    int32_t             get_page() const;

    void                set_next_page(int32_t next_page);
    int32_t             get_next_page() const;

    void                set_previous_page(int32_t previous_page);
    int32_t             get_previous_page() const;

    int32_t             get_total_pages() const;

    void                set_page_size(int32_t page_size);
    int32_t             get_page_size() const;

    // TODO: add support for counting the number of items in a list
    //       so that way we can calculate the last page and allow
    //       people to go there (the counting should happen in the
    //       backend whenever we update a list.)

private:
    snap_child *                    f_snap = nullptr;
    content::path_info_t &          f_ipath;                        // path to the list
    mutable bool                    f_retrieved_list_name = false;
    mutable QString                 f_list_name;                    // name used in query string
    int32_t                         f_maximum_number_of_items = -1; // maximum number of items
    mutable int32_t                 f_number_of_items = -1;         // total number of items
    int32_t                         f_start_offset = -1;            // if -1, ignore
    int32_t                         f_page = 1;                     // page count starts at 1
    mutable int32_t                 f_page_size = -1;               // number of item per page
    mutable int32_t                 f_default_page_size = -1;       // to know whether the query string should include the size
};







class list
        : public plugins::plugin
        , public server::backend_action
        , public layout::layout_content
        , public layout::layout_boxes
{
public:
    static int const LIST_PROCESSING_LATENCY = 10 * 1000000; // 10 seconds in micro-seconds
    static int const LIST_MAXIMUM_ITEMS = 10000; // maximum number of items returned by read_list()

    typedef int64_t             timeout_t;

    // pages given a smaller priority are processed earlier
    typedef unsigned char       priority_t;

    static priority_t const     LIST_PRIORITY_NOW       =   0;      // first page on the list
    static priority_t const     LIST_PRIORITY_IMPORTANT =  10;      // user / developer says thiss page is really important and should be worked on ASAP
    static priority_t const     LIST_PRIORITY_NEW_PAGE  =  20;      // a new page that was just created
    static priority_t const     LIST_PRIORITY_RESET     =  50;      // user asked for a manual reset of (many) pages
    static priority_t const     LIST_PRIORITY_UPDATES   = 180;      // updates from content.xml files
    static priority_t const     LIST_PRIORITY_SLOW      = 200;      // from this number up, do not process if any other pages were processed
    static priority_t const     LIST_PRIORITY_REVIEW    = 230;      // once in a while, review all the pages, just in case we missed something

    class safe_priority_t
    {
    public:
        safe_priority_t(priority_t priority)
            : f_list_plugin(list::instance())
            , f_priority(f_list_plugin->get_priority())
        {
            f_list_plugin->set_priority(priority);
        }

        ~safe_priority_t()
        {
            f_list_plugin->set_priority(f_priority);
        }

    private:
        list *              f_list_plugin = nullptr;
        priority_t          f_priority = LIST_PRIORITY_NEW_PAGE;
    };

    class safe_start_date_offset_t
    {
    public:
        safe_start_date_offset_t(int64_t start_date_offset)
            : f_list_plugin(list::instance())
            , f_start_date_offset(f_list_plugin->get_start_date_offset())
        {
            f_list_plugin->set_start_date_offset(start_date_offset);
        }

        ~safe_start_date_offset_t()
        {
            f_list_plugin->set_start_date_offset(f_start_date_offset);
        }

    private:
        list *              f_list_plugin = nullptr;
        int64_t             f_start_date_offset = 0;
    };

                        list();
                        ~list();

    // plugins::plugin implementation
    static list *       instance();
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // server::backend_action implementation
    virtual void        on_backend_action(QString const & action);

    // server signals
    void                on_backend_process();
    void                on_register_backend_cron(server::backend_action_set & actions);
    void                on_register_backend_action(server::backend_action_set & actions);
    void                on_attach_to_session();

    // layout::layout_content implementation
    virtual void        on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    // layout::layout_boxes implementation
    virtual void        on_generate_boxes_content(content::path_info_t & page_ipath, content::path_info_t & ipath, QDomElement & page, QDomElement & boxes);

    // content signals
    void                on_create_content(content::path_info_t & ipath, QString const & owner, QString const & type);
    void                on_modified_content(content::path_info_t & ipath);
    void                on_copy_branch_cells(QtCassandra::QCassandraCells & source_cells, QtCassandra::QCassandraRow::pointer_t destination_row, snap_version::version_number_t const destination_branch);
    void                on_modified_link(links::link_info const & link, bool const created);

    // filter signals
    void                on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token);
    void                on_token_help(filter::filter::token_help_t & help);

    void                set_priority(priority_t priority);
    priority_t          get_priority() const;

    void                set_start_date_offset(int64_t offset_us);
    int64_t             get_start_date_offset() const;

    int                 generate_list_for_page(content::path_info_t & page_key, content::path_info_t & list_ipath, int64_t update_request_time);

    list_item_vector_t  read_list(content::path_info_t & ipath, int start, int count);
    QString             generate_list(content::path_info_t & ipath, content::path_info_t & list_ipath, int start = 0, int count = -1, QString const & theme = "qrc:/xsl/list/default");

    // list plugin signals
    SNAP_SIGNAL_WITH_MODE(list_modified, (content::path_info_t & ipath), (ipath), NEITHER);

    // links test suite
    //SNAP_TEST_PLUGIN_SUITE_SIGNALS()

private:
    void                content_update(int64_t variables_timestamp);
    void                add_all_pages_to_list_table(QString const & f);
    int                 generate_all_lists(QString const & site_key);
    int                 generate_all_lists_for_page(QString const & site_key, QString const & row_key, int64_t update_request_time);
    int                 generate_new_lists(QString const & site_key);
    int                 generate_new_list_for_all_pages(QString const & site_key, content::path_info_t & list_ipath);
    int                 generate_new_list_for_descendants(QString const & site_key, content::path_info_t & list_ipath);
    int                 generate_new_list_for_children(QString const & site_key, content::path_info_t & list_ipath);
    int                 generate_new_list_for_all_descendants(content::path_info_t & list_ipath, content::path_info_t & parent, bool const descendants);
    int                 generate_new_list_for_public(QString const & site_key, content::path_info_t & list_ipath);
    int                 generate_new_list_for_type(QString const & site_key, content::path_info_t & list_ipath, QString const & type);
    int                 generate_new_list_for_hand_picked_pages(QString const & site_key, content::path_info_t & list_ipath, QString const & hand_picked_pages);
    bool                run_list_check(content::path_info_t & list_ipath, content::path_info_t & page_ipath);
    QString             run_list_item_key(content::path_info_t & list_ipath, content::path_info_t & page_ipath);
    int                 send_data_to_journal();

    // tests
    //SNAP_TEST_PLUGIN_TEST_DECL(test_add_page_twice)

    snap_child *                            f_snap = nullptr;
    snap_backend *                          f_backend = nullptr;
    snap_expr::expr::expr_map_t             f_check_expressions;
    snap_expr::expr::expr_map_t             f_item_key_expressions;
    bool                                    f_ping_backend = false;
    bool                                    f_list_link = false;
    priority_t                              f_priority = LIST_PRIORITY_NEW_PAGE;                // specific order in which pages should be worked on
    int64_t                                 f_start_date_offset = LIST_PROCESSING_LATENCY;      // minimum amount of time to wait for the next data
    int64_t                                 f_date_limit = 0;
};


} // namespace list
} // namespace snap
// vim: ts=4 sw=4 et
