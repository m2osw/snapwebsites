// Snap Websites Server -- manage the snapbounce settings
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// mailserver
//
#include "mailserver.h"


// snapmanager lib
//
#include <snapmanager/form.h>


// snaplogger lib
//
#include <snaplogger/message.h>


// snapdev lib
//
#include <snapdev/not_reached.h>
#include <snapdev/not_used.h>


// Qt lib
//
#include <QFile>
#include <QProcess>


// last include
//
#include <snapdev/poison.h>



SNAP_PLUGIN_START(mailserver, 1, 0)




namespace
{

QString const SETUP_MAILSERVER = "setup_mailserver";


void file_descriptor_deleter(int * fd)
{
    if(close(*fd) != 0)
    {
        int const e(errno);
        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
    }
}


} // no name namespace



/** \brief Get a fixed mailserver plugin name.
 *
 * The mailserver plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_MAILSERVER_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_MAILSERVER_...");

    }
    snapdev::NOT_REACHED();
}




/** \brief Initialize the mailserver plugin.
 *
 * This function is used to initialize the mailserver plugin object.
 */
mailserver::mailserver()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the mailserver plugin.
 *
 * Ensure the mailserver object is clean before it is gone.
 */
mailserver::~mailserver()
{
}


/** \brief Get a pointer to the mailserver plugin.
 *
 * This function returns an instance pointer to the mailserver plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the mailserver plugin.
 */
mailserver * mailserver::instance()
{
    return g_plugin_mailserver_factory.instance();
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
QString mailserver::description() const
{
    return "Manage the snapbounce settings.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString mailserver::dependencies() const
{
    return "|server|";
}


bool mailserver::is_installed() const
{
    // for now we just check whether the executable is here, this is
    // faster than checking whether the package is installed.
    //
    return access("/usr/sbin/named", R_OK | X_OK) == 0;
}


/** \brief Check whether updates are necessary.
 *
 * This function is ignored in snapmanager.cgi and snapmanagerdaemon plugins.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t mailserver::do_update(int64_t last_updated)
{
    snapdev::NOT_USED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize mailserver.
 *
 * This function terminates the initialization of the mailserver plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void mailserver::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(mailserver, "server", snap_manager::manager, retrieve_status, boost::placeholders::_1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void mailserver::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    if(!is_installed())
    {
        // no fields whatsoever if the package is not installed
        // (remember that we are part of snapmanagercgi and that's
        // going to be installed!)
        //
        return;
    }

    snap_manager::status_t const ctl(
            snap_manager::status_t::state_t::STATUS_STATE_INFO
            , get_plugin_name()
            , SETUP_MAILSERVER
            , QString()
            );
    server_status.set_field(ctl);
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
bool mailserver::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_ERROR)
    {
        return false;
    }

    snap_manager::form f(
            get_plugin_name()
            , s.get_field_name()
            , snap_manager::form::FORM_BUTTON_SAVE
            );
    snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                "Setup Mailserver Domain"
                , s.get_field_name()
                , s.get_value()
                , "Enter the mailserver domain. This will generate the"
                  " SPF, DKIM and DMARC keys and setup for bind."
                ));
    f.add_widget(field);
    f.generate(parent, uri);
    return true;
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
 * \return true if the new_value was applied successfully.
 */
bool mailserver::apply_setting(QString const & button_name
    , QString const & field_name
    , QString const & new_value
    , QString const & old_or_installation_value
    , std::set<QString> & affected_services)
{
    snapdev::NOT_USED(button_name, field_name, old_or_installation_value, affected_services);

    QFile setup_postfix_script( "/tmp/setup-postfix.sh" );
    //
    // Overwrite the script every time
    //
    setup_postfix_script.remove();
    if( !QFile::copy( ":/setup-postfix.sh", setup_postfix_script.fileName() ) )
    {
        QString const errmsg = QString("Cannot copy setup-postfix.sh file!");
        SNAP_LOG_ERROR(qPrintable(errmsg));
        return false;
    }
    //
    setup_postfix_script.setPermissions
        ( setup_postfix_script.permissions()
          | QFileDevice::ExeOwner
          | QFileDevice::ExeUser
          | QFileDevice::ExeGroup
        );

    if( QProcess::execute( setup_postfix_script.fileName(), {new_value} ) != 0 )
    {
        SNAP_LOG_ERROR("Could not execute spf/dkim/dmarc creation script! Params=")
            (qPrintable(new_value));
        return false;
    }

    return true;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
