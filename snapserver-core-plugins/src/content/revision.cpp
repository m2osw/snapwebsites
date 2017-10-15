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
 * \brief The implementation of the content plugin status class.
 *
 * This file contains the status class implementation.
 */

#include "content.h"

#include <snapwebsites/dbutils.h>
#include <snapwebsites/log.h>
#include <snapwebsites/snap_lock.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(content)

/** \brief Call if a revision control version is found to be invalid.
 *
 * While dealing with revision control information, this function may
 * be called if a branch or revision number if found to be incorrect.
 *
 * \note
 * Debug code should not call this function. Instead it should
 * throw an error which is much more effective to talk to programmers.
 *
 * \param[in] version  The version that generated the error.
 */
void content::invalid_revision_control(QString const& version)
{
    f_snap->die(snap_child::http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR, "Invalid Revision Control",
            "The revision control \"" + version + "\" does not look valid.",
            "The version does not seem to start with a valid decimal number.");
    NOTREACHED();
}


/** \brief Get the current branch.
 *
 * This function retrieves the current branch for data defined in a page.
 * The current branch is determined using the key of the page being
 * accessed.
 *
 * \note
 * The current branch number may not be the last branch number. The system
 * automatically forces branch 1 to become current when created. However,
 * the system does not set the newest branch as current when the user creates
 * a new branch. This way a new branch remains hidden until the user decides
 * that it should become current.
 *
 * \param[in] key  The key of the page concerned.
 * \param[in] working_branch  Whether the working branch (true) or the current
 *                            branch (false) is used.
 *
 * \return The current revision number.
 */
snap_version::version_number_t content::get_current_branch(QString const & key, bool working_branch)
{
    QString current_branch_key(QString("%1::%2")
                        .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                        .arg(get_name(working_branch
                                ? name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH
                                : name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH)));
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    if(content_table->exists(key)
    && content_table->row(key)->exists(current_branch_key))
    {
        return content_table->row(key)->cell(current_branch_key)->value().uint32Value();
    }

    return snap_version::SPECIAL_VERSION_UNDEFINED;
}


/** \brief Retrieve the current branch or create a new one.
 *
 * This function retrieves the current user branch which means it returns
 * the current branch as is unless it is undefined or is set to the
 * system branch. In those two cases the function creates a new branch.
 *
 * The function does not change the current branch information.
 *
 * \param[in] key  The key of the page concerned.
 * \param[in] locale  The locale to create the first revision of that branch.
 * \param[in] working_branch  Whether the working branch (true) or the current
 *                            branch (false) is used.
 *
 * \sa get_current_branch()
 * \sa get_new_branch()
 */
snap_version::version_number_t content::get_current_user_branch(QString const& key, QString const& locale, bool working_branch)
{
    snap_version::version_number_t branch(get_current_branch(key, working_branch));
    if(snap_version::SPECIAL_VERSION_UNDEFINED == branch
    || snap_version::SPECIAL_VERSION_SYSTEM_BRANCH == branch)
    {
        // not a valid user branch, first check whether there is a latest
        // user branch, if so, put the new data on the newest branch
        QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());

        // get the last branch number
        QString const last_branch_key(QString("%1::%2")
                            .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                            .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_BRANCH)));
        QtCassandra::QCassandraValue branch_value(content_table->row(key)->cell(last_branch_key)->value());
        if(!branch_value.nullValue())
        {
            // a branch exists, although it may still be a system branch
            branch = branch_value.uint32Value();
        }

        if(snap_version::SPECIAL_VERSION_UNDEFINED == branch
        || snap_version::SPECIAL_VERSION_SYSTEM_BRANCH == branch)
        {
            // well... no user branch exists yet, create one
            return get_new_branch(key, locale);
        }
    }

    return branch;
}


/** \brief Get the current revision.
 *
 * This function retrieves the current revision for data defined in a page.
 * The current branch is determined using the get_current_branch()
 * function with the same key, owner, and working_branch parameters.
 *
 * \note
 * The current revision number may have been changed by an editor to a
 * number other than the last revision number.
 *
 * \param[in] key  The key of the page concerned.
 * \param[in] branch  The branch number.
 * \param[in] locale  The language and country information.
 * \param[in] working_branch  Whether the working branch (true) or the current
 *                            branch (false) is used.
 *
 * \return The current revision number.
 */
