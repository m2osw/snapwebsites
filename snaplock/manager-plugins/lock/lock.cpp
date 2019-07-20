// Snap Websites Server -- manage the snaplock settings
// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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


// self
//
#include "lock.h"


// snapmanager lib
//
#include <snapmanager/form.h>


// snapwebsites lib
//
#include <snapwebsites/log.h>


// snapdev lib
//
#include <snapdev/not_reached.h>
#include <snapdev/not_used.h>


// last include
//
#include <snapdev/poison.h>



SNAP_PLUGIN_START(lock, 1, 0)


namespace
{


char const *    g_configuration_filename   = "snaplock";

char const *    g_configuration_d_filename   = "/etc/snapwebsites/snapwebsites.d/snaplock.conf";


} // no name namespace



/** \brief Get a fixed lock plugin name.
 *
 * The lock plugin makes use of different fixed names. This function
 * ensures that you always get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_SNAPMANAGERCGI_LOCK_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_LOCK_...");

    }
    NOTREACHED();
}




/** \brief Initialize the lock plugin.
 *
 * This function is used to initialize the lock plugin object.
 */
lock::lock()
{
}


/** \brief Clean up the lock plugin.
 *
 * Ensure the lock object is clean before it is gone.
 */
lock::~lock()
{
}


/** \brief Get a pointer to the lock plugin.
 *
 * This function returns an instance pointer to the lock plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the lock plugin.
 */
lock * lock::instance()
{
    return g_plugin_lock_factory.instance();
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
QString lock::description() const
{
    return "Manage the snaplock settings.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString lock::dependencies() const
{
    return "|server|";
}


/** \brief Check whether updates are necessary.
 *
 * This function is ignored in snapmanager.cgi and snapmanagerdaemon plugins.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t lock::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize lock.
 *
 * This function terminates the initialization of the lock plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void lock::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(lock, "server", snap_manager::manager, retrieve_status, _1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void lock::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    snap_config snap_lock_conf(g_configuration_filename);

    {
        QString priority(snap_lock_conf["candidate_priority"]);
        if(priority.isEmpty())
        {
            // default is 14
            //
            priority = "14";
        }
        snap_manager::status_t const priority_widget(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "candidate_priority"
                    , priority);
        server_status.set_field(priority_widget);
    }
}






/** \brief Transform a value to HTML for display.
 *
 * This function expects the name of a field and its value. It then adds
 * the necessary HTML to the specified element to display that value.
 *
 * If the value is editable, then the function creates a form with the
 * necessary information (hidden fields) to save the data as required
 * by that field (i.e. update a .conf/.xml file, create a new file,
 * remove a file, etc.)
 *
 * \param[in] server_status  The map of statuses.
 * \param[in] s  The field being worked on.
 *
 * \return true if we handled this field.
 */
bool lock::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "candidate_priority")
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        // we do not include 0 since that's reserved for already elected
        // leaders when a re-election happens
        //
        snap::snap_string_list priorities;
        priorities << "1";
        priorities << "2";
        priorities << "3";
        priorities << "4";
        priorities << "5";
        priorities << "6";
        priorities << "7";
        priorities << "8";
        priorities << "9";
        priorities << "10";
        priorities << "11";
        priorities << "12";
        priorities << "13";
        priorities << "14";
        priorities << "off";

        snap_manager::widget_select::pointer_t field(std::make_shared<snap_manager::widget_select>(
                          "Candidate Priority"
                        , s.get_field_name()
                        , priorities
                        , s.get_value()
                        , "<p>Select a priority for this candidate in the snaplock leaders election.</p>"
                         "<p>A lower priority means a greater chance to be elected as a leader.</p>"
                         "<p><strong>OFF</strong> means that the computer does not participate as a candidate.</p>"
                         "<p>Note that you must have at least three computers that are NOT turned OFF in your cluster.</p>"
                        ));
        f.add_widget(field);
        f.generate(parent, uri);
        return true;
    }

    return false;
}


/** \brief Save 'new_value' in field 'field_name'.
 *
 * This function saves 'new_value' in 'field_name'.
 *
 * \param[in] button_name  The name of the button the user clicked.
 * \param[in] field_name  The name of the field to update.
 * \param[in] new_value  The new value to save in that field.
 * \param[in] old_or_installation_value  The old value, just in case
 *            (usually ignored,) or the installation values (only
 *            for the self plugin that manages bundles.)
 * \param[in] affected_services  The list of services that were affected
 *            by this call.
 *
 * \return true if the new_value was recognized as one we manage.
 */
bool lock::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::set<QString> & affected_services)
{
    NOTUSED(old_or_installation_value);
    NOTUSED(button_name);

    if(field_name == "candidate_priority")
    {
        // TODO: check whether we switch between a priority of 15 and not 15?
        //       (i.e. that's the only good reason to restart snaplock in
        //       this case; otherwise continue running as it was)
        //
        affected_services.insert("snaplock");

        snap_config snap_lock_conf(g_configuration_filename);
        snap_lock_conf[field_name] = new_value;

        NOTUSED(f_snap->replace_configuration_value(g_configuration_d_filename, field_name, new_value));

        return true;
    }

    return false;
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
