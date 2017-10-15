// Snap Websites Server -- handle journaling content
// Copyright (C) 2011-2017  Made to Order Software Corp.
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


/** \file
 * \brief The implementation of the content plugin journal_list class.
 *
 * This file contains the journal_list class implementation.
 */

#include "content.h"

#include <snapwebsites/log.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(content)


journal_list* content::get_journal_list()
{
    f_to_process.push_back( journal_list(f_snap) );
    f_journal_list_stack.push( &(f_to_process.back()) );
    SNAP_LOG_DEBUG("content::get_journal_list(): f_to_process.size()=")(f_to_process.size())
            (", f_journal_list_stack.size()=")(f_journal_list_stack.size());
    return f_journal_list_stack.top();
}


void content::journal_list_pop()
{
    f_journal_list_stack.pop();
    SNAP_LOG_DEBUG("f_journal_list_stack.size()=")(f_journal_list_stack.size());
    //
    if( f_journal_list_stack.empty() )
    {
        finish_all_journals();
    }
}


void content::finish_all_journals()
{
    SNAP_LOG_DEBUG("++++ f_to_process.size()=")(f_to_process.size());
    for( auto & list : f_to_process )
    {
        list.finish_pages();
    }

    f_to_process.clear();
}


journal_list::journal_list( snap_child* snap )
    : f_snap(snap)
    , f_journal_table( f_snap->get_table(get_name(name_t::SNAP_NAME_CONTENT_JOURNAL_TABLE)) )
{
}


journal_list::~journal_list()
{
    if( !f_url_list.isEmpty() )
    {
        SNAP_LOG_ERROR("URL list is not empty!");
    }
}


/** \brief Flag a page for creation in the journal, with timestamp.
 *
 * This creates an entry in the journal table before page completion.
 * It saves the start date from the snap_child instance, along with the URL.
 *
 * \sa backend_process_journal(), journal_finish_page()
 */
void journal_list::add_page_url( QString const & full_url )
{
    if( f_finished_called )
    {
        throw content_exception_content_invalid_state( "journal_list is one use only!" );
    }

    auto field_timestamp ( get_name(name_t::SNAP_NAME_CONTENT_JOURNAL_TIMESTAMP)  );
    auto field_url       ( get_name(name_t::SNAP_NAME_CONTENT_JOURNAL_URL)        );

    auto current_journal_row( f_journal_table->row(full_url) );
    current_journal_row->cell(field_timestamp) ->setValue( QtCassandra::QCassandraValue(f_snap->get_start_date()) );
    current_journal_row->cell(field_url)       ->setValue( full_url );

    f_url_list << full_url;
}


/** \brief Pop this current item off of the journal stack, and save it for later processing
 */
void journal_list::done()
{
    SNAP_LOG_DEBUG("journal_list::done()");
    content::content::instance()->journal_list_pop();
}


/** \brief Finish the current page marked for creation.
 *
 * The current page URL entry is taken out of the journal table.
 *
 * \sa backend_process_journal(), journal_create_page()
 */
void journal_list::finish_pages()
{
    SNAP_LOG_DEBUG("++++ f_url_list=")(f_url_list.size());
    for( auto url : f_url_list )
    {
        // Drop the row, since it is completed and we are done.
        //
        SNAP_LOG_DEBUG("+++ dropping url=")(url);
        f_journal_table->dropRow(url);
    }

    f_url_list.clear();
    f_finished_called = true;
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
