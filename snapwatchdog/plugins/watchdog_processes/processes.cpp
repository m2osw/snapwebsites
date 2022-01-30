// Snap Websites Server -- watchdog processes
// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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


// self
//
#include "processes.h"


// snapwebsites lib
//
#include <snapwebsites/file_content.h>
#include <snapwebsites/glob_dir.h>
#include <snapwebsites/log.h>
#include <snapwebsites/process.h>
#include <snapwebsites/qdomhelpers.h>


// snapdev lib
//
#include <snapdev/not_used.h>


// Qt lib
//
#include <QFile>
#include <QRegExp>


// last include
//
#include <snapdev/poison.h>




SNAP_PLUGIN_START(processes, 1, 0)



namespace
{


char const * g_server_configuration_filename = "snapserver";

char const * g_configuration_apache2_maintenance = "/etc/apache2/snap-conf/snap-apache2-maintenance.conf";


/** \brief Check whether a service is enabled or not.
 *
 * The Snap! Watchdog does view a missing process as normal if the
 * corresponding service is marked as disabled. This function tells
 * us whether the service is considered up and running or not.
 *
 * When the XML file includes the \<service> tag, it calls this
 * function. If the function returns false, then no further test
 * is done and the process entry is ignored.
 *
 * \note
 * This means a process that's turned off for maintenance does not
 * generate errors for being turned off during that time OR AFTER
 * IF YOU FORGET TO TURN IT BACK ON. A later version may want to
 * have a way to know whether the process is expected to be on and
 * if so still generate an error after X hours of being down...
 * (or once the system is back up, i.e., it's not in maintenance
 * mode anymore.) However, at this point we do not know which
 * snapbackend are expected to be running.
 *
 * \param[in] service_name  The name of the service, as systemd understands
 *            it, to check on.
 *
 * \return true if the service is marked as enabled.
 */
bool is_service_enabled(QString const & service_name)
{
    // here I use the `show` command instead of the `is-enabled` to avoid
    // errors whenever the service is not even installed, which can happen
    // (i.e. clamav-freshclam is generally only installed on one system in
    // the entire cluster)
    //
    snap::process p("query service status");
    p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
    p.set_command("systemctl");
    p.add_argument("show");
    p.add_argument("-p");
    p.add_argument("UnitFileState");
    //p.add_argument("--value"); -- available since systemd 230, so not on Ubuntu 16.04
    p.add_argument(service_name);
    int const r(p.run());
    QString const output(p.get_output(true).trimmed());
    SNAP_LOG_INFO("\"show -p UnitFileState\" query output (")(r)("): ")(output);


    // we cannot use 'r' since it is 0 if the command works whether or not
    // the corresponding unit even exist on the system
    //
    // so instead we just have to test the output and it must be exactly
    // equal to the following
    //
    // (other possible values are static, disabled, and an empty value for
    // non-existant units.)
    //
    return output == "UnitFileState=enabled";


//    snap::process p("query service status");
//    p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
//    p.set_command("systemctl");
//    p.add_argument("is-enabled");
//    p.add_argument(service_name);
//    int const r(p.run());
//    QString const output(p.get_output(true).trimmed());
//    SNAP_LOG_INFO("\"is-enabled\" query output (")(r)("): ")(output);
//
//    // there is a special case with static services: the is-enabled returns
//    // true (r == 0) even when they are not enabled
//    //
//    if(output == "static")
//    {
//        return false;
//    }
//
//    return r == 0;
}


/** \brief Check whether a service is active or not.
 *
 * The Snap! Watchdog checks whether a service is considered active too.
 * A service may be marked as enabled but it may not be active.
 *
 * \param[in] service_name  The name of the service, as systemd understands
 *            it, to check on.
 *
 * \return true if the service is marked as active.
 */
bool is_service_active(QString const & service_name)
{
    snap::process p("query service status");
    p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
    p.set_command("systemctl");
    p.add_argument("is-active");
    p.add_argument(service_name);
    int const r(p.run());
    SNAP_LOG_INFO("\"is-active\" query output (")(r)("): ")(p.get_output(true).trimmed());
    return r == 0;
}


/** \brief Check whether the system is in maintenance mode.
 *
 * This function checks whether the standard maintenance mode is currently
 * turn on or not. This is done by checking the maintenance Apache
 * configuration file and see whether the lines between ##MAINTENANCE-START##
 * and ##MAINTENANCE-END## are commented out or not.
 *
 * \return true if the maintenance mode is ON.
 */
bool is_in_maintenance()
{
    snap::file_content conf(g_configuration_apache2_maintenance);
    if(!conf.exists())
    {
        // the maintenance file doesn't exist, assume the worst, that
        // we are not in maintenance
        //
        return false;
    }

    std::string const content(conf.get_content());
    std::string::size_type const pos(content.find("##MAINTENANCE-START##"));
    if(pos == std::string::npos)
    {
        // marker not found... consider we are live
        //
        return false;
    }

    char const * s(content.c_str() + pos + 21);
    while(isspace(*s))
    {
        ++s;
    }
    if(*s == '#')
    {
        // not in maintenance, fields are commented out
        //
        return false;
    }

    std::string::size_type const ra_pos(content.find("Retry-After"));
    if(ra_pos == std::string::npos)
    {
        // no Retry-After header?!
        //
        return false;
    }

    return true;
}


/** \brief Class used to read the list of processes to check.
 *
 * The class understands the following XML format:
 *
 * \code
 * <watchdog-processes>
 *   <process name="name" mandatory="mandatory" allow_duplicates="allow_duplicates">
 *      <command>...</command>
 *      <match>...</match>
 *   </process>
 *   <process name="..." ...>
 *      ...
 *   </process>
 *   ...
 * </watchdog-processes>
 * \endcode
 */
class watchdog_process_t
{
public:
    typedef std::vector<watchdog_process_t>     vector_t;

