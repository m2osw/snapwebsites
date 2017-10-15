// Snap Websites Server -- save_info_t implementation
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
 * \brief The implementation of save_info_t for the editor plugin.
 *
 * This file contains the implementation of the save_infot_t class.
 *
 * The class is used whenever the editor receives a POST from the client
 * and saves the data.
 */

#include "editor.h"

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(editor)


/** \brief Initialize a save_info_t object.
 *
 * This constructor saves the concerned ipath and the corresponding
 * revision, secret, and draft rows.
 *
 * The class also holds a "modified" flag, which is false by default.
 * The editor_save() function changes that flag to true when it finds
 * that a field changed. You may also change it within your own
 * implementation of the save_editor_fields() signal. That will force
 * the editor to mark the data as modified by changing LAST_MODIFIED
 * fields and calling the necessary functions.
 *
 * The class also holds a flag to know whether a validation generated
 * an error. That flag is protected and it cannot be changed after
 * the validation ran. If true, then an error occurred and some or
 * all of the data of this form may be invalid.
 *
 * \todo
 * Look into moving the determination of the revision, secret, and
 * draft rows to this class instead of the editor_save() function.
 *
 * \param[in] p_ipath  The path to the page being saved.
 * \param[in] p_revision_row  The row to access the revision data.
 * \param[in] p_secret_row  The row to access secret data.
 * \param[in] p_draft_row  The row to access the data saved as a draft
 * (i.e. on errors data is saved in the draft_row instead of the revision
 * or secret row)
 */
save_info_t::save_info_t(
            content::path_info_t & p_ipath,
            QDomDocument & p_editor_widgets,
            QtCassandra::QCassandraRow::pointer_t p_revision_row,
            QtCassandra::QCassandraRow::pointer_t p_secret_row,
            QtCassandra::QCassandraRow::pointer_t p_draft_row)
    : f_ipath(p_ipath)
    , f_editor_widgets(p_editor_widgets)
    , f_revision_row(p_revision_row)
    , f_secret_row(p_secret_row)
    , f_draft_row(p_draft_row)
    //, f_modified(false)
    //, f_has_errors(false)
{
}


/** \brief The ipath being saved.
 *
 * This path_info_t object represents the page being saved. When saving
 * a form, this represents the path to that form.
 *
 * \return The writable reference to the ipath representing the page being
 *         saved.
 */
content::path_info_t & save_info_t::ipath()
{
    return f_ipath;
}


/** \brief The editor_widgets XML document.
 *
 * This function returns a reference to the XML document with the list
 * of widgets being worked on.
 *
 * \return The writable reference to the QDomDocument representing the form
 *         being saved.
 */
QDomDocument & save_info_t::editor_widgets()
{
    return f_editor_widgets;
}


/** \brief Get a pointer to the revision row.
 *
 * This function returns a pointer to the revision row.
 *
 * \return A pointer to the reivision row.
 */
QtCassandra::QCassandraRow::pointer_t save_info_t::revision_row() const
{
    return f_revision_row;
}


/** \brief Get a pointer to the secret row.
 *
 * This function returns a pointer to the secret row.
 *
 * \return A pointer to the secret row.
 */
QtCassandra::QCassandraRow::pointer_t save_info_t::secret_row() const
{
    return f_secret_row;
}


/** \brief Get a pointer to the draft row.
 *
 * This function returns a pointer to the draft row.
 *
 * \return A pointer to the draft row.
 */
QtCassandra::QCassandraRow::pointer_t save_info_t::draft_row() const
{
    return f_draft_row;
}


/** \brief Lock further modification of various flags.
 *
 * This function locks this save_info_t object from futher
 * modifications.
 *
 * At this time, this is limited to the mark_as_having_errors()
 * function. Once this function was called, the validation
 * (f_has_errors) results cannot be changed. If you need to
 * validate something, you must put it in the validation
 * signal and returns an error then.
 */
void save_info_t::lock()
{
    f_locked = true;
}


/** \brief Mark that the data received modified the database.
 *
 * This function can be used to mark that the editor_save()
 * modified something and thus that it has to trigger the
 * necessary calls and mark the database as modified.
 */
void save_info_t::mark_as_modified()
{
    f_modified = true;
}


/** \brief Check whether the editor_save() modified the database.
 *
 * If this function returns true, then the save modified the database
 * already. Note that since you are likely to check this flag while
 * inside the save_editor_fields() signal, it may be false even though
 * a later implementation of the save_editor_fields() may set it to
 * true.
 *
 * \return true if the data is considered modified.
 */
bool save_info_t::modified() const
{
    return f_modified;
}


/** \brief Mark that a value is erroneous.
 *
 * This function is called whenever a validation fails against one of
 * the values proposed for that form.
 *
 * \exception editor_exception_locked
 * If the lock() function was already called, this function raises
 * this exception.
 */
void save_info_t::mark_as_having_errors()
{
    if(f_locked)
    {
        throw editor_exception_locked("the save_info_t is locked, the mark_as_having_errors() function cannot be called anymore.");
    }

    f_has_errors = true;
}


/** \brief Whether an error occurred while validating the data.
 *
 * This function returns true if the validation found one or more
 * fields as errorneous. Note that all fields are checked first,
 * then the data gets saved. That allows us to save all the fields
 * in the draft if one or more errors occurs.
 *
 * If you implement the save_editor_fields(), it is important that
 * you check this flag to know whether to save the data in the
 * draft row or the revision/secret rows. Also remember that in
 * the draft row you save all the fields as strings.
 *
 * In most cases, if this flag is true when your save_editor_fields()
 * function is called, you should return immediately.
 *
 * \return true if one or more validation errors were found.
 */
bool save_info_t::has_errors() const
{
    return f_has_errors;
}




SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
