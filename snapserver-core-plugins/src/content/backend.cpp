// Snap Websites Server -- all the user content and much of the system content
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
 * \brief The implementation of the content plugin class backend parts.
 *
 * This file contains the implementation of the various content backend
 * functions of the content plugin.
 */

#include "content.h"

#include <snapwebsites/compression.h>
#include <snapwebsites/dbutils.h>
#include <snapwebsites/log.h>

#include <csspp/assembler.h>
#include <csspp/compiler.h>
#include <csspp/exceptions.h>
#include <csspp/parser.h>

#include <iostream>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(content)


/** \brief Register the various content actions.
 *
 * This function registers this plugin as supporting various content
 * actions as listed below.
 *
 * This can be used by an administrator to force a reset of all the statuses
 * of all the nodes, see the list of available resources to a website,
 * destroy a page (as in do not even trash it,) etc.
 *
 * \li content::destroypage -- completely eliminate a page; this is
 *     considered VERY DANGEROUS; use at your own risk! That being said,
 *     quite practical for programmers so they don't have to reset their
 *     database all the time. The page to be destroyed MUST be specified
 *     as the parameter PAGE_URL. We did not try, but you certainly can
 *     destroy "/" which means the entire website will go away and not
 *     function at all anymore.
 * \li content::dirresources -- show a directory of the resources; this is
 *     done here so you can see the available resources once all the
 *     plugins of a given website are
 * \li content::extractresource -- write a resource file from the library
 *     or a plugin to a file.
 * \li content::forceresetstatus -- this is similar to the reset status only
 *     it resets all the pages whatever the current state; this means a
 *     page that's hidden or deleted will become normal again
 * \li content::newfile -- add the specified md5 to the new row of the
 *     files table so that way that file will get rescanned and reprocessed
 *     in case it were necessary to do so (because you changed the code,
 *     for example.)
 * \li content::rebuildindex -- this action requests the system to rebuild
 *     the entire '*index*' row of the content table.
 * \li content::resetstatus -- go through all the pages of a website and
 *     reset their status to Normal. This should be used by programmers
 *     when they make a mistake and mess up an entry; pages that are
 *     marked as Normal + something else will be changed to Normal
 *     + Not Working
 * \li content::snapbackend -- this is a special case which is used to
 *     implement the standard CRON backend processes; it calls the
 *     server::backend_process() signal and returns immediately after
 *
 * \note
 * To extract the content of one cell, use the snapsiteinfo tool instead:
 *
 * \code
 *      snapsiteinfo --host 127.0.0.1 \
 *                  --count 20000 \
 *                  --table revision \
 *                  http://my-domain.extension/and/path/... field::name \
 *                  --save-cell a.bin
 * \endcode
 *
 * \param[in,out] actions  The list of supported actions where we add ourselves.
 */
void content::on_register_backend_action(server::backend_action_set & actions)
{
    // this first one is a "special case" which is used to run
    // the CRON-like backend
    //
    actions.add_action(snap::get_name(snap::name_t::SNAP_NAME_CORE_SNAPBACKEND), this);

    actions.add_action(get_name(name_t::SNAP_NAME_CONTENT_RESETSTATUS),      this);
    actions.add_action(get_name(name_t::SNAP_NAME_CONTENT_FORCERESETSTATUS), this);
    actions.add_action(get_name(name_t::SNAP_NAME_CONTENT_DIRRESOURCES),     this);
    actions.add_action(get_name(name_t::SNAP_NAME_CONTENT_EXTRACTRESOURCE),  this);
    actions.add_action(get_name(name_t::SNAP_NAME_CONTENT_DESTROYPAGE),      this);
    actions.add_action(get_name(name_t::SNAP_NAME_CONTENT_NEWFILE),          this);
    actions.add_action(get_name(name_t::SNAP_NAME_CONTENT_REBUILDINDEX),     this);
}


/** \brief Process various backend tasks.
 *
 * The list of backend processes are defined in the
 * on_register_backend_action() function.
 *
 * \param[in] action  The action requested.
 */
void content::on_backend_action(QString const & action)
{
    if(action == snap::get_name(snap::name_t::SNAP_NAME_CORE_SNAPBACKEND))
    {
        // special case to handle the standard backend processes that
        // run through the snapinit CRON mechanism
        //
        f_snap->backend_process();
    }
    else if(action == get_name(name_t::SNAP_NAME_CONTENT_RESETSTATUS))
    {
        backend_action_reset_status(false);
    }
    else if(action == get_name(name_t::SNAP_NAME_CONTENT_FORCERESETSTATUS))
    {
        backend_action_reset_status(true);
    }
    else if(action == get_name(name_t::SNAP_NAME_CONTENT_DIRRESOURCES))
    {
        backend_action_dir_resources();
    }
    else if(action == get_name(name_t::SNAP_NAME_CONTENT_EXTRACTRESOURCE))
    {
        backend_action_extract_resource();
    }
    else if(action == get_name(name_t::SNAP_NAME_CONTENT_DESTROYPAGE))
    {
        backend_action_destroy_page();
    }
    else if(action == get_name(name_t::SNAP_NAME_CONTENT_NEWFILE))
    {
        backend_action_new_file();
    }
    else if(action == get_name(name_t::SNAP_NAME_CONTENT_REBUILDINDEX))
    {
        backend_action_rebuild_index();
    }
    else
    {
        // unknown action (we should not have been called with that name!)
        throw snap_logic_exception(QString("content.cpp:on_backend_action(): content::on_backend_action(\"%1\") called with an unknown action...").arg(action));
    }
}


