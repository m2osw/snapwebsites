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
 * \brief The implementation of the content plugin class cloning functionality.
 *
 * A page can be cloned for various reasons:
 *
 * * change the path to the page, in this case you want to move the page
 * * to delete the page, this is generally done by moving the page to
 *   the trashcan (so this is a move page too!)
 */

#include "content.h"

#include <snapwebsites/dbutils.h>
#include <snapwebsites/log.h>

#include <iostream>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(content)


/** \brief Destroy a page.
 *
 * \warning
 * This function DESTROYS a page RECURSIVELY. So the specified page and
 * all the children of that page will ALL get DESTROYED.
 *
 * This function can be used to DESTROY a page.
 *
 * 99.99% of the time, you should use the trash_page() function which
 * will safely move the existing page to the trashcan and destroy the
 * data only at a later time.
 *
 * In many other systesm, this function would probably be just called
 * delete_page(). However, in our case, we wanted to clearly stress
 * out the fact that this function is to be used as a last resort in
 * very very few cases.
 *
 * For example, you are a programmer and you created 1,000 pages by
 * mistake and just want to get rid of them without having to delete
 * the whole database and restart populating your database. That's
 * an acceptable use case of this function.
 *
 * \warning
 * DO NOT USE THIS FUNCTION. This function destroys a page and may
 * create all sorts of problems as a result. Many pages are necessary
 * for all sorts of reasons and just destroying them may generate
 * side effects in the code that are totally unexpected. Look into
 * using trash_page() instead.
 *
 * \bug
 * There is no locking mechanism. If some other process accesses the
 * page while it is being deleted, unexpected behavior may result.
 *
 * \bug
 * The deletions scans the ENTIRE revision and branch tables to find
 * all the entries to delete for a given page. This is SLOW.
 *
 * \bug
 * The deletion of children uses recursion on the stack. A website
 * with a very large number of children could use a lot of memory
 * for this process.
 *
 * \note
 * This signal is used by the content plugin itself to make the trashed
 * pages disappear after a certain amount of time. This applies to
 * both: the original page and the page in the trashcan. It may first
 * apply to the original page quickly (within a day or two) and then
 * to the trashed page after some time (we may actually add a minimum
 * amount of time the page would stay in the trashcan such as 2 months
 * and then it gets destroyed.) By default, trash is never deleted.
 * It is kept in the trashcan forever (which is the safest thing we
 * can do.)
 *
 * \param[in] ipath  The path to the page to destroy.
 *
 * \return This function always returns true.
 */
bool content::destroy_page_impl(path_info_t & ipath)
{
    links::links * links_plugin(links::links::instance());

    // here we check whether we have children, because if we do
    // we have to delete the children first
    try
    {
        links::link_info link_info(get_name(name_t::SNAP_NAME_CONTENT_CHILDREN), false, ipath.get_key(), ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links_plugin->new_link_context(link_info));
        links::link_info link_child_info;
        while(link_ctxt->next_link(link_child_info))
        {
            path_info_t child_ipath;
            child_ipath.set_path(link_child_info.key());
            destroy_page(child_ipath);
        }
    }
    catch( std::exception const & x )
    {
        SNAP_LOG_ERROR("exception caught while attempting to destroy page [")(ipath.get_key())("], what=[")(x.what())("]!");
    }
    catch( ... )
    {
        SNAP_LOG_ERROR("unknown exception caught while attempting to destroy page [")(ipath.get_key())("]!");
    }

    // the links plugin cannot include content.h (at least not the
    // header) so we have to implement the deletion of all the links
    // on this page here
    try
    {
        links::link_info_pair::vector_t all_links(links_plugin->list_of_links(ipath.get_key()));
        for(auto const & l : all_links)
        {
            links_plugin->delete_this_link(l.source(), l.destination());
        }
    }
    catch( std::exception const & x )
    {
        SNAP_LOG_ERROR("exception caught while attempting to unlink page [")(ipath.get_key())("], what=[")(x.what())("]!");
    }
    catch( ... )
    {
        SNAP_LOG_ERROR("unknown exception caught while attempting to unlink page [")(ipath.get_key())("]!");
    }

    return true;
}


