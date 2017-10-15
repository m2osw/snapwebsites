// Snap Websites Server -- manage the snapmanager.cgi and snapmanagerdaemon
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

// self
//
#include "self.h"

// our lib
//
#include "snapmanager/form.h"

// snapwebsites lib
//
#include <snapwebsites/addr.h>
#include <snapwebsites/file_content.h>
#include <snapwebsites/join_strings.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/process.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/string_pathinfo.h>
#include <snapwebsites/tokenize_string.h>

// Qt lib
//
#include <QFile>

// C lib
//
#include <sys/file.h>
#include <sys/sysinfo.h>

// last entry
//
#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(self, 1, 0)


namespace
{


char const * g_configuration_filename = "snapmanager";


void file_descriptor_deleter(int * fd)
{
    if(close(*fd) != 0)
    {
        int const e(errno);
        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
    }
}

} // no name namespace



/** \brief Get a fixed self plugin name.
 *
 * The self plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_SELF_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_SELF_...");

    }
    NOTREACHED();
}




/** \brief Initialize the self plugin.
 *
 * This function is used to initialize the self plugin object.
 */
self::self()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the self plugin.
 *
 * Ensure the self object is clean before it is gone.
 */
self::~self()
{
}


/** \brief Get a pointer to the self plugin.
 *
 * This function returns an instance pointer to the self plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the self plugin.
 */