/** \brief Reset the status of all pages.
 *
 * This function goes through the list of all pages in your website
 * and resets the status. When creating a page, the status is set in
 * such a way that the page cannot be changed by other processes.
 * Only, if your creation process fails, which happens... then
 * the page remains in an inconsistent state and it cannot be accessed
 * or deleted. This process resets that state.
 *
 * This action does not use any parameter at this time.
 */
void content::backend_action_reset_status(bool const force)
{
    SNAP_LOG_TRACE("content::backend_action_reset_status(): Content status reset status.");

    libdbproxy::table::pointer_t content_table(get_content_table());
    content_table->clearCache();

    // TODO: use the '*index*' row instead of the entire content table

    auto row_predicate = std::make_shared<libdbproxy::row_predicate>();
    QString const site_key(f_snap->get_site_key_with_slash());
    // process 100 in a row
    row_predicate->setCount(100);
    for(;;)
    {
        uint32_t const count(content_table->readRows(row_predicate));
        if(count == 0)
        {
            // no more pages to process
            break;
        }
        libdbproxy::rows const rows(content_table->getRows());
        for(libdbproxy::rows::const_iterator o(rows.begin());
                o != rows.end(); ++o)
        {
            QString const key(QString::fromUtf8(o.key().data()));
            if(key.startsWith(site_key)) // filter out other websites... (dead slow since we are reading ALL the rows to only process one website!)
            {
                path_info_t ipath;
                ipath.set_path(key);
                if(content_table->getRow(ipath.get_key())->exists(get_name(name_t::SNAP_NAME_CONTENT_STATUS)))
                {
                    // do not use the normal interface, force any normal (something) to normal (normal)
                    libdbproxy::value status(content_table->getRow(ipath.get_key())->getCell(get_name(name_t::SNAP_NAME_CONTENT_STATUS))->getValue());
                    if(status.nullValue())
                    {
                        // no valid status, mark the page as normal
                        int32_t s(static_cast<unsigned char>(static_cast<int>(path_info_t::status_t::state_t::NORMAL)));
                        status.setInt32Value(s);
                        content_table->getRow(ipath.get_key())->getCell(get_name(name_t::SNAP_NAME_CONTENT_STATUS))->setValue(status);
                    }
                    else
                    {
                        int32_t const current_status(status.int32Value());
                        int32_t s(current_status);
                        switch(s & 0xFF)
                        {
                        case static_cast<int>(path_info_t::status_t::state_t::NORMAL):
                        case static_cast<int>(path_info_t::status_t::state_t::HIDDEN):
                        case static_cast<int>(path_info_t::status_t::state_t::MOVED):
                        case static_cast<int>(path_info_t::status_t::state_t::DELETED):
                            break;

                        default:
                            // force back to normal if not considered
                            // valid (so keep normal, hidden, moved,
                            // and deleted pages as is)
                            s = (current_status & ~0x000FF) | static_cast<int>(path_info_t::status_t::state_t::NORMAL);
                            break;

                        }
                        // the working status is always reset to "not working"
                        //s = (s & ~0x0FF00) | (static_cast<int>(path_info_t::status_t::working_t::NOT_WORKING) * 256);

                        if(force || s != current_status)
                        {
                            status.setInt32Value(s);
                            content_table->getRow(ipath.get_key())->getCell(get_name(name_t::SNAP_NAME_CONTENT_STATUS))->setValue(status);
                        }
                    }
                }
            }
        }
    }
}


/** \brief Backend function to display the list of resources.
 *
 * This function lists the name of all the resources available in this
 * backend when the current website plugins are all loaded.
 *
 * This is useful to debug your code and make sure that all the resources
 * you expect to be available are.
 *
 * This action does not use any parameter at this time.
 */
void content::backend_action_dir_resources()
{
    f_snap->show_resources(std::cout);
}


/** \brief Extract a specific resource file and save it to a file.
 *
 * This function is used to read a resource file and save to a file
 * the user specifies. This is useful to verify that a file made it
 * to your server.
 *
 * The function expects a couple of parameters:
 *
 * \li RESOURCE_NAME -- the resource name as it appears in the
 *                      content::dirresources output.
 * \li OUTPUT_FILENAME -- the name where the file gets saved.
 */