/** \brief Finish up the destruction of a page.
 *
 * This function is called once all the other plugins were called and
 * deleted the data that they are responsible for.
 *
 * \bug
 * This function is bogus as it will destroy all the children of the
 * page without calling the properly destroy_page() event. Although
 * the children should have been deleted first, we would need to
 * make sure we do not do that.
 *
 * \param[in] ipath  The path of the page being deleted.
 */
void content::destroy_page_done(path_info_t & ipath)
{
    // here we actually drop the page data: all the revisions, branches
    // and the main content page

    QString const key(ipath.get_key());
    libdbproxy::table::pointer_t content_table(get_content_table());

    // if you have problems with the deletion of some parts of that page
    // (i.e. some things did not get deleted) then you will want to use
    // a manual process... look into using cassview to delete the remains
    // and fix the corresponding plugins for next time.
    if(!content_table->exists(key))
    {
        return;
    }

    // Revisions
    {
        // WARNING: to destroy all possible revisions, we currently go
        //          through the list using a full index through the
        //          entire revision table which is SLOW; remember that
        //          we cannot hope that predicates would work since our
        //          rows are not sorted; to palliate we may want to
        //          have an index ("*index*") that is sorted as we have
        //          it for the content rows; the one main limit imposed
        //          by this is 2 billion revisions...
        //
        snap_string_list revision_keys;
        libdbproxy::table::pointer_t revision_table(get_revision_table());
        revision_table->clearCache();
        auto row_predicate = std::make_shared<libdbproxy::row_predicate>();
        row_predicate->setCount(1000);
        for(;;)
        {
            uint32_t const count(revision_table->readRows(row_predicate));
            if(count == 0)
            {
                // no more revisions to process
                break;
            }
            libdbproxy::rows const rows(revision_table->getRows());
            for(libdbproxy::rows::const_iterator o(rows.begin());
                    o != rows.end(); ++o)
            {
                // within each row, check all the columns
                libdbproxy::row::pointer_t row(*o);
                QString const revision_key(QString::fromUtf8(o.key().data()));
                if(!revision_key.startsWith(key))
                {
                    // not this page, try another key
                    continue;
                }

                // this was part of this page
                revision_keys.push_back(revision_key);
            }
        }

        // now do the deletion (we avoid that in the loop to make sure the
        // loop works as expected)
        int const max_keys(revision_keys.size());
        for(int idx(0); idx < max_keys; ++idx)
        {
            destroy_revision(revision_keys[idx]);
        }
    }

    // TODO: create a separate signal to destroy branches
    //       and then a branch has to ask for the destruction
    //       of all of its revisions and links instead of the
    //       specialized way I have it now...

    // Branches
    {
        // WARNING: to destroy all possible revisions, we currently go
        //          through the list using a full index through the
        //          entire revision table which is SLOW; remember that
        //          we cannot hope that predicates would work since our
        //          rows are not sorted; to palliate we may want to
        //          have an index ("*index*") that is sorted as we have
        //          it for the content rows; the one main limit imposed
        //          by this is 2 billion branches...
        //
        snap_string_list branch_keys;
        libdbproxy::table::pointer_t branch_table(get_branch_table());
        branch_table->clearCache();
        auto row_predicate = std::make_shared<libdbproxy::row_predicate>();
        row_predicate->setCount(1000);
        for(;;)
        {
            uint32_t const count(branch_table->readRows(row_predicate));
            if(count == 0)
            {
                // no more revisions to process
                break;
            }
            libdbproxy::rows const rows(branch_table->getRows());
            for(libdbproxy::rows::const_iterator o(rows.begin());
                    o != rows.end(); ++o)
            {
                // within each row, check all the columns
                libdbproxy::row::pointer_t row(*o);
                QString const branch_key(QString::fromUtf8(o.key().data()));
                if(!branch_key.startsWith(key))
                {
                    // not this page, try another key
                    continue;
                }

                // this was part of this page
                branch_keys.push_back(branch_key);
            }
        }

        // now do the deletion (we avoid that in the loop to make sure the
        // loop works as expected)
        int const max_keys(branch_keys.size());
        for(int idx(0); idx < max_keys; ++idx)
        {
            branch_table->dropRow(branch_keys[idx]);
        }
    }

    // Finally, get rid of the content row
    {
        content_table->dropRow(ipath.get_key());
    }
}


