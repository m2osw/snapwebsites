// Snap Websites Server -- tests for the list
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

#include "list.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>

#include <iostream>

#include <snapwebsites/poison.h>

SNAP_PLUGIN_EXTENSION_START(list)


SNAP_TEST_PLUGIN_SUITE(list)
    SNAP_TEST_PLUGIN_TEST(list, test_add_page_twice)
SNAP_TEST_PLUGIN_SUITE_END()


namespace
{

/** \brief This function searches for a row that is not yet referenced.
 *
 * Various tests want to start testing a page that has not been referenced
 * (i.e. of which the URL does not appear in the listref table.)
 */
QtCassandra::QCassandraRow::pointer_t find_unreference_row(QString const & site_key, QString const & ref_key)
{
    content::content * content_plugin(content::content::instance());
    list * list_plugin(list::list::instance());

    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    content_table->clearCache();

    // if the ref_row does not exist, then no list were worked on for a while
    // and thus we can return the first row that the next loop returns
    QtCassandra::QCassandraTable::pointer_t listref_table(list_plugin->get_listref_table());
    QtCassandra::QCassandraRow::pointer_t ref_row(listref_table->row(ref_key));

    auto row_predicate = std::make_shared<QtCassandra::QCassandraRowPredicate>();
    row_predicate->setCount(1000);
    for(;;)
    {
        uint32_t const count(content_table->readRows(row_predicate));
        if(count == 0)
        {
            // no more lists to process
            SNAP_LOG_ERROR("content_table rows vs listref cells all failed (")(site_key)(")");
            return QtCassandra::QCassandraRow::pointer_t();
        }
        QtCassandra::QCassandraRows const rows(content_table->rows());
        for(QtCassandra::QCassandraRows::const_iterator o(rows.begin());
                o != rows.end(); ++o)
        {
            QString const key(QString::fromUtf8(o.key().data()));
            if(key.startsWith(site_key))
            {
                // check whether it exists in the listref table
                // if so, then we try with the next cell
                if(!ref_row
                || !ref_row->exists(key))
                {
                    return *o;
                }
            }
        }
    }
}

} // no name namespace


SNAP_TEST_PLUGIN_TEST_IMPL(list, test_add_page_twice)
{
    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
    QtCassandra::QCassandraTable::pointer_t list_table(get_list_table());
    QtCassandra::QCassandraTable::pointer_t listref_table(get_listref_table());

    QString const site_key(f_snap->get_site_key_with_slash());
    QString const ref_key(QString("%1#ref").arg(site_key));

    // row we get here is from the content table so its name
    // is the URI
    QtCassandra::QCassandraRow::pointer_t row(find_unreference_row(site_key, ref_key));

    // The test cannot really be applied if no free cell was found
    SNAP_TEST_PLUGIN_SUITE_ASSERT(row);

    // this key is to be found in the listref table as a cell
    QString const key(row->rowName());

    // this does not exist yet
    SNAP_TEST_PLUGIN_SUITE_ASSERT(!listref_table->row(ref_key)->exists(key));

    // we cannot really test whether it exists in the list table, we
    // have another test for that because we have to go through all the 
    // items to confirm the non-existance

    content::path_info_t ipath;
    ipath.set_path(key);
    content_plugin->modified_content(ipath);

    // help with debug
    SNAP_LOG_ERROR("key being tested: ")(key);

    // now this exists
    SNAP_TEST_PLUGIN_SUITE_ASSERT(listref_table->row(ref_key)->exists(key));

    // the page referenced also exists in the list table for up to
    // 10 seconds (see LIST_PROCESSING_LATENCY) -- this should change
    // with time and give us another problem to resolve while testing
    // the list capabilities...

    QtCassandra::QCassandraValue const value(listref_table->row(ref_key)->cell(key)->value());
    SNAP_TEST_PLUGIN_SUITE_ASSERT(static_cast<size_t>(value.size()) > sizeof(int64_t));

    int64_t const time_recorded(value.int64Value());
    SNAP_TEST_PLUGIN_SUITE_ASSERT(time_recorded == f_snap->get_start_date());

    QString const saved_key(value.stringValue(sizeof(int64_t)));
    SNAP_TEST_PLUGIN_SUITE_ASSERT(saved_key == ipath.get_key());

    // and there is a corresponding entry in the list table
    // we test here because that way we have the value which is our
    // key to check the availability in the list table
    SNAP_TEST_PLUGIN_SUITE_ASSERT(list_table->row(site_key)->exists(value.binaryValue()));

    // do it again, changes nothing
    content_plugin->modified_content(ipath);

    QtCassandra::QCassandraValue const again_value(listref_table->row(ref_key)->cell(key)->value());
    SNAP_TEST_PLUGIN_SUITE_ASSERT(again_value.size() == value.size());

    int64_t const again_time_recorded(again_value.int64Value());
    SNAP_TEST_PLUGIN_SUITE_ASSERT(again_time_recorded == f_snap->get_start_date());

    QString const again_saved_key(again_value.stringValue(sizeof(int64_t)));
    SNAP_TEST_PLUGIN_SUITE_ASSERT(again_saved_key == saved_key);

    // change the start date and we expect the previous instance to
    // be removed
    f_snap->init_start_date();
    content_plugin->modified_content(ipath);

    QtCassandra::QCassandraValue const changed_value(listref_table->row(ref_key)->cell(key)->value());
    SNAP_TEST_PLUGIN_SUITE_ASSERT(changed_value.size() == value.size());

    int64_t const changed_time_recorded(changed_value.int64Value());
    SNAP_TEST_PLUGIN_SUITE_ASSERT(changed_time_recorded == f_snap->get_start_date());

    QString const changed_saved_key(changed_value.stringValue(sizeof(int64_t)));
    SNAP_TEST_PLUGIN_SUITE_ASSERT(changed_saved_key == saved_key);

    // the new entry has to exist in the list, easy test
    SNAP_TEST_PLUGIN_SUITE_ASSERT(list_table->row(site_key)->exists(changed_value.binaryValue()));

    // now the old key has to have been deleted, we have to be careful
    // when testing this one because it could:
    // (1) be cached; and
    // (2) be visible in the database because we'd read it from a
    //     different instance or something of the sort
    list_table->clearCache();
    QtCassandra::QCassandraCell::pointer_t cell(list_table->row(site_key)->cell(value.binaryValue()));
    // the consistency used to save values is ONE so QUORUM here
    // would not be enough to make 100% sure that all nodes were
    // checked before returning the result (it also means you need
    // a 100% valid cassandra cluster...)
    cell->setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_ALL);
    QtCassandra::QCassandraValue no_value(cell->value());

    // value is 1 byte if the cell did not get deleted properly
    SNAP_TEST_PLUGIN_SUITE_ASSERT(no_value.nullValue());
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