void content::backend_action_extract_resource()
{
    QString const resource_name(f_snap->get_server_parameter("RESOURCE_NAME"));
    QString const output_filename(f_snap->get_server_parameter("OUTPUT_FILENAME"));
    f_snap->extract_resource(resource_name, output_filename);
}


/** \brief Destroy the specified page.
 *
 * This action lets a programmer or administrator destroy a page
 * completely. This actual blows up the page right there and
 * should NOT be used, ever, except by programmers who made small
 * mistakes and want to remove a page or two once in a while.
 *
 * This action makes use of the following parameters:
 *
 * \li PAGE_URL -- the URL to the page to be destroyed.
 */
void content::backend_action_destroy_page()
{
    path_info_t ipath;
    ipath.set_path(f_snap->get_server_parameter("PAGE_URL"));
    destroy_page(ipath);
}


/** \brief Mark a file as new so the backend process reprocess it.
 *
 * The content backend processes new files to determine whether they
 * are in need a compression and minification (optimization for CSS
 * and JavaScript.)
 *
 * If your code somehow changed, then you may need to mark some files
 * as in need for reprocessing.
 *
 * The action makes use of the following parameters:
 *
 * \li MD5 -- the md5 of the file to be pinged.
 *
 * \todo
 * Create a version of this action to reset all the files in case something
 * went wrong and all the files require a kick in the rear.
 */
void content::backend_action_new_file()
{
    libdbproxy::table::pointer_t files_table(get_files_table());
    libdbproxy::row::pointer_t new_row(files_table->getRow(get_name(name_t::SNAP_NAME_CONTENT_FILES_NEW)));

    QString const md5(f_snap->get_server_parameter("MD5"));
    QByteArray const key(dbutils::string_to_key(md5));
    unsigned char const changed(1);
    new_row->getCell(key)->setValue(changed);

    // we also have to reset all the reference back to 1 instead of 2
    // otherwise nothing happens...
    //
    libdbproxy::row::pointer_t file_row(files_table->getRow(key));
    file_row->clearCache();

    auto reference_column_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
    reference_column_predicate->setStartCellKey(get_name(name_t::SNAP_NAME_CONTENT_FILES_REFERENCE));
    reference_column_predicate->setEndCellKey(get_name(name_t::SNAP_NAME_CONTENT_FILES_REFERENCE) + QString(";"));
    reference_column_predicate->setCount(100);
    reference_column_predicate->setIndex(); // behave like an index
    unsigned char const one(1);
    for(;;)
    {
        file_row->readCells(reference_column_predicate);
        libdbproxy::cells const content_cells(file_row->getCells());
        if(content_cells.isEmpty())
        {
            break;
        }
        // handle one batch
        for(libdbproxy::cells::const_iterator cc(content_cells.begin());
                cc != content_cells.end();
                ++cc)
        {
            // get the email from the database
            // we expect empty values once in a while because a dropCell() is
            // not exactly instantaneous in Cassandra
            libdbproxy::cell::pointer_t content_cell(*cc);
            content_cell->setValue(one);
        }
    }
}


/** \brief Go through all the pages and rebuild the '*index*'.
 *
 * This function goes through all the pages defined in the content table
 * and rebuilds the '*index*' row.
 *
 * It also goes through the '*index*' row and makes sure all the pages
 * still exist in the content table.
 *
 * \warning
 * This code is website agnostic. Meaning that it runs against all the
 * websites of your Cassandra cluster.
 *
 * \warning
 * This action should be run against one specific and currently valid
 * website. Otherwise, it will run over the entire database once per
 * website instead of just once.
 *
 * \todo
 * Make sure to remember to add some code to run this process once in a
 * while, like once a month, in our backend process.
 */
