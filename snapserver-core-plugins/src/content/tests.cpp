// Snap Websites Server -- tests for content
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

#include "content.h"

#include <unistd.h>

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>

#include <snapwebsites/poison.h>

SNAP_PLUGIN_EXTENSION_START(content)


SNAP_TEST_PLUGIN_SUITE(content)
    SNAP_TEST_PLUGIN_TEST(content, test_journal_list)
SNAP_TEST_PLUGIN_SUITE_END()


SNAP_TEST_PLUGIN_TEST_IMPL(content, test_journal_list)
{
    auto journal_table( f_snap->get_table(get_name(name_t::SNAP_NAME_CONTENT_JOURNAL_TABLE)) );

    // Empty the journal table first
    //
    journal_table->clearCache();
    journal_table->truncate();

    // This is a pointer to a new journal_list at the top of the stack
    //
    journal_list* journal = nullptr;

    // Keep track of all of the paths we create
    //
    QStringList path_list;

    // Create the top-level content
    //
    auto create_all_content = [this,&path_list,&journal]()
    {
        auto create_test_content = [this,&path_list]( QString const & path, journal_list* jrnl )
        {
            path_info_t ipath;
            ipath.set_path( path );
            ipath.force_branch(snap_version::SPECIAL_VERSION_USER_FIRST_BRANCH);
            ipath.force_revision(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));
            ipath.force_locale("xx");
            jrnl->add_page_url(ipath.get_key());
            path_list << ipath.get_key();
            create_content( ipath, get_plugin_name(), "user-page" );
        };

        auto add_sub_content = [&]( journal_list* sub_journal, int const id )
        {
            QString sub_path( QString("/test/content%1").arg(id) );
            create_test_content( sub_path, sub_journal );
        };

        SNAP_LOG_DEBUG("flush the cache, destroy the top-level page first, and flush cache again for good measure");
        {
            f_snap->get_context()->clearCache();

            // Destroy the top-level page
            path_info_t ipath;
            ipath.set_path( "/test" );
            auto content_table( f_snap->get_table(get_name(name_t::SNAP_NAME_CONTENT_TABLE)) );
            if( content_table->exists(ipath.get_key()) )
            {
                destroy_page( ipath );
            }

            f_snap->get_context()->clearCache();
        }

        SNAP_LOG_DEBUG("create top level page");
        journal = get_journal_list();
        path_list.clear();
        QString const top_path( "/test" );
        create_test_content( top_path, journal );

        {
            SNAP_LOG_DEBUG("creating sub level pages");

            // sub page with journal
            journal_list* sub_journal( get_journal_list() );
            add_sub_content( sub_journal, 1 );
            add_sub_content( sub_journal, 2 );
            add_sub_content( sub_journal, 3 );
            sub_journal->done();
        }
    };

    auto verify_path_list = [this,&journal_table,&path_list]()
    {
        // loop through the journal table and find all entries. They will still be there
        //
        auto field_timestamp ( get_name(name_t::SNAP_NAME_CONTENT_JOURNAL_TIMESTAMP)                );
        auto field_url       ( get_name(name_t::SNAP_NAME_CONTENT_JOURNAL_URL)                      );

        for( auto path : path_list )
        {
            SNAP_TEST_PLUGIN_SUITE_ASSERT( journal_table->exists(path) );
            auto row( journal_table->getRow(path) );
            SNAP_TEST_PLUGIN_SUITE_ASSERT( row->getCell(field_timestamp) ->getValue().int64Value()  != 0LL  );
            SNAP_TEST_PLUGIN_SUITE_ASSERT( row->getCell(field_url)       ->getValue().stringValue() == path );
        }
    };

    auto verify_content_purge = [this,&path_list]()
    {
        auto content_table( f_snap->get_table(get_name(name_t::SNAP_NAME_CONTENT_TABLE)) );
        for( auto path : path_list )
        {
            path_info_t ipath;
            ipath.set_path( path );
            SNAP_TEST_PLUGIN_SUITE_ASSERT
               (  !content_table->exists(ipath.get_key())
               && !content_table->getRow(ipath.get_key())->exists(get_name(name_t::SNAP_NAME_CONTENT_CREATED))
               );
        }
    };

    auto verify_table_count = [this,&journal_table]( uint32_t const desired_count )
    {
        journal_table->clearCache();

        auto row_predicate = std::make_shared<libdbproxy::row_predicate>();
        row_predicate->setCount(100);

        uint32_t total_count = 0;
        for( ;; )
        {
            uint32_t const count = journal_table->readRows(row_predicate);
            SNAP_LOG_DEBUG("++++ count=")(count);
            if( count == 0 )
            {
                // last page was processed, done.
                break;
            }

            total_count += count; // divide by number of fields (which is two)
        }

        // table should be empty
        SNAP_LOG_DEBUG("total_count=")(total_count)(", desired_count=")(desired_count);
        SNAP_TEST_PLUGIN_SUITE_ASSERT( total_count == desired_count );
    };

    SNAP_LOG_DEBUG("creating test content");
    create_all_content();
    SNAP_LOG_DEBUG("content created! verifying, path_list.size()=")(path_list.size());
    verify_table_count( path_list.size() * 2 );
    SNAP_LOG_DEBUG("done verifying, verifying paths");
    verify_path_list();

    // now finish each entry.
    //
    journal->done();

    // Test to make sure list is indeed empty
    verify_table_count( 0 );

    // Now test error cases. Create content again.
    //
    create_all_content();
    verify_path_list();

    // Wait for a little longer than a minute so we can test the backend...
    SNAP_LOG_DEBUG("Wait 64 seconds...");
    sleep( 64 );

    // Check for one minute age...this should purge the rows we just added
    // to the journal table
    SNAP_LOG_DEBUG("Running backend process!");
    f_snap->init_start_date();      // reset the start date.
    backend_process_journal( 1 );

    // Verify that all records are purged, and that all content is gone.
    SNAP_LOG_DEBUG("verify!");
    verify_table_count( 0 );
    verify_content_purge();
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
