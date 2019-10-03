// Snap Websites Server -- index management (sort criteria)
// Copyright (c) 2019  Made to Order Software Corp.  All Rights Reserved
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
namespace index
{


enum class name_t
{
    SNAP_NAME_INDEX_DEFAULT_INDEX,
    SNAP_NAME_INDEX_NUMBER_OF_RECORDS,
    SNAP_NAME_INDEX_ORIGINAL_SCRIPTS,
    SNAP_NAME_INDEX_PAGE,
    SNAP_NAME_INDEX_PAGE_SIZE,
    SNAP_NAME_INDEX_REINDEX,
    SNAP_NAME_INDEX_TABLE,
    SNAP_NAME_INDEX_THEME,
};
char const * get_name(name_t name) __attribute__ ((const));


class index_exception : public snap_exception
{
public:
    explicit index_exception(char const *        what_msg) : snap_exception("index", what_msg) {}
    explicit index_exception(std::string const & what_msg) : snap_exception("index", what_msg) {}
    explicit index_exception(QString const &     what_msg) : snap_exception("index", what_msg) {}
};

class index_exception_invalid_parameter_type : public index_exception
{
public:
    explicit index_exception_invalid_parameter_type(char const *        what_msg) : index_exception(what_msg) {}
    explicit index_exception_invalid_parameter_type(std::string const & what_msg) : index_exception(what_msg) {}
    explicit index_exception_invalid_parameter_type(QString const &     what_msg) : index_exception(what_msg) {}
};

class index_exception_no_backend : public index_exception
{
public:
    explicit index_exception_no_backend(char const *        what_msg) : index_exception(what_msg) {}
    explicit index_exception_no_backend(std::string const & what_msg) : index_exception(what_msg) {}
    explicit index_exception_no_backend(QString const &     what_msg) : index_exception(what_msg) {}
};




typedef std::map<QString, QString>      variables_t;


class index_record_t
{
public:
    void                set_sort_key(QByteArray const & sort_key) { f_sort_key = sort_key; }
    void                set_uri(QString const & uri) { f_uri = uri; }

    QByteArray const &  get_sort_key() const { return f_sort_key; }
    QString const &     get_uri() const { return f_uri; }

private:
    QByteArray          f_sort_key = QByteArray();
    QString             f_uri = QString();
};
typedef QVector<index_record_t> index_record_vector_t;



class paging_t
{
public:
    static int32_t const    DEFAULT_PAGE_SIZE = 20;

                        paging_t(snap_child * snap, content::path_info_t & ipath, QString const & index_name = QString());

                        paging_t(paging_t const & rhs) = delete;
    paging_t &          operator = (paging_t const & rhs) = delete;

    index_record_vector_t
                        read_index();

    QString             get_index_name(bool empty_if_default = false) const;

    void                set_maximum_number_of_records(int32_t maximum_records);
    int32_t             get_maximum_number_of_records() const;

    int32_t             get_number_of_records() const;

    void                set_start_offset(int32_t start_offset);
    int32_t             get_start_offset() const;

    void                set_start_key(QString const & start_key);
    QString const &     get_start_key() const;

    void                process_query_string_info();
    QString             generate_query_string_info(int32_t page_offset) const;
    QString             generate_query_string_info_for_first_page() const;
    QString             generate_query_string_info_for_last_page() const;
    void                generate_index_navigation(QDomElement element, snap_uri uri, int32_t next_previous_count, bool const next_previous, bool const first_last, bool const next_previous_page) const;

    void                set_page(int32_t page);
    int32_t             get_page() const;

    void                set_next_page(int32_t next_page);
    int32_t             get_next_page() const;

    void                set_previous_page(int32_t previous_page);
    int32_t             get_previous_page() const;

    int32_t             get_total_pages() const;

    void                set_page_size(int32_t page_size);
    int32_t             get_page_size() const;

    // TODO: add support for counting the number of records in an index
    //       so that way we can calculate the last page and allow
    //       people to go there (the counting should happen in the
    //       backend whenever we update an index.)

private:
    snap_child *                    f_snap = nullptr;
    content::path_info_t &          f_ipath;                        // path to the index
    mutable bool                    f_retrieved_index_name = false;
    mutable QString                 f_index_name = QString();       // name used in query string
    QString                         f_start_key = QString();        // start of what your "k=..." script generates
    int32_t                         f_maximum_number_of_records = -1; // maximum number of records
    mutable int32_t                 f_number_of_records = -1;       // total number of records
    int32_t                         f_start_offset = -1;            // if -1, ignore
    int32_t                         f_page = 1;                     // page count starts at 1
    mutable int32_t                 f_page_size = -1;               // number of record per page
    mutable int32_t                 f_default_page_size = -1;       // to know whether the query string should include the size
};







class index
    : public plugins::plugin
    , public server::backend_action
    , public layout::layout_content
    , public layout::layout_boxes
{
public:
    static int const INDEX_MAXIMUM_RECORDS = 10000; // maximum number of records returned by read_index()

                        index();
                        index(index const & rhs) = delete;
    virtual             ~index() override;

    index &             operator = (index const & rhs) = delete;

    static index *      instance();

    // plugins::plugin implementation
    virtual QString     icon() const override;
    virtual QString     description() const override;
    virtual QString     dependencies() const override;
    virtual int64_t     do_update(int64_t last_updated) override;
    virtual void        bootstrap(snap_child * snap) override;

    // server::backend_action implementation
    virtual void        on_backend_action(QString const & action) override;

    // server signals
    void                on_backend_process();
    void                on_register_backend_cron(server::backend_action_set & actions);
    void                on_register_backend_action(server::backend_action_set & actions);
    void                on_attach_to_session();

    // layout::layout_content implementation
    virtual void        on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body) override;