void content::backend_action_rebuild_index()
{
    libdbproxy::table::pointer_t content_table(get_content_table());

    // first loop: check whether some entries in the content table were
    //             not properly defined in the index
    //
    // TODO: note that in the current implementation, it could very
    //       well mean that a function of the create_content() signal
    //       threw and thus that the page is not all proper... we will
    //       want to check on that and fix the signal behavior at
    //       some (i.e. create_content() is a function, it calls another
    //       signal with a try/catch and if that signal fails, make sure
    //       to destroy the intermediate/invalid page.)
    //
    {
        libdbproxy::value ready;
        ready.setSignedCharValue(1);

        content_table->clearCache();

        auto row_predicate = std::make_shared<libdbproxy::row_predicate>();
        // process 100 in a row
        row_predicate->setCount(100);
        for(;;)
        {
            uint32_t const count(content_table->readRows(row_predicate));
            if(count == 0)
            {
                // no more lists to process
                break;
            }
            libdbproxy::rows const rows(content_table->getRows());
            for(libdbproxy::rows::const_iterator o(rows.begin());
                    o != rows.end(); ++o)
            {
                QString const key(QString::fromUtf8(o.key().data()));
                if(key.isEmpty()
                || key[0] == '*')
                {
                    // skip the '*index*' row and any other similar row
                    // we add later
                    continue;
                }

                // TBD: do we need to check that the "content::created" field
                //      exists? I think it is a good safety net here... but
                //      then we probably need a process to remove pages without
                //      that field.
                //
                path_info_t ipath;
                ipath.set_path(key);
                if(content_table->getRow(ipath.get_key())->exists(get_name(name_t::SNAP_NAME_CONTENT_CREATED)))
                {
                    content_table->getRow(get_name(name_t::SNAP_NAME_CONTENT_INDEX))->getCell(ipath.get_key())->setValue(ready);
                }
                //else -- should we put those pages in a '*broken*' row
                //        so we can run another process to clean up the
                //        database of those broken pages
            }
        }
    }

    // second loop: check whether some entries in the index were
    //              removed from the content table by now (i.e. see the
    //              destroy_page() signal)
    //
    {
        libdbproxy::row::pointer_t row(content_table->getRow(get_name(name_t::SNAP_NAME_CONTENT_INDEX)));
        row->clearCache();

        auto column_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
        column_predicate->setCount(100);
        column_predicate->setIndex(); // behave like an index
        for(;;)
        {
            row->readCells(column_predicate);
            libdbproxy::cells const cells(row->getCells());
            if(cells.isEmpty())
            {
                break;
            }
            for(libdbproxy::cells::const_iterator c(cells.begin());
                    c != cells.end();
                    ++c)
            {
                QString const key(QString::fromUtf8(c.key().data()));

                path_info_t ipath;
                ipath.set_path(key);
                if(!content_table->getRow(ipath.get_key())->exists(get_name(name_t::SNAP_NAME_CONTENT_CREATED)))
                {
                    row->dropCell(ipath.get_key());
                }
            }
        }
    }
}





/** \brief Process various backend tasks.
 *
 * Content backend processes:
 *
 * \li Reset the status of pages that somehow got a working status
 *     but that status never got reset.
 *
 * \li Check new attachements as those files may be or include viruses.
 */
void content::on_backend_process()
{
    backend_process_status();
    backend_process_files();
    //
    // TODO: use "get_server_parameter()" function
    // to read such parameters. You get a QString that you can change to a number
    // with toInt() or toLong() or such.
    //
    QString const age_in_minutes(f_snap->get_server_parameter("backend_journal_age"));
    if( age_in_minutes.isEmpty() )
    {
        backend_process_journal( 5 ); // Five minute age default
    }
    else
    {
        backend_process_journal( age_in_minutes.toLongLong() );
    }
}


/** \brief Check whether a working process never reset its status.
 *
 * As the database is being worked on, the status of a page changes while
 * it gets processed. Unfortunately, once in while a page process breaks
 * and its status does not get restored as expected.
 *
 * The status handling saves the URI of the pages that get a status with
 * a working process in the processing table. The URI does not get deleted
 * for speed. This backend checks the pages, verifies the status and how
 * long it was set to a working state (if such is still the case) and
 * resets the working state to path_info_t::status_t::NOT_WORKING if the
 * working status was on for over 10 minutes.
 *
 * \note
 * A process that takes over 10 minutes can always update the date once a
 * minute or so to avoid getting erased by this backend. At this point the
 * 10 minutes was chosen somewhat arbitrarily and we may want to adjust
 * that with time and even possibly offer the administrator to change that
 * number for one's website.
 */
