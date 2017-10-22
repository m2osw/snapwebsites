// Snap Websites Server -- check uploaded files for viruses
// Copyright (C) 2014-2017  Made to Order Software Corp.
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

#include "antivirus.h"

#include "../output/output.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/process.h>

#include <QFile>
#include <QDateTime>

#include <sys/stat.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(antivirus, 1, 0)


/** \brief Get a fixed antivirus name.
 *
 * The antivirus plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_ANTIVIRUS_ENABLE:
        return "antivirus::enable";

    case name_t::SNAP_NAME_ANTIVIRUS_SETTINGS_PATH:
        return "admin/settings/antivirus";

    case name_t::SNAP_NAME_ANTIVIRUS_VERSION:
        return "antivirus::version";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_ANTIVIRUS_...");

    }
    NOTREACHED();
}


/** \class antivirus
 * \brief Check uploaded files for virus infections.
 *
 * This plugin runs clamav against uploaded files to verify whether these
 * are viruses or not. If a file is found to be a virus, it is then marked
 * as not secure and download of the file are prevented.
 */


/** \brief Initialize the antivirus plugin.
 *
 * This function is used to initialize the antivirus plugin object.
 */
antivirus::antivirus()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the antivirus plugin.
 *
 * Ensure the antivirus object is clean before it is gone.
 */
antivirus::~antivirus()
{
}


/** \brief Get a pointer to the antivirus plugin.
 *
 * This function returns an instance pointer to the antivirus plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the antivirus plugin.
 */
