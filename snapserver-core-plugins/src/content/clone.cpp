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

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(content)


/** \brief Handle the content specific links from a cloned page.
 *
 * This function repairs parent links.
 */
void content::repair_link_of_cloned_page(QString const & clone, snap_version::version_number_t branch_number, links::link_info const & source, links::link_info const & destination, bool const cloning)
{
    if(source.name() == get_name(name_t::SNAP_NAME_CONTENT_PARENT)
    && destination.name() == get_name(name_t::SNAP_NAME_CONTENT_CHILDREN))
    {
        // this is a special case as the cloned page parent is in most
        // cases not the same as the cloned page's parent page; for
        // example, if you put a page in the trashcan, the parent of
        // the new page is /trashcan/!
        path_info_t child;
        child.set_path(clone);
        path_info_t parent;
        child.get_parent(parent);
        links::link_info src(get_name(name_t::SNAP_NAME_CONTENT_PARENT), true, clone, branch_number);
        links::link_info dst(get_name(name_t::SNAP_NAME_CONTENT_CHILDREN), false, parent.get_key(), get_current_branch(parent.get_key(), true));
        links::links::instance()->create_link(src, dst);
    }
    else if(source.name() == get_name(name_t::SNAP_NAME_CONTENT_PAGE_TYPE)
    && destination.name() == get_name(name_t::SNAP_NAME_CONTENT_PAGE))
    {
        links::link_info src(get_name(name_t::SNAP_NAME_CONTENT_PAGE_TYPE), true, clone, branch_number);
        links::links::instance()->create_link(src, destination);
    }
    else if(!cloning
    && source.name() == get_name(name_t::SNAP_NAME_CONTENT_CHILDREN)
    && destination.name() == get_name(name_t::SNAP_NAME_CONTENT_PARENT))
    {
        // copy the children links only if we are not cloning
        links::link_info src(source.name(), false, clone, branch_number);
        links::links::instance()->create_link(src, destination);
    }
    // else -- ignore all others for now
}


/** \brief Get the page cloned.
 *
 * This signal is captured here because the links cannot work on
 * the cloned tree directly (the links.h header cannot include
 * the content.h file).
 *
 * So here we call functions on the links plugin to make it all
 * work. The good thing (side effect) is that all the links are
 * fixed by the time the other plugins page_cloned() function
 * gets called.
 *
 * \param[in] tree  The tree of nodes that were cloned.
 *
 * \return always true so other modules always receive the signal.
 */
bool content::page_cloned_impl(cloned_tree_t const& tree)
{
    links::links *link_plugin(links::links::instance());
    size_t const max_pages(tree.f_pages.size());
    for(size_t idx(0); idx < max_pages; ++idx)
    {
        cloned_page_t const& page(tree.f_pages[idx]);
        size_t const max_branches(page.f_branches.size());
        for(size_t branch(0); branch < max_branches; ++branch)
        {
            snap_version::version_number_t const b(page.f_branches[branch].f_branch);
            path_info_t source(page.f_source);
            path_info_t destination(page.f_destination);
            source.force_branch(b);
            destination.force_branch(b);
            link_plugin->adjust_links_after_cloning(source.get_branch_key(), destination.get_branch_key());
        }
    }

    // always return true
    return true;
}


