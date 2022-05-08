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
#include    "detectadblocker.h"


// other plugins
//
#include    "../output/output.h"
#include    "../users/users.h"


// snapwebsites
//
#include    <snapwebsites/xslt.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_reached.h>
#include    <snapdev/not_used.h>


// last include
//
#include    <snapdev/poison.h>



namespace snap
{
namespace detectadblocker
{


SERVERPLUGINS_START(detectadblocker, 1, 0)
    , ::serverplugins::description(
            "The detect ad blocker plugin is used to set a variable to"
            " know whether an ad blocker is active on the client browser."
            " If so, plugins attempting to show ads can instead do nothing.")
    , ::serverplugins::icon("/images/detectadblocker/detectadblocker-logo-64x64.png")
    , ::serverplugins::settings_path("/admin/settings/detectadblocker")
    , ::serverplugins::dependency("editor")
    , ::serverplugins::dependency("messages")
    , ::serverplugins::dependency("output")
    , ::serverplugins::dependency("path")
    , ::serverplugins::dependency("permissions")
    , ::serverplugins::dependency("users")
    , ::serverplugins::help_uri("https://snapwebsites.org/help")
    , ::serverplugins::categorization_tag("advertising")
SERVERPLUGINS_END(detectadblocker)


/** \class detectadblocker
 * \brief Offer a way to detect whether an blocker is active.
 *
 * This class adds a variable named adblocker_detected which is set to
 * true by default and then it attempts to load a scripts with a name
 * which Add Ons such as Adblock Plus will prevent loading of. If the
 * load succeeds, then the variables gets set to false.
 */


/** \brief Get a fixed detectadblocker name.
 *
 * The detectadblocker plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * Note that since this plugin is used to edit core and content data
 * more of the names come from those places.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_DETECTADBLOCKER_INFORM_SERVER:
        return "detectadblocker::inform_server";

    case name_t::SNAP_NAME_DETECTADBLOCKER_PATH:
        return "detectadblocker";

    case name_t::SNAP_NAME_DETECTADBLOCKER_PREVENT_ADS_DURATION:
        return "detectadblocker::prevent_ads_duration";

    case name_t::SNAP_NAME_DETECTADBLOCKER_SETTINGS_PATH:
        return "admin/settings/detectadblocker";

    case name_t::SNAP_NAME_DETECTADBLOCKER_STATUS_SESSION_NAME:
        return "detectadblocker_status";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_DETECTADBLOCKER_...");

    }
    snapdev::NOT_REACHED();
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
time_t detectadblocker::do_update(time_t last_updated, unsigned int phase)
{
    SERVERPLUGINS_PLUGIN_UPDATE_INIT();

    if(phase == 0)
    {
        SERVERPLUGINS_PLUGIN_UPDATE(2016, 6, 4, 0, 53, 15, content_update);
    }

    SERVERPLUGINS_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our detectadblocker references.
 *
 * Send our detectadblocker to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables
 *            added to the database by this update (in micro-seconds).
 */
void detectadblocker::content_update(int64_t variables_timestamp)
{
    snapdev::NOT_USED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the detectadblocker.
 *
 * This function terminates the initialization of the detectadblocker plugin
 * by registering for different events.
 */
void detectadblocker::bootstrap()
{
    SERVERPLUGINS_LISTEN0(detectadblocker, "server", server, detach_from_session);
    //SERVERPLUGINS_LISTEN(detectadblocker, "path", path::path, can_handle_dynamic_path, boost::placeholders::_1, boost::placeholders::_2);
    SERVERPLUGINS_LISTEN(detectadblocker, "layout", layout::layout, generate_header_content, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3);
}


void detectadblocker::on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    snapdev::NOT_USED(ipath, metadata);

    if(!f_detected)
    {
        content::content * content_plugin(content::content::instance());
        libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());

        content::path_info_t settings_ipath;
        settings_ipath.set_path(get_name(name_t::SNAP_NAME_DETECTADBLOCKER_SETTINGS_PATH));
        libdbproxy::row::pointer_t settings_row(revision_table->getRow(settings_ipath.get_revision_key()));
        int8_t const inform_server(settings_row->getCell(get_name(name_t::SNAP_NAME_DETECTADBLOCKER_INFORM_SERVER))->getValue().safeSignedCharValue(0, 1)); // AJAX On/Off

        QDomDocument doc(header.ownerDocument());

        QString const code(QString(
            "/* detectadblocker plugin */"
            "detectadblocker__inform_server=%1;")
                    .arg(inform_server));
        content_plugin->add_inline_javascript(doc, code);

        // Note: adframe.js depends on detectadblocker.js so we do not
        //       have to add both here, just adframe and both get added
        //       automatically (or we have a bug)
        //
        content_plugin->add_javascript(header.ownerDocument(), "adframe");
    }
}


void detectadblocker::on_detach_from_session()
{
    // Check user status in regard to ad blocker, if an ad blocker
    // was detected recently, the 'detected' flag will be set to true
    // so we can avoid adding ad plugins altogether (i.e. it will not
    // be necessary!)
    //
    // TODO: look into a way to avoid the detach on pages that do not
    //       require it (i.e. attachments)
    //
    users::users * users_plugin(users::users::instance());
    QString const status(users_plugin->get_from_session(get_name(name_t::SNAP_NAME_DETECTADBLOCKER_STATUS_SESSION_NAME)));
    f_detected = false;
    if(!status.isEmpty())
    {
        snap_string_list const time_status(status.split(','));
        if(time_status.size() == 2)
        {
            content::content * content_plugin(content::content::instance());
            libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());

            content::path_info_t settings_ipath;
            settings_ipath.set_path(get_name(name_t::SNAP_NAME_DETECTADBLOCKER_SETTINGS_PATH));
            libdbproxy::row::pointer_t settings_row(revision_table->getRow(settings_ipath.get_revision_key()));
            int64_t const prevent_ads_duration(settings_row->getCell(get_name(name_t::SNAP_NAME_DETECTADBLOCKER_PREVENT_ADS_DURATION))->getValue().safeInt64Value(0, 1)); // AJAX On/Off

            time_t const timeout(time_status[0].toLongLong());
            time_t const start_time(f_snap->get_start_time());
            bool const timed_out(timeout + 86400 * prevent_ads_duration < start_time);
            if(timed_out)
            {
                // delete only once it timed out; this has the side
                // effect of re-testing the client side on a future
                // hit
                //
                users_plugin->detach_from_session(get_name(name_t::SNAP_NAME_DETECTADBLOCKER_STATUS_SESSION_NAME));
            }
            f_detected = !timed_out && time_status[1] == "true";
        }
    }
}