antivirus * antivirus::instance()
{
    return g_plugin_antivirus_factory.instance();
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString antivirus::icon() const
{
    return "/images/antivirus/antivirus-logo-64x64.png";
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
QString antivirus::description() const
{
    return "The anti-virus plugin is used to verify that a file is not a"
        " virus. When a file that a user uploaded is found to be a virus"
        " this plugin marks that file as unsecure and the file cannot be"
        " downloaded by end users.";
}


/** \brief Return our dependencies
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString antivirus::dependencies() const
{
    return "|content|editor|output|versions|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t antivirus::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 12, 20, 17, 15, 45, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our antivirus references.
 *
 * Send our antivirus to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void antivirus::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);
    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the antivirus.
 *
 * This function terminates the initialization of the antivirus plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void antivirus::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(antivirus, "content", content::content, check_attachment_security, _1, _2, _3);
    SNAP_LISTEN(antivirus, "versions", versions::versions, versions_tools, _1);
}


/** \brief Generate the page main content.
 *
 * This function generates the main content of the page. Other
 * plugins will also have the event called if they subscribed and
 * thus will be given a chance to add their own content to the
 * main page. This part is the one that (in most cases) appears
 * as the main content on the page although the content of some
 * columns may be interleaved with this content.
 *
 * Note that this is NOT the HTML output. It is the \<page> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void antivirus::on_generate_main_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body)
{
    // our settings pages are like any standard pages
    output::output::instance()->on_generate_main_content(ipath, page, body);
}


/** \brief Check whether the specified file is safe.
 *
 * The content plugin generates this signals twice:
 *
 * 1) once when the attachment is first uploaded and we should test quickly
 *    (fast is set to true)
 *
 * 2) a second time when the backend runs, in this case we can check the
 *    security taking as much time as required (fast is set to false)
 *
 * \param[in] file  The file to check.
 * \param[in] secure  Tells the content plugin whether the file is
 *                    considered safe or not.
 * \param[in] fast  Whether we can take our time (false) or not (true) to
 *                  verify the file.
 */
void antivirus::on_check_attachment_security(content::attachment_file const & file, content::permission_flag & secure, bool const fast)
{
    if(fast)
    {
        // TODO: add support to check some extensions / MIME types that we
        //       do not want (for example we could easily forbid .exe files
        //       from being uploaded)
        return;
    }

    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());
    content::path_info_t settings_ipath;
    settings_ipath.set_path(get_name(name_t::SNAP_NAME_ANTIVIRUS_SETTINGS_PATH));
    libdbproxy::row::pointer_t revision_row(revision_table->getRow(settings_ipath.get_revision_key()));
    libdbproxy::value const enable_value(revision_row->getCell(get_name(name_t::SNAP_NAME_ANTIVIRUS_ENABLE))->getValue());
    int8_t const enable(enable_value.nullValue() || enable_value.safeSignedCharValue());
    if(!enable)
    {
        return;
    }
    if(!has_clamscan())
    {
        // TODO: signal the settings screen so the administrator can be in the known
        //
        SNAP_LOG_WARNING("the antivirus is enabled, but clamav is not installed.");
        return;
    }

    // retrieve the version only once, we do not need it reloaded for each
    // file! although it will happen any time a new file is checked...
    //
    // TODO: if the cluster has more than one backend running clamav
    //       we probably should make sure they all run the same version
    //       (although with upgrades run together that should be the case)
    //
    static bool antivirus_version_retrieved(false);
    if(!antivirus_version_retrieved)
    {
        antivirus_version_retrieved = true;

        process v("antivirus::clamscan-version");
        v.set_mode(process::mode_t::PROCESS_MODE_OUTPUT);
        v.set_command("clamscan");
        v.add_argument("--version");
        NOTUSED(v.run()); // result error info already printed by process class
        QString const output(v.get_output(true));
        revision_row->getCell(get_name(name_t::SNAP_NAME_ANTIVIRUS_VERSION))->setValue(output);
    }

    // slow test, here we check whether the file is a virus
    QString data_path(f_snap->get_server_parameter("data_path"));
    if(data_path.isEmpty())
    {
        // a default that works, not that /tmp is not considered secure
        // although this backend should be running on a computer that is
        // not shared between users
        data_path = "/tmp";
    }
    QString log_path(f_snap->get_server_parameter("log_path"));
    if(log_path.isEmpty())
    {
        // a default that works, not that /tmp is not considered secure
        // although this backend should be running on a computer that is
        // not shared between users
        log_path = "/var/log/snapwebsites";
    }

    SNAP_LOG_INFO("check filename \"")(file.get_file().get_filename())("\" for viruses.");

    // make sure the reset the temporary log file
    //
    QString const temporary_log(QString("%1/antivirus.log").arg(data_path));
    QFile in(temporary_log);
    in.remove();

    process p("antivirus::clamscan");
    p.set_mode(process::mode_t::PROCESS_MODE_INOUT);
    p.set_command("clamscan");
    p.add_argument("--tempdir=" + data_path);
    p.add_argument("--quiet");
    p.add_argument("--stdout");
    p.add_argument("--no-summary");
    p.add_argument("--infected");
    p.add_argument("--log=" + temporary_log);
    p.add_argument("-");
    p.set_input(file.get_file().get_data()); // pipe data in
    int const code(p.run());
    QString const output(p.get_output(true));

    if(!output.isEmpty())
    {
        // mark that it is not secure only if r is 1 (i.e. if
        // clamscan said a virus was found, and not on plain
        // messages or errors.)
        //
        if(code == 1)
        {
            secure.not_permitted("anti-virus: " + output);
        }

        // if an error occurred, also convert the logs
        //
        if(in.open(QIODevice::ReadOnly))
        {
            QFile out(QString("%1/antivirus.log").arg(log_path));
            if(out.open(QIODevice::Append))
            {
                // TODO: convert to use our logger?
                QString const timestamp(QDateTime::currentDateTimeUtc().toString("yyyy/MM/dd hh:mm:ss 'antivirus': "));
                QByteArray const timestamp_buf(timestamp.toUtf8());
                char buf[1024];
                for(;;)
                {
                    int const r(in.readLine(buf, sizeof(buf)));
                    if(r <= 0)
                    {
                        break;
                    }
                    if(strcmp(buf, "\n") == 0)
                    {
                        continue;
                    }
                    for(char const * s(buf); *s == '-'; ++s)
                    {
                        if(*s != '\n' && *s != '-')
                        {
                            // write lines that are not just '-'
                            out.write(timestamp_buf.data(), timestamp_buf.size());
                            out.write(buf, r);
                            break;
                        }
                    }
                }
            }
        }
    }
}


/** \brief Show the version of clamscan.
 *
 * The antivirus currently makes use of clamscan. This signal
 * adds the version of that tool to the specified token.
 *
 * \param[in] token  The token where the version is added.
 */
void antivirus::on_versions_tools(filter::filter::token_info_t & token)
{
    QString output;

    if(has_clamscan())
    {
        // if clamav is installed on this computer, dynamically check
        // the version immediately
        //
        process p("antivirus::clamscan-version");
        p.set_mode(process::mode_t::PROCESS_MODE_OUTPUT);
        p.set_command("clamscan");
        p.add_argument("--version");
        p.run();
        output = p.get_output(true);
    }
    else
    {
        // if not on this computer, check whether we have the info in
        // the database
        //
        content::content * content_plugin(content::content::instance());
        libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());
        content::path_info_t settings_ipath;
        settings_ipath.set_path(get_name(name_t::SNAP_NAME_ANTIVIRUS_SETTINGS_PATH));
        libdbproxy::row::pointer_t revision_row(revision_table->getRow(settings_ipath.get_revision_key()));
        libdbproxy::value const clamav_version(revision_row->getCell(get_name(name_t::SNAP_NAME_ANTIVIRUS_VERSION))->getValue());
        if(!clamav_version.nullValue())
        {
            output = clamav_version.stringValue();
        }
        else
        {
            // We did not yet get information about the clamav version,
            // post an explanation why we do not have it available...
            //
            output = "No version information for clamav available."
                    " In most cases that package only gets installed on the computer running the CRON backend."
                    " That computer is expected to transmit the information, but it looks like we did not yet receive such.";
        }
    }

    token.f_replacement += "<li>";
    token.f_replacement += output;
    token.f_replacement += "</li>";
}


/** \brief Check whether clamscan is available.
 *
 * The antivirus may not be installed as there is no direct dependency
 * on it in the package. This ensures that it is indeed available.
 *
 * \return true if the clamscan binary is available.
 */
bool antivirus::has_clamscan()
{
    struct stat st;
    return stat("/usr/bin/clamscan", &st) == 0;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