/** \brief Copy a page to another location and additional features.
 *
 * This function is used to properly copy a page to another location.
 *
 * This feature is used by many others such as the "trash page" in which
 * case the page is "moved" to the trashcan. In that case, the existing
 * page is copied to the trashcan and the source is marked as deleted
 * (path_info_t::status_t::DELETED)
 *
 * It can also be used to simply clone a page to another location before
 * working on that clone (i.e. that way you can offer templates for
 * various type of pages...)
 *
 * \warning
 * This function DOES NOT verify that a page can be cloned the way you
 * are requesting the page to be cloned. In other words, as a programmer,
 * you can create a big mess. This can be necessary when a module takes
 * over another module data, however, for end users, it is very dangerous.
 * It is preferable that you call another function such as the move_page()
 * and trash_page() functions.
 *
 * \important
 * A clone is a copy which becomes its very own version of the page. In
 * other words it is a page in its own right and it does not behave like
 * a hard or soft link (i.e. if you edit the original, the copy is not
 * affected and vice versa.)
 *
 * \todo
 * At this point the destination MUST be non-existant which works for our
 * main purposes. However, to restore a previously deleted object, or
 * move a page back and forth between two paths, we need to be able to
 * overwrite the current destination. We should have a form of mode for
 * the clone function, mode which defines what we do in various 
 * "complex" situations.
 *
 * \todo
 * As we add a mode, we may want to offer a way to create a clone with
 * just the latest branch and not all the branches and revisions. At
 * this point we are limited to copying everything (which is good when
 * sending a page to the trashcan, but not so good when doing a "quick
 * clone".)
 *
 * \param[in] source  The source being cloned.
 * \param[in] destination  The destination where the source gets copied.
 *
 * \return true if the cloning worked smoothly, false otherwise
 *
 * \sa move_page()
 * \sa trash_page()
 */