                                watchdog_process_t(QString const & name, bool mandatory, bool allow_duplicates);

    void                        set_mandatory(bool mandatory);
    void                        set_command(QString const & command);
    void                        set_service(QString const & service, bool backend);
    void                        set_match(QString const & match);

    QString const &             get_name() const;
    bool                        is_mandatory() const;
    bool                        is_backend() const;
    bool                        is_process_expected_to_run();
    bool                        allow_duplicates() const;
    bool                        match(QString const & name, QString const & cmdline);

private:
    static snap_string_list     g_valid_backends;

    QString                     f_name = QString();
    QString                     f_command = QString();
    QString                     f_service = QString();
    QSharedPointer<QRegExp>     f_match = QSharedPointer<QRegExp>();
    bool                        f_mandatory = false;
    bool                        f_allow_duplicates = false;
    bool                        f_service_is_enabled = true;
    bool                        f_service_is_active = true;
    bool                        f_service_is_backend = false;
};


// define static variable
//
snap_string_list     watchdog_process_t::g_valid_backends;


watchdog_process_t::vector_t    g_processes;


/** \brief Initializes a watchdog_process_t class.
 *
 * This function initializes the watchdog_process_t making it ready to
 * run the match() command.
 *
 * To complete the setup, when available, the set_command() and
 * set_match() functions should be called.
 *
 * \param[in] name  The name of the command, in most cases this is the
 *            same as the terminal command line name.
 * \param[in] mandatory  Whether the process is mandatory (error with a
 *            very high priority if not found.)
 * \param[in] allow_duplicates  Whether this entry can be defined more than
 *            once within various XML files.
 */
watchdog_process_t::watchdog_process_t(QString const & name, bool mandatory, bool allow_duplicates)
    : f_name(name)
    , f_mandatory(mandatory)
    , f_allow_duplicates(allow_duplicates)
{
}


/** \brief Set whether this process is mandatory or not.
 *
 * Change the mandatory flag.
 *
 * At the moment this is used by the loader to force the mandatory flag
 * when a duplicate is found and the new version is mandatory. In other
 * word, it is a logical or between all the instances of the process
 * found on the system.
 *
 * \param[in] mandatory  Whether this process is mandatory.
 */
void watchdog_process_t::set_mandatory(bool mandatory)
{
    f_mandatory = mandatory;
}


/** \brief Set the name of the expected command.
 *
 * The name of the watchdog process may be different from the exact
 * terminal command name. For example, the cassandra process runs
 * using "java" and not "cassandra". In that case, the command would
 * be set "java".
 *
 * \param[in] command  The name of the command to seek.
 */
void watchdog_process_t::set_command(QString const & command)
{
    f_command = command;
}


/** \brief Set the name of the service corresponding to this process.
 *
 * When testing whether a process is running, the watchdog can first
 * check whether that process is a service (i.e. when a service name was
 * specified in the XML.) When a process is a known service and the
 * service is disabled, then whether the service is running is none of
 * our concern. However, if enabled and the service is not running,
 * then there is a problem.
 *
 * Note that by default a process is not considered a service. You
 * have to explicitely mark it as such with the \<service> tag.
 * This also allows you to have a name for the service which is
 * different than the name of the executable (i.e. "snapwatchdog"
 * is the service and "snapwatchdogserver" is the executable.)
 *
 * You may reset the service to QString(). In that case, it resets
 * the flags to their defaults and ignores the \p backend parameter.
 *
 * \param[in] service  The name of the service to check.
 * \param[in] backend  Whether the service is a snapbackend.
 */
void watchdog_process_t::set_service(QString const & service, bool backend)
{
    // we check whether the service is running just once here
    // (otherwise we could end up calling that function once per
    // process!)
    //
    f_service = service;

    if(f_service.isEmpty())
    {
        f_service_is_enabled = true;
        f_service_is_active = true;
        f_service_is_backend = false;
    }
    else
    {
        f_service_is_enabled = is_service_enabled(service);
        f_service_is_active = f_service_is_enabled
                                    ? is_service_active(service)
                                    : false;
        f_service_is_backend = backend;
    }
}


/** \brief Define the match regular expression.
 *
 * If the process has a complex command line definition to be checked,
 * then this regular expression can be used. For example, to check
 * whether Cassandra is running, we search for a Java program which
 * runs the Cassandra system. This is done using a regular expression:
 *
 * \code
 *   <match>java.*org\.apache\.cassandra\.service\.CassandraDaemon</match>
 * \endcode
 *
 * (at the moment, though, we have a specialized Cassandra plugin and
 * thus this is not part of the list of processes in our XML files.)
 *
 * \param[in] match  A valid regular expression.
 */
void watchdog_process_t::set_match(QString const & match)
{
    f_match = QSharedPointer<QRegExp>(new QRegExp(match));
}


/** \brief Get the name of the process.
 *
 * This function returns the name of the process. Note that the
 * terminal command line may be different.
 *
 * \return The name that process was given.
 */
QString const & watchdog_process_t::get_name() const
{
    return f_name;
}


/** \brief Check whether this process is considered mandatory.
 *
 * This function returns the mandatory flag.
 *
 * By default processes are not considered mandatory. Add the
 * mandatory attribute to the tag to mark a process as mandatory.
 *
 * This flag tells us what priority to use when we generate an
 * error when a process can't be found. 60 when not mandatory
 * and 95 when mandatory.
 *
 * \return true if the mandatory flag is set.
 */
bool watchdog_process_t::is_mandatory() const
{
    return f_mandatory;
}


/** \brief Check whether this process is a backend service.
 *
 * Whenever a process is marked as a service, it can also specifically
 * be marked as a backend service.
 *
 * A backend service is not forcibly expected to be running whenever
 * the system is put in maintenance mode. This flag is used to test
 * that specific status.
 *
 * \return true if the process was marked as a backend service.
 */
bool watchdog_process_t::is_backend() const
{
    return f_service_is_backend;
}


/** \brief Check whether a backend is running or not.
 *
 * This functin is used to determine whether the specified backend service
 * is expected to be running or not.
 *
 * If the main flag (`backend_status`) is set to `disabled`, then the
 * backend service is viewed as disabled and this function returns
 * false.
 *
 * When the `backend_status` is not set to `disabled` the function further
 * checks on the backends list of services and determine whether the named
 * process is defined there. If so, then it is considered `enabled` (i.e.
 * it has to be running since the user asks for it to be running.)
 *
 * \return true if the backend is enabled (expected to run), false otherwise
 */
bool watchdog_process_t::is_process_expected_to_run()
{
    // is this even marked as a service?
    // if not then it has to be running
    //
    // (i.e. services which we do not offer to disable are expected to always
    // be running--except while upgrading or rebooting which we should also
    // look into TODO)
    //
    if(f_service.isEmpty())
    {
        return true;
    }

    // we have two cases:
    //
    // 1. backend services
    //
    // 2. other more general services
    //
    // we do not handle them the same way at all, backends have two flags
    // to check (first block below) and we completely ignore the status
    // of the service
    //
    // as for the more general services they just have their systemd status
    // (i.e. whether they are active or disabled)
    //
    if(is_backend())
    {
        // all the backend get disabled whenever the administrator sets
        // the "backend_status" flag to "disabled", this is global to all
        // the computer of a cluster (at least it is expected to be that way)
        //
        // whatever other status does not matter if this flag is set to
        // disabled then the backed is not expected to be running
        //
        // note: configuration files are cached so the following is rather
        //       fast the second time (i.e. access an std::map<>().)
        //
        snap_config snap_server_conf(g_server_configuration_filename);
        if(snap_server_conf["backend_status"] == "disabled")
        {
            // the administrator disabled all the backends
            //
            return false;
        }

        // okay, now check whether that specific backend is expected to
        // be running on this system because that varies "widely"
        //
        // note: we cache the list of backends once and reuse them as
        //       required (the g_valid_backends variable is static.)
        //
        if(g_valid_backends.isEmpty())
        {
            QString const expected_backends(snap_server_conf["backends"]);
            g_valid_backends = expected_backends.split(',');
            int const max(g_valid_backends.size());
            for(int idx(0); idx < max; ++idx)
            {
                // in case the admin edited that list manually, we need to
                // fix it before we use it (we should look into using our
                // tokenize_string instead because it can auto-trim, only
                // it uses std::string's)
                //
                g_valid_backends[idx] = g_valid_backends[idx].trimmed();
            }
        }

        // check the status the administrator expects for this backend
        //
        return g_valid_backends.contains(f_service);
    }

    // else -- this is a service, just not a backend (i.e. snapserver)
    //
    // so a service is expected to be running if enabled and/or active
    //
    return f_service_is_enabled
        || f_service_is_active;
}


/** \brief Wether duplicate definitions are allowed or not.
 *
 * If a process is required by more than one package, then it should
 * be defined in each one of them and it should be marked as a
 * possible duplicate.
 *
 * For example, the mysqld service is required by snaplog and snaplistd.
 * Both will have a defintion for mysqld (because one could be installed
 * on a backend and the other on another backend.) However, when they
 * both get installed on the same machine, you get two definitions with
 * the same process name. If this function returns false for either one,
 * then the setup throws.
 *
 * \return true if the process definitions can have duplicates for that process.
 */
bool watchdog_process_t::allow_duplicates() const
{
    return f_allow_duplicates;
}


/** \brief Match the name and command line against this process definition.
 *
 * If this process is connected to a service, we check whether that service
 * is enabled. If not, then we assume that the user explicitly disabled
 * that service and thus we can't expect the process as running.
 *
 * If we have a command (\<command> tag) then the \p name must match
 * that parameter.
 *
 * If we have a regular expression (\<match> tag), then we match it against
 * the command line (\p cmdline).
 *
 * If there is is no command and no regular expression, then the name of
 * the process is compared directly against the \p command parameter and
 * it has to match that.
 *
 * \param[in] command  The command being checked, this is the command line
 *            very first parameter with the path stripped.
 * \param[in] cmdline  The full command line to compare against the
 *            match regular expression when defined.
 *
 * \return The function returns true if the process is a match.
 */
bool watchdog_process_t::match(QString const & command, QString const & cmdline)
{
    if(!f_command.isEmpty())
    {
        if(f_command != command)
        {
            return false;
        }
    }

    if(f_match != nullptr)
    {
        if(f_match->indexIn(cmdline) == -1)
        {
            return false;
        }
    }

    if(f_command.isEmpty()
    && f_match == nullptr)
    {
        // if no command line and no match were specified then f_name
        // is the process name
        //
        if(f_name != command)
        {
            return false;
        }
    }

    return true;
}


/** \brief Load a process XML file.
 *
 * This function loads one XML file and transform it in a
 * watchdog_process_t object.
 *
 * \param[in] processes_filename  The name of an XML file representing processes.
 */
void load_xml(QString processes_filename)
{
    QFile input(processes_filename);
    if(input.open(QIODevice::ReadOnly))
    {
        QDomDocument doc;
        if(doc.setContent(&input, false)) // TODO: add error handling for debug
        {
            // we got the XML loaded
            //
            QDomNodeList processes(doc.elementsByTagName("process"));
            int const max(processes.size());
            for(int idx(0); idx < max; ++idx)
            {
                QDomNode p(processes.at(idx));
                if(!p.isElement())
                {
                    continue;
                }
                QDomElement process(p.toElement());
                QString const name(process.attribute("name"));
                if(name.isEmpty())
                {
                    throw processes_exception_invalid_process_name("the name of a process cannot be the empty string");
                }

                bool const mandatory(process.hasAttribute("mandatory"));
                bool const allow_duplicates(process.hasAttribute("allow_duplicates"));

                auto it(std::find_if(
                          g_processes.begin()
                        , g_processes.end()
                        , [name, allow_duplicates](auto & wprocess)
                        {
                            if(name == wprocess.get_name())
                            {
                                if(!allow_duplicates
                                || !wprocess.allow_duplicates())
                                {
                                    throw processes_exception_invalid_process_name("found process \"" + name + "\" twice and duplicates are not allowed.");
                                }
                                return true;
                            }
                            return false;
                        }));
                if(it != g_processes.end())
                {
                    // skip the duplicate, we assume that the command,
                    // match, etc. are identical enough for the system
                    // to still work as expected
                    //
                    if(mandatory)
                    {
                        it->set_mandatory(true);
                    }
                    continue;
                }

                watchdog_process_t wp(name, mandatory, allow_duplicates);

                QDomNodeList command_tags(process.elementsByTagName("command"));
                if(command_tags.size() > 0)
                {
                    QDomNode command_node(command_tags.at(0));
                    if(command_node.isElement())
                    {
                        QDomElement command(command_node.toElement());
                        wp.set_command(command.text());
                    }
                }

                QDomNodeList service_tags(process.elementsByTagName("service"));
                if(service_tags.size() > 0)
                {
                    QDomNode service_node(service_tags.at(0));
                    if(service_node.isElement())
                    {
                        QDomElement service(service_node.toElement());
                        wp.set_service(service.text(), service.hasAttribute("backend"));
                    }
                }

                QDomNodeList match_tags(process.elementsByTagName("match"));
                if(match_tags.size() > 0)
                {
                    QDomNode match_node(match_tags.at(0));
                    if(match_node.isElement())
                    {
                        QDomElement match(match_node.toElement());
                        wp.set_match(match.text());
                    }
                }

                g_processes.push_back(wp);
            }
        }
    }
}


/** \brief Load the list of watchdog processes.
 *
 * This function loads the XML from the watchdog and other packages.
 *
 * \param[in] processes_path  The path to the list of XML files declaring
 *            processes that should be running.
 */
void load_processes(QString processes_path)
{
    g_processes.clear();

    // get the path to the processes XML files
    //
    if(processes_path.isEmpty())
    {
        processes_path = "/usr/share/snapwebsites/snapwatchdog/processes";
    }

    glob_dir const script_filenames(processes_path + "/*.xml", GLOB_ERR | GLOB_NOSORT | GLOB_NOESCAPE);
    script_filenames.enumerate_glob(std::bind(&load_xml, std::placeholders::_1));
}


}
// no name namespace