snap_version::version_number_t content::get_current_revision(QString const & key, snap_version::version_number_t const branch, QString const & locale, bool working_branch)
{
    QString revision_key(QString("%1::%2::%3")
            .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
            .arg(get_name(working_branch
                    ? name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION
                    : name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION))
            .arg(branch));
    if(!locale.isEmpty())
    {
        revision_key += "::" + locale;
    }
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    if(content_table->exists(key)
    && content_table->row(key)->exists(revision_key))
    {
        return content_table->row(key)->cell(revision_key)->value().uint32Value();
    }

    return snap_version::SPECIAL_VERSION_UNDEFINED;
}


/** \brief Get the current revision.
 *
 * This function retrieves the current revision for data defined in a page.
 * The current branch is determined using the get_current_branch()
 * function with the same key, owner, and working_branch parameters.
 *
 * \note
 * The current revision number may have been changed by an editor to a
 * number other than the last revision number.
 *
 * \param[in] key  The key of the page concerned.
 * \param[in] locale  The language and country information.
 * \param[in] working_branch  Whether the working branch (true) or the current
 *                            branch (false) is used.
 *
 * \return The current revision number.
 */
snap_version::version_number_t content::get_current_revision(QString const & key, QString const & locale, bool working_branch)
{
    snap_version::version_number_t const branch(get_current_branch(key, working_branch));
    QString revision_key(QString("%1::%2::%3")
            .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
            .arg(get_name(working_branch
                    ? name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION
                    : name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION))
            .arg(branch));
    if(!locale.isEmpty())
    {
        revision_key += "::" + locale;
    }
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    if(content_table->exists(key)
    && content_table->row(key)->exists(revision_key))
    {
        return content_table->row(key)->cell(revision_key)->value().uint32Value();
    }

    return snap_version::SPECIAL_VERSION_UNDEFINED;
}


/** \brief Generate a new branch number and return it.
 *
 * This function generates a new branch number and returns it. This is used
 * each time the user requests to create a new branch.
 *
 * In most cases a user will create a new branch when he wants to be able to
 * continue to update the current branch until he is done with the new branch
 * of that page. This way the new branch can be written and moderated and
 * scheduled for publication on a future date without distrubing what visitors
 * see when they visit that page.
 *
 * The locale is used to generate the first revision of that branch. In most
 * cases this allows you to use revision 0 without having to request a new
 * revision by calling the get_new_revision() function (i.e. an early
 * optimization.) If empty, then no translations will be available for that
 * revision and no locale is added to the field name. This is different from
 * setting the locale to "xx" which still allows translation only this one
 * entry is considered neutral in terms of language.
 *
 * \note
 * Branch zero (0) is never created using this function. If no branch
 * exists this function returns one (1) anyway. This is because branch
 * zero (0) is reserved and used by the system when it saves the parameters
 * found in the content.xml file.
 *
 * \param[in] key  The key of the page concerned.
 * \param[in] locale  The locale used for this revision.
 *
 * \return The new branch number.
 *
 * \sa get_current_user_branch()
 */
snap_version::version_number_t content::get_new_branch(QString const & key, QString const & locale)
{
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());

    // get the last branch number
    QString const last_branch_key(QString("%1::%2")
                        .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                        .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_BRANCH)));

    // increase revision if one exists, otherwise we keep the user default (1)
    snap_version::version_number_t branch(snap_version::SPECIAL_VERSION_USER_FIRST_BRANCH);

    snap_lock lock(key);

    QtCassandra::QCassandraValue branch_value(content_table->row(key)->cell(last_branch_key)->value());
    if(!branch_value.nullValue())
    {
        // it exists, increase it
        branch = branch_value.uint32Value();
        if(snap_version::SPECIAL_VERSION_MAX_BRANCH_NUMBER > branch)
        {
            ++branch;
        }
        // else -- probably need to warn the user we reached 4 billion
        //         branches (this is pretty much impossible without either
        //         hacking the database or having a robot that generates
        //         many branches every day.)
    }
    content_table->row(key)->cell(last_branch_key)->setValue(static_cast<snap_version::basic_version_number_t>(branch));

    QString last_revision_key(QString("%1::%2::%3")
                    .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                    .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_REVISION))
                    .arg(branch));
    if(!locale.isEmpty())
    {
        last_revision_key += "::" + locale;
    }
    content_table->row(key)->cell(last_revision_key)->setValue(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));

    // unlock ASAP
    lock.unlock();

    return branch;
}


