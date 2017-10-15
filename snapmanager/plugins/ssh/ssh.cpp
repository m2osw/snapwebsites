// Snap Websites Server -- handle user SSH authorized_keys key
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

// ssh
//
#include "ssh.h"

// our lib
//
#include "snapmanager/form.h"

// snapwebsites lib
//
#include <snapwebsites/chownnm.h>
#include <snapwebsites/glob_dir.h>
#include <snapwebsites/log.h>
#include <snapwebsites/mkdir_p.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

// C++ lib
//
#include <fstream>

// QtCore
//
#include <QFile>
#include <QString>
#include <QStringList>
#include <QTextStream>

// last entry
//
#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(ssh, 1, 0)


namespace
{

const QString g_sshd_config        = "/etc/ssh/sshd_config";

//void file_descriptor_deleter(int * fd)
//{
//    if(close(*fd) != 0)
//    {
//        int const e(errno);
//        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
//    }
//}



void glob_deleter(glob_t * g)
{
    globfree(g);
}

int glob_error_callback(const char * epath, int eerrno)
{
    SNAP_LOG_ERROR("an error occurred while reading directory under \"")
                  (epath)
                  ("\". Got error: ")
                  (eerrno)
                  (", ")
                  (strerror(eerrno))
                  (".");

    // do not abort on a directory read error...
    return 0;
}

} // no name namespace


ssh_config::ssh_config( QString const & filepath )
    : f_filepath(filepath)
{
}


bool ssh_config::read()
{
    QFile file( f_filepath );
    if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        QString const errmsg = QString("Cannot open '%1' for reading!").arg(f_filepath);
        SNAP_LOG_ERROR(errmsg);
        return false;
    }

    QTextStream in(&file);
    while( !in.atEnd() )
    {
        f_lines << in.readLine();
    }

    return true;
}


bool ssh_config::write()
{
    QFile file( f_filepath );

    if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        QString const errmsg = QString("Cannot open '%1' for writing!").arg(f_filepath);
        SNAP_LOG_ERROR(errmsg);
        return false;
    }
    //
    // ...and stream the file out to disk so we have the node key for
    // node-to-node SSL connections.
    //
    QTextStream out( &file );
    out << f_lines.join("\n");

    return true;
}


QString ssh_config::get_entry( QString const& name, QString const& default_value ) const
{
    QRegExp re( QString("%1 *").arg(name), Qt::CaseSensitive, QRegExp::Wildcard );
    int const pos( f_lines.indexOf( re ) );
    if( pos == -1 )
    {
        return default_value;
    }

    QString const line( f_lines[pos].simplified() );
    QStringList pair( line.split(" ") );
    if( pair.size() == 2 )
    {
        return pair[1].simplified();
    }

    return default_value;
}


void ssh_config::set_entry( QString const& name, QString const& value )
{
    QString const newline( QString("%1 %2").arg(name).arg(value) );
    QRegExp re( QString("%1 *").arg(name), Qt::CaseSensitive, QRegExp::Wildcard );
    int const pos( f_lines.indexOf( re ) );
    if( pos == -1 )
    {
        f_lines << newline;
    }
    else
    {
        f_lines[pos] = newline;
    }
}




/** \brief Get a fixed ssh plugin name.
 *
 * The ssh plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_SSH_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_SSH_...");

    }
    NOTREACHED();
}




/** \brief Initialize the ssh plugin.
 *
 * This function is used to initialize the ssh plugin object.
 */
ssh::ssh()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the ssh plugin.
 *
 * Ensure the ssh object is clean before it is gone.
 */
ssh::~ssh()
{
}


/** \brief Get a pointer to the ssh plugin.
 *
 * This function returns an instance pointer to the ssh plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the ssh plugin.
 */