void content::backend_process_status()
{
    SNAP_LOG_TRACE("content::backend_process_status(): Content status auto adjustments.");

    libdbproxy::table::pointer_t content_table(get_content_table());
    libdbproxy::table::pointer_t processing_table(get_processing_table());
    processing_table->clearCache();

    // any page with this start date or less gets its processing state
    // reset by this backend; we may want the 10 minutes to be saved in
    // a site parameter so the administrator can tweak it...
    int64_t start_date(f_snap->get_start_date() - 10 * 60 * 1000000);

    // TODO: use the '*index*' row instead of the entire content table

    // only process files for the website currently being processed
    auto row_predicate = std::make_shared<libdbproxy::row_predicate>();
    QString const site_key(f_snap->get_site_key_with_slash());
    // These are not defined in alphabetical order, unfortunately
    //row_predicate->setStartRowName(site_key);
    //row_predicate->setEndRowName(site_key + libdbproxy::cell_predicate::last_char);
    // process 100 in a row
    row_predicate->setCount(100);
    for(;;)
    {
        uint32_t const count(processing_table->readRows(row_predicate));
        if(count == 0)
        {
            // no more lists to process
            break;
        }
        libdbproxy::rows const rows(processing_table->getRows());
        for(libdbproxy::rows::const_iterator o(rows.begin());
                o != rows.end(); ++o)
        {
            // TODO: we need to change this algorithm to run ONCE
            //       and not once per website, that being said, we
            //       are in a process initialized for site_key only
            //
            QString const key(QString::fromUtf8(o.key().data()));
            if(key.startsWith(site_key)) // filter out other websites... (dead slow since we are reading ALL the rows to only process one website!)
            {
                path_info_t ipath;
                ipath.set_path(key);
                if(content_table->exists(ipath.get_key())
                && content_table->getRow(ipath.get_key())->exists(get_name(name_t::SNAP_NAME_CONTENT_STATUS_CHANGED)))
                {
                    int64_t const last_changed(content_table->getRow(ipath.get_key())->getCell(get_name(name_t::SNAP_NAME_CONTENT_STATUS_CHANGED))->getValue().safeInt64Value());
                    if(last_changed < start_date)
                    {
                        // we are done with that page since we just reset the
                        // working status as expected so drop it (we do that first
                        // so in case it gets re-created in between, we will reset
                        // again later)
                        processing_table->dropRow(ipath.get_key());

                        // it has been more than 10 minutes, reset the state
                        path_info_t::status_t status(ipath.get_status());
                        status.set_status(static_cast<path_info_t::status_t::status_type>(content_table->getRow(ipath.get_key())->getCell(get_name(name_t::SNAP_NAME_CONTENT_STATUS))->getValue().uint32Value()));
                        if(status.get_state() == path_info_t::status_t::state_t::CREATE)
                        {
                            // a create failed, set it to normal...
                            // (should we instead set it to hidden and let the administrator know?)
                            status.set_state(path_info_t::status_t::state_t::NORMAL);
                            ipath.set_status(status);
                        }
                    }
                }
                else
                {
                    // the row was deleted in between... or something of
                    // the sort, just ignore that entry altogether
                    processing_table->dropRow(ipath.get_key());
                }
            }
        }
    }
}


/** \brief Process new attachments to make sure they are secure.
 *
 * As user upload new files to the server, we want to have them
 * processed in different ways. This backend process does part of
 * that work and allows other plugins to check files out to make
 * sure they are fine.
 *
 * Type of processes we are expecting to run against files:
 *
 * \li The Anti-Virus plugin checks that the file is not viewed as a
 *     virus using external tools such as clamscan. This is expected
 *     to be checked within the check_attachment_security() signal.
 *
 * \li The JavaScript plugin checks the syntax of all JavaScript files.
 *     It also minimizes them and save that minimized version.
 *
 * \li The Layout plugin checks the syntax of all the CSS files and
 *     it also minimizes them and save that minimized version.
 *
 * \li The layout plugin tries to fully load all Images, play movies,
 *     etc. to make sure that the files are valid. If that process
 *     fails, then the file is marked as invalid.
 *
 * When serving a file that is an attachment, plugins that own those
 * files are given a chance to serve the attachment themselves. If
 * they do, then the default code does not get used at all. This allows
 * plugins such as the JavaScript plugin to send their compressed and
 * minimized version of the file instead of the source version.
 *
 * \warning
 * This function generates two signals: check_attachment_security()
 * and process_attachment(). If your plugin can check the file for
 * security reason, implement the check_attachment_security(). In
 * all other cases, use the process_attachment(). It is important to
 * do that work in the right function because attempting to load a
 * virus or some other bad file may cause havoc on the server.
 *
 * \todo
 * The security checks may need to be re-run on all the files once
 * in a while since brand new viruses may not be detected when they
 * first get uploaded. One signal on that one could be to count the
 * number of time a file gets uploaded, if the counter increases
 * outregiously fast, it is probably not a good sign.
 *
 * \todo
 * When the process finds content that is considered non secure,
 * send an email to the content owner.
 */