/** \brief Copy a branch in another.
 *
 * This function is generally used when a user creates a new branch on
 * a page where another branch already exists.
 *
 * The function checks whether the source branch exists. If not, it
 * silently returns.
 *
 * The destination may already have some parameters. The copy attempts to
 * not modify existing data in the destination branch.
 *
 * The destination receives the content::created field as of this process
 * instance (i.e. snap_child::get_start_date()) unless the field already
 * exists.
 *
 * \param[in] key  The page where the branch is copied.
 * \param[in] source_branch  The branch that gets copied.
 * \param[in] destination_branch  The new branch you just "created" (the
 *                                branch itself does not need to already
 *                                exist in the database.)
 */
void content::copy_branch(QString const & key, snap_version::version_number_t const source_branch, snap_version::version_number_t const destination_branch)
{
    QtCassandra::QCassandraTable::pointer_t branch_table(get_branch_table());

    if(source_branch >= destination_branch)
    {
        throw snap_logic_exception(QString("trying to copy a newer branch (%1) in an older one (%2)")
                    .arg(source_branch)
                    .arg(destination_branch));
    }

    path_info_t source_uri;
    source_uri.set_path(key);
    source_uri.force_branch(source_branch);

    QtCassandra::QCassandraRow::pointer_t source_row(branch_table->row(source_uri.get_branch_key()));
    if(!source_row->exists(get_name(name_t::SNAP_NAME_CONTENT_CREATED)))
    {
        // no source, ignore
        return;
    }
    source_row->clearCache();

    path_info_t destination_uri;
    destination_uri.set_path(key);
    destination_uri.force_branch(destination_branch);

    QtCassandra::QCassandraRow::pointer_t destination_row(branch_table->row(destination_uri.get_branch_key()));

    auto column_predicate = std::make_shared<QtCassandra::QCassandraCellRangePredicate>();
    column_predicate->setCount(1000); // we have to copy everything also it is likely very small (i.e. 10 fields...)
    column_predicate->setIndex(); // behave like an index
    for(;;)
    {
        source_row->readCells(column_predicate);
        QtCassandra::QCassandraCells source_cells(source_row->cells());
        if(source_cells.isEmpty())
        {
            // done
            break;
        }
        copy_branch_cells(source_cells, destination_row, destination_branch);
    }
}


bool content::copy_branch_cells_impl(QtCassandra::QCassandraCells & source_cells, QtCassandra::QCassandraRow::pointer_t destination_row, snap_version::version_number_t const destination_branch)
{
    // we handle the links here because the links cannot include the
    // content.h header file...
    //
    links::links * link_plugin(links::links::instance());
    QString const links_namespace(QString("%1::").arg(links::get_name(links::name_t::SNAP_NAME_LINKS_NAMESPACE)));
    QByteArray const links_bytearray(links_namespace.toLatin1());

    QtCassandra::QCassandraCells left_cells;

    // handle one batch
    //
    for(QtCassandra::QCassandraCells::const_iterator nc(source_cells.begin());
            nc != source_cells.end();
            ++nc)
    {
        QtCassandra::QCassandraCell::pointer_t source_cell(*nc);
        QByteArray cell_key(source_cell->columnKey());

        if(cell_key == get_name(name_t::SNAP_NAME_CONTENT_MODIFIED)
        || destination_row->exists(cell_key))
        {
            // ignore the content::modified cell
            // ignore all the cells that already exist in the destination
            //
            // (TBD: we may want to limit those to content::... and
            //       links::... cells and leave the decision to each plugin
            //       for the others?)
            //
            continue;
        }

        if(cell_key == get_name(name_t::SNAP_NAME_CONTENT_CREATED))
        {
            // handle the content::created field
            //
            int64_t const now(f_snap->get_start_date());
            destination_row->cell(get_name(name_t::SNAP_NAME_CONTENT_CREATED))->setValue(now);
        }
        else if(cell_key.startsWith(links_bytearray))
        {
            // handle the links as a special case because the links plugin
            // cannot include content.h (circular includes...)
            //
            link_plugin->fix_branch_copy_link(source_cell, destination_row, destination_branch);
        }
        else
        {
            // keep the other branch fields as is, other plugins can handle
            // them as required by implementing this signal
            //
            // note that the map is a map a shared pointers so it is fast
            // to make a copy like this
            //
            left_cells[cell_key] = source_cell;
        }
    }

    // overwrite the source with the cells we allow to copy "further"
    //
    source_cells = left_cells;

    // continue process if there are still cells to handle
    // (often false since content::... and links::... were already worked on)
    //
    return !source_cells.isEmpty();
}


