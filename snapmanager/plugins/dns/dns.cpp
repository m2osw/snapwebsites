// Snap Websites Server -- handle user DNS installation
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

// dns
//
#include "dns.h"

// our lib
//
#include "snapmanager/form.h"

// snapwebsites lib
//
#include <snapwebsites/chownnm.h>
#include <snapwebsites/log.h>
#include <snapwebsites/mkdir_p.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

// Qt5
//
#include <QtCore>

// C++ lib
//
#include <fstream>

// last entry
//
#include <snapwebsites/poison.h>


namespace
{
    QString const CREATE_MASTER_ZONE = QString("create_master_zone");
    QString const CREATE_SLAVE_ZONE  = QString("create_slave_zone");
    QString const SHOW_SLAVE_ZONES   = QString("show_slave_zones");
}


SNAP_PLUGIN_START(dns, 1, 0)


/** \brief Get a fixed dns plugin name.
 *
 * The dns plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_DNS_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_DNS_...");

    }
    NOTREACHED();
}




/** \brief Initialize the dns plugin.
 *
 * This function is used to initialize the dns plugin object.
 */
dns::dns()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the dns plugin.
 *
 * Ensure the dns object is clean before it is gone.
 */
dns::~dns()
{
}


/** \brief Get a pointer to the dns plugin.
 *
 * This function returns an instance pointer to the dns plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the dns plugin.
 */
dns * dns::instance()
{
    return g_plugin_dns_factory.instance();
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
QString dns::description() const
{
    return "Manage the dnspublic key for users on a specific server.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString dns::dependencies() const
{
    return "|server|";
}


bool dns::is_installed()
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
int64_t dns::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize dns
 *
 * This function terminates the initialization of the dnsplugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void dns::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(dns, "server", snap_manager::manager, retrieve_status, _1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void dns::on_retrieve_status(snap_manager::server_status & server_status)
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

    // we want one field per user on the system, at this point we
    // assume that the system does not have hundreds of users since
    // only a few admins should be permitted on those computers
    // anyway...
    //
    QStringList exts    = {"zone"}; //{"key","crt","conf"};
    QStringList filters;
    for( auto const &ext : exts )
    {
        filters << QString("*.%1").arg(ext);
    }

    // Get all of the master zone files
    //
    {
        QDir keys_path;
        keys_path.setPath( "/etc/bind/" );
        keys_path.setNameFilters( filters );
        keys_path.setSorting( QDir::Name );
        keys_path.setFilter( QDir::Files );

        for( QFileInfo const &info : keys_path.entryInfoList() )
        {
            SNAP_LOG_TRACE("master zone file info=")(info.filePath());

            // create a field for this one, it worked
            //
            QString const filename = info.filePath();
            QFile file( filename );
            if( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
            {
                QTextStream in(&file);
                snap_manager::status_t const ctl
                    ( snap_manager::status_t::state_t::STATUS_STATE_INFO
                      , get_plugin_name()
                      , filename
                      , in.readAll()
                    );
                server_status.set_field(ctl);
            }
            else
            {
                QString const errmsg = QString("Cannot open '%1' for reading!").arg(info.filePath());
                SNAP_LOG_ERROR(qPrintable(errmsg));
                //throw dns_exception( errmsg );
            }
        }
    }

    // Look to see if we have slave zones on this machine. If so, show them
    // in one edit box.
    {
        QDir keys_path;
        keys_path.setPath( "/var/cache/bind/" );
        keys_path.setNameFilters( filters );
        keys_path.setSorting( QDir::Name );
        keys_path.setFilter( QDir::Files );

        QStringList slave_zone_list;

        for( QFileInfo const &info : keys_path.entryInfoList() )
        {
            SNAP_LOG_TRACE("slave zone file info=")(info.filePath());

            // create a field for this one, it worked
            //
            slave_zone_list << info.filePath();
        }

        if( !slave_zone_list.isEmpty() )
        {
            snap_manager::status_t const ctl
                ( snap_manager::status_t::state_t::STATUS_STATE_INFO
                  , get_plugin_name()
                  , SHOW_SLAVE_ZONES
                  , slave_zone_list.join("\n")
                );
            server_status.set_field(ctl);
        }
    }

    // Master Zone
    //
    {
        snap_manager::status_t const ctl(
            snap_manager::status_t::state_t::STATUS_STATE_INFO
            , get_plugin_name()
            , CREATE_MASTER_ZONE
            , QString()
            );
        server_status.set_field(ctl);
    }

    // Slave Zone
    //
    {
        snap_manager::status_t const ctl(
            snap_manager::status_t::state_t::STATUS_STATE_INFO
            , get_plugin_name()
            , CREATE_SLAVE_ZONE
            , QString()
            );
        server_status.set_field(ctl);
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
bool dns::display_value ( QDomElement parent
                        , snap_manager::status_t const & s
                        , snap::snap_uri const & uri
                        )
{
    QDomDocument doc(parent.ownerDocument());

    // in case of an error, we do not let the user do anything
    // so let the default behavior do its thing, it will show the
    // field in a non-editable manner
    //
    if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_ERROR)
    {
        return false;
    }

    if( s.get_field_name() == CREATE_MASTER_ZONE )
    {
        snap_manager::form f(
                get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_SAVE
                );
        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                    "Enter the domain to add, followed by the IP address and slave IP address, comma delimited:"
                    , s.get_field_name()
                    , s.get_value()
                    , "<p>For example:</p>"
                      " foobar.net, 123.4.5.6, 123.4.5.7"
                    ));
        f.add_widget(field);
        f.generate(parent, uri);
        return true;
    }

    if( s.get_field_name() == CREATE_SLAVE_ZONE )
    {
        snap_manager::form f(
                get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_SAVE
                );
        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                    "Enter the domain and master IP address of the slave zone you wish to mirror, comma delimited."
                    , s.get_field_name()
                    , ""
                    , "<p>For example:</p>"
                      " foobar.net, 123.4.5.6"
                    ));
        f.add_widget(field);
        f.generate(parent, uri);
        return true;
    }

    if( s.get_field_name() == SHOW_SLAVE_ZONES )
    {
        snap_manager::form f(
                get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_NONE
                );
        snap_manager::widget_text::pointer_t field(std::make_shared<snap_manager::widget_text>(
                    "Slave DNS Zones:"
                    , s.get_field_name()
                    , s.get_value()
                    , "This field is <b>READ-ONLY</b>."
                    ));
        f.add_widget(field);
        f.generate(parent, uri);
        return true;
    }

    // the list of client config files
    //
    snap_manager::form f(
            get_plugin_name()
            , s.get_field_name()
            , snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_RESET
            );
    snap_manager::widget_text::pointer_t field(std::make_shared<snap_manager::widget_text>(
                "DNS Zone file."
                , s.get_field_name()
                , s.get_value()
                , "Make modifications, and then click save. This cannot be reversed!"
                  " PS: don't forget to bump the serial number!"
                ));
    f.add_widget(field);
    f.generate(parent, uri);
    return true;
}