    // layout::layout_boxes implementation
    virtual void        on_generate_boxes_content(content::path_info_t & page_ipath, content::path_info_t & ipath, QDomElement & page, QDomElement & boxes) override;

    // content signals
    void                on_create_content(content::path_info_t & ipath, QString const & owner, QString const & type);
    void                on_modified_content(content::path_info_t & ipath);
    void                on_copy_branch_cells(libdbproxy::cells & source_cells, libdbproxy::row::pointer_t destination_row, snap_version::version_number_t const destination_branch);
    void                on_modified_link(links::link_info const & link, bool const created);

    // filter signals
    void                on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token);
    void                on_token_help(filter::filter::token_help_t & help);

    libdbproxy::table::pointer_t
                        get_index_table();

    void                index_pages(content::path_info_t & page_ipath
                                  , content::path_info_t & type_ipath
                                  , QString const & script);
    void                index_one_page(content::path_info_t & page_ipath
                                     , content::path_info_t & type_ipath
                                     , variables_t const & scripts);
    QString             get_key_of_index_page(content::path_info_t & page_ipath
                                            , content::path_info_t & type_ipath
                                            , variables_t const & scripts);

    int                 generate_index_for_page(content::path_info_t & page_key
                                              , content::path_info_t & index_ipath
                                              , int64_t update_request_time
                                              , links::link_info * child_info = nullptr);

    index_record_vector_t
                        read_index(content::path_info_t & ipath
                                 , QString const & name
                                 , int start
                                 , int count
                                 , QString const & start_key = QString());
    QString             generate_index(content::path_info_t & ipath
                                     , content::path_info_t & index_ipath
                                     , int start = 0
                                     , int count = -1
                                     , QString const & start_key = QString()
                                     , QString const & theme = "qrc:/xsl/index/default");

    // index plugin signals
    SNAP_SIGNAL_WITH_MODE(index_modified, (content::path_info_t & ipath), (ipath), NEITHER);

private:
    void                content_update(int64_t variables_timestamp);
    void                reindex();
//    void                add_all_pages_to_index_table(QString const & f);
//    int                 generate_all_indexes(QString const & site_key);
//    int                 generate_all_indexes_for_page(QString const & site_key, QString const & row_key, int64_t update_request_time);
//    int                 generate_new_indexes(QString const & site_key);
//    int                 generate_new_index_for_all_pages(QString const & site_key, content::path_info_t & index_ipath);
//    int                 generate_new_index_for_descendants(QString const & site_key, content::path_info_t & index_ipath);
//    int                 generate_new_index_for_children(QString const & site_key, content::path_info_t & index_ipath);
//    int                 generate_new_index_for_all_descendants(content::path_info_t & index_ipath, content::path_info_t & parent, bool const descendants);
//    int                 generate_new_index_for_public(QString const & site_key, content::path_info_t & index_ipath);
//    int                 generate_new_index_for_type(QString const & site_key, content::path_info_t & index_ipath, QString const & type);
//    int                 generate_new_index_for_hand_picked_pages(QString const & site_key, content::path_info_t & index_ipath, QString const & hand_picked_pages);
//    bool                run_index_check(content::path_info_t & index_ipath, content::path_info_t & page_ipath);
//    QString             run_index_record_key(content::path_info_t & index_ipath, content::path_info_t & page_ipath);

    // tests
    //SNAP_TEST_PLUGIN_TEST_DECL(test_add_page_twice)

    snap_child *                            f_snap = nullptr;
    snap_backend *                          f_backend = nullptr;
    snap_string_list                        f_page = snap_string_list();
    libdbproxy::table::pointer_t            f_index_table = libdbproxy::table::pointer_t();
    uint32_t                                f_count_processed = 0;

    //snap_expr::expr::expr_map_t             f_check_expressions = snap_expr::expr::expr_map_t();
    //snap_expr::expr::expr_map_t             f_record_key_expressions = snap_expr::expr::expr_map_t();
    //bool                                    f_ping_backend = false;
    //bool                                    f_index_link = false;
    //priority_t                              f_priority = INDEX_PRIORITY_NEW_PAGE;                // specific order in which pages should be worked on
    //int64_t                                 f_date_limit = 0;
};


} // namespace index
} // namespace snap
// vim: ts=4 sw=4 et