bool content::clone_page(clone_info_t & source, clone_info_t & destination)
{

// WARNING: This function is NOT yet functional, I am still looking into how
//          to make the cloning work so it is not all incorrect

    struct sub_function
    {
        sub_function(content * content_plugin, clone_info_t & source, clone_info_t & destination, int64_t const start_date)
            : f_content_plugin(content_plugin)
            , f_source(source)
            , f_destination(destination)
            , f_start_date(start_date)
            , f_content_table(f_content_plugin->get_content_table())
            , f_branch_table(f_content_plugin->get_branch_table())
            , f_revision_table(f_content_plugin->get_revision_table())
            , f_clones(source, destination)
            //, f_result(true) -- auto-init
        {
        }

        void clone_tree()
        {
            // make sure the destination does not exist, if it does,
            // we cannot create the clone
            //
            // if the parent does not exist, then all the children won't
            // exist either so we can do that test just once at the top
            //
            // TODO: add support for that case (i.e. to overwrite page A
            //       with page B data; we may want to first move page A
            //       to the trashcan though, and then allow the overwrite
            //       if the destination is marked as "deleted")
            //
            if(f_content_table->exists(f_destination.f_ipath.get_key()))
            {
                SNAP_LOG_ERROR("clone_page() called with a destination (")(f_destination.f_ipath.get_key())(") which already exists.");
                f_result = false;
                return;
            }

            // we can clone the parent most page as is, then we go through
            // the children, and the children of the children, etc.
            clone_page(f_source.f_ipath, f_destination.f_ipath);

            // now tell all the other plugins that we just cloned a page
            f_content_plugin->page_cloned(f_clones);
        }

        void clone_children(path_info_t & source_parent, path_info_t & destination_parent)
        {
            QString const source_key(source_parent.get_key());
            links::link_info info(get_name(name_t::SNAP_NAME_CONTENT_CHILDREN), false, source_key, source_parent.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
            links::link_info child_info;
            while(link_ctxt->next_link(child_info))
            {
                path_info_t src, dst;
                src.set_path(child_info.key());
                destination_parent.get_child(dst, child_info.key().mid(source_key.length()));
                clone_page(src, dst);
            }
        }

        void clone_page(path_info_t & source, path_info_t & destination)
        {
            // setup the status using RAII
            path_info_t::status_t src_now(source.get_status());
            // TODO: this is problematic; the old way was not really correct
            //       because we really have to first go through the entire
            //       tree to be cloned, lock all those pages, then do the
            //       cloning work... right now, it does not work that way at
            //       all!
            //
            //if(src_now.is_working())
            //{
            //    // we cannot work on a page when another process is already
            //    // working on that page...
            //    SNAP_LOG_ERROR("clone_page() called with a source (")(source.get_key())(") which is being processed now (working: ")(static_cast<int>(src_now.get_working()))(").");
            //    f_result = false;
            //    return;
            //}
            //path_info_t::raii_status_t source_state(source, f_source.f_processing_state, f_source.f_done_state);
            source.set_status(f_source.f_done_state);

            // nothing to check for the destination,
            // at this point the current status would be undefined
            // (should be extended in the future though...)
            //path_info_t::raii_status_t destination_state(destination, f_destination.f_processing_state, f_destination.f_done_state);

            // save the date when we cloned the page
            f_content_table->row(destination.get_key())->cell(get_name(name_t::SNAP_NAME_CONTENT_CLONED))->setValue(f_start_date);

            // the content table is just one row, we specialize it because
            // we can directly fix the branch/revision information (and that
            // makes it a lot easier and safer to manage the whole thing)
            copy_content(source, destination);

            // copy all branches and their revisions,
            //
            // the difference here is that we may have many branches and
            // thus many rows to copy; using the last_branch parameter we
            // can find all the branches with a simple sweap, then use the
            // dbutil copy function to copy the data
            //
            // Each branch has one or more revisions, these are copied at
            // the same time
            //
            // TODO: add support to only copy the current branches (current
            //       and working); or "the last few branches"
            cloned_page_t page;
            page.f_source = source;
            page.f_destination = destination;
            copy_branches(page);
            f_clones.f_pages.push_back(page);

            // now that the copy is done we can save the copy state
            //
            destination.set_status(f_destination.f_done_state);

            // finally clone the children if any
            //
            clone_children(source, destination);
        }

        void copy_content(path_info_t & source, path_info_t & destination)
        {
            QString revision_control(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL));
            QString current_branch_key(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH_KEY));
            QString current_working_branch_key(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH_KEY));
            QString current_revision_key(QString("::%1::").arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION_KEY)));
            QString current_working_revision_key(QString("::%1::").arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION_KEY)));
            // copy the main row in the content table by hand because
            // otherwise we would have problems with the status and a
            // few other things; also that way we can immediately fix
            // the branch and revision URIs
            QtCassandra::QCassandraRow::pointer_t source_row(f_content_table->row(source.get_key()));
            source_row->clearCache();
            QtCassandra::QCassandraRow::pointer_t destination_row(f_content_table->row(destination.get_key()));
            auto column_predicate = std::make_shared<QtCassandra::QCassandraCellRangePredicate>();
            column_predicate->setCount(1000); // we have to copy everything also it is likely very small (i.e. 10 fields...)
            column_predicate->setIndex(); // behave like an index
            for(;;)
            {
                source_row->readCells(column_predicate);
                QtCassandra::QCassandraCells const source_cells(source_row->cells());
                if(source_cells.isEmpty())
                {
                    // done
                    break;
                }
                // handle one batch
                for(QtCassandra::QCassandraCells::const_iterator nc(source_cells.begin());
                        nc != source_cells.end();
                        ++nc)
                {
                    QtCassandra::QCassandraCell::pointer_t source_cell(*nc);
                    QByteArray cell_key(source_cell->columnKey());
                    // ignore the status
                    if(strcmp(cell_key.data(), get_name(name_t::SNAP_NAME_CONTENT_STATUS)) != 0
                    && strcmp(cell_key.data(), get_name(name_t::SNAP_NAME_CONTENT_STATUS_CHANGED)) != 0
                    && strcmp(cell_key.data(), get_name(name_t::SNAP_NAME_CONTENT_CLONED)) != 0)
                    {
                        QString key(QString::fromLatin1(cell_key.data()));
                        if(key.startsWith(revision_control)
                        && (key.endsWith(current_branch_key)
                         || key.endsWith(current_working_branch_key)
                         || key.contains(current_revision_key)
                         || key.contains(current_working_revision_key)))
                        {
                            QString uri(source_cell->value().stringValue());
                            if(uri.startsWith(source.get_key()))
                            {
                                // fix the key so it matches the destination properly
                                uri = destination.get_key() + uri.mid(source.get_key().length());
                                destination_row->cell(cell_key)->setValue(uri);
                            }
                            else
                            {
                                // TODO: verify that this is not actually an error?
                                destination_row->cell(cell_key)->setValue(source_cell->value());
                            }
                        }
                        else
                        {
                            // anything else gets copied as is for now
                            destination_row->cell(cell_key)->setValue(source_cell->value());
                        }
                    }
                }
            }
        }

        void copy_branches(cloned_page_t & page)
        {
            // WARNING: Do not even remotely try to use a row predicate
            //          along the setStartRowName() and setEndRowName()
            //          functions because rows are NOT sorted using their
            //          key as is. Instead they use an MD5 checksum which
            //          is completely different.

            QString const source_key(page.f_source.get_key());
            QString const destination_key(page.f_destination.get_key());

            // retrieve the last branch (inclusive)
            QString last_branch_key(QString("%1::%2")
                    .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                    .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_BRANCH)));
            QtCassandra::QCassandraValue last_branch_value(f_content_table->row(source_key)->cell(last_branch_key)->value());
            snap_version::version_number_t last_branch;
            if(last_branch_value.nullValue())
            {
                // some assumption, the last branch should always be defined
                last_branch = 1;
            }
            else
            {
                last_branch = last_branch_value.uint32Value();
            }

            QString const links_namespace(QString("%1::").arg(links::get_name(links::name_t::SNAP_NAME_LINKS_NAMESPACE)));
            QByteArray const links_bytearray(links_namespace.toLatin1());

            // all the names end with '#' and the <branch> number
            // some branches may not exist (partial copy, branch zero)
            for(snap_version::version_number_t branch(0); branch <= last_branch; ++branch)
            {
                QString const source_uri(f_content_plugin->generate_branch_key(source_key, branch));
                QString const destination_uri(f_content_plugin->generate_branch_key(destination_key, branch));
                if(f_branch_table->exists(source_uri)
                && f_branch_table->row(source_uri)->exists(get_name(name_t::SNAP_NAME_CONTENT_CREATED)))
                {
                    cloned_branch_t cloned_branch;
                    cloned_branch.f_branch = branch;

                    //dbutils::copy_row(f_branch_table, source_uri, f_branch_table, destination_uri);
                    // Handle our own copy to avoid copying the links because
                    // it could cause all sorts of weird side effects (i.e.
                    // wrong parent, wrong children to cite only those two...)
                    QtCassandra::QCassandraRow::pointer_t source_row(f_branch_table->row(source_uri));
                    source_row->clearCache();
                    QtCassandra::QCassandraRow::pointer_t destination_row(f_branch_table->row(destination_uri));
                    auto column_predicate = std::make_shared<QtCassandra::QCassandraCellRangePredicate>();
                    column_predicate->setCount(1000); // we have to copy everything also it is likely very small (i.e. 10 fields...)
                    column_predicate->setIndex(); // behave like an index
                    for(;;)
                    {
                        source_row->readCells(column_predicate);
                        QtCassandra::QCassandraCells const source_cells(source_row->cells());
                        if(source_cells.isEmpty())
                        {
                            // done
                            break;
                        }
                        // handle one batch
                        for(QtCassandra::QCassandraCells::const_iterator nc(source_cells.begin());
                                nc != source_cells.end();
                                ++nc)
                        {
                            QtCassandra::QCassandraCell::pointer_t source_cell(*nc);
                            QByteArray cell_key(source_cell->columnKey());
                            // ignore all links
                            if(!cell_key.startsWith(links_bytearray))
                            {
                                // anything else gets copied as is for now
                                destination_row->cell(source_cell->columnKey())->setValue(source_cell->value());
                            }
                        }
                    }

                    // copy all revisions
                    //
                    // this is very similar to the branch copy, only it uses
                    // the revision table and the last revision information
                    // for that branch
                    copy_revisions(page, cloned_branch);

                    page.f_branches.push_back(cloned_branch);

                    // link both pages together in this branch
                    {
                        // note: we do not need a specific revision when
                        //       creating a link, however, we do need a
                        //       specific branch so we create a new path
                        //       info with the right branch, but leave
                        //       the revision to whatever it is by default
                        path_info_t si;
                        bool source_unique(false);
                        si.set_path(page.f_source.get_key());
                        si.force_branch(branch);
                        char const * clone_name(get_name(name_t::SNAP_NAME_CONTENT_CLONE));
                        links::link_info link_source(clone_name, source_unique, si.get_key(), si.get_branch());

                        path_info_t di;
                        bool destination_unique(true);
                        di.set_path(page.f_destination.get_key());
                        di.force_branch(branch);
                        char const * original_page_name(get_name(name_t::SNAP_NAME_CONTENT_ORIGINAL_PAGE));
                        links::link_info link_destination(original_page_name, destination_unique, di.get_key(), di.get_branch());

                        links::links::instance()->create_link(link_source, link_destination);
                    }
                }
            }
        }

        void copy_revisions(cloned_page_t & page, cloned_branch_t & cloned_branch)
        {
            // TODO: add support to only copy the current revisions
            //       (current and working, or a few latest revisions)
            QString const source_key(page.f_source.get_key());
            QString const destination_key(page.f_destination.get_key());

            // retrieve the last revision (inclusive)
            // we have to use a predicate because there may be various
            // languages for each branch; so we have a loop per
            // branch/language and then an inner loop for each revision
            QString last_revision_key(QString("%1::%2::%3")
                    .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                    .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_REVISION))
                    .arg(cloned_branch.f_branch));

            auto column_predicate(std::make_shared<QtCassandra::QCassandraCellRangePredicate>());
            column_predicate->setCount(10000); // 4 bytes per entry + row name of under 100 bytes, that's 1Mb max.
            column_predicate->setIndex(); // behave like an index
            column_predicate->setStartCellKey(last_revision_key); // no language (fully neutral) is a valid entry
            column_predicate->setEndCellKey(last_revision_key + "|"); // languages are limited to letters
            QtCassandra::QCassandraRow::pointer_t revision_row(f_content_table->row(source_key));
            revision_row->clearCache();
            for(;;)
            {
                revision_row->readCells(column_predicate);
                QtCassandra::QCassandraCells const new_cells(revision_row->cells());
                if(new_cells.isEmpty())
                {
                    break;
                }
                // handle one batch
                for(QtCassandra::QCassandraCells::const_iterator nc(new_cells.begin());
                        nc != new_cells.end();
                        ++nc)
                {
                    // verify the entry is valid
                    QtCassandra::QCassandraCell::pointer_t last_revision_cell(*nc);
                    if(!last_revision_cell->value().nullValue())
                    {
                        // the revision number is the cell value
                        // we need the row key to extract the language
                        QString column_name(last_revision_cell->columnName());
                        QString locale;
                        if(last_revision_key != column_name)
                        {
                            int const pos(column_name.lastIndexOf(":"));
                            if(pos < 0)
                            {
                                throw snap_logic_exception(QString("somehow the revision column_name \"%1\" does not include at least one ':'.")
                                            .arg(column_name));
                            }
                            // extract the locale (language name)
                            locale = column_name.mid(pos + 1);
                        }
                        snap_version::version_number_t last_revision(last_revision_cell->value().uint32Value());

                        // all the revision names end with:
                        //    '#' <locale> '/' <branch> '.' <revision>
                        //
                        // some revisions may not exist (partial copy)
                        for(snap_version::version_number_t revision(0); revision <= last_revision; ++revision)
                        {
                            QString const source_uri(f_content_plugin->generate_revision_key(source_key, cloned_branch.f_branch, revision, locale));
                            QString const destination_uri(f_content_plugin->generate_revision_key(destination_key, cloned_branch.f_branch, revision, locale));

                            if(f_revision_table->exists(source_uri)
                            && f_revision_table->row(source_uri)->exists(get_name(name_t::SNAP_NAME_CONTENT_CREATED)))
                            {
                                dbutils::copy_row(f_revision_table, source_uri, f_revision_table, destination_uri);

                                cloned_branch.f_revisions.push_back(revision);
                            }
                        }
                    }
                }
            }
        }

        bool result() const
        {
            return f_result;
        }

    private:
        content *                                   f_content_plugin;
        clone_info_t                                f_source;
        clone_info_t                                f_destination;
        int64_t const                               f_start_date;
        QtCassandra::QCassandraTable::pointer_t     f_content_table;
        QtCassandra::QCassandraTable::pointer_t     f_branch_table;
        QtCassandra::QCassandraTable::pointer_t     f_revision_table;
        cloned_tree_t                               f_clones;
        bool                                        f_result = true;
    };

    sub_function f(this, source, destination, f_snap->get_start_date());
    f.clone_tree();
    return f.result();
}