void content::backend_process_files()
{
    SNAP_LOG_TRACE("content::backend_process_files(): Content file processing (check for viruses, etc.)");

    // TODO: look into a way to either handle all the files from
    //       all the sites all at once, or filter in a different
    //       way (instead of reading all and then only working
    //       on a few)

    QString const site_key(f_snap->get_site_key_with_slash());
    QByteArray const site_key_buffer(site_key.toUtf8());
    char const * site_key_utf8(site_key_buffer.data());

    libdbproxy::table::pointer_t files_table(get_files_table());
    libdbproxy::row::pointer_t new_row(files_table->getRow(get_name(name_t::SNAP_NAME_CONTENT_FILES_NEW)));
    new_row->clearCache();

// test this file over and over again until it worked
//new_row->getCell(QByteArray::fromHex("1f2bcb1bd25c07eb88373f7c9f50adb6"))->setValue(true);
//signed char const ref(1);
//files_table->getRow(QByteArray::fromHex("1f2bcb1bd25c07eb88373f7c9f50adb6"))->getCell("content::files::reference::http://csnap.m2osw.com/css/finball/style.css")->setValue(ref);
//std::cerr << "-------------------------------------- backend_process_files() START\n";

    QString const file_reference(QString::fromUtf8(get_name(name_t::SNAP_NAME_CONTENT_FILES_REFERENCE)));

    auto column_predicate(std::make_shared<libdbproxy::cell_range_predicate>());
    column_predicate->setCount(100); // should this be a parameter?
    column_predicate->setIndex(); // behave like an index
    for(;;)
    {
        new_row->readCells(column_predicate);
        libdbproxy::cells const new_cells(new_row->getCells());
        if(new_cells.isEmpty())
        {
            break;
        }
        // handle one batch
        for(libdbproxy::cells::const_iterator nc(new_cells.begin());
                nc != new_cells.end();
                ++nc)
        {
            // get the email from the database
            // we expect empty values once in a while because a dropCell() is
            // not exactly instantaneous in Cassandra
            bool drop_row(true);
            libdbproxy::cell::pointer_t new_cell(*nc);
            if(!new_cell->getValue().nullValue())
            {
                QByteArray file_key(new_cell->columnKey());

                libdbproxy::row::pointer_t file_row(files_table->getRow(file_key));
                file_row->clearCache();

                auto reference_column_predicate(std::make_shared<libdbproxy::cell_range_predicate>());
                reference_column_predicate->setStartCellKey(file_reference);
                reference_column_predicate->setEndCellKey(file_reference + ";");
                reference_column_predicate->setCount(100);
                reference_column_predicate->setIndex(); // behave like an index
                permission_flag secure;
                bool first(true); // load the files only once each
                for(;;)
                {
                    file_row->readCells(reference_column_predicate);
                    libdbproxy::cells const content_cells(file_row->getCells());
                    if(content_cells.isEmpty())
                    {
                        break;
                    }
                    // handle one batch
                    for(libdbproxy::cells::const_iterator cc(content_cells.begin());
                            cc != content_cells.end();
                            ++cc)
                    {
                        // get the email from the database
                        // we expect empty values once in a while because a dropCell() is
                        // not exactly instantaneous in Cassandra
                        libdbproxy::cell::pointer_t content_cell(*cc);
                        if(!content_cell->getValue().nullValue())
                        {
                            int8_t const reference_value(content_cell->getValue().signedCharValue());
                            if(reference_value == 1)
                            {
                                QByteArray attachment_key(content_cell->columnKey().data() + (strlen(get_name(name_t::SNAP_NAME_CONTENT_FILES_REFERENCE)) + 2),
                                         static_cast<int>(content_cell->columnKey().size() - (strlen(get_name(name_t::SNAP_NAME_CONTENT_FILES_REFERENCE)) + 2)));

                                if(attachment_key.startsWith(site_key_utf8))
                                {
                                    if(first)
                                    {
                                        first = false;

                                        attachment_file file(f_snap);
                                        if(!load_attachment(attachment_key, file, true))
                                        {
                                            SNAP_LOG_ERROR("the files backend could not load attachment at \"")(attachment_key.data())("\".");

                                            // always save the secure flag
                                            //
                                            signed char const sflag(CONTENT_SECURE_UNDEFINED);
                                            file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURE))->setValue(sflag);
                                            file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURE_LAST_CHECK))->setValue(f_snap->get_start_date());
                                            file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURITY_REASON))->setValue(QString("Attachment could not be loaded from database."));

                                            // TODO generate a message about the error...
                                        }
                                        else
                                        {
                                            check_attachment_security(file, secure, false);

                                            // always save the secure flag
                                            //
                                            signed char const sflag(secure.allowed() ? CONTENT_SECURE_SECURE : CONTENT_SECURE_INSECURE);
                                            file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURE))->setValue(sflag);
                                            file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURE_LAST_CHECK))->setValue(f_snap->get_start_date());
                                            file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SECURITY_REASON))->setValue(secure.reason());

                                            if(secure.allowed())
                                            {
                                                // only process the attachment further if it is
                                                // considered secure
                                                //
                                                process_attachment(file_row, file);
                                            }
                                        }
                                    }
                                    if(!secure.allowed())
                                    {
                                        // TODO: warn the author that his file was
                                        //       quanranteened and will not be served;
                                        //       this should send a message and not
                                        //       a direct email...
                                        //
                                        // TBD: we also want to choose whether we
                                        //      send the message once per instance
                                        //      (since each instance may be a different
                                        //      user) or just once for all instances
                                        //
                                        //...sendmail()...
                                    }

                                    // mark that reference as checked
                                    int8_t const reference_checked(2);
                                    content_cell->setValue(reference_checked);
                                }
                                else
                                {
                                    // check whether all references were checked
                                    // because if not we need to not drop that
                                    // row (not yet)
                                    //
                                    drop_row = false;
                                }
                            }

                        }
                    }
                }
            }
            // we are done with that file, remove it from the list of new files
            if(drop_row)
            {
                new_row->dropCell(new_cell->columnKey());
            }
        }
    }
}


