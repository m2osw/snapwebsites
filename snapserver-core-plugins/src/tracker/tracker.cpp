// Snap Websites Server -- track users by saving all their actions in a table
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

#include "tracker.h"

#include "../users/users.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

#include <arpa/inet.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(tracker, 1, 0)


/** \brief Get a fixed tracker name.
 *
 * The tracker plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_TRACKER_TABLE:
        return "tracker";

    case name_t::SNAP_NAME_TRACKER_TRACKINGDATA:
        return "trackingdata";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_TRACKER_...");

    }
    NOTREACHED();
}




/** \class tracker
 * \brief The tracker plugin to track all user actions.
 *
 * The tracker plugin tracks all that the user accesses on the server.
 * This is particularly useful if you are working with someone who has
 * no clue what step they took to reach a certain situation.
 *
 * Of course, it will also be useful for marketing purposes if you look
 * at many of your users and what they do on your website.
 */










/** \brief Initialize the tracker plugin.
 *
 * This function is used to initialize the tracker plugin object.
 */
tracker::tracker()
    //: f_snap(nullptr) -- auto-init
    //, f_tracker_table(nullptr) -- auto-init
{
}


/** \brief Clean up the tracker plugin.
 *
 * Ensure the tracker object is clean before it is gone.
 */
tracker::~tracker()
{
}


/** \brief Get a pointer to the tracker plugin.
 *
 * This function returns an instance pointer to the tracker plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the tracker plugin.
 */