/** \brief Get a fixed processes plugin name.
 *
 * The processes plugin makes use of different names. This function ensures
 * that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_WATCHDOG_PROCESSES_PATH:
        return "watchdog_processes_path";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_WATCHDOG_PROCESSES_...");

    }
    snapdev::NOT_REACHED();
}




/** \brief Initialize the processes plugin.
 *
 * This function is used to initialize the processes plugin object.
 */
processes::processes()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the processes plugin.
 *
 * Ensure the processes object is clean before it is gone.
 */
processes::~processes()
{
}


/** \brief Get a pointer to the processes plugin.
 *
 * This function returns an instance pointer to the processes plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the processes plugin.
 */
processes * processes::instance()
{
    return g_plugin_processes_factory.instance();
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
QString processes::description() const
{
    return "Check whether a set of processes are running.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString processes::dependencies() const
{
    return "|server|";
}


/** \brief Check whether updates are necessary.
 *
 * This function is ignored in the watchdog.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t processes::do_update(int64_t last_updated)
{
    snapdev::NOT_USED(last_updated);
    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in watchdog
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize processes.
 *
 * This function terminates the initialization of the processes plugin
 * by registering for various events.
 *
 * \param[in] snap  The child handling this request.
 */
void processes::bootstrap(snap_child * snap)
{
    f_snap = static_cast<watchdog_child *>(snap);

    SNAP_LISTEN(processes, "server", watchdog_server, process_watch, boost::placeholders::_1);
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 */
void processes::on_process_watch(QDomDocument doc)
{
    SNAP_LOG_DEBUG("processes::on_process_watch(): processing");

    load_processes(f_snap->get_server_parameter(get_name(name_t::SNAP_NAME_WATCHDOG_PROCESSES_PATH)));

    QDomElement parent(snap_dom::create_element(doc, "watchdog"));
    QDomElement e(snap_dom::create_element(parent, "processes"));

    process_list list;

    list.set_field(process_list::field_t::COMMAND_LINE);
    list.set_field(process_list::field_t::STATISTICS);
    while(!g_processes.empty())
    {
        process_list::proc_info::pointer_t info(list.next());
        if(info == nullptr)
        {
            // some process(es) missing?
            //
            size_t const max_re(g_processes.size());
            for(size_t j(0); j < max_re; ++j)
            {
                QDomElement proc(doc.createElement("process"));
                e.appendChild(proc);

                proc.setAttribute("name", g_processes[j].get_name());
                if(g_processes[j].is_process_expected_to_run())
                {
                    // this process is expected to be running so having
                    // found it in this loop, it is an error (missing)
                    //
                    proc.setAttribute("error", "missing");

                    // TBD: what should the priority be on this one?
                    //      it's likely super important so more than 50
                    //      but probably not that important that it should be
                    //      close to 100?
                    //
                    int priority(50);
                    char const * message_fmt = nullptr;
                    if(g_processes[j].is_mandatory())
                    {
                        message_fmt = "can't find mandatory process \"%1\" in the list of processes.";
                        priority = 95;
                    }
                    else
                    {
                        message_fmt = "can't find expected process \"%1\" in the list of processes.";
                        priority = 60;
                    }

                    if(g_processes[j].is_backend()
                    && is_in_maintenance())
                    {
                        // a backend which is not running while we are in
                        // maintenance is a very low priority
                        //
                        priority = 5;
                    }

                    f_snap->append_error(
                              doc
                            , "processes"
                            , QString(message_fmt).arg(g_processes[j].get_name())
                            , priority);
                }
                else
                {
                    proc.setAttribute("resident", "no");
                }
            }
            break;
        }
        std::string name(info->get_process_name());

        // keep the full path in the cmdline parameter
        //
        QString cmdline(QString::fromUtf8(name.c_str()));

        // compute basename
        //
        std::string::size_type p(name.find_last_of('/'));
        if(p != std::string::npos)
        {
            name = name.substr(p + 1);
        }
        QString const utf8_name(QString::fromUtf8(name.c_str()));

        // add command line arguments
        //
        int const count_max(info->get_args_size());
        for(int c(0); c < count_max; ++c)
        {
            // skip empty arguments
            //
            if(info->get_arg(c) != "")
            {
                cmdline += " ";

                // IMPORTANT NOTE: we should escape special characters
                //                 only it would make the command line
                //                 regular expression more complicated
                //
                cmdline += QString::fromUtf8(info->get_arg(c).c_str());
            }
        }
        size_t const max_re(g_processes.size());
//std::cerr << "check process [" << name << "] -> [" << cmdline << "]\n";
        for(size_t j(0); j < max_re; ++j)
        {
            if(g_processes[j].match(utf8_name, cmdline))
            {
                QDomElement proc(doc.createElement("process"));
                e.appendChild(proc);

                proc.setAttribute("name", g_processes[j].get_name());

                // for backends we have a special case when they are running,
                // we may actually have them turned off and still running
                // which is not correct
                //
                if(g_processes[j].is_backend()
                && !g_processes[j].is_process_expected_to_run())
                {
                    proc.setAttribute("error", "running");

                    f_snap->append_error(
                              doc
                            , "processes"
                            , QString("found process \"%1\" running when disabled").arg(g_processes[j].get_name())
                            , 35);
                }

                proc.setAttribute("cmdline", cmdline);
                proc.setAttribute("pcpu", QString("%1").arg(info->get_pcpu()));
                proc.setAttribute("total_size", QString("%1").arg(info->get_total_size()));
                proc.setAttribute("resident", QString("%1").arg(info->get_resident_size()));
                proc.setAttribute("tty", QString("%1").arg(info->get_tty()));

                unsigned long long utime;
                unsigned long long stime;
                unsigned long long cutime;
                unsigned long long cstime;
                info->get_times(utime, stime, cutime, cstime);

                proc.setAttribute("utime", QString("%1").arg(utime));
                proc.setAttribute("stime", QString("%1").arg(stime));
                proc.setAttribute("cutime", QString("%1").arg(cutime));
                proc.setAttribute("cstime", QString("%1").arg(cstime));

                // remove from the list, if the list is empty, we are
                // done; if the list is not empty by the time we return
                // some processes are missing
                //
                g_processes.erase(g_processes.begin() + j);
                break;
            }
        }
    }
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