/** \brief Process journal entries for new pages and remove aged out entries
 *
 * When a new page group is created we want to make sure it completes "all the way".
 * I say "page group" because we can have a complex page being created, which entails
 * perhaps other dependency pages, plus any links and permissions.
 *
 * For example, imagine that you have a system that tracks a company for your user. The
 * company itself has other pages of content which will be created on company page creation
 * and linked to the company in the database (vendors, profit centers, location data,
 * customers, etc). At any point in the creation, it's possible it can fail, and that would
 * leave the page group partially created.
 *
 * We want to be able to treat this more atomically, so the journal table comes to the rescue.
 * When you start to create a page, add an entry into the journal table using content::journal_create_page(url).
 * Then when you finish, at the bottom of your creation procedure, call content::journal_finish_page().
 *
 * If your procedure fails, and throws an exception, that entry will sit with the starting date timestamp.
 * This process will find any that have aged out, and will call the content::destroy_page() signal.
 * Also, the entry will be removed.
 *
 * \sa journal_create_page(), journal_finish_page()
 */

void content::backend_process_journal( int64_t const age_in_minutes )
{
    f_snap->get_context()->clearCache();

    auto journal_table   ( f_snap->get_table(get_name(name_t::SNAP_NAME_CONTENT_JOURNAL_TABLE)) );
    auto field_timestamp ( get_name(name_t::SNAP_NAME_CONTENT_JOURNAL_TIMESTAMP)  );
    auto field_url       ( get_name(name_t::SNAP_NAME_CONTENT_JOURNAL_URL)        );

    auto row_predicate( std::make_shared<libdbproxy::row_predicate>() );
    row_predicate->setCount(100);

    // five minutes in the past
    //
    int64_t const aged_out_time(f_snap->get_start_date() - (age_in_minutes * 60LL * 1000000LL));

    // Now go through and find all user rows and change them to ids. Add index entries.
    //
    for( ;; )
    {
        uint32_t const count(journal_table->readRows(row_predicate));
        if( count == 0 )
        {
            // last page was processed, done.
            break;
        }

        QStringList pages_to_destroy;
        auto row_list(journal_table->getRows());
        for( QByteArray const & row_key : row_list.keys() )
        {
            auto row( row_list[row_key] );
            QString const url       ( row->getCell(field_url)->getValue().stringValue()      );
            int64_t const timestamp ( row->getCell(field_timestamp)->getValue().int64Value() );
            SNAP_LOG_DEBUG("++++ row_key=")(row_key.data())(", url=")(url);
            SNAP_LOG_DEBUG("++++++ timestamp = ")(timestamp)(", aged_out_time = ")(aged_out_time);
            if( timestamp < aged_out_time )
            {
                SNAP_LOG_DEBUG("++++++++ DESTROYING URL=")(url);

                // Mark these pages for destruction
                //
                pages_to_destroy << url;

                // Drop row since it's older than 5 minutes
                //
                journal_table->dropRow(row_key);
            }
        }

        f_snap->get_context()->clearCache();
        auto content_table( f_snap->get_table(get_name(name_t::SNAP_NAME_CONTENT_TABLE)) );

        for( auto url : pages_to_destroy )
        {
            // Destroy the path since it didn't complete
            //
            try
            {
                //QString const & page_key( f_snap->get_site_key_with_slash() + url );
                //if( content_table->exists(page_key) )
                //
                path_info_t ipath;
                ipath.set_path(url);
                SNAP_LOG_DEBUG("destroying page=")(ipath.get_key());
                destroy_page( ipath );
            }
            catch( std::exception const & x )
            {
                SNAP_LOG_ERROR( "Exception caught while trying to destroy page [")(url)("], what=")(x.what());
            }
            catch( ... )
            {
                SNAP_LOG_ERROR( "Unknown exception caught while trying to destroy page [")(url)("]!");
            }
        }

        f_snap->get_context()->clearCache();
    }
}


/** \brief Check the attachment for one thing or another.
 *
 * The startup function generates a compressed version of the file using
 * gzip as the compression mode.
 *
 * \param[in] file_row  The row to the new file being processed.
 * \param[in] file  The file being processed.
 */
bool content::process_attachment_impl(libdbproxy::row::pointer_t file_row, attachment_file const & file)
{
    backend_compressed_file(file_row, file);
    backend_minify_css_file(file_row, file);

    return true;
}


/** \brief Compress an attachment.
 *
 * The backend processing new files will ensure that backend files get
 * compressed by calling this function through the process_attachment()
 * signal startup function.
 *
 * The function compresses the file and saves the compressed version in
 * the database. It will be a lot faster to read compressed files
 * over time (since it is sent over the network from the database to the
 * child process) and especially to send them over to clients (Again
 * that goes over the network, usually a slower one than the backend
 * talking with the database.) So the database will grow more than
 * necessary, but it will overall be faster to use.
 *
 * Note that files that do not compress to a smaller size (i.e. a JPEG
 * image) are left alone and the compressed size is created as 0 meaning
 * that the file could not be compressed.
 *
 * \param[in] file_row  The row to the new file being processed.
 * \param[in] file  The file being processed.
 */
