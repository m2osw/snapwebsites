// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/
// contact@m2osw.com
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
#include    "snapmanager/manager.h"


// cppprocess
//
#include    <cppprocess/process.h>
#include    <cppprocess/io_capture_pipe.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/file_contents.h>
#include    <snapdev/glob_to_list.h>
#include    <snapdev/lockfile.h>
#include    <snapdev/mkdir_p.h>
#include    <snapdev/not_used.h>
#include    <snapdev/tokenize_string.h>


// Qt
//
#include    <QFile>


// C
//
#include    <fcntl.h>
#include    <sys/wait.h>


// last include
//
#include    <snapdev/poison.h>



namespace snap_manager
{



/** \brief Check whether a package is installed.
 *
 * This function runs a query to determine whether a named page
 * is installed or not.
 *
 * The output of the dpkg-query command we expect includes the
 * following four words:
 *
 * \code
 *      <version> install ok installed
 * \endcode
 *
 * The \<version> part will be the current version of that package.
 * The "install ok installed" part is the current status dpkg considered
 * the package in. When exactly that, it is considered that the package
 * is properly installed.
 *
 * \param[in] package_name  The name of the package to install.
 * \param[out] output  The output of the dpkg-query commmand.
 *
 * \return The exit code of the dpkg-query install command.
 */
int manager::package_status(std::string const & package_name, std::string & output)
{
    output.clear();

    cppprocess::process p("query package status");
    p.set_command("dpkg-query");
    p.add_argument("--showformat='${Version} ${Status}\\n'");
    p.add_argument("--show");
    p.add_argument(package_name);
    cppprocess::io_capture_pipe::pointer_t out(std::make_shared<cppprocess::io_capture_pipe>());
    p.set_output_io(out);
    int r(p.start());
    if(r == 0)
    {
        r = p.wait();
    }

    // the output is saved so we can send it to the user and log it...
    //
    if(r == 0)
    {
        output = out->get_output();
    }

    return r;
}


QString manager::count_packages_that_can_be_updated(bool check_cache)
{
    QString const cache_filename(QString("%1/apt-check.output").arg(f_cache_path));

    // check whether we have a cached version of the data, if so, use
    // the cache (which is dead fast in comparison to re-running the
    // apt-check function)
    //
    if(check_cache)
    {
        QFile cache(cache_filename);
        if(cache.open(QIODevice::ReadOnly))
        {
            QByteArray content_buffer(cache.readAll());
            if(content_buffer.size() > 0)
            {
                if(content_buffer.at(content_buffer.size() - 1) == '\n')
                {
                    content_buffer.resize(content_buffer.size() - 1);
                }
            }
            QString const content(QString::fromUtf8(content_buffer));
            snap::snap_string_list counts(content.split(";"));
            if(counts.size() == 1
            && counts[0] == "-1")
            {
                // the function to check that information was not available
                return QString();
            }
            if(counts.size() == 3)
            {
                time_t const now(time(nullptr));
                time_t const cached_on(counts[0].toLongLong());
                if(cached_on + 86400 >= now)
                {
                    // cache is still considered valid
                    //
                    if(counts[1] == "0")
                    {
                        // nothing needs to be upgraded
                        return QString();
                    }
                    // counts[1] packages can be upgraded
                    // counts[2] are security upgrades
                    return QString("%1;%2").arg(counts[1]).arg(counts[2]);
                }
            }
        }
    }

    // check whether we have an apt-check function were we expect it
    //
    QByteArray apt_check(f_apt_check.toUtf8());
    struct stat st;
    if(stat(apt_check.data(), &st) == 0
    && S_ISREG(st.st_mode)
    && (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0)  // make sure it is an executable
    {
        // without a quick apt-get update first the calculations from
        // apt-check are going to be off...
        //
        if(update_packages("update") == 0)
        {
            // apt-check is expected to be a python script and the output
            // will be written in 'stderr'
            //
            cppprocess::process p("apt-check");
            p.set_command(apt_check.data());
            p.add_argument("2>&1"); // python script sends output to STDERR
            cppprocess::io_capture_pipe::pointer_t out(std::make_shared<cppprocess::io_capture_pipe>());
            p.set_output_io(out);
            int r(p.start());
            if(r == 0)
            {
                r = p.wait();
            }
            if(r == 0)
            {
                QString const output(QString::fromUtf8(out->get_output().c_str()));
                if(!output.isEmpty())
                {
                    QFile cache(cache_filename);
                    if(cache.open(QIODevice::WriteOnly))
                    {
                        time_t const now(time(nullptr));
                        QString const cache_string(QString("%1;%2").arg(now).arg(output));
                        QByteArray const cache_utf8(cache_string.toUtf8());
                        cache.write(cache_utf8.data(), cache_utf8.size());
                        if(output == "0;0")
                        {
                            // again, if we have "0;0" there is nothing to upgrade
                            //
                            return QString();
                        }
                        return output;
                    }
                }
            }
        }
        else
        {
            // this should rarely happen (i.e. generally it would happen
            // whenever the database is in an unknown state)
            //
            SNAP_LOG_ERROR
                << "the \"apt-get update\" command, that we run prior to running the \"apt-check\" command, failed."
                << SNAP_LOG_SEND;

            // no ready at this point, we need to do an update and that
            // failed -- we will try again later
            //
            return QString();
        }
    }

    SNAP_LOG_ERROR
        << "the snapmanager library could not run \""
        << f_apt_check
        << "\" successfully or the output was invalid."
        << SNAP_LOG_SEND;

    {
        QFile cache(cache_filename);
        if(cache.open(QIODevice::WriteOnly))
        {
            // apt-check command is failing... do not try again
            //
            cache.write("-1", 2);
        }
        else
        {
            SNAP_LOG_ERROR
                << "the snapmanager library could not create \""
                << cache_filename
                << "\"."
                << SNAP_LOG_SEND;
        }
    }

    // pretend there is nothing to upgrade
    //
    return QString();
}


/** \brief Update the OS packages.
 *
 * This function updates the database of the OS packages.
 *
 * Since snapmanager is already installed, we do not have to do any extra
 * work to get that repository installed.
 *
 * \param[in] command  One of "update", "upgrade", "dist-upgrade", or
 * "autoremove".
 *
 * \return The exit code of the apt-get update command.
 */
int manager::update_packages(std::string const & command)
{
#ifdef _DEBUG
    std::vector<std::string> allowed_commands{"update", "upgrade", "dist-upgrade", "autoremove"};
    if(std::find(allowed_commands.begin(), allowed_commands.end(), command) == allowed_commands.end())
    {
        throw std::logic_error("update_packages() was called with an invalid command.");
    }
#endif

    cppprocess::process p("update");
    p.set_command("apt-get");
    p.add_argument("--quiet");
    p.add_argument("--assume-yes");
    if(command == "upgrade"
    || command == "dist-upgrade")
    {
        p.add_argument("--option");
        p.add_argument("Dpkg::Options::=--force-confdef");
        p.add_argument("--option");
        p.add_argument("Dpkg::Options::=--force-confold");
    }
    p.add_argument(command);
    if(command == "autoremove")
    {
        p.add_argument("--purge");
    }
    p.add_environ("DEBIAN_FRONTEND", "noninteractive");
    cppprocess::io_capture_pipe::pointer_t out(std::make_shared<cppprocess::io_capture_pipe>());
    p.set_output_io(out);
    int r(p.start());
    if(r == 0)
    {
        r = p.wait();
    }

    // the output is saved so we can send it to the user and log it...
    SNAP_LOG_INFO
        << command
        << " of packages returned (exit code: "
        << r
        << "): "
        << out->get_output()
        << SNAP_LOG_SEND;

    return r;
}



/** \brief Installs or Removes one Debian package.
 *
 * This function installs or removes ONE package as specified by
 * \p package_name.
 *
 * \param[in] package_name  The name of the package to install.
 * \param[in] command  One of "install", "remove", or "purge".
 *
 * \return The exit code of the apt-get install command.
 */
int manager::install_package(std::string const & package_name, std::string const & command)
{
#ifdef _DEBUG
    std::vector<std::string> allowed_commands{"install", "remove", "purge"};
    if(std::find(allowed_commands.begin(), allowed_commands.end(), command) == allowed_commands.end())
    {
        throw std::logic_error("install_package() was called with an invalid command.");
    }
#endif

    cppprocess::process p("install");
    p.set_command("apt-get");
    p.add_argument("--quiet");
    p.add_argument("--assume-yes");
    if(command == "install")
    {
        p.add_argument("--option");
        p.add_argument("Dpkg::Options::=--force-confdef");
        p.add_argument("--option");
        p.add_argument("Dpkg::Options::=--force-confold");
        p.add_argument("--no-install-recommends");
    }
    p.add_argument(command);
    p.add_argument(package_name);
    p.add_environ("DEBIAN_FRONTEND", "noninteractive");
    cppprocess::io_capture_pipe::pointer_t out(std::make_shared<cppprocess::io_capture_pipe>());
    p.set_output_io(out);
    int r(p.start());
    if(r == 0)
    {
        r = p.wait();
    }

    // the output is saved so we can send it to the user and log it...
    std::string const output(out->get_trimmed_output());
    if(output.empty())
    {
        SNAP_LOG_INFO
            << command
            << " of package named \""
            << package_name
            << "\" output nothing."
            << SNAP_LOG_SEND;
    }
    else
    {
        SNAP_LOG_INFO
            << command
            << " of package named \""
            << package_name
            << "\" output:\n"
            << output
            << SNAP_LOG_SEND;
    }

    return r;
}



void manager::reset_aptcheck()
{
    // cache is not unlikely wrong after that
    //
    QString const cache_filename(QString("%1/apt-check.output").arg(f_cache_path));
    unlink(cache_filename.toUtf8().data());

    // also make sure that the bundle-package-status directory content gets
    // regenerated (i.e. output of the dpkg-query calls)
    //
    snapdev::glob_to_list<std::list<std::string>> package_status;
    package_status.read_path<
        snapdev::glob_to_list_flag_t::GLOB_FLAG_NO_ESCAPE>(f_data_path + "/bundle-package-status/*.status");
    snapdev::enumerate(
          package_status
        , [](std::string const & path)
        {
            unlink(path.c_str());
        });

    snapdev::glob_to_list<std::list<std::string>> bundle_status;
    bundle_status.read_path<
        snapdev::glob_to_list_flag_t::GLOB_FLAG_NO_ESCAPE>(f_data_path + "/bundle-status/*.status");
    snapdev::enumerate(
          bundle_status
        , [](std::string const & path)
        {
            unlink(path.c_str());
        });

    //QString const bundles_status_filename(QString("%1/bundles.status").arg(f_bundles_path));
    //unlink(bundles_status_filename.toUtf8().data());

    // delete the bundles.last-update-time as well so that way on a restart
    // the snapmanagerdaemon will reload the latest bundles automatically
    // (the current version requires a restart because the bundles are loaded
    // by a thread which we start only once at the start of snapmanagerdaemon)
    //
    QString const bundles_last_update_time_filename(QString("%1/bundles.last-update-time").arg(f_bundles_path));
    unlink(bundles_last_update_time_filename.toUtf8().data());
}


bool manager::upgrader()
{
    // TODO: add command path/name to the configuration file?
    //
    snap::process p("upgrader");
    p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
    p.set_command("snapupgrader");
    if(f_opt->is_defined("config"))
    {
        p.add_argument("--config");
        p.add_argument( QString::fromUtf8( f_opt->get_string("config").c_str() ) );
    }
    p.add_argument("--data-path");
    p.add_argument(f_data_path);
    if(f_debug)
    {
        p.add_argument("--debug");
    }
    p.add_argument("--log-config");
    p.add_argument(f_log_conf);
    int const r(p.run());
    if(r != 0)
    {
        // TODO: get errors to front end...
        //
        // TODO: move the error handling to the snap::process code instead?
        //
        if(r < 0)
        {
            // could not even start the process
            //
            int const e(errno);
            SNAP_LOG_ERROR
                << "could not properly start snapupgrader (errno: "
                << e
                << ", "
                << strerror(e)
                << ")."
                << SNAP_LOG_SEND;
        }
        else
        {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            // process started but returned with an error
            //
            if(WIFEXITED(r))
            {
                int const exit_code(WEXITSTATUS(r));
                SNAP_LOG_ERROR
                    << "could not properly start snapupgrader (exit code: "
                    << exit_code
                    << ")."
                    << SNAP_LOG_SEND;
            }
            else if(WIFSIGNALED(r))
            {
                int const signal_code(WTERMSIG(r));
                bool const has_code_dump(!!WCOREDUMP(r));

                SNAP_LOG_ERROR
                    << "snapupgrader terminated because of OS signal \""
                    << strsignal(signal_code)
                    << "\" ("
                    << signal_code
                    << ")"
                    << (has_code_dump ? " and a core dump was generated" : "")
                    << "."
                    << SNAP_LOG_SEND;
            }
            else
            {
                // I do not think we can reach here...
                //
                SNAP_LOG_ERROR
                    << "snapupgrader terminated abnormally in an unknown way."
                    << SNAP_LOG_SEND;
            }
#pragma GCC diagnostic pop

        }
        return false;
    }

#if 0
    // if the parent dies, then we generally receive a SIGHUP which is
    // a big problem if we want to go on with the update... so here I
    // make sure we ignore the HUP signal.
    //
    // (this should really only happen if you have a terminal attached
    // to the process, but just in case...)
    //
    signal(SIGHUP, SIG_IGN);

    // setsid() moves us in the group instead of being viewed as a child
    //
    setsid();

    //pid_t const sub_pid(fork());
    //if(sub_pid != 0)
    //{
    //    // we are the sub-parent, by leaving now the child has
    //    // a parent PID equal to 1 meaning that it cannot be
    //    // killed just because snapmanagerdaemon gets killed
    //    //
    //    exit(0);
    //}

    bool const success(update_packages("update")       == 0
                    && update_packages("upgrade")      == 0
                    && update_packages("dist-upgrade") == 0
                    && update_packages("autoremove")   == 0);

    // we have to do this one here now
    //
    reset_aptcheck();

    exit(success ? 0 : 1);

    snapdev::NOT_REACHED();
#endif

    return true;
}


std::string manager::lock_filename() const
{
    return (f_lock_path + "/upgrading.lock").toUtf8().data();
}


bool manager::installer(QString const & bundle_name
                      , std::string const & command
                      , std::string const & install_values
                      , std::set<QString> & affected_services)
{
    bool success(true);

    // whether we are going to install or purge
    //
    bool const installing(command == "install");

    SNAP_LOG_INFO
        << (installing ? "Installing" : "Removing")
        << " bundle \""
        << bundle_name
        << "\" on host \""
        << f_server_name
        << "\"."
        << SNAP_LOG_SEND;

    // make sure we do not start an installation while an upgrade is
    // still going (and vice versa)
    //
    snapdev::lockfile lf(lock_filename(), snapdev::lockfile::mode_t::LOCKFILE_EXCLUSIVE);
    if(!lf.try_lock())
    {
        return false;
    }

    // for installation we first do an update of the packages,
    // otherwise it could fail the installation because of
    // outdated data
    //
    if(installing)
    {
        // we cannot "just upgrade" now because the upgrader() function
        // calls fork() and this the call would return early. Instead
        // we check the number of packages that are left to upgrade
        // and if not zero, emit an error and return...
        //success = upgrader();

        QString const count_packages(count_packages_that_can_be_updated(false));
        if(!count_packages.isEmpty())
        {
            // TODO: how do we tell the end user about that one?
            //
            SNAP_LOG_ERROR
                << "Installation of bundle \""
                << bundle_name
                << "\" on host \""
                << f_server_name
                << "\" did not proceed because some packages first need to be upgraded."
                << SNAP_LOG_SEND;
            return false;
        }
    }

    // load the XML file
    //
    QDomDocument bundle_xml;
    QString const filename(QString("%1/bundle-%2.xml").arg(f_bundles_path).arg(bundle_name));
    QFile input(filename);
    if(!input.open(QIODevice::ReadOnly)
    || !bundle_xml.setContent(&input, false))
    {
        SNAP_LOG_ERROR
            << "bundle \""
            << filename
            << "\" could not be opened or has invalid XML data. Skipping."
            << SNAP_LOG_SEND;
        return false;
    }

    // install_values is a string of variables that come from the list
    // of fields defined in the bundle file
    //
    std::string vars;
    if(installing)
    {
        // only installations offer variables at the moment
        //
        std::vector<std::string> variables;
        snapdev::NOT_USED(snapdev::tokenize_string(variables, install_values, "\r\n", true, " "));
        std::for_each(variables.begin(), variables.end(),
                    [&vars](auto const & v)
                    {
                        // TODO: move to a function, this is just too long for a lambda
                        //
                        vars += "BUNDLE_INSTALLATION_";
                        bool found_equal(false);
                        // make sure that double quotes get escaped within
                        // the string
                        //
                        for(auto const c : v)
                        //for(size_t p(0); p < v.length(); ++p)
                        {
                            if(c == '\r'
                            || c == '\n')
                            {
                                // these characters should not happen in those
                                // strings, but just in case...
                                //
                                continue;
                            }

                            if(found_equal)
                            {
                                if(c == '"' || c == '\\')
                                {
                                    vars += '\\';
                                }
                                vars += c;
                            }
                            else if(c == '=')
                            {
                                found_equal = true;
                                vars += "=\"";
                            }
                            else
                            {
                                if(c >= 'a' && c <= 'z')
                                {
                                    // force ASCII uppercase for the name
                                    //
                                    vars += c & 0x5f;
                                }
                                else
                                {
                                    vars += c;
                                }
                            }
                        }
                        if(!found_equal)
                        {
                            // empty case
                            //
                            vars += "=\"\"\n";
                        }
                        else
                        {
                            // always add a new line at the end
                            // (one variable per line)
                            //
                            vars += "\"\n";
                        }
                    });
    }

    // list of affected services (those that need a RELOADCONFIG after
    // this installation)
    //
    QDomNodeList affected_services_tags(bundle_xml.elementsByTagName("affected-services"));
    if(affected_services_tags.size() == 1)
    {
        QDomElement affected_services_element(affected_services_tags.at(0).toElement());
        std::vector<std::string> variables;
        if(snapdev::tokenize_string(variables, affected_services_element.text().toUtf8().data(), ",", true, " ") > 0)
        {
            // variables are 'std::string' and affected_services are 'QString' for now...
            //std::copy(variables.cbegin(), variables.cend(), std::inserter(affected_services, affected_services.begin()));

            std::for_each(
                    variables.cbegin(),
                    variables.cend(),
                    [&affected_services](auto const & str)
                    {
                        affected_services.insert(QString::fromUtf8(str.c_str()));
                    });
        }
    }

    // there may be some pre-installation instructions
    //
    QString const prename(installing ? "preinst" : "prerm");
    QDomNodeList bundle_precmd(bundle_xml.elementsByTagName(prename));
    if(bundle_precmd.size() == 1)
    {
        // create a <name>.precmd script that we can run
        //
        std::string path(f_cache_path.toUtf8().data());
        path += "/bundle-scripts/"; 
        path += bundle_name.toUtf8().data();
        path += ".";
        path += prename.toUtf8().data();
        snap::file_content script(path, true);
        QDomElement precmd(bundle_precmd.at(0).toElement());
        script.set_content("#!/bin/bash\n# auto-generated by snapmanagerdaemon\n" + vars + precmd.text().toUtf8().data());
        script.write_all();
        chmod(path.c_str(), 0755);
        snap::process p(prename);
        p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
        p.set_command(QString::fromUtf8(path.c_str()));
        int const r(p.run());
        if(r != 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR
                << "pre-installation script failed with "
                << r
                << " (errno: "
                << e
                << ", "
                << strerror(e)
                << SNAP_LOG_SEND;
            // if the pre-installation script fails, we do not attempt to
            // install the packages
            //
            return false;
        }
    }

    // get the list of expected packages, it may be empty/non-existant
    //
    QDomNodeList bundle_packages(bundle_xml.elementsByTagName("packages"));
    if(bundle_packages.size() == 1)
    {
        QDomElement package_list(bundle_packages.at(0).toElement());
        std::string const list_of_packages(package_list.text().toUtf8().data());
        std::vector<std::string> packages;
        snapdev::NOT_USED(snapdev::tokenize_string(packages, list_of_packages, ",", true, " "));
        std::for_each(packages.begin(), packages.end(),
                [=, &success](auto const & p)
                {
                    if(!p.empty())
                    {
                        // we want to call all the install even if a
                        // previous one (or the update) failed
                        //
                        // (using "this->" because we're inside a lambda)
                        //
                        success = this->install_package(p, command) != 0 && success;
                    }
                });

        // purging a package may leave other packages that were auto-installed
        // ready to be removed, we handle those here with the autoremove
        // command
        //
        if(!installing)
        {
            // this is an update command
            //
            success = update_packages("autoremove") == 0 && success;
        }
    }

    // there may be some post installation instructions
    //
    QString const postname(installing ? "postinst" : "postrm");
    QDomNodeList bundle_postcmd(bundle_xml.elementsByTagName(postname));
    if(bundle_postcmd.size() == 1)
    {
        // create a <name>.postinst script that we can run
        //
        std::string path(f_cache_path.toUtf8().data());
        path += "/bundle-scripts/"; 
        path += bundle_name.toUtf8().data();
        path += ".";
        path += postname.toUtf8().data();
        snap::file_content script(path, true);
        QDomElement postinst(bundle_postcmd.at(0).toElement());
        script.set_content("#!/bin/bash\n# auto-generated by snapmanagerdaemon\n" + vars + postinst.text().toUtf8().data());
        script.write_all();
        if(chmod(path.c_str(), 0755) != 0)
        {
            // this should not happen, it could be a security issue
            // (although we do not prevent others from looking at the script)
            //
            int const e(errno);
            SNAP_LOG_WARNING
                << "bundle script file mode could not be changed to 755, (errno: "
                << e
                << ", "
                << strerror(e)
                << ")"
                << SNAP_LOG_SEND;
        }
        snap::process p(postname);
        p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
        p.set_command(QString::fromUtf8(path.c_str()));
        int const r(p.run());
        if(r != 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR
                << "post installation script failed with "
                << r
                << " (errno: "
                << e
                << ", "
                << strerror(e)
                << ")"
                << SNAP_LOG_SEND;
            // not much we can do if the post installation fails
            // (we could remove the packages, but that could be dangerous too)
            success = false;
        }
    }

    // Note: we should receive a DPKGUPDATE message too
    //
    reset_aptcheck();

    return success;
}


/** \brief Reboot or shutdown a computer.
 *
 * This function sends the OS the necessary command(s) to reboot or
 * shutdown a computer system.
 *
 * In some cases, the shutdown is to be done cleanly, meaning that
 * the machine has to unregister itself first, making sure that all
 * others know that the machine is going to go down. Once that
 * disconnect was accomplished, then the shutdown happens.
 *
 * If the function is set to reboot, it will reconnect as expected
 * once it comes back.
 *
 * Also, if multiple machines (all?) are asked to reboot, then it
 * has to be done one after another and not all at once (all at
 * once would kill the cluster!)
 *
 * \param[in] reboot  Whether to reboot (true) or just shutdown (false).
 */
void manager::reboot(bool reboot)
{
    // TODO: we need many different ways to reboot a machine cleanly;
    //       especially front ends and database machines which need
    //       to first be disconnected by all, then rebooted;
    //       also shutdowns have to be coordinated between computers:
    //       one computer cannot decide by itself whether to it can
    //       go down or now...
    //
    //       Note: as mentioned in the TODO below, the use of a batch
    //       that goes on after a reboot would be wonderful and could
    //       actually be used to "script batch" an entire reboot
    //       process [it would be a bit more complicated if we want
    //       to make it fail safe, but we could have a version 1 which
    //       uses a single computer to manage the whole process...]
    //       the concept is to include all the steps in the batch and
    //       then execute them one by one, waiting for the results
    //       to confirm that we can move on to the next item in the
    //       batch... just thinking about it, it sounds great already!

    // TODO: we could test whether the installer is busy upgrading or
    //       installing something at least (see lockfile() in those
    //       functions.)
    //
    //       Note: once we have a batch system, we can add the boot
    //       at the end of the batch! But then we need a way to
    //       block adding any further items to the batch unless we
    //       want to support the idea/concept that the batch will
    //       continue after the reboot (which would be wonderful
    //       and should not be any more complicated!)

    // on nodes with Cassandra drain Cassandra first
    //
    if(access("/usr/sbin/cassandra", R_OK | X_OK) == 0)
    {
        // get the host IP as defined in "snapdbproxy.conf"
        //
        snap::snap_config dbproxy("snapdbproxy");

        std::string host("127.0.0.1");
        if(dbproxy.has_parameter("cassandra_host_list"))
        {
            host = dbproxy["cassandra_host_list"];
        }

        // run the "cass-stop" command
        //
        snap::process drain("cassandra drain");
        drain.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
        drain.set_command("/usr/bin/cass-stop");
        drain.add_argument(host);
        snapdev::NOT_USED(drain.run());
    }

    // now do the shutdown
    //
    snap::process p("shutdown");
    p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
    p.set_command("shutdown");
    if(reboot)
    {
        p.add_argument("--reboot");
    }
    else
    {
        p.add_argument("--poweroff");
    }
    p.add_argument("now");
    p.add_argument("Shutdown initiated by Snap! Manager Daemon");
    snapdev::NOT_USED(p.run());
}


/** \brief Replace one configuration value in a configuration file.
 *
 * This function replaces the value of the specified field \p field_name
 * with the new specified value \p new_value.
 *
 * The configuration file being tweaked is named \p filename. This is
 * expected to be the full path to the file.
 *
 * The \p flags can be used to tweak how the value is defined, especially,
 * whether it uses a colon or an equal sign, whether the value should be
 * quoted, etc.
 *
 * The \p trim_left strings allows you to ignore any number of characters
 * on the left side of the parameter name you are looking for. If you use
 * the \p trim_left parameter, you probably also want to use the
 * snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST flag otherwise
 * it will possible create the parameter with an invalid set of such
 * characters. Note that the "trimmed" characters are trimmed to compare
 * the \p field_name parameter with the field name found in the file. It
 * otherwise remains in the output file.
 *
 * The available flags are:
 *
 * \li snap_manager::REPLACE_CONFIGURATION_VALUE_NO_FLAGS
 *
 * No flags specified; this can be used as a default if none of the
 * other flag values are required.
 *
 * \li snap_manager::REPLACE_CONFIGURATION_VALUE_CREATE_BACKUP
 *
 * Create a backup of filename (with .bak appended) if the file
 * already exists.
 *
 * \li snap_manager::REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE
 *
 * Save the value in double quotes. The \p new_value parameter is expected
 * to not already include double quotes.
 *
 * \li snap_manager::REPLACE_CONFIGURATION_VALUE_SINGLE_QUOTE
 *
 * Save the value in single quotes. The \p new_value parameter is expected
 * to not already include single quotes.
 *
 * \li snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
 *
 * Replace the value if it exists. If the value is not found in the
 * existing file, it does not get added to the file (which is the
 * default when this flag is not specified.)
 *
 * \li snap_manager::REPLACE_CONFIGURATION_VALUE_COLON
 *
 * The field name and the value are separated by a colon instead of
 * an equal if you use that flag. Note that this function will not
 * replace an existing equal with a colon, or an existing colon with
 * an equal sign. It has to be the correct version. If you may have
 * either, you probably want to use
 * snap_manager::REPLACE_CONFIGURATION_VALUE_FILE_MUST_EXIST alongside.
 *
 * \li snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
 *
 * Make sure there is a space after the equal sign (or colon if you
 * specified snap_manager::REPLACE_CONFIGURATION_VALUE_COLON). The
 * space does not need to already be there, it will be added if
 * it were missing before.
 *
 * \li snap_manager::REPLACE_CONFIGURATION_VALUE_HASH_COMMENT
 *
 * The variable may be commented out. If the input starts with a
 * hash character, then it gets removed along with spaces up to
 * the non-hash or space character. If that represents your variable
 * then the hashes and spaces get removed and the new value saved.
 *
 * If you need to support other characters for comments (i.e. the
 * semicolon is used in .ini files, for example) then you may be
 * able to use the \p trim_left parameter with the
 * snap_manager::REPLACE_CONFIGURATION_VALUE_TRIM_RESULT flag.
 *
 * \li snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION
 *
 * This file format uses .ini like sections: names defined between
 * square brackets that define what looks like a namespace. To search
 * and replace in a specific section, make sure to include the section
 * name in the \p field_name parameter. For example:
 *
 * \code
 * # For a file with:
 * [Unit]
 * After=network
 * ...
 *
 * // Use field_name such as:
 * replace_configuration_value(..., "Unit::After", ...);
 * \endcode
 *
 * \li snap_manager::REPLACE_CONFIGURATION_VALUE_FILE_MUST_EXIST
 *
 * This flag is used to make sure we do not create a new configuration
 * file if none exist. But default a new .conf file gets created if
 * none exist at the time this function gets called.
 *
 * \li snap_manager::REPLACE_CONFIGURATION_VALUE_TRIM_RESULT
 *
 * When the \p trim_left parameter matches various characters on the left
 * side of the field name, those characters do not get removed from the
 * output. This flag can be used to have those characters removed, which
 * is similar to trimming comment introducer characters such as ';'.
 *
 * \param[in] filename  The name of the configuration file being edited.
 * \param[in] field_name  The name of the field being updated.
 * \param[in] new_value  The new value for that field.
 * \param[in] flags  A set of flags to tweak the behavior of the function.
 * \param[in] trim_left  Whether we ignore some characters to the left of
 *                       the field name (in the file.)
 *
 * \return true if the function encountered no problem. If the "must exist"
 *         flag was used, the function may return true when the field was
 *         not updated nor appended.
 */
bool manager::replace_configuration_value(
                  QString const & filename
                , QString const & field_name
                , QString const & new_value
                , replace_configuration_value_t const flags
                , QString const & trim_left)
{
    QString const equal((flags & REPLACE_CONFIGURATION_VALUE_COLON) != 0 ? ":" : "=");
    QString const quote((flags & REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE) == 0
                            ? ((flags & REPLACE_CONFIGURATION_VALUE_SINGLE_QUOTE) == 0
                                    ? ""
                                    : "'")
                            : "\"");
    QString const space_after((flags & REPLACE_CONFIGURATION_VALUE_SPACE_AFTER) != 0 ? " " : "");

    QString section;
    QString name(field_name);
    if((flags & REPLACE_CONFIGURATION_VALUE_SECTION) != 0)
    {
        // if we are required to have a section, break it up in two names
        int const pos(field_name.indexOf(':'));
        if(pos <= 0
        || pos + 1 >= field_name.length()
        || field_name[pos + 1] != ':')
        {
            throw snapmanager_exception_invalid_parameters("the REPLACE_CONFIGURATION_VALUE_SECTION cannot be used with a field that does not include the section name (<section>::<field-name>).");
        }
        section = "[" + field_name.mid(0, pos) + "]";
        name = field_name.mid(pos + 2);
        if(name.isEmpty())
        {
            throw snapmanager_exception_invalid_parameters("the name part cannot be empty when a section is specified");
        }
    }

    // WARNING: using concatenation (+) because "%1%2%3..." can cause
    //          problems if one of the values include a % followed by
    //          a number.
    //
    QString const line(name + equal + space_after + quote + new_value + quote + "\n");
    QByteArray const utf8_line(line.toUtf8());

    // add a new line character at the end of that line
    //
    QString const section_line(section + "\n");
    QByteArray const utf8_section_line(section_line.toUtf8());

    // get a UTF-8 version of trim_left although really we only check bytes
    // not UTF-8 characters...
    //
    QByteArray const utf8_trim_left(trim_left.toUtf8());

    // make sure the parent folders all exist
    // (this is important for /etc/systemd/systemd/<name> folders which by
    // default do no exist.)
    //
    if(snap::mkdir_p(filename, true) != 0)
    {
        return false;
    }

    QByteArray const field_name_utf8((name + equal).toUtf8());

    // make sure to create the file if it does not exist
    // we expect the filename parameter to be something like
    //     /etc/snapwebsites/snapwebsites.d/<filename>
    //
    int fd(open(filename.toUtf8().data(), O_RDWR));
    if(fd == -1)
    {
        if((flags & (REPLACE_CONFIGURATION_VALUE_MUST_EXIST | REPLACE_CONFIGURATION_VALUE_FILE_MUST_EXIST)) != 0)
        {
            SNAP_LOG_WARNING
                << "configuration file \""
                << filename
                << "\" does not exist and we are not allowed to create it."
                << SNAP_LOG_SEND;
            return false;
        }

        fd = open(filename.toUtf8().data(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if(fd == -1)
        {
            SNAP_LOG_INFO
                << "could not create file \""
                << filename
                << "\" to save new configuration value."
                << SNAP_LOG_SEND;
            return false;
        }
        snapdev::raii_fd_t safe_fd(fd);

        std::string const comment("# This file was auto-generated by snapmanager.cgi\n"
                                  "# Feel free to do additional modifications here as\n"
                                  "# snapmanager.cgi will be aware of them as expected.\n");
        if(::write(fd, comment.c_str(), comment.size()) != static_cast<ssize_t>(comment.size()))
        {
            int const e(errno);
            SNAP_LOG_ERROR
                << "write of header to \""
                << filename
                << "\" failed (errno: "
                << e
                << ", "
                << strerror(e)
                << ")"
                << SNAP_LOG_SEND;
            return false;
        }
        if(!section.isEmpty())
        {
            if(::write(fd, utf8_section_line.data(), utf8_section_line.size()) != utf8_section_line.size())
            {
                int const e(errno);
                SNAP_LOG_ERROR
                    << "writing of new parameter to \""
                    << filename
                    << "\" failed (errno: "
                    << e
                    << ", "
                    << strerror(e)
                    << ")"
                    << SNAP_LOG_SEND;
                return false;
            }
        }
        // the timer has to be reset so we have to write an empty version
        // version first
        //
        if((flags & REPLACE_CONFIGURATION_VALUE_RESET_TIMER) != 0)
        {
            if(::write(fd, field_name_utf8.data(), field_name_utf8.size()) != field_name_utf8.size()
            || ::write(fd, "\n", 1) != 1)
            {
                int const e(errno);
                SNAP_LOG_ERROR
                    << "writing of new parameter to \""
                    << filename
                    << "\" failed (errno: "
                    << e
                    << ", "
                    << strerror(e)
                    << ")"
                    << SNAP_LOG_SEND;
                return false;
            }
        }
        if(::write(fd, utf8_line.data(), utf8_line.size()) != utf8_line.size())
        {
            int const e(errno);
            SNAP_LOG_ERROR
                << "writing of new parameter to \""
                << filename
                << "\" failed (errno: "
                << e
                << ", "
                << strerror(e)
                << ")"
                << SNAP_LOG_SEND;
            return false;
        }
    }
    else
    {
        snapdev::raii_fd_t safe_fd(fd);

        // go to the end, gives us the size as a side effect
        //
        off_t const size(lseek(fd, 0, SEEK_END));
        if(size < 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR
                << "could not get size of \""
                << filename
                << "\", errno: "
                << e
                << ", "
                << strerror(e)
                << ")"
                << SNAP_LOG_SEND;
            return false;
        }

        // rewind so we can read the file
        //
        off_t const back_pos(lseek(fd, 0, SEEK_SET));
        if(back_pos < 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR
                << "could not come back to the start of \""
                << filename
                << "\", errno: "
                << e
                << ", "
                << strerror(e)
                << ")"
                << SNAP_LOG_SEND;
            return false;
        }

        std::unique_ptr<char> buf(new char[size]);
        if(read(fd, buf.get(), size) != size)
        {
            int const e(errno);
            SNAP_LOG_ERROR
                << "writing of new line to \""
                << filename
                << "\" failed -- could not read input file (errno: "
                << e
                << ", "
                << strerror(e)
                << ")"
                << SNAP_LOG_SEND;
            return false;
        }

        // TBD: Offer administrators a way to define the backup extension?
        if((flags & REPLACE_CONFIGURATION_VALUE_CREATE_BACKUP) != 0)
        {
            snapdev::raii_fd_t bak(open((filename + ".bak").toUtf8().data(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
            if(!bak)
            {
                SNAP_LOG_INFO
                    << "could not create backup file \""
                    << filename
                    << ".bak\" to save new configuration value."
                    << SNAP_LOG_SEND;
                return false;
            }
            if(::write(bak.get(), buf.get(), size) != size)
            {
                SNAP_LOG_INFO
                    << "could not save buffer to backup file \""
                    << filename
                    << ".bak\" before generating a new version."
                    << SNAP_LOG_SEND;
                return false;
            }
        }

        // TODO: we do not need to truncate the whole file, only from
        //       the field if it is found, and even only if the new
        //       value is not the exact same size
        //
        snapdev::NOT_USED(lseek(fd, 0, SEEK_SET));
        snapdev::NOT_USED(::ftruncate(fd, 0));

        QByteArray const section_utf8(section.toUtf8());
        bool in_section(section.isEmpty());
        bool found_section(false);

        bool found_field(false);

        int count_timer(0);

        char const * s(buf.get());
        char const * end(s + size);
        for(char const * start(s); s < end; ++s)
        {
            if(*s == '\n')
            {
                // length without the '\n'
                //
                size_t len(s - start);

                // at this time, sections cannot be uncommented by this function
                //
                char const * trimmed_start(start);
                if(!section_utf8.isEmpty())
                {
                    if(len >= static_cast<size_t>(section_utf8.size())
                    && strncmp(start, section_utf8.data(), section_utf8.size()) == 0)
                    {
                        in_section = true;
                        found_section = true;
                    }
                    else if(in_section && len > 0 && start[0] == '[')
                    {
                        // we found another section
                        //
                        in_section = false;

                        // if the field was not found in the last section, we have to save it
                        //
                        if(!found_field)
                        {
                            // we "pretend" it was found
                            //
                            found_field = true;

                            if((flags &  REPLACE_CONFIGURATION_VALUE_MUST_EXIST) != 0)
                            {
                                SNAP_LOG_ERROR
                                    << "configuration file \""
                                    << filename
                                    << "\" does not have a \""
                                    << field_name
                                    << "\" field and we are not allowed to append it."
                                    << SNAP_LOG_SEND;
                                return false;
                            }

                            if((flags & REPLACE_CONFIGURATION_VALUE_RESET_TIMER) != 0
                            && count_timer == 0)
                            {
                                // if count_timer != 0 then we already found an empty entry
                                // so we do not add a second one here
                                //
                                if(::write(fd, field_name_utf8.data(), field_name_utf8.size()) != field_name_utf8.size()
                                || ::write(fd, "\n", 1) != 1)
                                {
                                    int const e(errno);
                                    SNAP_LOG_ERROR
                                        << "writing of new parameter to \""
                                        << filename
                                        << "\" failed (errno: "
                                        << e
                                        << ", "
                                        << strerror(e)
                                        << ")"
                                        << SNAP_LOG_SEND;
                                    return false;
                                }
                            }
                            if(::write(fd, utf8_line.data(), utf8_line.size()) != utf8_line.size())
                            {
                                int const e(errno);
                                SNAP_LOG_ERROR
                                    << "writing of new parameter at the end of \""
                                    << filename
                                    << "\" failed (errno: "
                                    << e
                                    << ", "
                                    << strerror(e)
                                    << ")"
                                    << SNAP_LOG_SEND;
                                return false;
                            }
                        }
                    }
                }
                else
                {
                    if((flags & REPLACE_CONFIGURATION_VALUE_HASH_COMMENT) != 0)
                    {
                        // remove the introducing '#' and space characters if any
                        //
                        if(*trimmed_start == '#')
                        {
                            ++trimmed_start;
                            while(isspace(*trimmed_start) || *trimmed_start == '#')
                            {
                                ++trimmed_start;
                            }
                            len = s - trimmed_start;
                        }
                    }

                    if(!trim_left.isEmpty())
                    {
                        for(; trimmed_start < s && strchr(utf8_trim_left, *trimmed_start) != nullptr; ++trimmed_start);
                        len = s - trimmed_start;
                    }
                }
                if(in_section
                && len >= static_cast<size_t>(field_name_utf8.size())
                && strncmp(trimmed_start, field_name_utf8.data(), field_name_utf8.size()) == 0)
                {
                    // do not replace the first empty line if it exists
                    //
                    if((flags & REPLACE_CONFIGURATION_VALUE_RESET_TIMER) != 0
                    && len == static_cast<size_t>(field_name_utf8.size())
                    && count_timer == 0)
                    {
                        count_timer = 1;

                        // keep that line (same code as else block below...)
                        //
                        len = s - start + 1;
                        if(::write(fd, start, len) != static_cast<ssize_t>(len))
                        {
                            int const e(errno);
                            SNAP_LOG_ERROR
                                << "writing back of line to \""
                                << filename
                                << "\" failed (errno: "
                                << e
                                << ", "
                                << strerror(e)
                                << ")"
                                << SNAP_LOG_SEND;
                            return false;
                        }
                    }
                    else
                    {
                        // we found the field the user is asking to update
                        //
                        found_field = true;
                        if(trimmed_start > start
                        && (flags & REPLACE_CONFIGURATION_VALUE_TRIM_RESULT) == 0)
                        {
                            ssize_t const trimmed_len(trimmed_start - start);
                            if(::write(fd, start, trimmed_len) != trimmed_len)
                            {
                                int const e(errno);
                                SNAP_LOG_ERROR
                                    << "writing of new line to \""
                                    << filename
                                    << "\" failed (errno: "
                                    << e
                                    << ", "
                                    << strerror(e)
                                    << ")"
                                    << SNAP_LOG_SEND;
                                return false;
                            }
                        }
                        if(::write(fd, utf8_line.data(), utf8_line.size()) != utf8_line.size())
                        {
                            int const e(errno);
                            SNAP_LOG_ERROR
                                << "writing of new line to \""
                                << filename
                                << "\" failed (errno: "
                                << e
                                << ", "
                                << strerror(e)
                                << ")"
                                << SNAP_LOG_SEND;
                            return false;
                        }
                    }
                }
                else
                {
                    // recalculate length and include the '\n'
                    // (we MUST recalculate in case the string was left trimmed
                    //
                    len = s - start + 1;
                    if(::write(fd, start, len) != static_cast<ssize_t>(len))
                    {
                        int const e(errno);
                        SNAP_LOG_ERROR
                            << "writing back of line to \""
                            << filename
                            << "\" failed (errno: "
                            << e
                            << ", "
                            << strerror(e)
                            << ")"
                            << SNAP_LOG_SEND;
                        return false;
                    }
                }
                start = s + 1;
            }
        }
        if(!found_field)
        {
            if((flags &  REPLACE_CONFIGURATION_VALUE_MUST_EXIST) != 0)
            {
                SNAP_LOG_ERROR
                    << "configuration file \""
                    << filename
                    << "\" does not have a \""
                    << field_name
                    << "\" field and we are not allowed to append it."
                    << SNAP_LOG_SEND;
                return false;
            }

            // if we reach here with a section then it could be that the
            // section does not yet exist, so create it
            //
            if(!section.isEmpty()
            && !in_section)
            {
                if(::write(fd, utf8_section_line.data(), utf8_section_line.size()) != utf8_section_line.size())
                {
                    int const e(errno);
                    SNAP_LOG_ERROR
                        << "writing of new parameter to \""
                        << filename
                        << "\" failed (errno: "
                        << e
                        << ", "
                        << strerror(e)
                        << ")"
                        << SNAP_LOG_SEND;
                    return false;
                }
            }
            // the timer has to be reset so we have to write an empty version
            // version first
            //
            if((flags & REPLACE_CONFIGURATION_VALUE_RESET_TIMER) != 0)
            {
                if(::write(fd, field_name_utf8.data(), field_name_utf8.size()) != field_name_utf8.size()
                || ::write(fd, "\n", 1) != 1)
                {
                    int const e(errno);
                    SNAP_LOG_ERROR
                        << "writing of new parameter to \""
                        << filename
                        << "\" failed (errno: "
                        << e
                        << ", "
                        << strerror(e)
                        << ")"
                        << SNAP_LOG_SEND;
                    return false;
                }
            }
            if(::write(fd, utf8_line.data(), utf8_line.size()) != utf8_line.size())
            {
                int const e(errno);
                SNAP_LOG_ERROR
                    << "writing of new parameter at the end of \""
                    << filename
                    << "\" failed (errno: "
                    << e
                    << ", "
                    << strerror(e)
                    << ")"
                    << SNAP_LOG_SEND;
                return false;
            }
        }
    }

    // successfully done
    //
    return true;
}


/** \brief Search for a parameter in a string.
 *
 * This function searches for a named parameter in a string representing
 * a text file.
 *
 * The search is very lose. The parameter does not have to start in the
 * first column, the line may be commented, the case can be ignored.
 *
 * \param[in] configuration  The file to be searched.
 * \param[in] parameter_name  The name of the parameter to search.
 * \param[in] start_pos  The starting position of the search.
 * \param[in] ignore_case  Whether to ignore (true) case or not (false.)
 *
 * \return The position of the parameter in the string.
 */
std::string::size_type manager::search_parameter(std::string const & configuration, std::string const & parameter_name, std::string::size_type const start_pos, bool const ignore_case)
{
    if(start_pos >= configuration.length())
    {
        return std::string::size_type(-1);
    }

    // search for a string that matches, we use this search mechanism
    // so we can support case sensitive or insensitive
    //
    auto const it(std::search(
            configuration.begin() + start_pos,
            configuration.end(),
            parameter_name.begin(),
            parameter_name.end(),
            [ignore_case](char c1, char c2)
            {
                return ignore_case ? std::tolower(c1) == std::tolower(c2)
                                   : c1 == c2;
            }));

    // return the position if found and -1 otherwise
    //
    return it == configuration.end() ? std::string::size_type(-1) : it - configuration.begin();
}


/** \brief Check the current status of a service.
 *
 * This function checks whether a service is available (i.e. installed)
 * and if so what it's current status is.
 *
 * \li SERVICE_STATUS_NOT_INSTALLED -- the service is not even installed
 * \li SERVICE_STATUS_DISABLED -- the service is installed, but currently
 * disabled
 * \li SERVICE_STATUS_ENABLED -- the service is enabled, but not active
 * (running) nor did it fail earlier
 * \li SERVICE_STATUS_ACTIVE -- the service is enabled and running right
 * now
 * \li SERVICE_STATUS_FAILED -- the service is enabled, was active, but
 * crashed or exited in such a way that it is viewed as failed
 *
 * The function expects the path and filename of the service. This is used
 * to make sure the service was installed. One can use the systemctl
 * command with list-unit-files to see whether a unit is installed.
 * However, that command cannot be used to determine whether a service
 * is installed or not. All the other commands generate errors, but
 * errors that cannot properly be distinguished from expected errors
 * when probing the systemd environment.
 *
 * The \p service_name parameter is the exact name you use when
 * running systemctl on the command line. It may include the .service
 * extension, although we usually do not include the extension.
 *
 * \param[in] service_filename  The path and filename to the service.
 * \param[in] service_name  The name of the service as defined by the
 *                          .service file.
 *
 * \return One of the SERVICE_STATUS_... values.
 */
service_status_t manager::service_status(std::string const & service_filename, std::string const & service_name)
{
    // does the service binary exists, if not, then it is not currently
    // installed and that's it
    //
    if(access(service_filename.c_str(), R_OK | X_OK) != 0)
    {
        return service_status_t::SERVICE_STATUS_NOT_INSTALLED;
    }

    // when first installed a service may be:
    //
    //      static        (most backend services)
    //      disabled
    //      enabled
    //      active        (nearly all other services)
    //
    // later, a service can be any one of those four statuses; note that
    // we convert "static" in "disabled" since it is pretty much the same
    //
    snap::process p1("query service status");
    p1.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
    p1.set_command("systemctl");
    p1.add_argument("is-enabled");
    p1.add_argument(QString::fromUtf8(service_name.c_str()));
    int const r1(p1.run());
    QString const status(p1.get_output(true).trimmed());
    SNAP_LOG_INFO
        << "\"is-enabled\" query output ("
        << r1
        << "): "
        << status
        << SNAP_LOG_SEND;
    if(r1 != 0)
    {
        // it is not enabled, so it cannot be active, thus it is disabled
        //
        return service_status_t::SERVICE_STATUS_DISABLED;
    }
    if(status == "static")
    {
        // this is a particular case and when "static" it is similar to
        // "disabled" (as in, there is no link that will allow for an
        // auto-start of that daemon)
        //
        // WARNING: the service may be static and active, we do not
        //          handle that case correctly since we say DISABLED
        //          in that case... however, we do not have that kind
        //          of intermediate state (that is, we handle enabled
        //          and disabled well, but no "active but not enabled")
        //
        return service_status_t::SERVICE_STATUS_DISABLED;
    }

    snap::process p2("query service status");
    p2.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
    p2.set_command("systemctl");
    p2.add_argument("is-active");
    p2.add_argument(QString::fromUtf8(service_name.c_str()));
    int const r2(p2.run());
    SNAP_LOG_INFO
        << "\"is-active\" query output ("
        << r2
        << "): "
        << p2.get_output(true).trimmed()
        << SNAP_LOG_SEND;
    if(r2 != 0)
    {
        // it is enabled and not active, it could be failed though
        //
        snap::process p3("query service status");
        p3.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
        p3.set_command("systemctl");
        p3.add_argument("is-failed");
        p3.add_argument(QString::fromUtf8(service_name.c_str()));
        int const r3(p3.run());
        SNAP_LOG_INFO
            << "\"is-failed\" query output ("
            << r3
            << "): "
            << p3.get_output(true).trimmed()
            << SNAP_LOG_SEND;
        if(r3 != 0)
        {
            // it is enabled and not active, thus we return "enabled"
            //
            return service_status_t::SERVICE_STATUS_ENABLED;
        }

        return service_status_t::SERVICE_STATUS_FAILED;
    }

    // the service is enabled and active
    //
    return service_status_t::SERVICE_STATUS_ACTIVE;
}


char const * manager::service_status_to_string(service_status_t const status)
{
    switch(status)
    {
    case service_status_t::SERVICE_STATUS_NOT_INSTALLED:
        return "not_installed";

    case service_status_t::SERVICE_STATUS_DISABLED:
        return "disabled";

    case service_status_t::SERVICE_STATUS_ENABLED:
        return "enabled";

    case service_status_t::SERVICE_STATUS_ACTIVE:
        return "active";

    case service_status_t::SERVICE_STATUS_FAILED:
        return "failed";

    default:
        return "unknown";

    }
}


service_status_t manager::string_to_service_status(std::string const & status)
{
    if(status == "not_installed")
    {
        return service_status_t::SERVICE_STATUS_NOT_INSTALLED;
    }

    // we consider "static" as the same as "disabled"
    // note that we should not call this function with "static", but just
    // in case it happens, we catch it in this way
    //
    if(status == "disabled"
    || status == "static")
    {
        return service_status_t::SERVICE_STATUS_DISABLED;
    }

    if(status == "enabled")
    {
        return service_status_t::SERVICE_STATUS_ENABLED;
    }

    if(status == "active")
    {
        return service_status_t::SERVICE_STATUS_ACTIVE;
    }

    if(status == "failed")
    {
        return service_status_t::SERVICE_STATUS_FAILED;
    }

    return service_status_t::SERVICE_STATUS_UNKNOWN;
}


void manager::service_apply_status(std::string const & service_name, service_status_t const status, std::string const & wanted_by)
{
    auto systemctl = [](QString const & command, QString const & service, QString const & extra = QString())
        {
            // setup process
            //
            snap::process p(command + " service");
            p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
            p.set_command("systemctl");
            p.add_argument(command);
            p.add_argument(service);
            if(!extra.isEmpty())
            {
                p.add_argument(extra);
            }

            // run process
            //
            int const r(p.run());

            // show process stdout
            //
            SNAP_LOG_INFO
                << "\""
                << command
                << "\" function output: "
                << p.get_output(true)
                << SNAP_LOG_SEND;

            // if no success, emit an error
            //
            if(r != 0)
            {
                SNAP_LOG_ERROR
                    << command
                    << " of service \""
                    << service
                    << "\" failed."
                    << (extra.isEmpty() ? "" : " (" + extra + ")")
                    << SNAP_LOG_SEND;
            }
        };

    auto systemctl_enable = [wanted_by, systemctl](QString const & service)
        {
            if(wanted_by.empty())
            {
                systemctl("enable", service);
            }
            else
            {
                std::vector<std::string> wants;
                snapdev::NOT_USED(snapdev::tokenize_string(wants, wanted_by, " ", true, " "));
                for(auto const & w : wants)
                {
                    systemctl("add-wants", QString::fromUtf8(w.c_str()), service);
                }
            }
        };

    QString service(QString::fromUtf8(service_name.c_str()));
    switch(status)
    {
    case service_status_t::SERVICE_STATUS_DISABLED:
        systemctl("stop", service);
        systemctl("disable", service);
        break;

    case service_status_t::SERVICE_STATUS_ENABLED:
        systemctl("stop", service);
        systemctl_enable(service);
        break;

    case service_status_t::SERVICE_STATUS_ACTIVE:
        systemctl_enable(service);
        systemctl("start", service);
        if(service.endsWith(".timer"))
        {
            // the service needs a kick otherwise it never starts until
            // the next reboot
            //
            systemctl("start", service.left(service.length() - 6));
        }
        break;

    default:
        // invalid status request
        SNAP_LOG_ERROR
            << "you cannot apply status \""
            << service_status_to_string(status)
            << "\" to a service."
            << SNAP_LOG_SEND;
        break;

    }
}


} // namespace snap_manager
// vim: ts=4 sw=4 et