ssh * ssh::instance()
{
    return g_plugin_ssh_factory.instance();
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
QString ssh::description() const
{
    return "Manage the ssh public key for users on a specific server.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString ssh::dependencies() const
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
int64_t ssh::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize ssh.
 *
 * This function terminates the initialization of the ssh plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void ssh::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(ssh, "server", snap_manager::manager, retrieve_status, _1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void ssh::on_retrieve_status(snap_manager::server_status & server_status)
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

    // SNAP-521: Add ability to change PasswordAuthentication
    //
    // Create a field for password authentication for the system sshd
    // service. If this is set to yes (or undefined), this is a warning
    // we want to post because this is an vulnerability to allow anyone to
    // auth using passwords. We want to enforce key authentication only.
    //
    ssh_config sc( g_sshd_config );
    sc.read();
    QString const default_value = sc.get_entry("PasswordAuthentication","yes");
    snap_manager::status_t::state_t const the_state ( default_value == "yes"
                                                    ? snap_manager::status_t::state_t::STATUS_STATE_WARNING
                                                    : snap_manager::status_t::state_t::STATUS_STATE_INFO
                                                    );
    snap_manager::status_t const password_auth
            ( the_state
            , get_plugin_name()
            , "sshd_password_auth"
            , default_value
            );
    server_status.set_field(password_auth);

    // we want one field per user on the system, at this point we
    // assume that the system does not have hundreds of users since
    // only a few admins should be permitted on those computers
    // anyway...
    //
    glob_dir    dir;
    try
    {
        dir.set_path( "/home/*", GLOB_NOESCAPE );
    }
    catch( std::exception const & x )
    {
        SNAP_LOG_ERROR("Exception caught! what=")(x.what());
        return;
    }
    catch( ... )
    {
        SNAP_LOG_ERROR("Unknown exception caught!");
        return;
    }

    // check each user
    // (TBD: how to "blacklist" some users so they do not appear here?)
    //
    dir.enumerate_glob( [&]( QString path )
    {
        // TODO: replace the direct handling of the file with a file_content object
        //
        std::string const home_path(path.toUtf8().data());
        std::string const user_name(home_path.substr(6));
        std::string const authorized_keys_path(home_path + "/.ssh/authorized_keys");
        std::ifstream authorized_keys_in;
        authorized_keys_in.open(authorized_keys_path, std::ios::in | std::ios::binary);
        if(authorized_keys_in.is_open())
        {
            authorized_keys_in.seekg(0, std::ios::end);
            std::ifstream::pos_type const size(authorized_keys_in.tellg());
            authorized_keys_in.seekg(0, std::ios::beg);
            std::string key;
            key.resize(size, '?');
            authorized_keys_in.read(&key[0], size);
            if(!authorized_keys_in.fail()) // note: eof() will be true so good() will return false
            {
                // create a field for this one, it worked
                //
                snap_manager::status_t const rsa_key(
                                  snap_manager::status_t::state_t::STATUS_STATE_INFO
                                , get_plugin_name()
                                , QString::fromUtf8(("authorized_keys::" + user_name).c_str())
                                , QString::fromUtf8(key.c_str()));
                server_status.set_field(rsa_key);
            }
            else
            {
                SNAP_LOG_DEBUG("could not read \"")(authorized_keys_path)("\" file for user \"")(user_name)("\".");

                // create an error field which is not editable
                //
                snap_manager::status_t const rsa_key(
                                  snap_manager::status_t::state_t::STATUS_STATE_ERROR
                                , get_plugin_name()
                                , QString::fromUtf8(("authorized_keys::" + user_name).c_str())
                                , QString());
                server_status.set_field(rsa_key);
            }
        }
        else
        {
            // no authorized_keys file for that user
            // create an empty field so one can add that file
            //
            snap_manager::status_t const rsa_key(
                              snap_manager::status_t::state_t::STATUS_STATE_INFO
                            , get_plugin_name()
                            , QString::fromUtf8(("authorized_keys::" + user_name).c_str())
                            , QString());
            server_status.set_field(rsa_key);
        }
    });
}


bool ssh::is_installed()
{
    // for now we just check whether the executable is here, this is
    // faster than checking whether the package is installed and
    // should be enough proof that the server is installed and
    // running... and thus offer the editing of
    // /home/*/.ssh/authorized_keys files
    //
    return access("/usr/sbin/sshd", R_OK | X_OK) == 0;
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
bool ssh::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    //QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "sshd_password_auth" )
    {
        // SNAP-521: Add ability to change PasswordAuthentication
        //
        // the list of authorized_keys files
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        QStringList switch_list;
        switch_list << "yes";
        switch_list << "no";
        snap_manager::widget_select::pointer_t field(std::make_shared<snap_manager::widget_select>
                       ( "Password authentication for ssh"
                       , s.get_field_name()
                       , switch_list
                       , s.get_value()
                       , "Enter either 'yes' or 'no' in this field and click Save, or Save Everywhere."
                         " If this is in yellow, then you need to take action. This feature should be set to"
                         " 'no' on a production server as this is a vulnerability."
                       ));
        f.add_widget(field);
        f.generate(parent, uri);
        return true;
    }

    if(s.get_field_name().startsWith("authorized_keys::"))
    {
        // in case of an error, we do not let the user do anything
        // so let the default behavior do its thing, it will show the
        // field in a non-editable manner
        //
        if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_ERROR)
        {
            return false;
        }

        // the list of authorized_keys files
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                  | snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        QString const user_name(s.get_field_name().mid(17));
        snap_manager::widget_text::pointer_t field(std::make_shared<snap_manager::widget_text>(
                          "Authorized keys for \"" + user_name + "\""
                        , s.get_field_name()
                        , s.get_value()
                        , "<p>Enter your authorized_keys file in this field and click Save (or Save Everywhere, but see warning below)."
                          " Then you will have access to this server via ssh. Use the"
                          " \"Restore Default\" button to remove the file from this server.</p>"
                          "<p><b>WARNING:</b> This could prove to be a security risk if you send public keys"
                          " over a hostile network--make sure you have adequate firewall protection before proceeding!</p>"
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
 *
 * \return true if the new_value was applied successfully.
 */
bool ssh::apply_setting
    ( QString const & button_name
    , QString const & field_name
    , QString const & new_value
    , QString const & old_or_installation_value
    , std::set<QString> & affected_services
    )
{
    NOTUSED(old_or_installation_value);

    if( field_name == "sshd_password_auth" )
    {
        // SNAP-521: Add ability to change PasswordAuthentication
        //
        if( button_name == "save" || button_name == "save_everywhere" )
        {
            ssh_config sc( g_sshd_config );
            sc.read();
            sc.set_entry( "PasswordAuthentication", new_value );
            sc.write();
            affected_services.insert("ssh");
        }
    }

    // we support Save and Restore Default of the authorized_keys file
    //
    if(field_name.startsWith("authorized_keys::"))
    {
        // generate the path to the authorized_keys file
        QString const user_name(field_name.mid(17));
        std::string authorized_keys_path("/home/");
        authorized_keys_path += user_name.toUtf8().data();
        std::string const ssh_path(authorized_keys_path + "/.ssh");
        authorized_keys_path += "/.ssh/authorized_keys";

        // first check whether the user asked to restore the defaults
        //
        if(button_name == "restore_default")
        {
            // "Restore Default" means deleting the file (i.e. no more SSH
            // access although we do not yet break existing connection which
            // we certainly should do too...)
            //
            unlink(authorized_keys_path.c_str());
            return true;
        }

        // next make sure the .ssh directory exists, if not create it
        // as expected by ssh
        //
        struct stat s;
        if(stat(ssh_path.c_str(), &s) != 0)
        {
            QString const q_ssh_path(QString::fromUtf8(ssh_path.c_str()));
            if(mkdir_p(q_ssh_path, false) != 0)
            {
                SNAP_LOG_ERROR("we could not create the .ssh directory \"")(ssh_path)("\"");
                return false;
            }
            if(chmod(ssh_path.c_str(), S_IRWXU) != 0) // 0700
            {
                int const e(errno);
                SNAP_LOG_WARNING("could not setup the .ssh directory mode, (errno: ")(e)(", ")(strerror(e))(")");
            }
            if(chownnm(q_ssh_path, user_name, user_name) != 0)
            {
                int const e(errno);
                SNAP_LOG_WARNING("could not setup the .ssh ownership, (errno: ")(e)(", ")(strerror(e))(")");
            }
        }

        if( button_name == "save" || button_name == "save_everywhere" )
        {
            // TODO: replace the direct handling of the file with a file_content object
            //
            std::ofstream authorized_keys_out;
            authorized_keys_out.open(authorized_keys_path, std::ios::out | std::ios::trunc | std::ios::binary);
            if(authorized_keys_out.is_open())
            {
                authorized_keys_out << new_value.trimmed().toUtf8().data() << std::endl;

                if(chmod(authorized_keys_path.c_str(), S_IRUSR | S_IWUSR) != 0)
                {
                    int const e(errno);
                    SNAP_LOG_WARNING("could not setup the authorize_keys file mode, (errno: ")(e)(", ")(strerror(e))(")");
                }

                // WARNING: we would need to get the default name of the
                // user main group instead of assuming it is his name
                //
                if(chownnm(QString::fromUtf8(authorized_keys_path.c_str()), user_name, user_name) != 0)
                {
                    int const e(errno);
                    SNAP_LOG_WARNING("could not setup the authorize_keys file mode, (errno: ")(e)(", ")(strerror(e))(")");
                }
                return true;
            }

            SNAP_LOG_ERROR("we could not open authorized_keys file \"")(authorized_keys_path)("\"");
        }
    }

    return false;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