/** \brief Move a page from one URI to another.
 *
 * This function moves the source page to the destination page. The source
 * is then marked as deleted.
 *
 * At this point the destination page must not exist yet.
 *
 * \note
 * Since the page does not get deleted, we do not make a copy in the
 * trashcan even though the source page ends up being marked as deleted.
 *
 * \param[in] ipath_source  The path to the source page being moved.
 * \param[in] ipath_destination  The path to the destination page.
 *
 * \return true if the move succeeds.
 *
 * \sa clone_page()
 * \sa trash_page()
 */
bool content::move_page(path_info_t & ipath_source, path_info_t & ipath_destination)
{
    // is the page deletable? (and thus movable?)
    //
    // (administrative pages, those created from content.xml, are nearly
    // all marked as not deletable by default!)
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    QtCassandra::QCassandraValue prevent_delete(content_table->row(ipath_source.get_key())->cell(get_name(name_t::SNAP_NAME_CONTENT_PREVENT_DELETE))->value());
    if(!prevent_delete.nullValue() && prevent_delete.signedCharValue())
    {
        f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN, "Forbidden Move",
                "Sorry. this page is marked as undeletable and as such it cannot be moved.",
                QString("User tried to move page \"%1\", which is locked (marked as undeletable).").arg(ipath_source.get_key()));
        NOTREACHED();
    }

    // setup the clone parameters
    clone_info_t source;
    source.f_ipath = ipath_source;
    //source.f_processing_state.set_state(path_info_t::status_t::state_t::NORMAL);
    //source.f_processing_state.set_working(path_info_t::status_t::working_t::CLONING);
    source.f_done_state.set_state(path_info_t::status_t::state_t::MOVED);

    clone_info_t destination;
    destination.f_ipath = ipath_destination;
    //destination.f_processing_state.set_state(path_info_t::status_t::state_t::CREATE);
    //destination.f_processing_state.set_working(path_info_t::status_t::working_t::CREATING);
    destination.f_done_state = ipath_source.get_status();

    return clone_page(source, destination);
}