/** \brief Copy a set of branch cells as is.
 *
 * If your plugin, on a "copy branch", is required to copy some of its
 * own fields and all can be copied as is, then call this function.
 *
 * If you need specialization, then you will want to duplicate this loop
 * and tweak it as required.
 *
 * The function removes all the cells that get copied from the \p source_cells
 * parameter.
 *
 * \param[in,out] source_cells  The array of cells in the source.
 * \param[in] destination_row  The destination for those cells.
 * \param[in] plugin_namespace  The name of your plugin copying those cells.
 */
void content::copy_branch_cells_as_is(QtCassandra::QCassandraCells & source_cells, QtCassandra::QCassandraRow::pointer_t destination_row, QString const & plugin_namespace)
{
    QString const cell_namespace(QString("%1::").arg(plugin_namespace));
    QByteArray const cell_bytearray(cell_namespace.toLatin1());

    QtCassandra::QCassandraCells left_cells;

    // handle one batch
    for(QtCassandra::QCassandraCells::const_iterator nc(source_cells.begin());
            nc != source_cells.end();
            ++nc)
    {
        QtCassandra::QCassandraCell::pointer_t source_cell(*nc);
        QByteArray cell_key(source_cell->columnKey());

        if(cell_key.startsWith(cell_bytearray))
        {
            // copy our fields as is
            destination_row->cell(cell_key)->setValue(source_cell->value());
        }
        else
        {
            // keep the other branch fields as is, other plugins can handle
            // them as required by implementing this signal
            //
            // note that the map is a map a shared pointers so it is fast
            // to make a copy like this
            left_cells[cell_key] = source_cell;
        }
    }

    // overwrite the source with the cells we allow to copy "further"
    source_cells = left_cells;
}


/** \brief Generate a new revision number and return it.
 *
 * This function generates a new revision number and returns it. This is used
 * each time the system or a user saves a new revision of content to a page.
 *
 * The function takes in the branch in which the new revision is to be
 * generated which means the locale needs to also be specified. However,
 * it is possible to set the locale parameter to the empty string in which
 * case the data being revisioned cannot be translated. Note that this is
 * different from setting the value to "xx" since in that case it means
 * that specific entry is neutral whereas using the empty string prevents
 * translations altogether (because the language/country are not taken
 * in account.)
 *
 * The \p repeat parameter is used to determine whether the data is expected
 * to be copied from the previous revision if there is one. Note that at this
 * time no data gets automatically copied if you create a new revision for a
 * new language. We will most certainly change that later so we can copy the
 * data from a default language such as "xx" or "en"...
 *
 * Note that the repeated data includes the date when the entry gets created.
 * The entry is adjusted to use the start date of the child process, which
 * means that you do not have to re-update the creation time of the revision
 * after this call. However, this function does NOT update the branch last
 * modification time. To do so, make sure to call the content_modified()
 * function once you are done with your changes.
 *
 * \note
 * In debug mode the branch number is verified for validity. It has to
 * be an existing branch.
 *
 * \note
 * This function may return zero (0) if the concerned locale did not
 * yet exist for this page.
 *
 * \todo
 * We probably should be using the path_info_t class to generate the
 * URIs. At this point these are done "by hand" here.
 *
 * \todo
 * We may want to create a class that allows us to define a set of the new
 * fields so instead of copying we can immediately save the new value. Right
 * now we are going to write the same field twice (once here in the repeat
 * to save the old value and once by the caller to save the new value.)
 *
 * \param[in] key  The key of the page concerned.
 * \param[in] branch  The branch for which this new revision is being created.
 * \param[in] locale  The locale used for this revision.
 * \param[in] repeat  Whether the existing data should be duplicated in the
 *                    new revision.
 * \param[in] old_branch  The previous branch.
 *
 * \return The new revision number.
 */
snap_version::version_number_t content::get_new_revision(
                QString const & key,
                snap_version::version_number_t const branch,
                QString const & locale,
                bool repeat,
                snap_version::version_number_t const old_branch)
{
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    snap_version::version_number_t const previous_branch(
        old_branch == snap_version::SPECIAL_VERSION_UNDEFINED
            ? branch
            : old_branch);

    // define the keys
    QString last_revision_key(QString("%1::%2::%3")
                .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_REVISION))
                .arg(branch));
    if(!locale.isEmpty())
    {
        last_revision_key += "::" + locale;
    }
    QString current_revision_key(QString("%1::%2::%3")
                .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION))
                .arg(previous_branch));
    if(!locale.isEmpty())
    {
        current_revision_key += "::" + locale;
    }

    // increase revision if one exists, otherwise we keep the default (0)
    snap_version::version_number_t revision(snap_version::SPECIAL_VERSION_FIRST_REVISION);

    snap_lock lock(key);

