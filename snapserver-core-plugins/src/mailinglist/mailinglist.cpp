// Snap Websites Server -- manage mailing lists for other plugins
// Copyright (C) 2013-2017  Made to Order Software Corp.
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

#include "mailinglist.h"

#include "../content/content.h"

#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(mailinglist, 1, 0)


/** \brief Get a fixed mailinglist plugin name.
 *
 * The mailinglist plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_MAILINGLIST_TABLE:
        return "mailinglist";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_MAILINGLIST_...");

    }
    NOTREACHED();
}


/** \brief Initialize the mailing list.
 *
 * The list is initialized by creating the table object, the
 * row object with the corresponding list (specified by \p name)
 * and reseting the different flags.
 *
 * The list is then ready to be read with the next() function.
 *
 * The \p parent parameter is used to allocate the table object
 * with the get_mailinglist_table() function.
 *
 * \param[in] parent  The mailinglist object initializing this list.
 * \param[in] list_name  The name of the list to attach to.
 *
 * \sa get_mailinglist_table()
 */
mailinglist::list::list(mailinglist * parent, QString const & list_name)
    : f_parent(parent)
    , f_name(list_name)
    , f_table(f_parent->get_mailinglist_table())
    , f_row(f_table->getRow(f_name))
    , f_column_predicate(std::make_shared<libdbproxy::cell_range_predicate>())
    //, f_cells() -- auto-init
    , f_c(f_cells.end())
    //, f_done(false) -- auto-init;
{
    f_column_predicate->setCount(1000);
    f_column_predicate->setIndex();
    //f_c = f_cells.end();
    f_row->clearCache();  // make sure no query is laying around (cannot do that in next() anymore)
}


/** \brief Clean up the mailing list.
 *
 * This function cleans up the list object.
 */
mailinglist::list::~list()
{
}


/** \brief Return the name of the mailing list being read.
 *
 * This function returns the name of the list being read.
 *
 * \return The name as passed when creating the list object.
 */
QString mailinglist::list::name() const
{
    return f_name;
}


/** \brief Read the next email from a mailing list.
 *
 * This function reads the next email from a mailing list. Note that
 * 1000 emails are cached at once so if there are 1000 or less users
 * then this function will access the database only once.
 *
 * \return The next email or an empty string once the end of the list is reached.
 */
QString mailinglist::list::next()
{
    if(f_done)
    {
        return "";
    }

    if(f_c == f_cells.end())
    {
        //f_row->clearCache();
        f_row->readCells(f_column_predicate);
        f_cells = f_row->getCells();
        f_c = f_cells.begin();
        if(f_c == f_cells.end())
        {
            f_done = true;
            return "";
        }
    }

    libdbproxy::value value((*f_c)->getValue());
    ++f_c;
    // TODO: write a loop so we properly handle the case of an empty
    //       entry (although it should not happen, we never know!)
    return value.stringValue();
}




/** \brief Initialize the mailinglist plugin.
 *
 * This function is used to initialize the mailinglist plugin object.
 */
mailinglist::mailinglist()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the mailinglist plugin.
 *
 * Ensure the mailinglist object is clean before it is gone.
 */
mailinglist::~mailinglist()
{
}


/** \brief Get a pointer to the mailinglist plugin.
 *
 * This function returns an instance pointer to the mailinglist plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the mailinglist plugin.
 */
mailinglist * mailinglist::instance()
{
    return g_plugin_mailinglist_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString mailinglist::settings_path() const
{
    return "/admin/settings/mailinglist";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString mailinglist::icon() const
{
    return "/images/mailinglist/mailinglist-logo-64x64.png";
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString mailinglist::description() const
{
    return "Handle lists of emails for systems such as newsletters."
        " This plugin is responsible to offer users a way to subscribe"
        " and unsubscribe from a mailing list. Note that there is a"
        " higher level ban capability for users to make sure their email"
        " is just never ever used by us.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString mailinglist::dependencies() const
{
    return "|content|editor|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t mailinglist::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 2, 20, 20, 16, 56, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our administration pages, etc.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables
 *            added to the database by this update (in micro-seconds).
 */
void mailinglist::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize mailinglist.
 *
 * This function terminates the initialization of the mailinglist plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void mailinglist::bootstrap(snap_child * snap)
{
    f_snap = snap;
}


/** \brief Initialize the emails table.
 *
 * This function creates the users table if it doesn't exist yet. Otherwise
 * it simple returns the existing Cassandra table.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The table is a list of emails (row keys) and passwords. Additional user
 * data is generally added by other plugins (i.e. address, phone number,
 * what the user bought before, etc.)
 *
 * \return The pointer to the users table.
 */
libdbproxy::table::pointer_t mailinglist::get_mailinglist_table()
{
    return f_snap->get_table(get_name(name_t::SNAP_NAME_MAILINGLIST_TABLE));
}


/** \fn void mailinglist::name_to_list(const QString& name, QSharedPointer<list>& emails)
 * \brief Prepare the email for the filter_email signal.
 *
 * This function checks the parameters validity and returns true if it will be
 * possible to ready a list of emails from the name of a list.
 *
 * \param[in] name  The name of the list to access.
 * \param[in] emails  The resulting list of emails.
 */


/** \brief Prepare the email for the filter_email signal.
 *
 * This function checks the parameters validity and returns true if it will be
 * possible to ready a list of emails from the name of a list.
 *
 * Note that if the input emails pointer is not nullptr then the function does not
 * change the pointer. This allows any other plugin to define a mailing list
 * first. This means if you loop over a list of emails and check whether the
 * bane is a mailing list name, you'll want to clear the pointer before each
 * call to the name_to_list() signal:
 *
 * \code
 *  for(;;)
 *  {
 *      QSharedPointer emails; // nullptr by default
 *      mailinglist::name_to_list(name, emails);
 *      ...
 *  }
 * \endcode
 *
 * \code
 *  QSharedPointer emails; // nullptr by default
 *  for(;;)
 *  {
 *      emails.clear(); // clear before call
 *      mailinglist::name_to_list(name, emails);
 *      ...
 *  }
 * \endcode
 *
 * Once returned, the list of emails can be read using the next() call
 * if the emails until the function returns an empty string.
 *
 * The function does not set the emails shared pointer to anything if the
 * name does not name an existing mailing list. This means the function
 * returns a nullptr pointer if no mailing list with that name exists.
 *
 * \param[in] name  The name of the list to access.
 * \param[in,out] emails  The resulting list of emails.
 */
bool mailinglist::name_to_list_impl(QString const & name, QSharedPointer<list> & emails)
{
    // only set if not already set
    if(!emails)
    {
        // first make sure that the row exists, if not, it is not a mailing list
        libdbproxy::table::pointer_t table(get_mailinglist_table());
        if(table->exists(name))
        {
            emails = QSharedPointer<list>(new list(this, name));
        }
    }

    return true;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
