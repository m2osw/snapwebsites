// Snap Websites Server -- handle user VPN installation
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

// vpn
//
#include "vpn.h"

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
    QString const CLIENT_ADDNEW_NAME = QString("add_new_client");
    QString const CLIENT_CONFIG_NAME = QString("vpn_client_configuration");
    QString const CLIENT_SERVER_IP   = QString("server_ip");
    QString const SERVER_IP_FILENAME = QString("server_ip_address.conf");
}


SNAP_PLUGIN_START(vpn, 1, 0)


/** \brief Get a fixed vpn plugin name.
 *
 * The vpn plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_VPN_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_VPN_...");

    }
    NOTREACHED();
}




/** \brief Initialize the vpn plugin.
 *
 * This function is used to initialize the vpn plugin object.
 */
vpn::vpn()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the vpn plugin.
 *
 * Ensure the vpn object is clean before it is gone.
 */
vpn::~vpn()
{
}


/** \brief Get a pointer to the vpn plugin.
 *
 * This function returns an instance pointer to the vpn plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the vpn plugin.
 */
vpn * vpn::instance()
{
    return g_plugin_vpn_factory.instance();
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
QString vpn::description() const
{
    return "Manage the vpn public key for users on a specific server.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString vpn::dependencies() const
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
int64_t vpn::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize vpn.
 *
 * This function terminates the initialization of the vpn plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void vpn::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(vpn, "server", snap_manager::manager, retrieve_status, _1);
}


QString vpn::get_server_ip() const
{
    QString server_ip;
    QFile file( QString("%1/%2").arg(f_snap->get_data_path()).arg(SERVER_IP_FILENAME) );
    if( file.open( QIODevice::ReadOnly| QIODevice::Text ) )
    {
        QTextStream in(&file);
        server_ip = in.readAll();
    }
    if( server_ip.isEmpty() )
    {
        server_ip = f_snap->get_public_ip();
    }
    return server_ip;
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void vpn::on_retrieve_status(snap_manager::server_status & server_status)
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
    QStringList exts    = {"conf"}; //{"key","crt","conf"};
    QStringList filters;
    for( auto const &ext : exts )
    {
        filters << QString("*.%1").arg(ext);
    }

    QDir keys_path;
    keys_path.setPath( "/etc/openvpn/easy-rsa/keys/" );
    keys_path.setNameFilters( filters );
    keys_path.setSorting( QDir::Name );
    keys_path.setFilter( QDir::Files );

    for( QFileInfo const &info : keys_path.entryInfoList() )
    {
        SNAP_LOG_TRACE("file info=")(info.filePath());

        if( info.baseName() == "server" )
        {
            continue; // Ignore the server configuration
        }

        // create a field for this one, it worked
        //
        for( auto const & ext : exts )
        {
            QFile file( QString("%1/%2.%3")
                .arg(info.path())
                .arg(info.baseName())
                .arg(ext)
                );
            if( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
            {
                QString const name(QString("%1.%2").arg(info.baseName()).arg(ext));
                QTextStream in(&file);
                snap_manager::status_t const ctl
                    ( snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , name
                    , in.readAll()
                    );
                server_status.set_field(ctl);
            }
            else
            {
                QString const errmsg(QString("Cannot open '%1' for reading!").arg(info.filePath()));
                SNAP_LOG_ERROR(qPrintable(errmsg));
                //throw vpn_exception( errmsg );
            }
        }
    }

    // If this is a server, allow client keys to be created.
    //
    QFile file( "/etc/openvpn/server.conf" );
    if( file.exists() )
    {
        // Server IP
        //
        {
            snap_manager::status_t const ctl(
                snap_manager::status_t::state_t::STATUS_STATE_INFO
                , get_plugin_name()
                , CLIENT_SERVER_IP
                , get_server_ip()
                );
            server_status.set_field(ctl);
        }

        // Add new client
        //
        {
            snap_manager::status_t const ctl(
                snap_manager::status_t::state_t::STATUS_STATE_INFO
                , get_plugin_name()
                , CLIENT_ADDNEW_NAME
                , QString()
                );
            server_status.set_field(ctl);
        }
    }
    else
    {
        // Else, create the display for the client cert, if you want a client to run on this machine
        //
        QString contents;
        QFile client_file( "/etc/openvpn/client.conf" );
        if( client_file.open( QIODevice::ReadOnly| QIODevice::Text ) )
        {
            QTextStream in(&client_file);
            contents = in.readAll();
        }

        snap_manager::status_t const ctl(
                snap_manager::status_t::state_t::STATUS_STATE_INFO
                , get_plugin_name()
                , CLIENT_CONFIG_NAME
                , contents
                );
        server_status.set_field(ctl);
    }
}


bool vpn::is_installed()
{
    // for now we just check whether the executable is here, this is
    // faster than checking whether the package is installed.
    //
    return access("/usr/sbin/openvpn", R_OK | X_OK) == 0;
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
bool vpn::display_value ( QDomElement parent
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

    if( s.get_field_name() == CLIENT_SERVER_IP )
    {
        snap_manager::form f(
                get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_SAVE
                );
        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                    "Enter the Public (inter data center) or Private IP address of this server:"
                    , s.get_field_name()
                    , s.get_value()
                    , "<p>Do <b>not</b> enter the VPN address from the tun0 interface.</p>"
                     " In most cases, this is a Private IP Address and NOT a Public IP"
                     " Address. In most cases, Private IP Addresses look like"
                     " 192.168.x.x or 10.x.x.x. Public IP Addresses are accessible"
                     " by anyone who has an Internet connection."
                    ));
        f.add_widget(field);
        f.generate(parent, uri);
        return true;
    }

    if( s.get_field_name() == CLIENT_ADDNEW_NAME )
    {
        snap_manager::form f(
                get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_SAVE
                );
        snap_manager::widget_text::pointer_t field(std::make_shared<snap_manager::widget_text>(
                    "Enter one or more names of the clients you wish to add, one per line."
                    , s.get_field_name()
                    , ""
                    , "<p>You may reuse this form to add more clients at any time.</p>"
                    ));
        f.add_widget(field);
        f.generate(parent, uri);
        return true;
    }

    if( s.get_field_name() == CLIENT_CONFIG_NAME )
    {
        snap_manager::form f(
                get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_SAVE
                );
        snap_manager::widget_text::pointer_t field(std::make_shared<snap_manager::widget_text>(
                    "Client OpenVPN configuration file."
                    , s.get_field_name()
                    , s.get_value()
                    , "Paste in the file that was generated on the VPN server page, if you want to run a client on this system."
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
            , snap_manager::form::FORM_BUTTON_NONE
            );
    snap_manager::widget_text::pointer_t field(std::make_shared<snap_manager::widget_text>(
                "Paste this into the client system to activate."
                , s.get_field_name()
                , s.get_value()
                , "NOTE: This has been read in from the generated client file on the system."
                "This field is READ ONLY! Any changes to the text will not be persisted."
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
bool vpn::apply_setting ( QString const & button_name
                        , QString const & field_name
                        , QString const & new_value
                        , QString const & old_or_installation_value
                        , std::set<QString> & affected_services
                        )
{
    NOTUSED(button_name);
    NOTUSED(old_or_installation_value);
    NOTUSED(affected_services);

    if( field_name == CLIENT_SERVER_IP )
    {
        QFile file( QString("%1/%2").arg(f_snap->get_data_path()).arg(SERVER_IP_FILENAME) );
        if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
        {
            QString const errmsg = QString("Cannot open '%1' for writing!").arg(file.fileName());
            SNAP_LOG_ERROR(qPrintable(errmsg));
            return false;
        }

        QTextStream out( &file );
        out << new_value;
        return true;
    }

    if( field_name == CLIENT_ADDNEW_NAME )
    {
        // TODO: use variable for "/var/lib/snapwebsites"
        //
        QString const cache_path(f_snap->get_cache_path());
        QString const script_filename( QString("%1/create_client_certs.sh").arg(cache_path) );
        QFile create_script( script_filename );
        //
        // Overwrite the script every time
        //
        create_script.remove();
        if( !QFile::copy( ":/manager-plugins/vpn/create_client_certs.sh", create_script.fileName() ) )
        {
            SNAP_LOG_ERROR("Cannot copy \"")(create_script.fileName())("\" file!");
            return false;
        }
        //
        create_script.setPermissions(create_script.permissions() | QFileDevice::ExeOwner);

        QStringList clients;
        //
        if( new_value.contains("\n") )
        {
            clients = new_value.split("\n");
        }
        else
        {
            clients << new_value;
        }

        QString const server_ip( get_server_ip() );
        //
        // I deliberately want a copy, not a reference, as I'm going to modify the client
        // string and remove any stray CRs
        //
        for( auto client : clients )
        {
            client.remove('\r'); // Get rid of CRs
            int const r(QProcess::execute( script_filename, {server_ip, client} ));
            if( r != 0 )
            {
                SNAP_LOG_ERROR("Could not execute client creation script! IP=")
                    (qPrintable(server_ip))
                    (", client=")(qPrintable(client))
                    (", exitcode=")(r);
            }
        }
        return true;
    }

    if( field_name == CLIENT_CONFIG_NAME )
    {
        QProcess::execute( "systemctl stop openvpn@client" );

        {
            QFile file( "/etc/openvpn/client.conf" );
            if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
            {
                QString const errmsg = QString("Cannot open '%1' for writing!").arg(file.fileName());
                SNAP_LOG_ERROR(qPrintable(errmsg));
                //throw vpn_exception( errmsg );
                return false;
            }

            QTextStream out( &file );
            out << new_value;
        }

        QProcess::execute( "systemctl enable openvpn@client" );
        QProcess::execute( "systemctl start openvpn@client" );

        return true;
    }

    return false;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