#ifdef DEBUG
    {
        // verify correctness of branch
        QString const last_branch_key(QString("%1::%2")
                        .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                        .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_BRANCH)));
        QtCassandra::QCassandraValue branch_value(content_table->row(key)->cell(last_branch_key)->value());
        if(!branch_value.nullValue()
        && branch > branch_value.uint32Value())
        {
            // the 'branch' parameter cannot be larger than the last branch allocated
            throw snap_logic_exception(QString("trying to create a new revision for branch %1 which does not exist (last branch is %2)")
                        .arg(branch)
                        .arg(branch_value.uint32Value()));
        }
    }
#endif

    QtCassandra::QCassandraValue revision_value(content_table->row(key)->cell(last_revision_key)->value());
    if(!revision_value.nullValue())
    {
        // it exists, increase it
        revision = revision_value.uint32Value();
        if(snap_version::SPECIAL_VERSION_MAX_BRANCH_NUMBER > revision)
        {
            ++revision;
        }
        // else -- probably need to warn the user we reached 4 billion
        //         revisions (this is assuming we delete old revisions
        //         in the meantime, but even if you make 10 changes a
        //         day and say it makes use of 20 revision numbers each
        //         time, it would still take... over half a million
        //         YEARS to reach that many revisions in that one
        //         branch...)
    }
    content_table->row(key)->cell(last_revision_key)->setValue(static_cast<snap_version::basic_version_number_t>(revision));

    // copy from the current revision at this point
    // (the editor WILL tell us to copy from a specific revisions at some
    // point... it is important because if user A edits revision X, and user
    // B creates a new revision Y in the meantime, we may still want to copy
    // revision X at the time A saves his changes.)
    snap_version::version_number_t previous_revision(revision);
    QtCassandra::QCassandraValue current_revision_value(content_table->row(key)->cell(current_revision_key)->value());
    if(!current_revision_value.nullValue())
    {
        previous_revision = current_revision_value.uint32Value();
    }

    // TBD: should the repeat be done before or after the lock?
    //      it seems to me that since the next call will now generate a
    //      new revision, it is semi-safe (problem is that the newer
    //      version may miss some of the fields...)
    //      also the caller will lose the lock too!

    if(repeat
    && (revision != snap_version::SPECIAL_VERSION_FIRST_REVISION
       || old_branch != snap_version::SPECIAL_VERSION_UNDEFINED)
    && (previous_branch != branch
       || previous_revision != revision))
    {
        // get two revision keys like:
        // http://csnap.m2osw.com/verify-credentials#en/0.2
        // and:
        // http://csnap.m2osw.com/verify-credentials#en/0.3
        QString const previous_revision_key(generate_revision_key(key, previous_branch, previous_revision, locale));
        QString const revision_key(generate_revision_key(key, branch, revision, locale));
        QtCassandra::QCassandraTable::pointer_t revision_table(get_revision_table());

        dbutils::copy_row(revision_table, previous_revision_key, revision_table, revision_key);

        // change the creation date
        QtCassandra::QCassandraValue created;
        created.setInt64Value(f_snap->get_start_date());
        revision_table->row(revision_key)->cell(get_name(name_t::SNAP_NAME_CONTENT_CREATED))->setValue(created);
    }

    // unlock ASAP
    lock.unlock();

    return revision;
}


/** \brief Generate a key from a branch, revision, and locale.
 *
 * This function transforms a page key and a branch number in a key that
 * is to be used to access the user information in the data table.
 *
 * The branch is used as is in the key because it is very unlikely that
 * can cause a problem as all the other extended keys do not start with
 * a number.
 *
 * \param[in] key  The key of the page being worked on.
 * \param[in] working_branch  Whether we use the working branch or not.
 *
 * \return A string representing the branch key in the data table.
 */
QString content::get_branch_key(QString const& key, bool working_branch)
{
    // key in the content table
    QString const current_key(QString("%1::%2")
                .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(working_branch
                        ? name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH_KEY
                        : name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH_KEY)));

    // get the data key from the content table
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    QtCassandra::QCassandraValue const value(content_table->row(key)->cell(current_key)->value());
    return value.stringValue();
}


