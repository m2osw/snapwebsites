// Snap Websites Server -- handle journaling content
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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
 * \warning
 * The journal_list class is NOT an RAII because the point of this class
 * is to NOT get the content::finish_all_journals() function called if
 * something goes wrong. This is BY DESIGN.
 *
 * This file contains the journal_list class implementation.
 *
 * \attention
 * This journal has nothing to do with the "list" plugin (i.e. the plugin
 * that creates lists of pages.)
 *
 * The journal is used to know what pages we've created in case something
 * goes wrong, because when it does go wrong, we'd be left with partial
 * pages which often do not work right (prevent the rest of the software
 * from working as expected.) The software, using a backend which runs once
 * in a while, will delete pages that did not make it after a given timeout.
 *
 * As far as you are concerned, what you want to do is the following:
 *
 * \code
 *      // step one make sure you can get a journal entry
 *      // (pointer returned is never nullptr, it throws if it fails)
 *      //
 *      journal_list * journal( content->get_journal_list() );
 *
 *      // do various work to prepare the creation of your page
 *      // for example you may want to make sure the page doesn't
 *      // already exist
 *      //
 *      ...
 *      QString path("some/full/path/here");
 *      ...
 *
 *      journal->add_page_url(path);
 *      ...
 *
 *      // now actually create the page in the database
 *      //
 *      content->create_content(...);
 *
 *      // add other fields to the page
 *      //
 *      ...
 *
 *      // once all the important data is added, mark the page as valid
 *      //
 *      journal->done();
 *
 *      ... // if necessary a few other things, but no tweak to the page
 *          // after done() is called
 * \endcode
 *
 * At some point we want to change the page creation in a "start create",
 * "accumulate data", "save". So that way we can avoid this journal
 * handling, although that other method has a very similar effect, we
 * could avoid creating anything in the database if any one part fails
 * before we hit the "save". The current method is very similar to an
 * SQL BEGIN, INSERT, INSERT, ..., COMMIT or ROLLBACK. Normally there are
 * no such facility in a NOSQL database which is why we had to simulate
 * that mechanism. We also do it in such a way that part of the work is
 * offloaded to a background process that can run on a backend computer.
 *
 * The journal has four possible states as represented below:
 *
 * \code
 *                   +-------------+
 *          Finished |             |       Done          Create Sub-Page
 *         +---------+ Remove URLs +<------------+    +---------+
 *         |         |             |             |    |         |
 *         v         +-------------+             |    v         |
 *   +-----+----+                       +--------+----+---+     |
 *   |          |  Create Page          |                 |     |
 *   |  Idle    +---------------------->+  Creating Page  +-----+
 *   |          |                       |                 |
 *   +-----+----+                       +--------+--------+
 *         ^                                     |
 *         |                                     | Creation Never Finished
 *         |                                     | (asynchronous part
 *         |                                     | run on backend)
 *         |                                     v
 *         |                            +--------+--------+
 *         |  Page Removed              |                 |
 *         +----------------------------+  Destroy Page   |
 *                                      |                 |
 *                                      +-----------------+
 * \endcode
 *
 * \li Idle
 *
 * The journal is not being used. Pages were created or removed as required.
 *
 * \li Creating Page
 *
 * The developer called content::create_page() and is working on adding
 * fields to the pages.
 *
 * Here is where something can go wrong and if it does, we never call the
 * journal->done() function. As a result, the state stays in Creating Page
 * until it times out and the Destroy Page state is called.
 *
 * If nothing goes wrong and the page is created and is valid, then the
 * done() function was called and the journal state goes back to Idle.
 *
 * Note that the Creating Page is recursive. While creating page A, you
 * may start creating page B. While creating page B, you may start creating
 * page C. If any one of these fails, then all of them get removed later.
 *
 * \li Remove URLs
 *
 * Once the done() function was called on all the journal_list objects
 * all the URLs that were added to the journal get removed at once.
 * As far as Cassandra is concerned, this is not a very good scheme
 * (i.e. Cassandra expects one write, many reads, and no delete; here
 * we do one write, one delete, at least in normal circumstances. We do
 * not foresee too many problems on that one because not so many pages
 * get created in a row except on first installation. An Import function
 * could run in some problems with such, though. But a properly implemented
 * import should not use the journal_list at all.)
 *
 * Once the URLs are removed from the journal, it is considered IDLE.
 *
 * The removal of all the URLs is a simple loop sending drop orders
 * to Cassandra. Loop which is generally unlikely to break (Even though
 * we have a try/catch there, just in case.) The case were a drop fails
 * is not otherwise properly handled at this time.
 *
 * \li Destroy Page
 *
 * A backend process checks how long ago a journal was created. If more
 * than a certain amount of time has passed after the creation started,
 * we assume that the creation process is over and we can safely assume
 * that the attempt to create the page failed. At that point, we remove
 * the page from the database.
 *
 * This is somewhat of an ugly hack, although remember that in some
 * situation we create multiple pages. One page may require a tag and
 * a type and a permission and this and that... If any one of those pages
 * fails, then * possibly all of them need to be removed. The journal is
 * here to take care of that.
 * 
 * Note that the Destroy Page process happens in the
 * backend_process_journal() which uses the journal table to know when
 * a page URL that was eventually partially created timed out.
 *
 * \sa backend_process_journal()
 * \sa create_page()
 * \sa finish_all_journals()
 * \sa journal_list::done()
 */


