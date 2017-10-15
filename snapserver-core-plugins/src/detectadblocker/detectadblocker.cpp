// Snap Websites Server -- detectadblocker plugin to know whether we should skip on showing ads
// Copyright (C) 2013-2017  Made to Order Software Corp.
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

#include "detectadblocker.h"

#include "../output/output.h"
#include "../users/users.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/xslt.h>

#include <snapwebsites/poison.h>

SNAP_PLUGIN_START(detectadblocker, 1, 0)


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
    NOTREACHED();
}


/** \brief Initialize the detectadblocker plugin.
 *
 * This function is used to initialize the detectadblocker plugin object.
 */
detectadblocker::detectadblocker()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the detectadblocker plugin.
 *
 * Ensure the detectadblocker object is clean before it is gone.
 */
detectadblocker::~detectadblocker()
{
}


/** \brief Get a pointer to the detectadblocker plugin.
 *
 * This function returns an instance pointer to the detectadblocker plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the detectadblocker plugin.
 */
detectadblocker * detectadblocker::instance()
{
    return g_plugin_detectadblocker_factory.instance();
}


/** \brief Send users to the detectadblocker settings.
 *
 * This path represents the detectadblocker settings.
 */
QString detectadblocker::settings_path() const
{
    return "/admin/settings/detectadblocker";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString detectadblocker::icon() const
{
    return "/images/detectadblocker/detectadblocker-logo-64x64.png";
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
QString detectadblocker::description() const
{
    return "The detect ad blocker plugin is used to set a variable to"
          " know whether an ad blocker is active on the client browser."
          " If so, plugins attempting to show ads can instead do nothing.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString detectadblocker::dependencies() const
{
    return "|editor|messages|output|path|permissions|users|";
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
int64_t detectadblocker::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 6, 4, 0, 53, 15, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
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
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the detectadblocker.
 *
 * This function terminates the initialization of the detectadblocker plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void detectadblocker::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN0(detectadblocker, "server", server, detach_from_session);
    //SNAP_LISTEN(detectadblocker, "path", path::path, can_handle_dynamic_path, _1, _2);
    SNAP_LISTEN(detectadblocker, "layout", layout::layout, generate_header_content, _1, _2, _3);
}


void detectadblocker::on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    NOTUSED(ipath);
    NOTUSED(metadata);

    if(!f_detected)
    {
        content::content * content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());

        content::path_info_t settings_ipath;
        settings_ipath.set_path(get_name(name_t::SNAP_NAME_DETECTADBLOCKER_SETTINGS_PATH));
        QtCassandra::QCassandraRow::pointer_t settings_row(revision_table->row(settings_ipath.get_revision_key()));
        int8_t const inform_server(settings_row->cell(get_name(name_t::SNAP_NAME_DETECTADBLOCKER_INFORM_SERVER))->value().safeSignedCharValue(0, 1)); // AJAX On/Off

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
            QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());

            content::path_info_t settings_ipath;
            settings_ipath.set_path(get_name(name_t::SNAP_NAME_DETECTADBLOCKER_SETTINGS_PATH));
            QtCassandra::QCassandraRow::pointer_t settings_row(revision_table->row(settings_ipath.get_revision_key()));
            int64_t const prevent_ads_duration(settings_row->cell(get_name(name_t::SNAP_NAME_DETECTADBLOCKER_PREVENT_ADS_DURATION))->value().safeInt64Value(0, 1)); // AJAX On/Off

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


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
