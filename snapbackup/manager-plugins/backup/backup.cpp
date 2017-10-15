// Snap Websites Server -- manage the snapbackup settings
// Copyright (C) 2016-2017  Made to Order Software Corp.
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

// backup
//
#include "backup.h"

// our lib
//
#include "snapmanager/form.h"

// snapwebsites lib
//
#include <snapwebsites/file_content.h>
#include <snapwebsites/join_strings.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qdomxpath.h>
#include <snapwebsites/string_pathinfo.h>

// Qt lib
//
#include <QFile>

// C lib
//
#include <sys/file.h>

// last entry
//
#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(backup, 1, 0)


namespace
{

char const * g_cron_filename = "/etc/cron.daily/snapbackup";



//void file_descriptor_deleter(int * fd)
//{
//    if(close(*fd) != 0)
//    {
//        int const e(errno);
//        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
//    }
//}


} // no name namespace



/** \brief Get a fixed backup plugin name.
 *
 * The backup plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_BACKUP_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_BACKUP_...");

    }
    NOTREACHED();
}




/** \brief Initialize the backup plugin.
 *
 * This function is used to initialize the backup plugin object.
 */
backup::backup()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the backup plugin.
 *
 * Ensure the backup object is clean before it is gone.
 */
backup::~backup()
{
}


/** \brief Get a pointer to the backup plugin.
 *
 * This function returns an instance pointer to the backup plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the backup plugin.
 */
backup * backup::instance()
{
    return g_plugin_backup_factory.instance();
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
QString backup::description() const
{
    return "Manage the snapbackup settings.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString backup::dependencies() const
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
int64_t backup::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize backup.
 *
 * This function terminates the initialization of the backup plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void backup::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(backup, "server", snap_manager::manager, retrieve_status, _1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void backup::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    file_content in(g_cron_filename);
    if(in.read_all())
    {
        std::string const & content(in.get_content());
        std::string::size_type const start(content.find("HOST="));
        if(start != std::string::npos)
        {
            std::string::size_type const end(content.find_first_of("\r\n", start + 5));
            if(end != std::string::npos)
            {
                snap_manager::status_t const host(snap_manager::status_t::state_t::STATUS_STATE_INFO,
                                                      get_plugin_name(),
                                                      "host",
                                                      QString::fromUtf8(content.substr(start + 5, end - start - 5).c_str()));
                server_status.set_field(host);
            }
        }
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
bool backup::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "host")
    {
        // the list if frontend snapmanagers that are to receive statuses
        // of the cluster computers; may be just one computer; should not
        // be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Cassandra Host IP"
                        , s.get_field_name()
                        , s.get_value()
                        , "Enter the IP address of one of your Cassandra node. The Cassandra C++ Driver will actually connect to any number of nodes as required. Obviously, if that one node is down, the backup may fail (I do not think that the C++ driver caches possible connection points.)"
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
 * \return true if the new_value was applied successfully.
 */
bool backup::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::set<QString> & affected_services)
{
    NOTUSED(old_or_installation_value);
    NOTUSED(affected_services);

    // restore defaults?
    //
    bool const use_default_value(button_name == "restore_default");

    if(field_name == "host")
    {
        file_content in_out(g_cron_filename);
        if(in_out.read_all())
        {
            std::string const & content(in_out.get_content());
            std::string::size_type const start(content.find("HOST="));
            if(start != std::string::npos)
            {
                std::string::size_type const end(content.find_first_of("\r\n", start + 5));
                if(end != std::string::npos)
                {
                    std::string const new_content(content.substr(0, start + 5)
                                                + (use_default_value ? "127.0.0.1" : new_value)
                                                + content.substr(end));
                    in_out.set_content(new_content);
                    if(in_out.write_all())
                    {
                        return true;
                    }

                    SNAP_LOG_ERROR("could not overwrite the snapbackup CRON script");
                }
                else
                {
                    SNAP_LOG_ERROR("could not find the end of the line for the HOST=... variable");
                }
            }
            else
            {
                SNAP_LOG_ERROR("could not find the HOST=... variable");
            }
        }
        else
        {
            SNAP_LOG_ERROR("could not read the snapbackup CRON script");
        }

        return false;
    }

    return false;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