// self
//
#include "content.h"


// snapwebsites lib
//
#include <snapwebsites/log.h>


// last include
//
#include <snapdev/poison.h>




SNAP_PLUGIN_EXTENSION_START(content)


/** \brief Get one Journal List to create one page.
 *
 * This function allocates a Journal and returns its pointer.
 *
 * Once you have the URL of the page you are working on, call the
 * journal_list::add_page_url() function.
 *
 * If you created more than one page, you may call the add_page_url()
 * multiple times or get another journal_list for each one of them.
 * You can call the add_page_url() funciton any number of times until
 * you call the done() function.
 *
 * \note
 * The function never returns a nullptr. If the journal_list can't be
 * allocated, it will throw.
 *
 * \warning
 * DO NOT CREATE AN RAII object to ensure "deletion" (call to done())
 * since we do NOT want such to happen if the creation of a page doesn't
 * finish as expected.
 *
 * \return A pointer to a journal_list.
 *
 * \sa journal_list::add_page_url()
 * \sa journal_list::done()
 */
journal_list * content::get_journal_list()
{
    journal_list * journal( new journal_list(f_snap) );
    f_to_process.push_back( journal );
    f_journal_list_stack.push( journal );
    SNAP_LOG_DEBUG("content::get_journal_list(): created new journal_list: ")
            (" f_to_process.size()=")(f_to_process.size())
            (", f_journal_list_stack.size()=")(f_journal_list_stack.size());

    return journal;
}


/** \brief Pop the Journal List you allocated with get_journal_list.
 *
 * This function is called whenever you call the journal_list::done()
 * function. It is private so you can't call it yourself for good reasons.
 *
 * Note that if you create sub-pages, you must make sure to call the
 * functions in the correct order.
 *
 * \code
 *    journal_list * journal(get_journal_list());
 *
 *    // create a sub-page here
 *    ...
 *
 *    journal->add_page_url(url);
 *
 *    // or create a sub-page there
 *    ...
 *
 *    journal->done();
 * \endcode
 *
 * If you creaet several pages within the same function, you may call
 * the add_page_url() multiple times to add all the pages URLs. If you
 * have sub-functions that can be called directly or from other page
 * creation functions, you want to create a new journal_list object
 * and use that within those sub-functions.
 *
 * \param[in] journal  The journal we expect to pop (for verification.)
 *
 * \sa journal_list::add_page_url()
 * \sa journal_list::done()
 */
void content::journal_list_pop(journal_list * journal)
{
    if( f_journal_list_stack.top() != journal )
    {
        throw content_exception_invalid_sequence("you are trying to pop the wrong journal, they must be popped in the opposite order that they were created.");
    }

    f_journal_list_stack.pop();
    SNAP_LOG_DEBUG("f_journal_list_stack.size() after pop() =")(f_journal_list_stack.size());
    //
    if( f_journal_list_stack.empty() )
    {
        finish_all_journals();
    }
}


/** \brief Mark the pages as done.
 *
 * This function gets called automatically once all the journals of a
 * page creation were popped.
 *
 * Finishing the work means marking the pages as done which means
 * deleting the URLs from the journal table in our Cassandra database.
 *
 * This function is private to make sure it only gets called as required.
 *
 * \sa journal_list::finish_pages()
 */
void content::finish_all_journals()
{
    SNAP_LOG_DEBUG("++++ f_to_process.size()=")(f_to_process.size());
    for( auto & list : f_to_process )
    {
        list->finish_pages();
        delete list;
    }

    f_to_process.clear();
}


/** \brief Initialize a journal_list object.
 *
 * This function saves the snap pointer and get a pointer to the
 * Cassandra "journal" table.
 *
 * At this point, the journal is not tracking anything. To start
 * tracking a page creation you want to call the add_page_url()
 * function with your page URL.
 *
 * \param[in] snap  A pointer back to the snap_child object.
 *
 * \sa add_page_url()
 * \sa done()
 */
journal_list::journal_list( snap_child * snap )
    : f_snap(snap)
    , f_journal_table( f_snap->get_table(get_name(name_t::SNAP_NAME_CONTENT_JOURNAL_TABLE)) )
{
}