/** \brief This function drops the specified revision.
 *
 * This function is used to drop one revision. We have a separate
 * function because a revision may have a reference to a file
 * and that reference needs to be managed properly.
 *
 * \param[in] revision_key  The Cassandra key directly to the revision
 *                          to be destroyed.
 */
bool content::destroy_revision_impl(QString const & revision_key)
{
    libdbproxy::table::pointer_t revision_table(get_revision_table());

    // check whether there is an attachment MD5
    libdbproxy::value const attachment_md5(revision_table->getRow(revision_key)->getCell(get_name(name_t::SNAP_NAME_CONTENT_ATTACHMENT))->getValue());
    if(attachment_md5.size() == 16)
    {
        // the name of the cell is the content key, which is the
        // revision without the '#...', preceded by "content::files::reference::"
        //
        int const pos(revision_key.indexOf('#'));
        if(pos > 0)
        {
            libdbproxy::table::pointer_t branch_table(get_branch_table());
            libdbproxy::table::pointer_t files_table(get_files_table());
            QString const key(revision_key.mid(0, pos));

            // remove reference from the "files" table
            QString const files_reference(get_name(name_t::SNAP_NAME_CONTENT_FILES_REFERENCE));
            QString const reference_name(QString("%1::%2")
                                    .arg(files_reference)
                                    .arg(key));
            libdbproxy::row::pointer_t files_row(files_table->getRow(attachment_md5.binaryValue()));
            files_row->dropCell(reference_name);

            // remove the reference from the "branch" table
            QByteArray attachment_ref;
            attachment_ref.append(get_name(name_t::SNAP_NAME_CONTENT_ATTACHMENT_REFERENCE));
            attachment_ref.append("::");
            attachment_ref.append(attachment_md5.binaryValue()); // binary md5

            // check whether this was the last reference, if so, then we can
            // drop the file itself too since it won't be useful anymore
            //
            auto column_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
            column_predicate->setCount(1); // if there is 1 or more, we cannot delete
            column_predicate->setIndex(); // behave like an index
            column_predicate->setStartCellKey(files_reference + "::"); // start/end keys are reversed
            column_predicate->setEndCellKey(files_reference + ":;");
            files_row->clearCache();
            files_row->readCells(column_predicate);
            libdbproxy::cells const cells(files_row->getCells());
            if(cells.isEmpty())
            {
                // no more references, get rid of the file itself
                files_table->dropRow(attachment_md5.binaryValue());
            }

            // "calculate" the branch key from the revision key
            int pos_slash(revision_key.indexOf('/', pos + 1));
            if(pos_slash == -1)
            {
                pos_slash = pos + 1;
            }
            else
            {
                ++pos_slash;
            }
            int pos_dot(revision_key.indexOf('.', pos_slash));
            if(pos_dot == -1)
            {
                pos_dot = revision_key.length();
            }
            QString const branch_key(QString("%1#%2").arg(key).arg(revision_key.mid(pos_slash, pos_dot - pos_slash)));
            branch_table->getRow(branch_key)->dropCell(attachment_ref);
        }
    }

    // give a chance to other plugins to destroy information related
    // to this revision in other places
    return true;
}

/** \brief Destroy all the remaining fields.
 *
 * The done part of the destroy revision drops the row itself
 * which in effect destroys all the fields in that revision.
 *
 * \param[in] revision_key  The Cassandra key directly to the revision
 *                          to be destroyed.
 */
void content::destroy_revision_done(QString const & revision_key)
{
    libdbproxy::table::pointer_t revision_table(get_revision_table());

    // this destroys the rest
    revision_table->dropRow(revision_key);
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
