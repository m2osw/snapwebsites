// Snap Websites Server -- watchdog processes
// Copyright (c) 2013-2018  Made to Order Software Corp.  All Rights Reserved
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

// ourselves
//
#include "processes.h"

// snapwebsites lib
//
#include <snapwebsites/glob_dir.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/process.h>
#include <snapwebsites/qdomhelpers.h>

// Qt lib
//
#include <QFile>
#include <QRegExp>

// last include
//
#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(processes, 1, 0)



namespace
{


/** \brief Class used to read the list of processes to check.
 *
 * The class understands the following XML format:
 *
 * \code
 * <watchdog-processes>
 *   <process name="name" mandatory="mandatory">
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

                                watchdog_process_t(QString const & name, bool mandatory);

    void                        set_command(QString const & command);
    void                        set_match(QString const & match);

    QString const &             get_name() const;
    bool                        is_mandatory() const;
    bool                        match(QString const & name, QString const & cmdline);

private:
    QString                     f_name;
    QString                     f_command;
    QSharedPointer<QRegExp>     f_match;
    bool                        f_mandatory = false;
};


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
 */
watchdog_process_t::watchdog_process_t(QString const & name, bool mandatory)
    : f_name(name)
    , f_mandatory(mandatory)
{
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


/** \brief Match the name and command line against this process definition.
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
        // if no command line and no match were specified then the name
        // is the process name
        //
        if(f_name != command)
        {
            return false;
        }
    }

    return true;
}




/** \brief Load an XML file.
 *
 * This function loads one XML file and transform it in a
 * watchdog_process_t structure.
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
                watchdog_process_t wp(name, mandatory);

                QDomNodeList cmdline_tags(process.elementsByTagName("cmdline"));
                if(cmdline_tags.size() > 0)
                {
                    QDomNode cmdline_node(cmdline_tags.at(0));
                    if(cmdline_node.isElement())
                    {
                        QDomElement cmdline(cmdline_node.toElement());
                        wp.set_command(cmdline.text());
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
        processes_path = "/var/lib/snapwebsites/snapwatchdog/processes";
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
    NOTREACHED();
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
    NOTUSED(last_updated);
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

    SNAP_LISTEN(processes, "server", watchdog_server, process_watch, _1);
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 */
void processes::on_process_watch(QDomDocument doc)
{
    SNAP_LOG_TRACE("processes::on_process_watch(): processing");

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
            size_t const max_re(g_processes.size());
            for(size_t j(0); j < max_re; ++j)
            {
                QDomElement proc(doc.createElement("process"));
                e.appendChild(proc);

                proc.setAttribute("name", g_processes[j].get_name());
                proc.setAttribute("error", "missing");

                // TBD: what should the priority be on this one?
                //      it's likely super important so more than 50
                //      but probably not that important that it should be
                //      close to 100?
                //
                if(g_processes[j].is_mandatory())
                {
                    f_snap->append_error(doc
                                       , "processes"
                                       , QString("can't find mandatory process \"%1\" in the list of processes.")
                                                .arg(g_processes[j].get_name())
                                       , 95);
                }
                else
                {
                    f_snap->append_error(doc
                                       , "processes"
                                       , QString("can't find expected process \"%1\" in the list of processes.")
                                                .arg(g_processes[j].get_name())
                                       , 60);
                }
            }
            break;
        }
        std::string name(info->get_process_name());

        // keep the full path in the cmdline parameter
        //
        QString cmdline(QString::fromUtf8(name.c_str()));

        std::string::size_type p(name.find_last_of('/'));
        if(p != std::string::npos)
        {
            name = name.substr(p + 1);
        }
        QString const utf8_name(QString::fromUtf8(name.c_str()));

        int const count_max(info->get_args_size());
        for(int c(0); c < count_max; ++c)
        {
            // skip empty arguments
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
                g_processes.erase(g_processes.begin() + j);
                break;
            }
        }
    }
}




SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