/** \brief Destroys a journal_list object.
 *
 * This function checks whether the URL list was properly emptied. If not,
 * then it emits an error. In debug mode, it even terminates the process
 * right there so we have a much better chance to detect a problem (i.e.
 * the HTML output won't be generated yet by then and thus we get a
 * WSOD in the browser.)
 */
journal_list::~journal_list()
{
    if(!f_url_list.isEmpty())
    {
#ifdef _DEBUG
        SNAP_LOG_FATAL("URL list is not empty in journal_list::~journal_list(), did you call done()?");
        std::cerr << "See the logs for more info." << std::endl;
        std::terminate();
#else
        SNAP_LOG_ERROR("URL list is not empty in journal_list::~journal_list(), did you call done()?");
#endif
    }
}


/** \brief Flag a page for creation in the journal, with timestamp.
 *
 * This creates an entry in the journal table before page completion.
 * It saves the start date from the snap_child instance, along with the URL.
 *
 * To detect that the page creation failed, we use the start date in a
 * backend to see that the page creation attempt was somewhat in the past.
 * If far enough in the past, whatever exists in the failed page gets
 * deleted.
 *
 * \exception content_exception_content_invalid_state
 * If the done() function was called, this exception is raised. This is
 * a missuse of this class.
 *
 * \param[in] full_url  The URL to page being created.
 *
 * \sa content::backend_process_journal()
 * \sa content::finish_all_journals()
 * \sa done()
 */
void journal_list::add_page_url( QString const & full_url )
{
    if( f_finished_called )
    {
        throw content_exception_content_invalid_state( "journal_list is one use only!" );
    }

    auto field_timestamp ( get_name(name_t::SNAP_NAME_CONTENT_JOURNAL_TIMESTAMP)  );
    auto field_url       ( get_name(name_t::SNAP_NAME_CONTENT_JOURNAL_URL)        );

    auto current_journal_row( f_journal_table->getRow(full_url) );
    current_journal_row->getCell(field_timestamp) ->setValue( libdbproxy::value(f_snap->get_start_date()) );
    current_journal_row->getCell(field_url)       ->setValue( full_url );

    f_url_list << full_url;
}


/** \brief Mark this journal_list as done.
 *
 * Pop this current item off of the journal stack.
 *
 * If multiple journal_list were created, then nothing more happens.
 *
 * If the journal_list was the last one, then the content plugin calls
 * all the finish functions which removes the URLs from the journal table.
 * This allows for those pages to survive as expected (i.e. the creation
 * of the pages succeeded.
 *
 * In effect, this mechanism allows us to create one page that depends on
 * several other pages being created along this one. If the creation of
 * any one of those pages fails, then they all will get deleted later
 * because the finish_pages() never gets called if all the journal_list
 * done() function don't all get called.
 *
 * \note
 * We do not currently enforce the order in which the journal_list
 * get created and then released with a call to the done() function.
 * However, it is expected to be in the same order as we use a stack.
 *
 * \exception content_exception_content_invalid_state
 * If the done() function was already called, this exception is raised. This
 * is a missuse of this class. Note that the object may even already have
 * been deleted by now...
 *
 * \sa done()
 * \sa finish_pages()
 */
void journal_list::done()
{
    SNAP_LOG_DEBUG("done with this journal_list object");

    // we cannot call done() twice; the list cannot be popped
    // more than once and still have the system working, not only
    // that the journal_list may have been deleted in between
    //
    if( f_finished_called )
    {
        throw content_exception_content_invalid_state( "journal_list done() cannot be called more than once; it may already have been deleted!" );
    }

    // prevent further add_page_url() calls
    //
    f_finished_called = true;

    // pop from the stack
    //
    content::content::instance()->journal_list_pop(this);
}


/** \brief Finish the pages marked for creation.
 *
 * The current page URL entry is taken out of the journal table. This
 * means the page creation succeeded and as a result the page won't
 * get deleted.
 *
 * This function gets called by the content plugin for all the journal
 * of a group of pages that were created in a row. So whether there
 * were several journal_list or just one with many calls to the
 * add_page_url() function, it gets called once at the end of the
 * entire process which does not 100% ensure that the journal is
 * properly managed, but it should be 99.9999% of the time at least.
 *
 * \sa content::get_journal_list()
 * \sa content::finish_all_journals()
 * \sa content::backend_process_journal()
 * \sa add_page_url()
 */
void journal_list::finish_pages()
{
    SNAP_LOG_DEBUG("++++ f_url_list=")(f_url_list.size());

    // prevent further add_page_url() calls
    //
    f_finished_called = true;

    for( auto url : f_url_list )
    {
        // Drop the row, since it is completed and we are done.
        //
        SNAP_LOG_DEBUG("+++ dropping url=")(url);
        try
        {
            f_journal_table->dropRow(url);
        }
        catch(libdbproxy::exception & e)
        {
            SNAP_LOG_WARNING("an exception occurred while dropping journal_list rows: ")(e.what());
        }
    }

    f_url_list.clear();
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