/** \brief Generate the key to use in the data table for a branch.
 *
 * This function generates the key of the row used in the data table
 * to access branch specific data, whatever the revision.
 *
 * \param[in] key  The key to the page concerned.
 * \param[in] branch  The number of the new current branch.
 *
 * \return This function returns a copy of the current branch key.
 */
QString content::generate_branch_key(QString const& key, snap_version::version_number_t branch)
{
    return QString("%1#%2").arg(key).arg(branch);
}


/** \brief Set the current (working) branch.
 *
 * This function is used to save the \p branch. This is rarely used since
 * in most cases the branch is created when getting a new branch.
 *
 * \param[in] key  The key to the page concerned.
 * \param[in] branch  The number of the new current branch.
 * \param[in] working_branch  Update the current working branch (true) or the current branch (false).
 *
 * \return This function returns a copy of the current branch key.
 */
void content::set_branch(QString const& key, snap_version::version_number_t branch, bool working_branch)
{
    // key in the content table
    QString const current_key(QString("%1::%2")
                .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(working_branch
                        ? name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH
                        : name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH)));

    // save the data key in the content table
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    content_table->row(key)->cell(current_key)->setValue(static_cast<snap_version::basic_version_number_t>(branch));

    // Last branch
    QString const last_branch_key(QString("%1::%2")
                    .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                    .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_BRANCH)));
    QtCassandra::QCassandraValue last_branch_value(content_table->row(key)->cell(last_branch_key)->value());
    if(last_branch_value.nullValue())
    {
        // last branch does not exist yet, create it
        content_table->row(key)->cell(last_branch_key)->setValue(static_cast<snap_version::basic_version_number_t>(branch));
    }
    else
    {
        snap_version::version_number_t const last_branch(last_branch_value.uint32Value());
        if(branch > last_branch)
        {
            content_table->row(key)->cell(last_branch_key)->setValue(static_cast<snap_version::basic_version_number_t>(branch));
        }
    }
}


/** \brief Set the current (working) branch key.
 *
 * This function is used to mark that \p branch is now the current branch or
 * the current working branch.
 *
 * The current branch is the one shown to your anonymous visitors. By default
 * only editors can see the other branches and revisions.
 *
 * \param[in] key  The key to the page concerned.
 * \param[in] branch  The number of the new current branch.
 * \param[in] working_branch  Update the current working branch (true) or the current branch (false).
 *
 * \return This function returns a copy of the current branch key.
 */
QString content::set_branch_key(QString const& key, snap_version::version_number_t branch, bool working_branch)
{
    // key in the data table
    QString const current_branch_key(generate_branch_key(key, branch));

    // key in the content table
    QString const current_key(QString("%1::%2")
                .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(working_branch
                        ? name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH_KEY
                        : name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH_KEY)));

    // save the data key in the content table
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    content_table->row(key)->cell(current_key)->setValue(current_branch_key);
    return current_branch_key;
}


/** \brief Initialize the system branch for a specific key.
 *
 * This function initializes all the branch values for the specified
 * path. This is used by the system to initialize a system branch.
 *
 * \todo
 * We have to initialize branches and a similar function for user content
 * will be necessary. User content starts with branch 1. I'm not entirely
 * sure anything more is required than having a way to specify the branch
 * on the call...
 *
 * \param[in] key  The path of the page concerned.
 */
void content::initialize_branch(QString const & key)
{
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());

    // *** BRANCH ***
    //
    snap_version::version_number_t branch_number(snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
    {
        // Last branch
        //
        QString const last_branch_key(QString("%1::%2")
                        .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                        .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_BRANCH)));
        QtCassandra::QCassandraValue branch_value(content_table->row(key)->cell(last_branch_key)->value());
        if(branch_value.nullValue())
        {
            // last branch does not exist yet, create it with zero (0)
            //
            content_table->row(key)->cell(last_branch_key)->setValue(static_cast<snap_version::basic_version_number_t>(branch_number));
        }
        else
        {
            branch_number = branch_value.uint32Value();
        }
    }

    {
        QString const current_branch_key(QString("%1::%2")
                .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH)));
        QtCassandra::QCassandraValue branch_value(content_table->row(key)->cell(current_branch_key)->value());
        if(branch_value.nullValue())
        {
            content_table->row(key)->cell(current_branch_key)->setValue(static_cast<snap_version::basic_version_number_t>(branch_number));
        }
    }

    {
        QString const current_branch_key(QString("%1::%2")
                .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH)));
        QtCassandra::QCassandraValue branch_value(content_table->row(key)->cell(current_branch_key)->value());
        if(branch_value.nullValue())
        {
            content_table->row(key)->cell(current_branch_key)->setValue(static_cast<snap_version::basic_version_number_t>(branch_number));
        }
    }

    {
        // Current branch key
        //
        QString const current_branch_key(get_branch_key(key, false));
        if(current_branch_key.isEmpty())
        {
            // there is no branch yet, create one
            //
            set_branch_key(key, branch_number, false);
        }
    }

    {
        // Current working branch key
        //
        QString const current_branch_key(get_branch_key(key, true));
        if(current_branch_key.isEmpty())
        {
            // there is no branch yet, create one
            //
            set_branch_key(key, branch_number, true);
        }
    }
}


