// Snap Websites Server -- manage APT sources
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

// apt
//
#include "apt.h"

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
    QString     const APT_SOURCE_DIR     = QString("/etc/apt/sources.list.d");
    QString     const APT_PREFS_DIR      = QString("/etc/apt/preferences.d");
    QString     const SNAPCPP_APT_SOURCE = QString("snapcpp_apt_source");
    QString     const OLD_APT_SOURCE     = QString("old_apt_source");
    QString     const GPG_KEY            = QString("gpg_key");
    QString     const RELEASE_PIN        = QString("release_pin");

    QStringList const EXTENSIONS         = {"list"};
}


SNAP_PLUGIN_START(apt, 1, 0)


/** \brief Get a fixed apt plugin name.
 *
 * The apt plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_APT_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_APT_* name...");

    }
    NOTREACHED();
}




/** \brief Initialize the apt plugin.
 *
 * This function is used to initialize the apt plugin object.
 */
apt::apt()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the apt plugin.
 *
 * Ensure the apt object is clean before it is gone.
 */
apt::~apt()
{
}


/** \brief Get a pointer to the apt plugin.
 *
 * This function returns an instance pointer to the apt plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the apt plugin.
 */
apt * apt::instance()
{
    return g_plugin_apt_factory.instance();
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
QString apt::description() const
{
    return "Manage the apt public key for users on a specific server.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString apt::dependencies() const
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
int64_t apt::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize apt.
 *
 * This function terminates the initialization of the apt plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void apt::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(apt, "server", snap_manager::manager, retrieve_status, _1);
}


/** \brief Sanity check to make sure APT is installed
 *
 * \return true if /usr/bin/apt is present on the system, false otherwise.
 */
bool apt::is_installed()
{
    // for now we just check whether the executable is here, this is
    // faster than checking whether the package is installed.
    //
    return access("/usr/bin/apt", R_OK | X_OK) == 0;
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void apt::on_retrieve_status(snap_manager::server_status & server_status)
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

    QStringList filters;
    for( auto const &ext : EXTENSIONS )
    {
        filters << QString("*.%1").arg(ext);
    }

    QDir keys_path;
    keys_path.setPath( APT_SOURCE_DIR );
    keys_path.setNameFilters( filters );
    keys_path.setSorting( QDir::Name );
    keys_path.setFilter( QDir::Files );

    bool snapcpp_list_found = false;

    for( QFileInfo const &info : keys_path.entryInfoList() )
    {
        //SNAP_LOG_TRACE("file info=")(info.filePath());

        if( info.baseName() == "snapcpp" )
        {
            // create a field for this one, it worked
            //
            for( auto const & ext : EXTENSIONS )
            {
                QFile file( QString("%1/%2.%3")
                            .arg(info.path())
                            .arg(info.baseName())
                            .arg(ext)
                            );
                if( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
                {
                    QTextStream in(&file);
                    snap_manager::status_t const list
                            ( snap_manager::status_t::state_t::STATUS_STATE_INFO
                              , get_plugin_name()
                              , SNAPCPP_APT_SOURCE
                              , in.readAll()
                              );
                    server_status.set_field(list);
                    snapcpp_list_found = true;
                }
                else
                {
                    QString const errmsg(QString("Cannot open '%1' for reading!").arg(info.filePath()));
                    SNAP_LOG_ERROR(errmsg);
                    //throw apt_exception( errmsg );
                }
            }
            break;
        }
    }

    if( !snapcpp_list_found )
    {
        snap_manager::status_t const outofdate(
                    snap_manager::status_t::state_t::STATUS_STATE_HIGHLIGHT
                    , get_plugin_name()
                    , OLD_APT_SOURCE
                    , "APT sources are out of date!");
        server_status.set_field(outofdate);
    }

    // Add GPG key field for the apt source
    {
        snap_manager::status_t const conf_field(
                    snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , GPG_KEY
                    , "");
        server_status.set_field(conf_field);
    }

    // Add release-pin field (stable, unstable or other).
    {
        QString pin_name("none");
        QFile file( QString("%1/snapcpp").arg(APT_PREFS_DIR) );
        if( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
        {
            const QString preamble("Pin: release a=");
            QTextStream in(&file);
            QStringList lines(in.readAll().split('\n'));;
            for( auto const & line : lines )
            {
                if( line.startsWith(preamble) )
                {
                    pin_name = line.mid(preamble.length());
                    break;
                }
            }
        }
        snap_manager::status_t const conf_field(
                    snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , RELEASE_PIN
                    , pin_name);
        server_status.set_field(conf_field);
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
bool apt::display_value ( QDomElement parent
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

    QString const field_name(s.get_field_name());
    QString default_value;
    if( field_name == SNAPCPP_APT_SOURCE )
    {
        default_value = s.get_value();
    }
    else if( field_name == OLD_APT_SOURCE )
    {
        default_value = 
            "# M2OSW source for SnapCPP\n"
            "#\n"
            "deb https://debian:<PASSWORD>@build.m2osw.com/stable xenial main contrib non-free\n"
            ;
    }
    else if( field_name == GPG_KEY )
    {
        snap_manager::form f(
                    get_plugin_name()
                    , field_name
                    , snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                    );
        snap_manager::widget_text::pointer_t field(std::make_shared<snap_manager::widget_text>(
                                                       "GPG public key used to sign the archive:"
                                                       , field_name
                                                       , ""
                                                       , "<p>Paste in the key here.</p>"
                                                       ));
        f.add_widget(field);
        f.generate(parent, uri);
        return true;
    }
    else if( field_name == RELEASE_PIN )
    {
        snap_manager::form f(
                    get_plugin_name()
                    , field_name
                    , snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                    );
        snap_manager::widget_text::pointer_t field(std::make_shared<snap_manager::widget_text>(
                                                       "Enter APT release pin name:"
                                                       , field_name
                                                       , s.get_value()
                                                       , "<p>Available options:</p>"
                                                       " <b>none</b>, <b>stable</b>, <b>unstable</b>, or <b><i>codename</i></b>"
                                                       ", where <i>codename</i> any the name of the distribution"
                                                       ", or <b>none</b> to remove the pin entirely."
                                                       ));
        f.add_widget(field);
        f.generate(parent, uri);
        return true;
    }
    else
    {
        SNAP_LOG_WARNING("Field name '")(field_name)("' is unknown!");
        return true;
    }

    snap_manager::form f(
            get_plugin_name()
            , field_name
            , snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
            );
    snap_manager::widget_text::pointer_t field(std::make_shared<snap_manager::widget_text>(
                "Enter or edit the APT source which points to the SnapCPP sources:"
                , field_name
                , default_value
                , "<p>The form should be as follows:</p>"
                " <code>deb http[s]://<i>server/path platform</i> main contrib non-free</code>"
                " <p>where `server/path` is the full path to the archive, platform is the release (like <i>xenial</i>),"
                " and the components you require.</p>"
                " <p>When you are satisfied, click the 'Refresh' button to force an update of the APT sources.</p>"
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
bool apt::apply_setting ( QString const & button_name
                        , QString const & field_name
                        , QString const & new_value
                        , QString const & old_or_installation_value
                        , std::set<QString> & affected_services
                        )
{
    NOTUSED(button_name);
    NOTUSED(old_or_installation_value);
    NOTUSED(affected_services);

    if( field_name == OLD_APT_SOURCE )
    {
        // Blow away the old legacy apt sources
        //
        QStringList filters;
        for( auto const &ext : EXTENSIONS )
        {
            filters << QString("*.%1").arg(ext);
        }

        QDir keys_path;
        keys_path.setPath( APT_SOURCE_DIR );
        keys_path.setNameFilters( filters );
        keys_path.setSorting( QDir::Name );
        keys_path.setFilter( QDir::Files );

        for( QFileInfo const &info : keys_path.entryInfoList() )
        {
            SNAP_LOG_TRACE("file info=")(info.filePath());

            if(    info.baseName() == "doug"
                || info.baseName() == "snap"
                || info.baseName() == "exdox"
              )
            {
                for( auto const & ext : EXTENSIONS )
                {
                    QFile file( QString("%1/%2.%3")
                                .arg(info.path())
                                .arg(info.baseName())
                                .arg(ext)
                                );
                    file.remove();
                }
            }
        }
    }
    else if( field_name == GPG_KEY )
    {
        QFile file( QString("/tmp/key_%1.asc").arg(getpid()) );
        if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
        {
            QString const errmsg = QString("Cannot open '%1' for writing!").arg(file.fileName());
            SNAP_LOG_ERROR(errmsg);
            return false;
        }
        //
        QTextStream out( &file );
        out << new_value;
        //
        file.flush();
        file.close();

        bool success = true;
        QStringList params;
        params << "add" << file.fileName();
        if( QProcess::execute( "apt-key", params ) != 0 )
        {
            SNAP_LOG_ERROR("Cannot import GPG key!");
            success = false;
        }

        // Remove the temporary file
        file.remove();

        // And...done!
        return success;
    }
    else if( field_name == RELEASE_PIN )
    {
        // Write out the pin to the preferences
        //
        QFile file( QString("%1/snapcpp").arg(APT_PREFS_DIR) );
        if( new_value == "none" )
        {
            file.remove();
        }
        else if( file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
        {
            QTextStream out( &file );
            out << "Package: *\n"
                << "Pin: release a=" << new_value << "\n"
                << "Pin-Priority: 1001\n";
        }
        else
        {
            QString const errmsg = QString("Cannot open '%1' for writing!").arg(file.fileName());
            SNAP_LOG_ERROR(errmsg);
            return false;
        }
        return true;
    }

    // Default write to snapcpp.list
    //
    {
        QFile file( QString("%1/snapcpp.list").arg(APT_SOURCE_DIR) );
        if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
        {
            QString const errmsg = QString("Cannot open '%1' for writing!").arg(file.fileName());
            SNAP_LOG_ERROR(errmsg);
            return false;
        }

        QTextStream out( &file );
        out << new_value;
    }
    return true;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