/** \brief Does nothing
 *
 * \param[in] button_name  The name of the button the user clicked.
 * \param[in] field_name  The name of the field to update.
 * \param[in] new_value  The new value to save in that field.
 * \param[in] old_or_installation_value  The old value, just in case
 *            (usually ignored,) or the installation values (only
 *            for the self plugin that manages bundles.)
 *
 * \return true if the new_value was applied successfully.
 */
bool dns::apply_setting ( QString const & button_name
                        , QString const & field_name
                        , QString const & new_value
                        , QString const & old_or_installation_value
                        , std::set<QString> & affected_services
                        )
{
    NOTUSED(affected_services);

    if( field_name == CREATE_MASTER_ZONE )
    {
        QFile add_dns_zone( "/tmp/add_dns_zone.sh" );
        //
        // Overwrite the script every time
        //
        add_dns_zone.remove();
        if( !QFile::copy( ":/add_dns_zone.sh", add_dns_zone.fileName() ) )
        {
            QString const errmsg = QString("Cannot copy add_dns_zone.sh file!");
            SNAP_LOG_ERROR(qPrintable(errmsg));
            return false;
        }
        //
        add_dns_zone.setPermissions
                ( add_dns_zone.permissions()
                | QFileDevice::ExeOwner
                | QFileDevice::ExeUser
                | QFileDevice::ExeGroup
                );

        QStringList params = new_value.split(",");
        SNAP_LOG_TRACE("add_dns_zone.sh ")(params.join(" "));

        if( QProcess::execute( "/tmp/add_dns_zone.sh", params ) != 0 )
        {
            SNAP_LOG_ERROR("Could not execute zone creation script! Params=")
                (qPrintable(new_value));
            return false;
        }

        return true;
    }

    if( field_name == CREATE_SLAVE_ZONE )
    {
        QFile add_slave_zone( "/tmp/add_slave_zone.sh" );
        //
        // Overwrite the script every time
        //
        add_slave_zone.remove();
        if( !QFile::copy( ":/add_slave_zone.sh", add_slave_zone.fileName() ) )
        {
            QString const errmsg = QString("Cannot copy add_slave_zone.sh file!");
            SNAP_LOG_ERROR(qPrintable(errmsg));
            return false;
        }
        //
        add_slave_zone.setPermissions
                ( add_slave_zone.permissions()
                | QFileDevice::ExeOwner
                | QFileDevice::ExeUser
                | QFileDevice::ExeGroup
                );

        QStringList params = new_value.split(",");

        if( QProcess::execute( "/tmp/add_slave_zone.sh", params ) != 0 )
        {
            SNAP_LOG_ERROR("Could not execute slave zone creation script! Params=")
                (qPrintable(new_value));
            return false;
        }

        return true;
    }

    if( field_name == SHOW_SLAVE_ZONES )
    {
        return true;
    }

    if   ( button_name == "save"
        || button_name == "reset"
         )
    {
        QFile file( field_name );
        if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
        {
            QString const errmsg = QString("Cannot open '%1' for writing!").arg(file.fileName());
            SNAP_LOG_ERROR(qPrintable(errmsg));
            return false;
        }

        QTextStream out( &file );
        if( button_name == "save" )
        {
            out << new_value;
        }
        else
        {
            out << old_or_installation_value;
        }
        return true;
    }

    return false;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