/** \brief Generate a key from a branch, revision, and locale.
 *
 * This function transforms a page key, a branch number, a revision number,
 * and a locale (\<language> or \<language>_\<country>) to a key that
 * is to be used to access the user information in the data table.
 *
 * \param[in] key  The key of the page being worked on.
 * \param[in] branch  The concerned branch.
 * \param[in] locale  The language and country information.
 * \param[in] working_branch  The current branch (false) or the current working branch (true).
 *
 * \return A string representing the end of the key
 */
QString content::get_revision_key(QString const & key, snap_version::version_number_t branch, QString const & locale, bool working_branch)
{
    // key in the content table
    QString current_key(QString("%1::%2::%3")
                .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(working_branch
                        ? name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION_KEY
                        : name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION_KEY))
                .arg(branch));
    if(!locale.isEmpty())
    {
        current_key += "::" + locale;
    }

    // get the data key from the content table
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    QtCassandra::QCassandraValue const value(content_table->row(key)->cell(current_key)->value());
    return value.stringValue();
}


/** \brief Generate the data table key from different parameters.
 *
 * This function generates a data table key using the path to the data
 * (key), the branch and revision, and the locale (language and country).
 * The locale parameter is not mandatory. If empty, then no locale is
 * added to the key. This is legal for any data that cannot be translated.
 *
 * The resulting key looks like:
 *
 * \code
 * <path>#<language>_<country>/<branch>.<revision>
 * \endcode
 *
 * The the language and country being optional. If language is not specified
 * then no country can be specified. The slash is not added when no language
 * is specified.
 *
 * \param[in] key  The key to the page concerned.
 * \param[in] branch  The branch this revision is being saved for.
 * \param[in] revision  The new revision number.
 * \param[in] locale  The language and country information.
 *
 * \return The revision key as used in the data table.
 */
QString content::generate_revision_key(QString const & key, snap_version::version_number_t branch, snap_version::version_number_t revision, QString const & locale)
{
    if(locale.isEmpty())
    {
        return QString("%1#%2.%3").arg(key).arg(branch).arg(revision);
    }

    return QString("%1#%2/%3.%4").arg(key).arg(locale).arg(branch).arg(revision);
}


/** \brief Generate the data table key from different parameters.
 *
 * This function generates a data table key using the path to the data
 * (key), a predefined revision, and the locale (language and country).
 * The locale parameter is not mandatory. If empty, then no locale is
 * added to the key. This is legal for any data that cannot be translated.
 *
 * This function is used whenever your revision number is managed by
 * you and not by the content system. For example the JavaScript and
 * CSS attachment files are read for a Version field. That version may
 * use a different scheme than the normal system version limited to
 * a branch and a revision number. (Although our system is still
 * limited to only numbers, so a version such as 3.5.7b is not supported
 * as is.)
 *
 * The resulting key looks like:
 *
 * \code
 * <path>#<language>_<country>/<revision>
 * \endcode
 *
 * The the language and country being optional. If language is not specified
 * then no country can be specified. The slash is not added when no language
 * is specified.
 *
 * \param[in] key  The key to the page concerned.
 * \param[in] revision  The new branch and revision as a string.
 * \param[in] locale  The language and country information.
 *
 * \return The revision key as used in the data table.
 */
QString content::generate_revision_key(QString const & key, QString const & revision, QString const & locale)
{
    if(locale.isEmpty())
    {
        return QString("%1#%2").arg(key).arg(revision);
    }

    return QString("%1#%2/%3").arg(key).arg(locale).arg(revision);
}