/** \brief This function is used to capture the AJAX requests.
 *
 * This function receives the AJAX sent to the /detectadblocker page.
 *
 * \param[in] ipath  The canonicalized path being managed.
 *
 * \return true if the content is properly generated, false otherwise.
 */
bool detectadblocker::on_path_execute(content::path_info_t & ipath)
{
    QString const cpath(ipath.get_cpath());
    bool detected(false);
    if(cpath == QString("%1/true").arg(get_name(name_t::SNAP_NAME_DETECTADBLOCKER_PATH)))
    {
        detected = true;
        f_detected = true;
    }
    else if(cpath == QString("%1/false").arg(get_name(name_t::SNAP_NAME_DETECTADBLOCKER_PATH)))
    {
        detected = true;
        f_detected = false;
    }
    if(detected)
    {
        users::users * users_plugin(users::users::instance());
        time_t const start_time(f_snap->get_start_time());
        users_plugin->attach_to_session(get_name(name_t::SNAP_NAME_DETECTADBLOCKER_STATUS_SESSION_NAME),
                                        QString("%1,%2").arg(start_time).arg(f_detected ? "true" : "false"));

        // TODO: add two counters to know how many accesses we get with
        //       ad blockers and how many without ad blockers

        server_access::server_access * server_access_plugin(server_access::server_access::instance());
        server_access_plugin->create_ajax_result(ipath, true);

        // create AJAX response
        //
        server_access_plugin->ajax_output();
        return true;
    }

    // let the output plugin take care of other pages owned by us
    // (there should be no others...)
    //
    return output::output::instance()->on_path_execute(ipath);
}


/** \brief Call before adding ads to the website.
 *
 * If you are developing a plugin that displays ads which make use
 * of a technique that will get blocked by ad blocker add ons of
 * browsers, you want to depend on the detectadblocker plugin
 * and you want to check whether an add blocker was detected. If
 * so, do not send any ads to the client.
 *
 * Note that this must also be coordinated with the DetectAdBlocker
 * JavaScript object. That is, the first time a client accesses one
 * of our pages, we do not yet know whether an ad can be displayed
 * or not. If not, then the DetectAdBlocker will set the 'present'
 * parameter to true meaning that an ad blocker is running and will
 * prevent ads.
 *
 * \return true if an ad blocker was detected on an earlier access.
 */
bool detectadblocker::was_detected() const
{
    return f_detected;
}



} // namespace detectadblocker
} // namespace snap
// vim: ts=4 sw=4 et