tracker * tracker::instance()
{
    return g_plugin_tracker_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString tracker::settings_path() const
{
    return "/admin/settings/tracker";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString tracker::icon() const
{
    return "/images/tracker/tracker-logo-64x64.png";
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
QString tracker::description() const
{
    return "Log all movements of all the users accessing your website.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString tracker::dependencies() const
{
    return "|users|";
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
int64_t tracker::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 12, 20, 22, 22, 0, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void tracker::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the tracker.
 *
 * This function terminates the initialization of the tracker plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void tracker::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN0(tracker, "server", server, attach_to_session);
    SNAP_LISTEN0(tracker, "server", server, detach_from_session);
    SNAP_LISTEN(tracker, "server", server, register_backend_action, _1);
}


/** \brief Initialize the tracker table.
 *
 * This function creates the tracker table if it doesn't exist yet. Otherwise
 * it simple initializes the f_tracker_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The tracker table is used to record all the user clicks on the website.
 * The table is setup as a "write-only" table since 99% of the time you
 * will only write to it. Only some backend tools and developers are
 * expected to check that data so it can be a bit slow on the read side
 * whereas it should be as fast as possible on the write side.
 *
 * New tracking includes data from the user which is saved serialized.
 * The index of each row is the user email address or the user IP address
 * for anonymous users.
 *
 * \return The pointer to the tracker table.
 */
QtCassandra::QCassandraTable::pointer_t tracker::get_tracker_table()
{
    if(!f_tracker_table)
    {
        f_tracker_table = f_snap->get_table(get_name(name_t::SNAP_NAME_TRACKER_TABLE));
    }
    return f_tracker_table;
}


/** \brief Make sure we run before the path::on_execute().
 *
 * This function grabs the user data before we run on_execute().
 *
 * At this point the user is already logged in if the users plugin
 * decided that the session was still valid.
 *
 * \note
 * We should probably have a different type of message although
 * at this point this message works for us just fine.
 * See SNAP-129
 */
void tracker::on_detach_from_session()
{
    users::users * users_plugin(users::users::instance());
    QString email;

    QDomElement parent_tag(f_doc.createElement("trackdata"));
    f_doc.appendChild(parent_tag);

    // save the URI, this is actually the point of this whole ordeal!
    // (we would also need to determine the type of page... i.e. attachment
    // or not and a few other things...)
    //
    QDomElement uri_tag(f_doc.createElement("uri"));
    parent_tag.appendChild(uri_tag);
    snap_uri const & uri(f_snap->get_uri());
    QDomText uri_text(f_doc.createTextNode(uri.get_uri()));
    uri_tag.appendChild(uri_text);

    QDomElement login_status(f_doc.createElement("login-status"));
    parent_tag.appendChild(login_status);

    // get the user path
    if(users_plugin->user_is_a_spammer())
    {
        login_status.setAttribute("level", "spammer");
    }
    else
    {
        users::users::user_info_t const & user_info(users_plugin->get_user_info());
        email = user_info.get_user_email();
        if(email.isEmpty())
        {
            login_status.setAttribute("level", "visitor");
        }
        else
        {
            // email is not available, user must not be logged in, get
            // the IP address instead (we do not care whether it is a
            // returning user or a logged in user per se.)
            //
            login_status.setAttribute("level", users_plugin->user_is_logged_in() ? "registered" : "returning-registered-user");

            // although it would be easy to provide the email otherwise,
            // it is easier to do it here and makes it easier when looking
            // at the data too
            //
            QDomElement email_tag(f_doc.createElement("email"));
            parent_tag.appendChild(email_tag);
        }
    }

    // the email is the key, if still empty, put the IP in there
    // (we need to use IPv6 to make sure we are forward compatible)
    //
    if(email.isEmpty())
    {
        QString const remote_addr(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REMOTE_ADDR)));

        // TODO: make use of the snap::addr object to convert to IPv4/6
        //
        struct in6_addr addr6;
        int const r6(inet_pton(AF_INET6, remote_addr.toUtf8().data(), &addr6));
        if(r6 == 1)
        {
            char buf[INET6_ADDRSTRLEN];
            char const * ptr(inet_ntop(AF_INET6, &addr6, buf, sizeof(buf)));
            if(ptr != nullptr)
            {
                // converted string to canonicalized IPv6 format
                email = ptr;
            }
        }
        else
        {
            struct in_addr addr4;
            int const r4(inet_pton(AF_INET, remote_addr.toUtf8().data(), &addr4));
            if(r4 == 1)
            {
                char buf[INET_ADDRSTRLEN];
                char const * ptr(inet_ntop(AF_INET, &addr4, buf, sizeof(buf)));
                if(ptr != nullptr)
                {
                    // converted string to canonicalized IPv4 format
                    email = ptr;
                }
            }
        }

        if(email.isEmpty())
        {
            SNAP_LOG_ERROR("remote address \"")(remote_addr)("\" could not be canonicalized to an IPv6 address.");

            // XXX: should we really register these using a special name?
            email = "*unknown*";
        }
    }

    // also save the email (or IP address) in XML data
    parent_tag.setAttribute("key", email);

    // the key is the date (64 bit in microseconds)
    int64_t const start_date(f_snap->get_start_date());
    QByteArray start_date_key;
    QtCassandra::setInt64Value(start_date_key, start_date);

    // also save the date in the data to make it easy to get later
    parent_tag.setAttribute("date", static_cast<qlonglong>(start_date));

    // the data is that XML file in the form of a string
    QtCassandra::QCassandraValue value;
    value.setTtl(86400 * 31);       // keep for 1 month (TODO: make it an editable preference)
    value.setStringValue(f_doc.toString(-1));

    // now save the result in Cassandra
    QtCassandra::QCassandraTable::pointer_t tracker_table(get_tracker_table());
    tracker_table->row(email)->cell(start_date_key)->setValue(value);
}


/** \brief Once we re-attach a session, we have the return code.
 *
 * This function is expected to be called (but at times that fails,
 * hence the on_detach_from_session() implementation) when the
 * request is furfilled one way or the other.
 *
 * Here we complement the document if we reach this function.
 * Especially we want to save the HTTP code, especially errors,
 * since by now it has to be known (saved in the headers.)
 */
void tracker::on_attach_to_session()
{
    bool changed(false);

    QDomElement parent_tag(f_doc.documentElement());

    // get the "Status: ..." if defined
    if(f_snap->has_header(get_name(snap::name_t::SNAP_NAME_CORE_STATUS_HEADER)))
    {
        changed = true;

        QString const status(f_snap->get_header(get_name(snap::name_t::SNAP_NAME_CORE_STATUS_HEADER)));

        parent_tag.setAttribute("status", status);
    }

    if(changed)
    {
        QString const email(parent_tag.attribute("key"));

        // the key is the date (64 bit in microseconds)
        int64_t const start_date(f_snap->get_start_date());
        QByteArray start_date_key;
        QtCassandra::setInt64Value(start_date_key, start_date);

        // the data is that XML file in the form of a string
        QtCassandra::QCassandraValue value;
        value.setTtl(86400 * 31);       // keep for 1 month (TODO: make it an editable preference)
        value.setStringValue(f_doc.toString(-1));

        // now save the result in Cassandra
        QtCassandra::QCassandraTable::pointer_t tracker_table(get_tracker_table());
        tracker_table->row(email)->cell(start_date_key)->setValue(value);
    }
}


/** \brief Register the backend actions.
 *
 * This function registers this plugin as supporting the actions:
 *
 * \li trackingdata
 *
 * The "trackingdata" action saves the information for a user in an XML file.
 *
 * \param[in,out] actions  The list of supported actions where we add ourselves.
 */
void tracker::on_register_backend_action(server::backend_action_set & actions)
{
    actions.add_action(get_name(name_t::SNAP_NAME_TRACKER_TRACKINGDATA), this);
}


/** \brief Execute a backend action.
 *
 * Execute the backend action of the tracker.
 *
 * \param[in] action  The action this function is being called with.
 */
void tracker::on_backend_action(QString const & action)
{
    if(action == get_name(name_t::SNAP_NAME_TRACKER_TRACKINGDATA))
    {
        // extract all the tracking data for a given user or IP address
        // and save that in a file
        //
        // remember that data is kept only for one month by default
        // so after a little while it "disappear" meaning that the
        // resulting files will not grow indefinitively
        //
        on_backend_tracking_data();
    }
    else
    {
        // unknown action (we should not have been called with that name!)
        throw snap_logic_exception(QString("tracker.cpp:on_backend_action(): tracker::on_backend_action(\"%1\") called with an unknown action...").arg(action));
    }
}


/** \brief Create an XML file of the tracking data of a given user.
 *
 */
void tracker::on_backend_tracking_data()
{
    // TODO: actually implement such?!
    // At this point I am more thinking of a raw UI view of the data
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