/** \brief Save the revision as current.
 *
 * This function saves the specified \p revision as the current revision.
 * The function takes a set of parameters necessary to generate the
 * key of the current revision.
 *
 * \param[in] key  The page concerned.
 * \param[in] branch  The branch which current revision is being set.
 * \param[in] revision  The revision being set as current.
 * \param[in] locale  The locale (\<language>_\<country>) or an empty string.
 * \param[in] working_branch  Whether this is the current branch (true)
 *                            or current working branch (false).
 */
void content::set_current_revision(QString const& key, snap_version::version_number_t branch, snap_version::version_number_t revision, QString const& locale, bool working_branch)
{
    // revision key in the content table
    QString current_key(QString("%1::%2::%3")
                .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(working_branch
                        ? name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION
                        : name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION))
                .arg(branch));

    // key to the last revision
    QString last_revision_key(QString("%1::%2::%3")
                    .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                    .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_REVISION))
                    .arg(branch));

    if(!locale.isEmpty())
    {
        // append locale if defined
        current_key += "::" + locale;
        last_revision_key += "::" + locale;
    }

    // get the data key from the content table
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    content_table->row(key)->cell(current_key)->setValue(static_cast<snap_version::basic_version_number_t>(revision));

    // avoid changing the revision if defined and larger or equal
    QtCassandra::QCassandraValue const revision_value(content_table->row(key)->cell(last_revision_key)->value());
    if(revision_value.nullValue())
    {
        // last revision does not exist yet, create it
        content_table->row(key)->cell(last_revision_key)->setValue(static_cast<snap_version::basic_version_number_t>(revision));
    }
    else
    {
        snap_version::version_number_t const last_revision(revision_value.uint32Value());
        if(revision > last_revision)
        {
            content_table->row(key)->cell(last_revision_key)->setValue(static_cast<snap_version::basic_version_number_t>(revision));
        }
    }
}


/** \brief Set the current (working) revision key.
 *
 * This function saves the current revision key or current working revision
 * key in the database as a string. This is the string used when people access
 * the data (read-only mode).
 *
 * This function is often called when creating a new revision key as the
 * user, in most cases, will want the latest revision to become the current
 * revision.
 *
 * You may call the generate_revision_key() function to regenerate the
 * revision key without saving it in the database too.
 *
 * \param[in] key  The key to the page concerned.
 * \param[in] branch  The branch this revision is being saved for.
 * \param[in] revision  The new revision number.
 * \param[in] locale  The language and country information.
 * \param[in] working_branch  The current branch (false) or the current working branch (true).
 *
 * \return This function returns a copy of the revision key just computed.
 */
QString content::set_revision_key(QString const & key, snap_version::version_number_t branch, snap_version::version_number_t revision, QString const & locale, bool working_branch)
{
    // key in the data table
    QString const current_revision_key(generate_revision_key(key, branch, revision, locale));

    // key in the content table
    QString current_key(QString("%1::%2::%3")
                .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(working_branch
                        ? name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION_KEY
                        : name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION_KEY))
                .arg(branch));
    if(!locale.isEmpty())
    {
        current_key += "::" + locale;
    }

    // save the data key in the content table
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    content_table->row(key)->cell(current_key)->setValue(current_revision_key);
    return current_revision_key;
}


/** \brief Save a revision key from a revision string.
 *
 * This function is used when the branching mechanism is used with a scheme
 * that does not follow the internal \<branch>.\<revision> scheme. For example
 * a JavaScript source must define a version and that version most often will
 * have 2 or 3 numbers ([0-9]+) separated by periods (.). These are handled
 * with this function.
 *
 * You may call the generate_revision_key() function to regenerate the
 * revision key without saving it in the database too.
 *
 * \param[in] key  The key to the page concerned.
 * \param[in] branch  The branch of the page.
 * \param[in] revision  The new revision string.
 * \param[in] locale  The language and country information.
 * \param[in] working_branch  The current branch (false) or the current
 *                            working branch (true).
 *
 * \return A copy of the current revision key saved in the database.
 */
QString content::set_revision_key(QString const & key, snap_version::version_number_t branch, QString const & revision, QString const & locale, bool working_branch)
{
    // key in the data table
    QString const current_revision_key(generate_revision_key(key, revision, locale));

    // key in the content table
    QString current_key(QString("%1::%2::%3")
                .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(working_branch
                        ? name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION_KEY
                        : name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION_KEY))
                .arg(branch));
    if(!locale.isEmpty())
    {
        current_key += "::" + locale;
    }

    // save the data key in the content table
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    content_table->row(key)->cell(current_key)->setValue(current_revision_key);
    return current_revision_key;
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