self * self::instance()
{
    return g_plugin_self_factory.instance();
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
QString self::description() const
{
    return "Manage the snapmanager.cgi and snapmanagerdaemon settings.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString self::dependencies() const
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
int64_t self::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize self.
 *
 * This function terminates the initialization of the self plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void self::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN  ( self, "server", snap_manager::manager, retrieve_status,          _1     );
    SNAP_LISTEN  ( self, "server", snap_manager::manager, add_plugin_commands,      _1     );
    SNAP_LISTEN  ( self, "server", snap_manager::manager, process_plugin_message,   _1, _2 );
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void self::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    {
        snap_manager::status_t const up(snap_manager::status_t::state_t::STATUS_STATE_INFO, get_plugin_name(), "status", "up");
        server_status.set_field(up);
    }

    {
        snap_manager::status_t const ip(snap_manager::status_t::state_t::STATUS_STATE_INFO, get_plugin_name(), "ip", f_snap->get_public_ip());
        server_status.set_field(ip);
    }

    {
        snap_addr::addr::vector_t interfaces( snap_addr::addr::get_local_addresses() );
        for( auto const & addr : interfaces )
        {
            if( !addr.is_ipv4() )
            //||  addr.get_network_type() != snap_addr::addr::network_type_t::NETWORK_TYPE_PRIVATE )
            {
                continue;
            }

            SNAP_LOG_TRACE("get interface ")(addr.get_iface_name())(", ip addr=")(addr.get_ipv4_string());
            snap_manager::status_t const iface ( snap_manager::status_t::state_t::STATUS_STATE_INFO
                                               , get_plugin_name()
                                               , QString("if::%1 (%2)")
                                                 .arg(addr.get_iface_name().c_str())
                                                 .arg(addr.get_network_type_string().c_str())
                                               , addr.get_ipv4_string().c_str()
                                               );
            server_status.set_field(iface);
        }
    }

    {
        struct sysinfo info;
        if(sysinfo(&info) == 0)
        {
            // TODO: also add uptime, loads and how/much used memory
            //       also change the Kb to Mb, Gb, Tb... as required
            //
            QString meminfo(
                    QString("RAM: %1Kb - Swap: %2Kb")
                        .arg((static_cast<long long>(info.totalram) + (static_cast<long long>(info.totalhigh) << 32)) * static_cast<long long>(info.mem_unit) / 1024LL)
                        .arg(static_cast<long long>(info.totalswap) * static_cast<long long>(info.mem_unit) / 1024LL));
            snap_manager::status_t::state_t status(snap_manager::status_t::state_t::STATUS_STATE_INFO);
            struct stat st;
            bool const has_cassandra(stat("/usr/sbin/cassandra", &st) == 0);
            if(info.totalswap > 0)
            {
                // there should not be a swap file along Cassandra
                //
                if(has_cassandra)
                {
                    status = snap_manager::status_t::state_t::STATUS_STATE_HIGHLIGHT;
                    meminfo += " (WARNING: You have a swap file on a system running Cassandra. This is not recommended.)";
                }
            }
            else
            {
                // there should probably be a swap file when Cassandra is
                // not installed on a machine
                //
                if(!has_cassandra)
                {
                    status = snap_manager::status_t::state_t::STATUS_STATE_HIGHLIGHT;
                    meminfo += " (WARNING: You do not have a swap file on this system. This is recommended on most computers except those running Cassandra.)";
                }
            }
            snap_manager::status_t const memory(
                                  status
                                , get_plugin_name()
                                , "memory"
                                , meminfo);
            server_status.set_field(memory);
        }
    }

    {
        // right now we have ONE level for ALL .properties, later we should
        // probably duplicate this code and allow each .properties file to
        // be edited as required
        //
        snap::snap_config logger_properties("/etc/snapwebsites/logger/snapmanagerdaemon.properties");
        std::string const level_appenders(logger_properties["log4cplus.logger.snap"]);
        std::string level("INFO");
        std::string::size_type const pos(level_appenders.find_first_of(','));
        if(pos > 0)
        {
            level = level_appenders.substr(0, pos);
        }
        snap_manager::status_t const up(
                            snap_manager::status_t::state_t::STATUS_STATE_INFO,
                            get_plugin_name(),
                            "log_level",
                            QString::fromUtf8(level.c_str()));
        server_status.set_field(up);
    }

    bool no_installs(false);
    {
        QString const updates(f_snap->count_packages_that_can_be_updated(true));
        if(!updates.isEmpty())
        {
            QStringList msg;
            if( f_system_active )
            {
                msg << "<b>CLUSTER IS NOT IN MAINTENANCE MODE!</b>"
                      "<br/><i>It is highly recommended that your cluster be put in maintenance mode "
                      "before upgrading to avoid data loss.</i><br/><br/>";
            }
            //
            if( f_backends_active )
            {
                msg << QString("<b>%1 BACKENDS ARE RUNNING!</b>"
                      "<br/><i>It is highly recommended that disable the all of the backends on your cluster "
                      "before upgrading to avoid data loss.</i><br/><br/>")
                    .arg(f_backends_active);
            }
            SNAP_LOG_DEBUG("f_system_active=")(f_system_active)(", f_backends_active=")(f_backends_active)(", msg=")(msg.join("\n"));
            //
            snap_manager::status_t const upgrade_required(
                              snap_manager::status_t::state_t::STATUS_STATE_WARNING
                            , get_plugin_name()
                            , "upgrade_required"
                            , QString("%1;%2").arg(msg.join(" ")).arg(updates)
                            );
            server_status.set_field(upgrade_required);
            no_installs = true;
        }
    }

    {
        // TODO: offer a way to define "/run/reboot-required" in
        //       the snapmanager.conf file
        //
        struct stat st;
        if(stat("/run/reboot-required", &st) == 0)
        {
            QString msg;
            if( f_system_active )
            {
                msg = "<b>CLUSTER IS NOT IN MAINTENANCE MODE!</b>"
                      "<br/><i>It is highly recommended that your cluster be put in maintenance mode "
                      "before rebooting to avoid data loss.</i><br/><br/>";
            }

            // TBD: should we put the content of that file as the message?
            //      (it could be tainted though...)
            //
            snap_manager::status_t const reboot_required(
                              snap_manager::status_t::state_t::STATUS_STATE_WARNING
                            , get_plugin_name()
                            , "reboot_required"
                            , QString("%1Server \"%2\" requires a reboot.")
                                .arg(msg)
                                .arg(f_snap->get_server_name()));
            server_status.set_field(reboot_required);
        }
    }

#if 0
    // this does not currently work properly
    {
        snap::snap_string_list const & frontend_servers(f_snap->get_snapmanager_frontend());
        snap_manager::status_t const frontend(
                      frontend_servers.empty()
                            ? snap_manager::status_t::state_t::STATUS_STATE_WARNING
                            : snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "snapmanager_frontend"
                    , frontend_servers.join(","));
        server_status.set_field(frontend);
    }
#endif

    {
        std::string const redirect_unwanted(f_snap->get_parameter("redirect_unwanted"));
        snap_manager::status_t const redirect(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "redirect_unwanted"
                    , QString::fromUtf8(redirect_unwanted.c_str()));
        server_status.set_field(redirect);
    }

    {
        std::vector<std::string> const & bundle_uri(f_snap->get_bundle_uri());
        snap_manager::status_t const bundle(
                      bundle_uri.empty()
                            ? snap_manager::status_t::state_t::STATUS_STATE_WARNING
                            : snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "bundle_uri"
                    , QString::fromUtf8(snap::join_strings(bundle_uri, ",").c_str()));
        server_status.set_field(bundle);
    }

    // if an upgrade is required, avoid offering users a way to install
    // something (this test is not rock solid, but we have another "instant"
    // test in the installer anyway, still that way we will avoid many
    // installation errors.)
    //
    if(!no_installs)
    {
        retrieve_bundles_status(server_status);
    }
}


void self::retrieve_bundles_status(snap_manager::server_status & server_status)
{
    // TODO: make sure that the type of lock we use on the /var/lib/dpkg/lock
    //       file is indeed the one apt-get and Co. are using; note that the
    //       file does not get deleted between accesses
    //
    // if the lock created by dpkg and apt-get is in place, then do
    // nothing; note obviously that this is not a very good test since
    // we test the flag once and then go in a loop that's going to be
    // rather slow and a process may lock the database at that point
    //
    int lock_fd(open("/var/lib/dpkg/lock", O_RDONLY | O_CLOEXEC, 0));
    if(lock_fd != -1)
    {
        // RAII for the close
        //
        std::shared_ptr<int> auto_close(&lock_fd, file_descriptor_deleter);

        // the lock file exists, attempt a lock
        //
        if(::flock(lock_fd, LOCK_EX) != 0)
        {
            return;
        }

        // the lock file is inactive, we are good
        //
        // (TBD: should we keep the lock active while running the next loop?)
    }

    // check whether we have a bundles status file, if so we may just use
    // the data in that file instead of checking each package again and
    // again (which would be really slow, disk intensive, etc.)
    //
    QString const bundles_status_filename(QString("%1/bundles.status").arg(f_snap->get_bundles_path()));
    QFile bundles_status_file(bundles_status_filename);
    if(bundles_status_file.exists())
    {
        if(bundles_status_file.open(QIODevice::ReadOnly))
        {
            char buf[1024 * 10]; // 10Kb!
            qint64 r(bundles_status_file.readLine(buf, sizeof(buf)));
            if(r > 0)
            {
                if(buf[r - 1] == '\n')
                {
                    buf[r - 1] = '\0';
                }
                time_t const last_updated(std::stol(buf));
                if(last_updated + 86400 >= time(nullptr))
                {
                    // last updaed recently enough, use that data instead of
                    // gathering it again and again every minute
                    //
                    for(;;)
                    {
                        // one line per status
                        //
                        r = bundles_status_file.readLine(buf, sizeof(buf));
                        if(r <= 0)
                        {
                            break;
                        }
                        if(buf[r - 1] == '\n')
                        {
                            buf[r - 1] = '\0';
                        }
                        QString line(QString::fromUtf8(buf));

                        // find the end of the package name
                        //
                        int const first_colon(line.indexOf(':'));
                        if(first_colon == -1)
                        {
                            SNAP_LOG_WARNING("bundle status line \"")(buf)("\" is not valid (':' missing after name).");
                            continue;
                        }
                        QString const name(line.mid(0, first_colon));

                        // find the end end of the status letter
                        // (second_colon should always be first_colon + 2)
                        //
                        int const second_colon(line.indexOf(':', first_colon + 1));
                        if(second_colon == -1)
                        {
                            SNAP_LOG_WARNING("bundle status line \"")(buf)("\" is not valid (':' missing after status letter).");
                            continue;
                        }
                        QString const status_letter(line.mid(first_colon + 1, second_colon - first_colon - 1));

                        // hide that bundle? if so do not add t he field
                        //
                        if(status_letter != "H")
                        {
                            // the message is after that
                            //
                            QString const status_info(line.mid(second_colon + 1));

                            snap_manager::status_t const package_status(
                                        status_letter == "E" ? snap_manager::status_t::state_t::STATUS_STATE_ERROR
                                                             : status_letter == "I" ? snap_manager::status_t::state_t::STATUS_STATE_INFO
                                                                                    : snap_manager::status_t::state_t::STATUS_STATE_HIGHLIGHT,
                                        get_plugin_name(),
                                        "bundle::" + name,
                                        status_info);
                            server_status.set_field(package_status);
                        }
                    }

                    // we can return now, if something changed in the
                    // system, we missed it... but if we stay in control
                    // then the bundles status file gets deleted when
                    // we do a modifying apt-get command
                    //
                    return;
                }
            }

            bundles_status_file.close();
        }
        else
        {
            SNAP_LOG_WARNING("bundle status file \"")(bundles_status_filename)("\" exists but it could not be opened for reading.");
        }
        // bundle status file exists but it is invalid or out of date
    }

    if(bundles_status_file.open(QIODevice::WriteOnly))
    {
        time_t const created_on(time(nullptr));
        std::string const first_line(std::to_string(created_on));
        bundles_status_file.write(first_line.c_str(), first_line.length());
        bundles_status_file.putChar('\n');
    }
    else
    {
        SNAP_LOG_WARNING("bundle status file \"")(bundles_status_filename)("\" could not be opened for writing.");
    }

    std::vector<std::string> bundles(f_snap->get_list_of_bundles());

    for(auto const & b : bundles)
    {
        bool good_bundle(true);
        bool hide_bundle(false);
        bool has_error(false);

        QString filename(QString::fromUtf8(b.c_str()));
        std::string const name_from_filename(snap::string_pathinfo_basename(b, ".xml", "bundle-"));
        QString name(QString::fromUtf8(name_from_filename.c_str()));
        QString description;
        std::string package_name_and_version;

        // the Install form may include a few fields (values that are
        // otherwise difficult to change once the package was installed)
        //
        QDomElement fields;

        QDomDocument bundle_xml;
        QFile input(filename);
        if(input.open(QIODevice::ReadOnly)
        && bundle_xml.setContent(&input, false))
        {
            QDomElement root(bundle_xml.documentElement());
            // get the name, we show the name as part of the field name
            //
            QDomElement bundle_name(root.firstChildElement("name"));
            if(!bundle_name.isNull())
            {
                name = bundle_name.text();
            }
            else
            {
                good_bundle = false;
                has_error = true;
            }

            // get the description, we will add the description in
            // the status for now (TBD: look into whether the
            // snapmanager.cgi binary could read that from the XML
            // file instead?)
            //
            QDomElement bundle_description(root.firstChildElement("description"));
            if(!bundle_description.isNull())
            {
                description = snap_dom::xml_children_to_string(bundle_description);
            }
            else
            {
                good_bundle = false;
                has_error = true;
            }

            // list of fields to capture and send along the installation
            // processes
            //
            fields = root.firstChildElement("fields");

            // get the list of expected packages, it may be empty
            //
            std::vector<std::string> packages;
            QDomNodeList bundle_packages(bundle_xml.elementsByTagName("packages"));
            if(bundle_packages.size() == 1)
            {
                QDomElement package_list(bundle_packages.at(0).toElement());
                std::string const list_of_packages(package_list.text().toUtf8().data());
                snap::tokenize_string(packages, list_of_packages, ",", true, " ");
            }
            for(auto p : packages)
            {
                // skip empty entries (should not happen though)
                if(p.empty())
                {
                    continue;
                }
                bool const inverted(p[0] == '!');
                if(inverted)
                {
                    p.erase(p.begin());
                }

                std::string output;
                int const r(f_snap->package_status(p, output));

                bool success(false);
                if(r == 0)
                {
                    // search the space after the version
                    //
                    std::string::size_type pos(output.find(' '));

                    // check the actual status
                    //
                    if(output.compare(pos + 1, 20, "install ok installed") == 0)
                    {
                        success = true;
                        if(inverted)
                        {
                            hide_bundle = true;
                        }
                        package_name_and_version += "<li class='installed-package'>";
                        package_name_and_version += p;
                        package_name_and_version += " (";
                        package_name_and_version += output.substr(0, pos);
                        package_name_and_version += ")</li>";
                    }
                }
                if(!success)
                {
                    good_bundle = false;

                    package_name_and_version += "<li class='uninstalled-package'>";
                    package_name_and_version += p;
                    package_name_and_version += " (";
                    package_name_and_version += output.empty() ? "unknown" : output;
                    package_name_and_version += ")</li>";
                }
            }
        }

        QDomNodeList bundle_is_installed(bundle_xml.elementsByTagName("is-installed"));
        if(bundle_is_installed.size() == 1)
        {
            std::string path(f_snap->get_cache_path().toUtf8().data());
            path += "/bundle-scripts/";
            path += name.toUtf8().data();
            path += ".is-installed";
            snap::file_content script(path);
            QDomElement is_installed(bundle_is_installed.at(0).toElement());
            script.set_content(std::string("#!/bin/bash\n# auto-generated by snapmanagerdaemon (self plugin)\n") + is_installed.text().toUtf8().data());
            script.write_all();
            chmod(path.c_str(), 0755);
            snap::process p("is-installed");
            p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
            p.set_command(QString::fromUtf8(path.c_str()));
            int const r(p.run());
            if(r != 0)
            {
                int const e(errno);
                SNAP_LOG_ERROR("is-installed script failed with ")(r)(" (errno: ")(e)(", ")(strerror(e));
                // errors do not prevent us from going forward with the
                // other entries
                package_name_and_version += "<li>This bundle includes a script to test whether it is installed. That script FAILED.</li>";
                good_bundle = false;
            }
            else
            {
                QString const output(p.get_output(true));
                if(output.trimmed() == "install ok installed")
                {
                    package_name_and_version += "<li>Bundle \"";
                    package_name_and_version += name.toUtf8().data();
                    package_name_and_version += "\" is installed.</li>";
                }
                else
                {
                    good_bundle = false;
                }
            }
        }
        else if(package_name_and_version.empty())
        {
            package_name_and_version = "<li>No package name and version available for this bundle.</li>";
        }

        QString const status_info(QString("%1<p>%2</p><ul>%3</ul>")
                        .arg(snap_dom::xml_to_string(fields))
                        .arg(description)
                        .arg(QString::fromUtf8(package_name_and_version.c_str())));

        // one line per entry
        //
        if(bundles_status_file.isOpen())
        {
            // should hidden bundle just not be included?
            //
            QByteArray const name_utf8(name.toUtf8());
            bundles_status_file.write(name_utf8.data(), name_utf8.size());
            bundles_status_file.putChar(':');
            bundles_status_file.putChar(
                        has_error
                            ? 'E'
                            : hide_bundle
                                ? 'H'
                                : good_bundle
                                    ? 'I'
                                    : 'W');
            bundles_status_file.putChar(':');
            QByteArray status_info_utf8(status_info.toUtf8());
            // TODO: we probably want to also escape \\ here?
            status_info_utf8.replace("\n", "\\n")
                            .replace("\r", "\\r");
            bundles_status_file.write(status_info_utf8.data(), status_info_utf8.size());
            bundles_status_file.putChar('\n');
        }

        if(!hide_bundle)
        {
            snap_manager::status_t const package_status(
                        has_error ? snap_manager::status_t::state_t::STATUS_STATE_ERROR
                                  : good_bundle ? snap_manager::status_t::state_t::STATUS_STATE_INFO
                                                : snap_manager::status_t::state_t::STATUS_STATE_HIGHLIGHT,
                        get_plugin_name(),
                        "bundle::" + name,
                        status_info);
            server_status.set_field(package_status);
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
bool self::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "refresh")
    {
        // create a form with one Refresh button
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_REFRESH
                );

        snap_manager::widget_description::pointer_t field(std::make_shared<snap_manager::widget_description>(
                          "Click Refresh to request a new status from all the snapcommunicators, including this one."
                        , s.get_field_name()
                        , "This button makes sure that all snapcommunicators resend their status data so that way you get the latest."
                          " Note that the resending is not immediate. The thread handling the status wakes up once every minute or so,"
                          " therefore you will get new data for snapmanager.cgi within 1 or 2 minutes."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

#if 0
    if(s.get_field_name() == "snapmanager_frontend")
    {
        // the list if frontend snapmanagers that are to receive statuses
        // of the cluster computers; may be just one computer; should not
        // be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "List of Front End Servers"
                        , s.get_field_name()
                        , s.get_value()
                        , QString("This is a list of Front End servers that accept requests to snapmanager.cgi."
                                 " Only the few computers that accept such request need to be named here."
                                 " Names are expected to be comma separated.%1")
                                .arg(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_WARNING
                                        ? " <span style=\"color: red;\">The Warning Status is due to the fact that"
                                          " the list on this computer is currently empty. If it was not defined yet,"
                                          " add the value. If it is defined on other servers, you may want to go on"
                                          " one of those servers page and click Save Everywhere from there.</span>"
                                        : "")
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }
#endif

    if(s.get_field_name() == "redirect_unwanted")
    {
        // the list of URIs from which we can download software bundles;
        // this should not be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "A URI to redirect unwanted hits"
                        , s.get_field_name()
                        , s.get_value()
                        , "Whenever a user who is not in the list of clients=... hits the snapmanager.cgi script, he will be redirected to this URI. Absolutely any URI can be used."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "bundle_uri")
    {
        // the list of URIs from which we can download software bundles;
        // this should not be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "List of URIs to Directories of Bundles"
                        , s.get_field_name()
                        , s.get_value()
                        , QString("This is a list of comma separated URIs specifying the location of Directory Bundles. Usually, this is just one URI.")
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "reboot_required")
    {
        // the OS declared that a reboot was required, offer the option
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_REBOOT
                );

        snap_manager::widget_description::pointer_t field(std::make_shared<snap_manager::widget_description>(
                          "Reboot Required"
                        , s.get_field_name()
                        , s.get_value() // the value is the description!
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "log_level")
    {
        // the current log level (at least in snapmanagerdaemon.properties)
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Enter Log Level"
                        , s.get_field_name()
                        , s.get_value()
                        , "<p>The log level can be any one of the following:</p>"
                          "<ul>"
                            "<li>TRACE -- trace level, you get everything!</li>"
                            "<li>DEBUG -- debug level, you get additional logs about things that may be problems.</li>"
                            "<li>INFO -- normal informational level, this is the default.</li>"
                            "<li>WARNING -- only display warnings, errors and fatal errors, no additional information.</li>"
                            "<li>ERROR -- only display errors and fatal errors.</li>"
                            "<li>FATAL -- only display messages about fatal errors (why a service quits abnormally when it has a chance to log such.)</li>"
                          "</ul>"
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "upgrade_required")
    {
        // the OS declared that a reboot was required, offer the option
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_UPGRADE
                  | snap_manager::form::FORM_BUTTON_UPGRADE_EVERYWHERE
                );

        snap::snap_string_list counts(s.get_value().split(";"));
        if(counts.size() < 3)
        {
            // put some defaults to avoid crashes
            counts << "0";
            counts << "0";
            counts << "0";
        }

        QString const description(QString("%1%2 packages can be updated.<br/>%3 updates are security updates.")
                            .arg(counts[0])
                            .arg(counts[1])
                            .arg(counts[2])
                            );

        snap_manager::widget_description::pointer_t field(std::make_shared<snap_manager::widget_description>(
                          "Upgrade Required"
                        , s.get_field_name()
                        , description
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name().startsWith("bundle::"))
    {
        // offer the end user to install (not yet installed) or
        // uninstall (already installed) the bundle
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_INFO
                        ? snap_manager::form::FORM_BUTTON_UNINSTALL
                        : snap_manager::form::FORM_BUTTON_INSTALL
                );

        // the value is the description, although it may include fields
        // which we want to extract if they are present...
        //
        QString fields;
        QString value(s.get_value());
        if(value.startsWith("<fields>"))
        {
            // make sure to at least remove the fields from the value,
            // but if we are in the Uninstall mode, then ignore the
            // fields entirely
            //
            int const pos(value.indexOf("</fields>"));
            if(s.get_state() != snap_manager::status_t::state_t::STATUS_STATE_INFO)
            {
                fields = value.mid(0, pos + 9);
            }
            value = value.mid(pos + 9);
        }

        snap_manager::widget_description::pointer_t description_field(std::make_shared<snap_manager::widget_description>(
                          "Bundle Details"
                        , s.get_field_name()
                        , value
                    ));
        f.add_widget(description_field);

        if(!fields.isEmpty())
        {
            QDomDocument fields_doc;
            fields_doc.setContent(fields, false);
            QDomNodeList field_tags(fields_doc.elementsByTagName("field"));
            int const max_fields(field_tags.size());
            for(int idx(0); idx < max_fields; ++idx)
            {
                QDomElement field_tag(field_tags.at(idx).toElement());

                QString const field_name(field_tag.attribute("name"));
                //QString const field_type(f.attribute("type")); -- add this once we need it, right now it's all about input fields

                QString label;
                QString initial_value;
                QString description;

                QDomElement c(field_tag.firstChildElement());
                while(!c.isNull())
                {
                    if(c.tagName() == "label")
                    {
                        label = c.text();
                    }
                    else if(c.tagName() == "description")
                    {
                        // description may include HTML tags
                        description = snap_dom::xml_children_to_string(c);
                    }
                    else if(c.tagName() == "initial-value")
                    {
                        initial_value = c.text();
                    }
                    c = c.nextSiblingElement();
                }

                snap_manager::widget_input::pointer_t install_field(std::make_shared<snap_manager::widget_input>(
                                  label
                                , QString("bundle_install_field::%1").arg(field_name)
                                , initial_value
                                , description
                            ));
                f.add_widget(install_field);
            }
        }

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
 * \return true if the new_value was applied and the affected_services
 *         should be sent their RELOADCONFIG message.
 */
bool self::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::set<QString> & affected_services)
{
    // refresh is a special case in the "self" plugin only
    //
    if(button_name == "refresh")
    {
        {
            // setup the message to send to other snapmanagerdaemons
            //
            snap::snap_communicator_message resend;
            resend.set_service("*");
            resend.set_command("MANAGERRESEND");
            resend.add_parameter("kick", "now");

            // we just send a UDP message in this case, no acknowledgement
            //
            snap::snap_communicator::snap_udp_server_message_connection::send_message(f_snap->get_signal_address()
                                                                                    , f_snap->get_signal_port()
                                                                                    , resend);
        }
        {
            snap::snap_communicator_message cgistatus;
            cgistatus.set_service("*");
            cgistatus.set_command("CGISTATUS_REQUEST");

            // we just send a UDP message in this case, no acknowledgement
            //
            snap::snap_communicator::snap_udp_server_message_connection::send_message(f_snap->get_signal_address()
                                                                                    , f_snap->get_signal_port()
                                                                                    , cgistatus);
        }
        {
            snap::snap_communicator_message backendstatus;
            backendstatus.set_service("*");
            backendstatus.set_command("BACKENDSTATUS_REQUEST");

            // we just send a UDP message in this case, no acknowledgement
            //
            snap::snap_communicator::snap_udp_server_message_connection::send_message(f_snap->get_signal_address()
                                                                                    , f_snap->get_signal_port()
                                                                                    , backendstatus);
        }

        // message sent...
        //
        return true;
    }

    // installation is a special case in the "self" plugin only (or at least
    // it should most certainly only be specific to this plugin.)
    //
    bool const install(button_name == "install");
    if(install
    || button_name == "uninstall")
    {
        if(!field_name.startsWith("bundle::"))
        {
            SNAP_LOG_ERROR("install or uninstall with field_name \"")(field_name)("\" is invalid, we expected a name starting with \"bundle::\".");
            return false;
        }
        QByteArray values(old_or_installation_value.toUtf8());
        NOTUSED(f_snap->installer(field_name.mid(8), install ? "install" : "purge", values.data(), affected_services));
        return true;
    }

    // after installations and upgrades, a reboot may be required
    //
    if(button_name == "reboot")
    {
        f_snap->reboot();
        return true;
    }

    // once in a while packages get an update, the upgrade button appears
    // and when clicked this funtion gets called
    //
    if(button_name == "upgrade"
    || button_name == "upgrade_everywhere")
    {
        NOTUSED(f_snap->upgrader());
        //f_snap->reset_aptcheck(); -- this is too soon, the upgrader() call
        //                             now creates a child process with fork()
        //                             to make sure we can go on even when
        //                             snapmanagerdaemon gets upgraded

        // TBD: we need to add something to the affected_services?
        //      (the snapupgrader tool should restart the whole stack
        //      anyway so we should be fine...)
        return true;
    }

    // restore defaults?
    //
    bool const use_default_value(button_name == "restore_default");

    bool const reset_bundle_uri(field_name == "bundle_uri");
    if(reset_bundle_uri)
    {
        // if a failure happens, we do not create the last update time
        // file, that means we will retry to read the bundles each time;
        // so deleting that file is like requesting an immediate reload
        // of the bundles
        //
        QString const reset_filename(QString("%1/bundles.reset").arg(f_snap->get_bundles_path()));
        QFile reset_file(reset_filename);
        if(!reset_file.open(QIODevice::WriteOnly))
        {
            SNAP_LOG_WARNING("failed to create the \"")(reset_filename)("\", changes to the bundles URI may not show up as expected.");
        }
    }

    // WARNING: since we commented out the snapmanager_frontend for now
    //          we should never get here with such a field name so we
    //          do not have to do that here
    //
    if(field_name == "snapmanager_frontend"
    || reset_bundle_uri)
    {
        affected_services.insert("snapmanagerdaemon");

        QString value(new_value);
        if(use_default_value)
        {
            if(field_name == "snapmanager_frontend")
            {
                value = "";
            }
            else if(reset_bundle_uri)
            {
                value = "http://bundles.snapwebsites.info/";
            }
        }

        // TODO: the path to the snapmanager.conf is hard coded, it needs to
        //       use the path of the file used to load the .conf in the
        //       first place (I'm just not too sure how to get that right
        //       now, probably from the "--config" parameter, but how do
        //       we do that for each service?)
        //
        NOTUSED(f_snap->replace_configuration_value("/etc/snapwebsites/snapwebsites.d/snapmanager.conf", field_name, value));
        return true;
    }

    if(field_name == "redirect_unwanted")
    {
        // replace the value in that field so it shows updated in the
        // interface as well
        //
        snap::snap_config snapmanager(g_configuration_filename);
        snapmanager["redirect_unwanted"] = new_value;

        // we do not need to restart anything because the value is used
        // by the snapmanager.cgi which is started on each user access
        //
        NOTUSED(f_snap->replace_configuration_value("/etc/snapwebsites/snapwebsites.d/snapmanager.conf", field_name, new_value));
        return true;
    }

    // user wants a new log level
    //
    if(field_name == "log_level")
    {
        // we have to restart all the services, by restarting snapcommunicator
        // though, it restarts everything.
        //
        affected_services.insert("snapcommunicator");

        SNAP_LOG_DEBUG("Running command: snapchangeloglevel ")(new_value);
        int const r(system(("snapchangeloglevel " + new_value).toUtf8().data()));
        if(r != 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR("snapchangeloglevel failed with error: ")(e)(", ")(strerror(e));
        }
        return true;
    }

    return false;
}


void self::on_add_plugin_commands(snap::snap_string_list & understood_commands)
{
    understood_commands << "BACKENDSTATUS";
    understood_commands << "CGISTATUS";
}


void self::on_process_plugin_message(snap::snap_communicator_message const & message, bool & processed)
{
    QString const command(message.get_command());
    SNAP_LOG_TRACE("self::on_process_plugin_message(), command=[")(command)("]");

    if(command == "BACKENDSTATUS")
    {
        f_backends_active = 0;

        auto const & params( message.get_all_parameters() );
        for( auto const & key : params.keys() )
        {
            if( !key.startsWith("backend_") )
            {
                continue;
            }
            //
            snap_manager::service_status_t status( snap_manager::manager::string_to_service_status(params[key].toUtf8().data()) );
            switch( status )
            {
                case snap_manager::service_status_t::SERVICE_STATUS_UNKNOWN       :
                case snap_manager::service_status_t::SERVICE_STATUS_NOT_INSTALLED :
                case snap_manager::service_status_t::SERVICE_STATUS_DISABLED      :
                    break;

                case snap_manager::service_status_t::SERVICE_STATUS_ENABLED       :
                case snap_manager::service_status_t::SERVICE_STATUS_ACTIVE        :
                case snap_manager::service_status_t::SERVICE_STATUS_FAILED        :
                    ++f_backends_active;
                    break;
            }
        }

        SNAP_LOG_DEBUG("BACKENDSTATUS recieved! f_backends_active=")(f_backends_active);
        processed = true;
    }
    else if( command == "CGISTATUS" )
    {
        f_system_active = (message.get_integer_parameter("status") == 0);
        SNAP_LOG_DEBUG("CGISTATUS recieved! f_system_active=")(f_system_active);
        processed = true;
    }
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