void content::backend_compressed_file(libdbproxy::row::pointer_t file_row, attachment_file const & file)
{
    if(!file_row->exists(get_name(name_t::SNAP_NAME_CONTENT_FILES_SIZE_GZIP_COMPRESSED)))
    {
        QString compressor_name("gzip");
        QByteArray compressed_file(compression::compress(compressor_name, file.get_file().get_data(), 100, false));
        if(compressor_name == "gzip")
        {
            // compression succeeded
            file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_DATA_GZIP_COMPRESSED))->setValue(compressed_file);
            uint32_t const compressed_size(compressed_file.size());
            file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SIZE_GZIP_COMPRESSED))->setValue(compressed_size);
        }
        else
        {
            // no better when compressed, mark such with a size of zero
            uint32_t const empty_size(0);
            file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SIZE_GZIP_COMPRESSED))->setValue(empty_size);
        }
    }
}


/** \brief Minify a CSS file and compress it.
 *
 * The backend processing new files wants to minify CSS files. This
 * function gets called each time a new file is uploaded to the database.
 * The function checks the extension of the file, if it is CSS, then it
 * gets parsed by the csspp library. If the parsing and compiling works,
 * then it gets saved minified.
 *
 * The minified also gets compressed by gzip and saved as a minified
 * compressed version of the file.
 *
 * If we ever create a CSS plugin (i.e. to let the end users edit CSS,
 * for example) we certainly should move this processing in that
 * plugin instead.
 *
 * \param[in] file_row  The row to the new file being processed.
 * \param[in] file  The file being processed.
 */
void content::backend_minify_css_file(libdbproxy::row::pointer_t file_row, attachment_file const & file)
{
    bool const is_css(file.get_parent_cpath().startsWith("css/"));
    if(is_css)
    {
        // this is considered a CSS file
        std::stringstream error_output;
        csspp::error::instance().set_error_stream(error_output);
        QByteArray data(file.get_file().get_data());
        std::stringstream in;
        in << data.data();
        csspp::position pos(file.get_file().get_filename().toUtf8().data());
        csspp::lexer::pointer_t l(new csspp::lexer(in, pos));
        csspp::error_happened_t error_tracker;
        csspp::parser p(l);
        csspp::node::pointer_t root(p.stylesheet());
        if(!error_tracker.error_happened())
        {
            csspp::compiler c;
            c.set_root(root);
            c.set_date_time_variables(time(nullptr));
            QString const csspp_paths(f_snap->get_server_parameter("csspp_scripts"));
            snap_string_list const path_list(csspp_paths.split(':'));
            int const max_paths(path_list.size());
            for(int i(0); i < max_paths; ++i)
            {
                c.add_path(path_list[i].toUtf8().data());
            }
            try
            {
                c.compile(false);
            }
            catch(std::exception const & e)
            {
                // this happens when a fatal error occurs, in most
                // cases it will be an exit exception
            }
            if(!error_tracker.error_happened())
            {
                std::stringstream out;
                csspp::assembler a(out);
                a.output(c.get_root(), csspp::output_mode_t::COMPRESSED);
                if(!error_tracker.error_happened())
                {
                    // it all worked so save the result
                    // (the filename should be <filename>.min.css for this specific entry)
                    std::string const result(out.str());
                    QByteArray minified(result.c_str(), result.length());
                    file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_DATA_MINIFIED))->setValue(minified);
                    uint32_t const minified_size(minified.size());
                    file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SIZE_MINIFIED))->setValue(minified_size);

                    // now attempt to compress (it should pretty much always
                    // get compressed since it is text)
                    QString compressor_name("gzip");
                    QByteArray compressed_file(compression::compress(compressor_name, minified, 100, false));
                    if(compressor_name == "gzip")
                    {
                        // compression succeeded
                        file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_DATA_MINIFIED_GZIP_COMPRESSED))->setValue(compressed_file);
                        uint32_t const compressed_size(compressed_file.size());
                        file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SIZE_MINIFIED_GZIP_COMPRESSED))->setValue(compressed_size);
                    }
                    else
                    {
                        // no better when compressed, mark such with a size of zero
                        uint32_t const empty_size(0);
                        file_row->getCell(get_name(name_t::SNAP_NAME_CONTENT_FILES_SIZE_MINIFIED_GZIP_COMPRESSED))->setValue(empty_size);
                    }
                }
            }
        }
        std::string const messages(error_output.str());
        if(!messages.empty())
        {
            if(error_tracker.error_happened())
            {
                SNAP_LOG_ERROR("backend_process: CSS compiler errors: ")(messages);
            }
            else if(error_tracker.warning_happened())
            {
                SNAP_LOG_WARNING("backend_process: CSS compiler warnings: ")(messages);
            }
            else
            {
                SNAP_LOG_INFO("backend_process: CSS compiler messages: ")(messages);
            }
        }
    }
}




SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