/** \brief Put the specified page in the trashcan.
 *
 * This function "deletes" a page by making a copy of it in the trashcan.
 *
 * The original page remains as DELETED for a while. After that while it
 * gets 100% deleted from Cassandra.
 *
 * The pages in the trashcan can be restored at a later time. The time
 * pages are kept in the trashcan is controlled by the website
 * administrator. It can be very short (1 day) or very long (forever).
 *
 * \param[in] ipath  The path to move to the trash.
 *
 * \return true if the cloning worked as expected.
 *
 * \sa clone_page()
 * \sa move_page()
 */
bool content::trash_page(path_info_t & ipath)
{
    // is the page deletable?
    //
    // (administrative pages, those created from content.xml, are nearly
    // all marked as not deletable by default!)
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    QtCassandra::QCassandraValue prevent_delete(content_table->row(ipath.get_key())->cell(get_name(name_t::SNAP_NAME_CONTENT_PREVENT_DELETE))->value());
    if(!prevent_delete.nullValue() && prevent_delete.signedCharValue())
    {
        f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN, "Forbidden Removal",
                "Sorry. This page is marked as undeletable.",
                QString("User tried to delete page \"%1\", which is locked.").arg(ipath.get_key()));
        NOTREACHED();
    }

    // create a destination path in the trashcan
    QString trashcan_path("trashcan");

    // path can be changed by administrator
    QtCassandra::QCassandraValue trashcan_path_value(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_CONTENT_TRASHCAN)));
    if(!trashcan_path_value.nullValue())
    {
        // administrators can move the trashcan around up until something
        // gets deleted
        trashcan_path = trashcan_path_value.stringValue();
    }

    // make sure that path exists
    if(!content_table->exists(trashcan_path))
    {
        // TODO: it looks like we are not going to create the parents if
        //       they do not exist...
        path_info_t trashcan_ipath;

        trashcan_ipath.set_path(trashcan_path);
        trashcan_ipath.force_branch(snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
        trashcan_ipath.force_revision(snap_version::SPECIAL_VERSION_FIRST_REVISION);

        // TODO: would we have a language attached to the trashcan?
        //       (certainly because the title should change depending on
        //       the language, right?)
        trashcan_ipath.force_locale("xx");

        // TODO: the owner is the first person who deletes something on the
        //       website; that's probably wrong!
        create_content(trashcan_ipath, get_name(name_t::SNAP_NAME_CONTENT_PRIMARY_OWNER), "system-page");

        // save the creation date, title, and description
        QtCassandra::QCassandraTable::pointer_t revision_table(get_revision_table());
        QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(trashcan_ipath.get_revision_key()));
        int64_t const start_date(f_snap->get_start_date());
        revision_row->cell(get_name(name_t::SNAP_NAME_CONTENT_CREATED))->setValue(start_date);
        // TODO: add support for translation
        QString const title("Trashcan");
        revision_row->cell(get_name(name_t::SNAP_NAME_CONTENT_TITLE))->setValue(title);
        QString const empty_string;
        revision_row->cell(get_name(name_t::SNAP_NAME_CONTENT_BODY))->setValue(empty_string);
    }

    // new page goes under a randomly generated number
    trashcan_path += "/";
    trashcan_path += f_snap->get_unique_number();

    // setup the clone parameters
    clone_info_t source;
    source.f_ipath = ipath;
    //source.f_processing_state.set_state(path_info_t::status_t::state_t::NORMAL);
    //source.f_processing_state.set_working(path_info_t::status_t::working_t::REMOVING);
    source.f_done_state.set_state(path_info_t::status_t::state_t::DELETED);

    clone_info_t destination;
    destination.f_ipath.set_path(trashcan_path);
    destination.f_ipath.force_branch(snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
    destination.f_ipath.force_revision(snap_version::SPECIAL_VERSION_FIRST_REVISION);
    destination.f_ipath.force_locale("xx"); // TBD: should the language be set as... maybe the page being deleted?
    //destination.f_processing_state.set_state(path_info_t::status_t::state_t::CREATE);
    //destination.f_processing_state.set_working(path_info_t::status_t::working_t::CREATING);
    destination.f_done_state.set_state(path_info_t::status_t::state_t::HIDDEN);
    //destination.f_done_state = ipath_source.get_status(); -- TODO: should we save the source status instead of forcing it to HIDDEN?

    return clone_page(source, destination);
}




SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
