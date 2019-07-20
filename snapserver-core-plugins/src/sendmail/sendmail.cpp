// Snap Websites Server -- manage sendmail (record, display)
// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// self
//
#include "sendmail.h"


// plugins
//
#include "../locale/snap_locale.h"
#include "../output/output.h"
#include "../users/users.h"


// snapwebsites lib
//
#include <snapwebsites/flags.h>
#include <snapwebsites/http_strings.h>
#include <snapwebsites/log.h>
#include <snapwebsites/mkgmtime.h>
#include <snapwebsites/process.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qdomxpath.h>
#include <snapwebsites/quoted_printable.h>
#include <snapwebsites/snap_magic.h>
#include <snapwebsites/snap_pipe.h>


// snapdev lib
//
#include <snapdev/not_used.h>


// libtld
//
#include <libtld/tld.h>


// libdbproxy
//
#include <libdbproxy/value.h>


// Qt Serialization library
//
#include <QtSerialization/QSerializationComposite.h>
#include <QtSerialization/QSerializationFieldString.h>
#include <QtSerialization/QSerializationFieldTag.h>


// C++
//
#include <fstream>
#include <iostream>


// Qt
//
#include <QFile>


// last include
//
#include <snapdev/poison.h>



SNAP_PLUGIN_START(sendmail, 1, 0)


/** \brief Get a fixed sendmail plugin name.
 *
 * The sendmail plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_SENDMAIL:
        return "sendmail";

    case name_t::SNAP_NAME_SENDMAIL_BOUNCED:
        return "bounced";

    case name_t::SNAP_NAME_SENDMAIL_BOUNCED_ARRIVAL_DATE:
        return "sendmail::bounce_arrival_date";

    case name_t::SNAP_NAME_SENDMAIL_BOUNCED_DIAGNOSTIC_CODE:
        return "sendmail::bounce_diagnostic_code";

    case name_t::SNAP_NAME_SENDMAIL_BOUNCED_EMAIL:
        return "sendmail::bounce_email";

    case name_t::SNAP_NAME_SENDMAIL_BOUNCED_FAILED:
        return "bounced_failed";

    case name_t::SNAP_NAME_SENDMAIL_BOUNCED_NOTIFICATION:
        return "sendmail::bounce_notification";

    case name_t::SNAP_NAME_SENDMAIL_BOUNCED_RAW:
        return "bounced_raw";

    case name_t::SNAP_NAME_SENDMAIL_BYPASS_BLACKLIST:
        return "Bypass-Blacklist";

    case name_t::SNAP_NAME_SENDMAIL_CREATED:
        return "sendmail::created";

    case name_t::SNAP_NAME_SENDMAIL_EMAIL:
        return "sendmail::email";

    case name_t::SNAP_NAME_SENDMAIL_EMAIL_ENCRYPTION:
        return "sendmail::email_encryption";

    case name_t::SNAP_NAME_SENDMAIL_EMAIL_FREQUENCY:
        return "Email-Frequency";

    case name_t::SNAP_NAME_SENDMAIL_EMAILS_TABLE:
        return "emails";

    case name_t::SNAP_NAME_SENDMAIL_FIELD_EMAIL:
        return "email";

    case name_t::SNAP_NAME_SENDMAIL_FIELD_LEVEL:
        return "level";

    case name_t::SNAP_NAME_SENDMAIL_FREQUENCY:
        return "sendmail::frequency";

    case name_t::SNAP_NAME_SENDMAIL_FREQUENCY_DAILY:
        return "daily";

    case name_t::SNAP_NAME_SENDMAIL_FREQUENCY_IMMEDIATE:
        return "immediate";

    case name_t::SNAP_NAME_SENDMAIL_FREQUENCY_MONTHLY:
        return "monthly";

    case name_t::SNAP_NAME_SENDMAIL_FREQUENCY_WEEKLY:
        return "weekly";

    case name_t::SNAP_NAME_SENDMAIL_INDEX:
        return "*index*";

    case name_t::SNAP_NAME_SENDMAIL_IS_ADMIN:
        return "sendmail::is_admin";

    case name_t::SNAP_NAME_SENDMAIL_LAYOUT_NAME:
        return "sendmail";

    case name_t::SNAP_NAME_SENDMAIL_LEVEL_ANGRYLIST:
        return "angrylist";

    case name_t::SNAP_NAME_SENDMAIL_LEVEL_BLACKLIST:
        return "blacklist";

    case name_t::SNAP_NAME_SENDMAIL_LEVEL_ORANGELIST:
        return "orangelist";

    case name_t::SNAP_NAME_SENDMAIL_LEVEL_PURPLELIST:
        return "purplelist";

    case name_t::SNAP_NAME_SENDMAIL_LEVEL_WHITELIST:
        return "whitelist";

    case name_t::SNAP_NAME_SENDMAIL_LISTS:
        return "lists";

    case name_t::SNAP_NAME_SENDMAIL_MAXIMUM_TIME:
        return "Maximum-Time";

    case name_t::SNAP_NAME_SENDMAIL_MINIMUM_TIME:
        return "Minimum-Time";

    case name_t::SNAP_NAME_SENDMAIL_NEW:
        return "new";

    case name_t::SNAP_NAME_SENDMAIL_SENDING_STATUS:
        return "sendmail::sending_status";

    case name_t::SNAP_NAME_SENDMAIL_STATUS:
        return "sendmail::status";

    case name_t::SNAP_NAME_SENDMAIL_STATUS_DELETED:
        return "deleted";

    case name_t::SNAP_NAME_SENDMAIL_STATUS_FAILED:
        return "failed";

    case name_t::SNAP_NAME_SENDMAIL_STATUS_INVALID:
        return "invalid";

    case name_t::SNAP_NAME_SENDMAIL_STATUS_LOADING:
        return "loading";

    case name_t::SNAP_NAME_SENDMAIL_STATUS_NEW:
        return "new";

    case name_t::SNAP_NAME_SENDMAIL_STATUS_READ:
        return "read";

    case name_t::SNAP_NAME_SENDMAIL_STATUS_SENDING:
        return "sending";

    case name_t::SNAP_NAME_SENDMAIL_STATUS_SENT:
        return "sent";

    case name_t::SNAP_NAME_SENDMAIL_STATUS_SPAM:
        return "spam";

    case name_t::SNAP_NAME_SENDMAIL_STATUS_UNSUBSCRIBED:
        return "unsubscribed";

    case name_t::SNAP_NAME_SENDMAIL_STOP:
        return "STOP";

    case name_t::SNAP_NAME_SENDMAIL_UNSUBSCRIBE_ON:
        return "sendmail::unsubscribe_on";

    case name_t::SNAP_NAME_SENDMAIL_UNSUBSCRIBE_PATH:
        return "unsubscribe";

    case name_t::SNAP_NAME_SENDMAIL_UNSUBSCRIBE_SELECTION:
        return "sendmail::unsubscribe_selection";

    case name_t::SNAP_NAME_SENDMAIL_USER_AGENT:
        // it would be better with the version of the sendmail plugin...
        return "Snap! C++ Sendmail User Agent v" SNAPWEBSITES_VERSION_STRING;

    default:
        // invalid index
        throw snap_logic_exception(QString("Invalid name_t::SNAP_NAME_SENDMAIL_...").arg(static_cast<int>(name)));

    }
    NOTREACHED();
}


/** \brief Initialize the sendmail plugin.
 *
 * This function is used to initialize the sendmail plugin object.
 */
sendmail::sendmail()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the sendmail plugin.
 *
 * Ensure the sendmail object is clean before it is gone.
 */
sendmail::~sendmail()
{
}


/** \brief Get a pointer to the sendmail plugin.
 *
 * This function returns an instance pointer to the sendmail plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the sendmail plugin.
 */
sendmail * sendmail::instance()
{
    return g_plugin_sendmail_factory.instance();
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString sendmail::icon() const
{
    return "/images/sendmail/sendmail-logo-64x64.png";
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
QString sendmail::description() const
{
    return "Handle sending emails from your website environment."
        " This version of sendmail requires a backend process to"
        " actually process the emails and send them out.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString sendmail::dependencies() const
{
    return "|filter|layout|output|path|sessions|users|";
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
int64_t sendmail::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 12, 25, 4, 16, 12, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our administration pages, etc.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void sendmail::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());

    layout::layout::instance()->add_layout_from_resources(get_name(name_t::SNAP_NAME_SENDMAIL_LAYOUT_NAME));
}


/** \brief Initialize sendmail.
 *
 * This function terminates the initialization of the sendmail plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void sendmail::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(sendmail, "server", server, register_backend_cron, _1);
    SNAP_LISTEN(sendmail, "filter", filter::filter, replace_token, _1, _2, _3);
    SNAP_LISTEN(sendmail, "filter", filter::filter, token_help, _1);
    SNAP_LISTEN(sendmail, "users", users::users, check_user_security, _1);

    SNAP_TEST_PLUGIN_SUITE_LISTEN(sendmail);
}


/** \brief Initialize the emails table.
 *
 * This function creates the "emails" table if it doesn't exist yet. Otherwise
 * it simple returns the existing Cassandra table.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The table is used for several purposes:
 *
 * \li List of emails to be sent
 *
 * Whenever a plugin sends an email, it makes use of this table via the
 * post_email() function. This function adds an entry under the "new" row
 * which is used to post new emails to the backend. The backend is started
 * with the special "sendmail" action to actually handle the emails.
 *
 * \li Email lists to handle multi-users send
 *
 * This table has a special entry named "lists" which is a list of emails
 * that are used by end users to create mailing lists. For example, all your
 * staff could be listed under a list named "staff@m2osw.com". Sending an
 * email to "staff" results in an email sent to all the people listed in that
 * list. Note that if you create such lists in your mail server then this
 * Snap! Websites feature is not required.
 *
 * Since a list needs to be specific to a website (or at least a
 * well defined group of websites) the names in such lists include the name
 * of the website (i.e. m2osw.com:staff; the name of the site is taken from
 * the f_snap->get_site_key() function.) The name looks like this:
 *
 * \code
 * <site-key>: <list-name>
 * \endcode
 *
 * Where the site key parameter comes from the email site key (set when you
 * call the post_email() function) and the list name parameter comes from
 * the actual To: email list.
 *
 * \li List of user email addresses
 *
 * Each user has one entry in the table which is keyed by their email address
 * (since emails are considered unique.) The fact that we save the emails in
 * this table rather than the user table is to avoid problems (i.e. growth
 * of the user table could result in some unwanted slowness.)
 *
 * The list includes each email using the email key as the cell name and the
 * email data as the email contents. Later we may want to have a key which
 * includes the date or the name of the sender in order to be able to quickly
 * list the emails to the user in such or such order.
 *
 * The table also manages information about the emails such as whether it
 * was looked at, deleted, spam, etc. Spam emails do not get deleted so that
 * way we can easily eliminate duplicates (Although we may want to make use
 * of a spam user at some point.)
 *
 * Speaking of duplicates, when we detect that another email was sent to
 * tell something to the user (i.e. you got new comments) and the user did
 * not yet come back on the site, then the new version should not be saved.
 * This feature makes use of the cumulative flag.
 *
 * Emails that were sent (i.e. using the /usr/bin/sendmail tool) are marked
 * as sent so we avoid sending them again.
 *
 * \return The pointer to the users table.
 */
libdbproxy::table::pointer_t sendmail::get_emails_table()
{
    return f_snap->get_table(get_name(name_t::SNAP_NAME_SENDMAIL_EMAILS_TABLE));
}


/** \fn void sendmail::filter_email(email& e)
 * \brief Prepare the email for the filter_email signal.
 *
 * This function readies the email parameter (\p e) for the filter_email
 * signal.
 *
 * At this point this function processes the email using the token plugin
 * so as to convert all the tags in the body (text and HTML) before
 * proceeding further.
 *
 * \param[in] e  The email to be filtered by other plugins.
 */


/** \brief Check whether an email is considered valid.
 *
 * This function calls the users plugin check_user_security() function to
 * verify the specified user email address.
 *
 * This is mainly a helper function that will make sure the call uses
 * the blacklist flag (name_t::SNAP_NAME_SENDMAIL_BYPASS_BLACKLIST)
 * parameter as expected.
 *
 * \param[in] user_email  The email address to check.
 * \param[in] e           The email that you want to use to that email address.
 *
 * \return true if the email validates; false if you should not call the
 *         post_email() function.
 */
bool sendmail::validate_email(QString const & user_email, email const * e)
{
    users::users * users_plugin(users::users::instance());

SNAP_LOG_TRACE("sendmail::validate_email(): user_email=")(user_email)(", e=")(e);

    // prevent attempts of sending an email to an example email address
    // (even if the example address is a valid email address)
    //
    users::users::user_info_t const user_info(users_plugin->get_user_info_by_email(user_email));
    if(user_info.user_is_an_example_from_email())
    {
        return false;
    }

    bool bypass_blacklist(false);
    if(e != nullptr)
    {
        QString const bypass(e->get_parameter(get_name(name_t::SNAP_NAME_SENDMAIL_BYPASS_BLACKLIST)));
        bypass_blacklist = bypass == "true";
    }

    // we use "!" for the password because we do not want to have
    // any password checked.
    //
    //content::permission_flag secure; // TODO: not used?!
    users::users::user_security_t security;
    security.set_user_info(user_info, QString(), true);
    //security.set_password("!"); -- leave the default
    //security.set_policy("users"); -- leave the default
    security.set_bypass_blacklist(bypass_blacklist);
    users_plugin->check_user_security(security);

    // here we also test whether the email address is an example email
    // address or not; it could be that later we find out that certain
    // other domains are clear examples for various types of domain
    // names (i.e. "exemple.fr") although at this time it looks like
    // this is limited to what is defined in RFC 2606.
    //
    // See: https://tools.ietf.org/html/rfc2606
    //
    return security.get_secure().allowed() && !security.get_example();
}


/** \brief Check whether an email is considered valid.
 *
 * When sending an email to a specific individual, you may call this function
 * to know whether the individual email address is considered valid. An
 * invalid email is one of a user that decided to block this Snap! Websites
 * instance from sending him emails or a previous email was rejected with
 * a 5XX code.
 *
 * The function may return any one of these statuses:
 *
 * \li EMAIL_STATUS_VALID -- the email is considered valid and can be emailed
 *     to;
 * \li EMAIL_STATUS_INVALID -- the email was used previously and the remote
 *     MTA said it is not valid;
 * \li EMAIL_STATUS_BLOCKED -- the email is valid, but the owner asked us
 *     not to send him/her any emails
 *
 * \note
 * The function will return EMAIL_STATUS_VALID when the user blocks our
 * email, but the email to be sent represents an important email that
 * needs to be sent for the process to be accomplished (i.e. the user
 * blacklisted a certain site and later asks to register at that same
 * site.)
 *
 * \note
 * The validate_email() function can be used instead of directly calling
 * the signal since that way you do not have to guess how to call the
 * signal. The validate_email() accepts the email you are about to
 * post_email() so it can check the bypass flag of that email.
 *
 * \param[in,out] security  The user security parameters.
 */
void sendmail::on_check_user_security(users::users::user_security_t & security)
{
    // at the moment, only valid users have a security check here
    // (i.e. if their email address bounces, then we place them in our
    // "semi-blacklist" for a while to avoid sending repetitive emails
    // to a server that does not accept those emails.)
    //
    users::users::user_info_t const & user_info( security.get_user_info() );
    QString const & user_email(security.get_email());
    if(!security.get_secure().allowed()
    || user_email.isEmpty()
    || !user_info.is_valid())
    {
        return;
    }

    //users::users * users_plugin(users::users::instance());

    // should we allow 2 or 3 attempts? it seems to me that with just
    // one attempt, if it returns a 5XX the email is plainly not valid.
    //
    {
        QString diagnostic;
        QString const bounce_diagnostic_name(QString("%1%2").arg(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_DIAGNOSTIC_CODE)).arg(0));
        //users::users::user_info_t const & user_info(security.get_user_info());
        if(user_info.load_user_parameter(bounce_diagnostic_name, diagnostic))
        {
            if(diagnostic.startsWith("5."))
            {
                // a diagnostic that matches with 5.x.y is considered
                // totally invalid and it cannot be retried... (really
                // no need to)
                //
                // there is one problem with this one: a host that does
                // not exist will generate a 5.x.y error; if later that
                // very domain name is registered, we will still ignore
                // it for that very user... (which is probably just fine
                // because someone should not try to register with us
                // until they know that their mail server works as
                // expected.) For now, our "fix" is to block such email
                // addresses for 4 months "only".
                //
                int64_t arrival_date_us(0);
                QString const bounce_arrival_date_name(QString("%1%2").arg(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_ARRIVAL_DATE)).arg(0));
                if(user_info.load_user_parameter(bounce_arrival_date_name, arrival_date_us))
                {
                    // if we tried more than 4 months ago, we can try again
                    // (i.e. the user may have been created in the meantime)
                    //
                    if(f_snap->get_start_date() > arrival_date_us + 86400LL * 124LL * 1000000LL)
                    {
                        arrival_date_us = 0;
                    }
                }
                if(arrival_date_us != 0)
                {
                    bool is_admin(false);
                    libdbproxy::value is_admin_value;
                    if(user_info.load_user_parameter(get_name(name_t::SNAP_NAME_SENDMAIL_IS_ADMIN), is_admin_value))
                    {
                        is_admin = is_admin_value.signedCharValue();
                    }
                    if(is_admin)
                    {
                        // this is an administrator, do not prevent sending emails
                        // even though they may always bounce back however
                        // admins are not unlikely to have multiple emails
                        // and if one bounces back the others are not unlikely
                        // to work just fine...
                        //
                        SNAP_LOG_WARNING("Email \"")
                                        (user_email)
                                        ("\" will be sent a message even though it was marked as invalid (")
                                        (diagnostic)
                                        (").");

                        // generate a low priority flag too
                        //
                        // a flag doesn't work well because we'd need one per
                        // email... at this point I'm not too sure what
                        // we could do about it anyway; the names can't
                        // include an email address...
                        //
                        snap::snap_flag::pointer_t flag(SNAP_FLAG_UP(
                                  "snapserver-plugin"
                                , "sendmail"
                                , "email-bounced"
                                , "email(s) sent to \""
                                    + user_email
                                    + "\" bounced (warning, this flag only records information about the last bounced email, you may have others...)"));
                        flag->set_priority(3);
                        flag->set_manual_down(true); // use "manual" because we don't have a good way to remove this flag
                        flag->save();
                    }
                    else
                    {
SNAP_LOG_TRACE("arrival_date_us=")(arrival_date_us);
                        security.get_secure().not_permitted(QString("\"%1\" does not look like a valid email address since it returned a 5XX error code.").arg(user_email));
                        security.set_status(users::users::status_t::STATUS_BLOCKED);
                        return;
                    }
                }
            }
        }
    }

    QString level;
    if(user_info.load_user_parameter(get_name(name_t::SNAP_NAME_SENDMAIL_UNSUBSCRIBE_SELECTION), level)
    || user_info.load_user_parameter(QString("%1::%2").arg(get_name(name_t::SNAP_NAME_SENDMAIL_UNSUBSCRIBE_SELECTION)).arg(f_snap->get_site_key()), level))
    {
        // If the user was put in the Angry List then we have no way
        // to send any emails... so the user cannot register or change
        // their password if they have an existing account!
        //
        // However, if in the blacklist, the bypass_blacklist allows
        // one to ignore the fact (i.e. the user will be sent the
        // email.)
        //
        if(level == get_name(name_t::SNAP_NAME_SENDMAIL_LEVEL_BLACKLIST)
        && security.get_bypass_blacklist())
        {
            // allow these emails
            //
            level = get_name(name_t::SNAP_NAME_SENDMAIL_LEVEL_WHITELIST);
        }
        if(level == get_name(name_t::SNAP_NAME_SENDMAIL_LEVEL_BLACKLIST)
        || level == get_name(name_t::SNAP_NAME_SENDMAIL_LEVEL_ANGRYLIST))
        {
            security.get_secure().not_permitted(QString("\"%1\" is in the blacklist or angrylist (i.e. user is unsubscribed).").arg(user_email));
            security.set_status(users::users::status_t::STATUS_BLOCKED);
            return;
        }
    }

    // nothing prevented this email from being used, keep it valid
    //
}


/** \brief Post an email.
 *
 * This function posts an email.
 *
 * The email is not sent immediately, instead it gets added to the
 * Cassandra database and processed later by the sendmail backend
 * (i.e. using "snapbackend -a sendmail".)
 *
 * Note that the message is not processed here at all. The backend
 * is fully responsible. The processing determines all the users
 * being emailed, when to send each email, whether the user wants
 * HTML or not, etc.
 *
 * This function sets the From parameter to the default if you did
 * not defined it. The default is the website wide email address,
 * which in most cases is the website owner's email address.
 *
 * If you know you are sending the email to one specific user (i.e.
 * the destination email is specific to one user instead of being
 * a list), then you may first want to call valid_email() to know
 * whether the email of that user is considered valid. If the
 * Snap! Websites already tried to send that user an email and it
 * failed, then the valid_email() function will return false. When
 * that happens, you should display an error message to the user.
 *
 * \note
 * We save the original data to the Cassandra database so it can be
 * processed by the backend instead of the frontend. That way we can save
 * time in the frontend.
 *
 * \exception sendmail_exception_invalid_argument
 * An invalid argument exception is raised if no content was specified before
 * you call this function. The email is considered empty if no attachments
 * were added and no email path was defined.
 *
 * \param[in] e  Email to post.
 */
void sendmail::post_email(email const & e)
{
    // we do not accept to send an empty email
    //
    if(e.get_attachment_count() == 0
    && e.get_email_path().isEmpty())
    {
        throw sendmail_exception_invalid_argument("An email must have at least one attachment or the email path defined");
    }

    email copy(e);

    // setup the FROM email address if not yet defined;
    // administrator can define this email address in "sites" table
    //
    if(!copy.has_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_FROM)))
    {
        copy.set_from(default_from());
    }

    copy.set_site_key(f_snap->get_site_key());
    QString const key(f_snap->get_unique_number());
    copy.set_email_key(key);
    libdbproxy::table::pointer_t emails_table(get_emails_table());
    libdbproxy::value value;
    QString const data(copy.serialize());
    value.setStringValue(data);
    emails_table->getRow(get_name(name_t::SNAP_NAME_SENDMAIL_NEW))->getCell(key)->setValue(value);

    // signal the sendmail backend server with a PING
    //
    f_snap->udp_ping(get_name(name_t::SNAP_NAME_SENDMAIL));
}


/** \brief Return the default content of the From field.
 *
 * This function can be used to retrieve the default From field.
 *
 * If you do not define the From field in an email, this value is used.
 * It comes from the name_t::SNAP_NAME_CORE_ADMINISTRATOR_EMAIL site parameter.
 * If that parameter is not defined, then an internal default is returned:
 *
 * \code
 * contact@example.com
 * \endcode
 *
 * \return The default value of the From field.
 */
QString sendmail::default_from() const
{
    libdbproxy::value from(f_snap->get_site_parameter(get_name(snap::name_t::SNAP_NAME_CORE_ADMINISTRATOR_EMAIL)));
    if(from.nullValue())
    {
        // some hard coded fallback default...
        from.setStringValue("contact@example.com");
    }
    return from.stringValue();
}



/** \brief Register the "sendmail" action.
 *
 * This function registers this plugin as supporting the "sendmail" CRON
 * action. This backend is used on a backend that supports a standard Unix
 * sendmail tool, which is used to send emails to users.
 *
 * When the front end sends emails, it actually calls the post_email()
 * function which registers the email in the database table named "emails".
 * The backend process retrieves those emails and processes them by
 * determining the final users who are to receive the email and send
 * a seperate copy of the email to each destination user.
 *
 * \param[in,out] actions  The list of supported actions where we add ourselves.
 */
void sendmail::on_register_backend_cron(server::backend_action_set & actions)
{
    actions.add_action(get_name(name_t::SNAP_NAME_SENDMAIL), this);
}


/** \brief Run the sendmail server once.
 *
 * The sendmail server processes emails that the system wants to send
 * to varoius users. All the email data is found in the Cassandra
 * database.
 *
 * The CRON action is defined as:
 *
 * \code
 *      snapbackend --action sendmail::sendmail
 * \endcode
 *
 * The process stops as soon as possible if it receives a STOP event.
 *
 * The server should be stopped with the snapsignal tool using the
 * STOP event as follow:
 *
 * \code
 *      snapsignal -a sendmail::sendmail STOP
 * \endcode
 *
 * \note
 * The \p action parameter is here because some plugins may
 * understand multiple actions in which case we need to know
 * which action is waking us up.
 *
 * \param[in] action  The action this function is being called with.
 */
void sendmail::on_backend_action(QString const & action)
{
    if(action == get_name(name_t::SNAP_NAME_SENDMAIL))
    {
        try
        {
            f_backend = dynamic_cast<snap_backend *>(f_snap);
            if(!f_backend)
            {
                throw sendmail_exception_no_backend("could not determine the snap_backend pointer");
            }

            // process emails that are in the database and are ready to go
            // (i.e. their time is in the past or now)
            //
            check_bounced_emails();
            process_emails();
            run_emails();
        }
        catch( snap_exception const & e )
        {
            SNAP_LOG_FATAL("sendmail::on_backend_action(): snap exception caught: ")(e.what());
            exit(1);
            NOTREACHED();
        }
        catch( libdbproxy::exception const & e )
        {
            SNAP_LOG_FATAL("sendmail::on_backend_action(): exception caught: ")(e.what());
            for( auto bt_line : e.get_stack_trace() )
            {
                SNAP_LOG_ERROR("exception backtrace: ")(bt_line);
            }
            exit(1);
            NOTREACHED();
        }
        catch( std::exception const & e )
        {
            SNAP_LOG_FATAL("sendmail::on_backend_action(): exception caught: ")(e.what())(" (not a snap_exception nor a exception!)");
            exit(1);
            NOTREACHED();
        }
        catch( ... )
        {
            SNAP_LOG_FATAL("sendmail::on_backend_action(): unknown exception caught!");
            exit(1);
            NOTREACHED();
        }
    }
    else
    {
        // unknown action (we should not have been called with that name!)
        throw snap_logic_exception(QString("sendmail::on_backend_action(\"%1\") called with an unknown action...").arg(action));
    }
}


/** \brief Check the "bounced" row in the "emails" table.
 *
 * This function goes through the list of emails that were sent back to
 * use (bounced) and copy the data (notification, headers) in the
 * user's account so next time we want to email that user we have
 * information about his mailbox status.
 */
void sendmail::check_bounced_emails()
{
    //SNAP_LOG_TRACE("process raw bounced emails");

    // TODO: this one needs to be protected if we are to allow multi-computer
    //       processing of emails
    //
    libdbproxy::table::pointer_t emails_table(get_emails_table());
    libdbproxy::row::pointer_t raw_row(emails_table->getRow(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_RAW)));
    raw_row->clearCache();
    auto all_column_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
    all_column_predicate->setCount(100); // should this be a parameter?
    all_column_predicate->setIndex(); // behave like an index
    for(;;)
    {
        raw_row->readCells(all_column_predicate);
        libdbproxy::cells const cells(raw_row->getCells());
        if(cells.isEmpty())
        {
            break;
        }
        // handle one batch
        for(libdbproxy::cells::const_iterator c(cells.begin());
                c != cells.end();
                ++c)
        {
            // get the email from the database
            // we expect empty values once in a while because a dropCell() is
            // not exactly instantaneous in Cassandra
            libdbproxy::cell::pointer_t cell(*c);
            QString const bounce_report(cell->getValue().stringValue());
            reorganize_bounce_email(cell->columnKey(), bounce_report);
            raw_row->dropCell(cell->columnKey());

            // quickly end this process if the user requested a stop
            if(f_backend->stop_received())
            {
                // clean STOP
                return;
            }
        }
    }

    QString const website_key(f_snap->get_website_key());

    //SNAP_LOG_TRACE("process \"")(website_key)("\" bounced emails");

    libdbproxy::row::pointer_t row(emails_table->getRow(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED)));
    row->clearCache();
    auto column_predicate(std::make_shared<libdbproxy::cell_range_predicate>());
    column_predicate->setStartCellKey(website_key + "/");
    column_predicate->setEndCellKey(website_key + "0");
    column_predicate->setCount(100); // should this be a parameter?
    column_predicate->setIndex(); // behave like an index
    for(;;)
    {
        row->readCells(column_predicate);
        libdbproxy::cells const cells(row->getCells());
        if(cells.isEmpty())
        {
            break;
        }
        // handle one batch
        for(libdbproxy::cells::const_iterator c(cells.begin());
                c != cells.end();
                ++c)
        {
            // get the email from the database
            // we expect empty values once in a while because a dropCell() is
            // not exactly instantaneous in Cassandra
            libdbproxy::cell::pointer_t cell(*c);
            QString const bounce_report(cell->getValue().stringValue());
            process_bounce_email(cell->columnKey(), bounce_report, nullptr);
            row->dropCell(cell->columnKey());

            // quickly end this process if the user requested a stop
            if(f_backend->stop_received())
            {
                // clean STOP
                return;
            }
        }
    }
}


/** \brief Reorganize a bounce email report by website.
 *
 * The emails are saved in the database by snapbounce. Unfortunately,
 * snapbounce does not have access to the parse_email() function
 * (the main reason being that the definition of the email class
 * is in sendmail; we could move it out to a class in the library
 * but at this point on snapbounce would need it and I do not think
 * it is worth the trouble. Also we do not want to slow down that
 * process.)
 *
 * This function takes a bounced email, searches for the original
 * header in which we expect to find the Message-ID field. That
 * field gives us the name of the website and the session identifier.
 * Since we may be running in any other website at this point, the
 * make use of that information to save that data in another table
 * where it will specifically be managed by a process running in
 * that very website. (TBD: we certainly could immediately manage
 * those emails that are in the website currently running?)
 *
 * So process is:
 *
 * 1) read the emails from "bounced_raw"
 *
 * 2) parse the header to find Message-ID
 *
 * 3) save the email in "bounced" as \<website>/\<date>/\<session-id>
 *
 * Note that we drop the uuid and use the session identifier instead.
 * Since the session is unique, we get a similar safe unique identifier
 * for that row.
 *
 * \param[in] column_key  The key of the column being worked on.
 * \param[in] bounce_report  The email being checked.
 */
void sendmail::reorganize_bounce_email(QByteArray const & column_key, QString const & bounce_report)
{
    email e;
    if(!parse_email(bounce_report, e, true))
    {
        return;
    }

    int const max_attachment_count(e.get_attachment_count());
    for(int idx(0); idx < max_attachment_count; ++idx)
    {
        email::attachment & attachment(e.get_attachment(idx));
        QCaseInsensitiveString const content_description(attachment.get_header("Content-Description"));
        if(content_description == "Undelivered Message Headers")
        {
            // the headers of the undelivered message should include
            // the Message-ID that we are interested in
            //
            if(attachment.get_related_count() >= 1)
            {
                // get the message, the encoding is as follow:
                //
                //     '<' <session-id> '.' "snapwebsites" '@' <website> '>'
                //
                email::attachment const & message_headers(attachment.get_related(0));
                QString const message_id(message_headers.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_MESSAGE_ID)));
                int const period(message_id.indexOf('.'));
                int const at(message_id.indexOf('@'));
                if(period > 1
                && at > period
                && message_id.at(0) == '<'
                && message_id.at(message_id.length() - 1) == '>')
                {
                    // extract the website URI, the date, and
                    // session identifier
                    //
                    QString const website(message_id.mid(at + 1, message_id.length() - at - 2));
                    int64_t const date(libdbproxy::safeInt64Value(column_key, 0, 0));
                    QString const session_id(message_id.mid(1, period - 1));

                    // the website needs to have a session with that identifier
                    // otherwise we may end up with entries that stick to the
                    // "bounced" table for 3 months which would be a big waste
                    // of time...
                    //
                    if(!sessions::sessions::instance()->session_exists(website, session_id))
                    {
                        // save a copy in the bounced_failed table so
                        // someone can still check those out to see whether
                        // there is a problem with it
                        //
                        libdbproxy::table::pointer_t emails_table(get_emails_table());
                        libdbproxy::value report;
                        report.setStringValue(bounce_report);
                        report.setTtl(86400 * 93); // keep for about 3 months
                        emails_table->getRow(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_FAILED))->getCell(column_key)->setValue(report);
                        SNAP_LOG_INFO("ignoring bounce email with website \"")(website)("\" since no sessions use it.");
                        return;
                    }

                    // build the new column key
                    //
                    QString const key(QString("%1/%2/%3").arg(website).arg(date, 19, 10, QChar('0')).arg(session_id));
                    if(website == f_snap->get_website_key())
                    {
                        // since we are working on this website, no need to
                        // waste time by saving the data back in the database
                        // to delete it right after! so we directly process
                        // that email
                        //
                        process_bounce_email(key.toUtf8(), bounce_report, &e);
                    }
                    else
                    {
                        // set a TTL because it can happen that the session
                        // gets deleted or is somehow invalid and the message
                        // would stick around forever...
                        //
                        libdbproxy::table::pointer_t emails_table(get_emails_table());
                        libdbproxy::value report;
                        report.setStringValue(bounce_report);
                        report.setTtl(86400 * 93); // keep for about 3 months
                        emails_table->getRow(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED))->getCell(key)->setValue(report);
                    }
                    return;
                    NOTREACHED();
                }
            }
        }
    }
}


/** \brief Process a bounce email report.
 *
 * This function searches for a few parts of interest and save
 * that information in the Cassandra cluster under the corresponding
 * user.
 *
 * The data of interest are:
 *
 * 1. Message-ID
 *
 * The section with the original email headers includes our Message-ID.
 * This gives us a way to find the session this email was linked to.
 * That session includes a reference back to the user we sent the email
 * to. That is the place were we will save the data from the bounce.
 *
 * Since the Message-ID is extracted in the previous process, here
 * it is actually ignored. We already have the session identifier from
 * the column key.
 *
 * 2. Notification
 *
 * The notification section includes the text from the server that bounced
 * the email. This is considered a human readable message so we save that
 * message and display it to the administrator who checks on such statuses.
 *
 * 3. Delivery Report, Diagnostic-Code
 *
 * The delivery report is a machine readable set of fields which we have
 * transformed as a set of header fields in the bounce email. If the
 * Diagnostic-Code is defined, it should always be, then we can use
 * that code to save along the other data. This gives us an idea of
 * whether we could try again (4XX) or not (5XX). Especially, if we
 * get a 554, it is probably never useful to retry.
 *
 * 4. Delivery Report, Action/Status
 *
 * Similar to Diagnostic-Code, the Action and Status could help us with
 * determining whether to ever try again to send an email to that user.
 *
 * 5. Deliver Report, Final-Recipient
 *
 * The postfix system canonicalize the email address before forwarding
 * the email. This canonicalization result is saved in the Final-Recipient.
 * I do not think we can do much with this because the email is likely
 * to represent something else than what we want.
 *
 * \param[in] column_key  The key of the column being worked on.
 * \param[in] bounce_report  The raw bounce report as saved by snapbounce.
 * \param[in] e  The input email if available.
 */
void sendmail::process_bounce_email(QByteArray const & column_key, QString const & bounce_report, email const * e)
{
    // before parsing the email, we can actually check the session
    // since we have the identifier in the column key
    //
    QString const key(QString::fromUtf8(column_key.data()));
    snap_string_list const key_parts(key.split('/')); // TODO: could the domain include '/' too?
    QString const session_id(key_parts.last());
    sessions::sessions::session_info info;
    sessions::sessions::instance()->load_session(session_id, info, false);
    if(info.get_session_type() != sessions::sessions::session_info::session_info_type_t::SESSION_INFO_VALID)
    {
        // this session is not valid, ignore the request altogether
        //
        return;
    }

    // retrieve the user email address and verify the status
    // if the status is not satisfactory, ignore the information
    // (we do not really need it if the user is blocked)
    //
    QString const page_path(info.get_page_path());
    QString const object_path(info.get_object_path());
    if(!page_path.startsWith("/users/")
    || !object_path.startsWith("/email/"))
    {
        return;
    }
    users::users * users_plugin(users::users::instance());
    QString const user_email(page_path.mid(7));
    QString status_key;
    users::users::user_info_t user_info(users_plugin->get_user_info_by_email(user_email));
    users::users::status_t const user_status(users_plugin->user_status_from_email(user_email, status_key));
    if(user_status != users::users::status_t::STATUS_VALID
    && user_status != users::users::status_t::STATUS_NEW
    && user_status != users::users::status_t::STATUS_AUTO
    && user_status != users::users::status_t::STATUS_PASSWORD
    && user_status != users::users::status_t::STATUS_UNKNOWN) // a status from another plugin than the "users" plugin
    {
        // user is blocked, not found, undefined...
        //
        return;
    }

    // the session is valid, retrieve the info from the email
    // (if we were called from reorganize_bounce_email() then we already have
    // the email in the e pointer, avoid re-parsing)
    //
    email em;
    if(e == nullptr)
    {
        if(!parse_email(bounce_report, em, true))
        {
            libdbproxy::table::pointer_t emails_table(get_emails_table());
            emails_table->getRow(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_FAILED))->getCell(column_key)->setValue(bounce_report);
            return;
        }
        e = &em;
    }

    QString notification;
    QString computer_diagnostic;
    QString arrival_date;
    QString diagnostic_code;

    int const max_attachment_count(e->get_attachment_count());
    for(int idx(0); idx < max_attachment_count; ++idx)
    {
        email::attachment & attachment(e->get_attachment(idx));
        QCaseInsensitiveString const content_description(attachment.get_header("Content-Description"));
        if(content_description == "Notification")
        {
            // this is the human message we want to display to the end user
            //
            // the email parser will take care of checking the charset and
            // do the necessary conversions and save the result in the
            // string... right now we are good with a QString::fromUtf8()
            //QString const content_type(attachment.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)));
            //snap_string_list content_type_parts(content_type.split(';'));
            //for(int j(1); j < content_type_parts.size(); ++j)
            //{
            //    content_type_parts[j] = content_type_parts[j].trimmed();
            //    if(content_type_parts[j].startsWith("charset="))
            //    {
            //    }
            //}
            QByteArray const data(attachment.get_data());
            notification = QString::fromUtf8(data.data());
        }
        else if(content_description == "Delivery report")
        {
            if(attachment.get_related_count() >= 2)
            {
                // I would imagine that it will not ever be swapped, although
                // we could test both related too for certain fields to know
                // what the order really is...
                //
                {
                    email::attachment const & reporting_mta(attachment.get_related(0));
                    arrival_date = reporting_mta.get_header("Arrival-Date");
                }

                {
                    email::attachment const & remote_mta(attachment.get_related(1));
                    diagnostic_code = remote_mta.get_header("Diagnostic-Code");
                    if(diagnostic_code.startsWith("smtp;"))
                    {
                        // we can get this code
                        //
                        computer_diagnostic = diagnostic_code.mid(5).trimmed();
                    }

                    // the status tells us whether the email was a total failure
                    // (5.x.y) or could have a chance later to work as expected
                    // (4.x.y). The code also appears in the Diagnostic-Code
                    // field but this one is already parsed out.
                    //
                    diagnostic_code = remote_mta.get_header("Status");
                }
            }
        }
    }

    // make sure we have at least a notification and a session identifier
    //
    if(notification.isEmpty())
    {
        notification = computer_diagnostic;
    }
    if(notification.isEmpty())
    {
        libdbproxy::table::pointer_t emails_table(get_emails_table());
        libdbproxy::value report;
        report.setStringValue(bounce_report);
        report.setTtl(86400 * 93); // keep for about 3 months
        emails_table->getRow(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_FAILED))->getCell(column_key)->setValue(report);
        SNAP_LOG_ERROR("could not parse message, it is missing a notification and/or a message identifier.");
        return;
    }

    // to keep the last 5 notifications, we copy the first four to the
    // next four and then save the new one as first
    //
    libdbproxy::value value;
    for(int i(4); i >= 0; --i)
    {
        // notification
        {
            QString const previous_bounce_notification_name(QString("%1%2").arg(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_NOTIFICATION)).arg(i));
            if(user_info.load_user_parameter(previous_bounce_notification_name, value))
            {
                QString const next_bounce_notification_name(QString("%1%2").arg(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_NOTIFICATION)).arg(i + 1));
                user_info.save_user_parameter(next_bounce_notification_name, value);
            }
        }

        // diagnostic code
        {
            QString const previous_bounce_diagnostic_name(QString("%1%2").arg(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_DIAGNOSTIC_CODE)).arg(i));
            if(user_info.load_user_parameter(previous_bounce_diagnostic_name, value))
            {
                QString const next_bounce_diagnostic_name(QString("%1%2").arg(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_DIAGNOSTIC_CODE)).arg(i + 1));
                user_info.save_user_parameter(next_bounce_diagnostic_name, value);
            }
        }

        // arrival date
        {
            QString const previous_bounce_arrival_date_name(QString("%1%2").arg(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_ARRIVAL_DATE)).arg(i));
            if(user_info.load_user_parameter(previous_bounce_arrival_date_name, value))
            {
                QString const next_bounce_arrival_date_name(QString("%1%2").arg(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_ARRIVAL_DATE)).arg(i + 1));
                user_info.save_user_parameter(next_bounce_arrival_date_name, value);
            }
        }

        // email
        {
            QString const previous_bounce_email_name(QString("%1%2").arg(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_EMAIL)).arg(i));
            if(user_info.load_user_parameter(previous_bounce_email_name, value))
            {
                QString const next_bounce_email_name(QString("%1%2").arg(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_EMAIL)).arg(i + 1));
                user_info.save_user_parameter(next_bounce_email_name, value);
            }
        }
    }

    int64_t arrival_date_us(0);
    if(!arrival_date.isEmpty())
    {
        QDateTime dt(QDateTime::fromString(arrival_date));
        if(dt.isValid())
        {
            // we want microseconds in our date, so save date x 1000
            //
            arrival_date_us = dt.toMSecsSinceEpoch() * 1000;
        }
    }
    if(arrival_date_us == 0)
    {
        // Arrival-Date was not defined or had an unsupported format then
        // use now
        //
        arrival_date_us = f_snap->get_start_date();
    }

    // save the new status
    {
        QString const bounce_notification_name(QString("%1%2").arg(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_NOTIFICATION)).arg(0));
        user_info.save_user_parameter(bounce_notification_name, notification);
    }

    {
        QString const bounce_diagnostic_name(QString("%1%2").arg(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_DIAGNOSTIC_CODE)).arg(0));
        user_info.save_user_parameter(bounce_diagnostic_name, diagnostic_code);
    }

    {
        QString const bounce_arrival_date_name(QString("%1%2").arg(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_ARRIVAL_DATE)).arg(0));
        user_info.save_user_parameter(bounce_arrival_date_name, arrival_date_us);
    }

    {
        // This is a reference to the email; we can find the email in the
        // "emails" table as: emails/<user email>/<object path>
        //
        QString const bounce_email_name(QString("%1%2").arg(get_name(name_t::SNAP_NAME_SENDMAIL_BOUNCED_EMAIL)).arg(0));
        // mid(7) to skip the "/email/" introducer, no need here
        user_info.save_user_parameter(bounce_email_name, object_path.mid(7));
    }
}


/** \brief Process all the emails received in Cassandra.
 *
 * This function goes through the list of "new" emails received
 * in the Cassandra cluster as the post_email() function deposits
 * them there.
 *
 * First, the emails are processed in memory and then saved in
 * each destination user's mailbox in Cassandra (all email
 * addresses exist in our database whether someone wants it or
 * not!) Finally, users who request to receive a copy or
 * notifications have those sent to their usual mailbox. A mailbox
 * can also be marked as a blackhole (destination does not
 * exist) or a "do not contact" mailbox (destination does not
 * want to receive anything from us.)
 *
 * Mailing lists are managed at the bext level. These lists are
 * used to transform a name in an email into a list of email addresses.
 * See the attach_email() function for more details.
 */
void sendmail::process_emails()
{
    // the site key defined in the email data does not include the slash
    // (see post_email() for proof)
    QString const site_key(f_snap->get_site_key());

    libdbproxy::table::pointer_t emails_table(get_emails_table());
    libdbproxy::row::pointer_t row(emails_table->getRow(get_name(name_t::SNAP_NAME_SENDMAIL_NEW)));
    row->clearCache();
    auto column_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
    column_predicate->setCount(100); // should this be a parameter?
    column_predicate->setIndex(); // behave like an index
    for(;;)
    {
        row->readCells(column_predicate);
        libdbproxy::cells const cells(row->getCells());
        if(cells.isEmpty())
        {
            // no more new emails
            //
            break;
        }
        // handle one batch
        //
        for(libdbproxy::cells::const_iterator c(cells.begin());
                c != cells.end();
                ++c)
        {
            // get the email from the database
            // we expect empty values once in a while because a dropCell() is
            // not exactly instantaneous in Cassandra
            //
            libdbproxy::cell::pointer_t cell(*c);
            libdbproxy::value const value(cell->getValue());
            bool done(false);
            if(!value.nullValue())
            {
                email e;
                e.unserialize(value.stringValue());
                if(site_key == e.get_site_key())
                {
                    // only process emails from this website, otherwise
                    // we can have problems (because we check whether the
                    // user is on the orange list before sending emails
                    // to him)
                    //
                    attach_email(e);
                    done = true;
                }
            }
            else
            {
                // it is invalid anyway
                //
                done = true;
            }
            if(done)
            {
                // we are done with that email, get rid of it
                //
                row->dropCell(cell->columnKey());
            }

            // quickly end this process if the user requested a stop
            //
            if(f_backend->stop_received())
            {
                // clean STOP
                //
                return;
            }
        }
    }
}


/** \brief Process one email.
 *
 * This function processes one email. This means changing each destination
 * found in the To: field with the corresponding list of users (in case the
 * name references a mailing list) and then sending the email to the user's
 * account.
 *
 * Note that at this point this process does not actually send any emails.
 * It merely posts them to each user. This allows us to avoid sending the
 * same user multiple times the same email, to group emails, send emails
 * to a given user at most once a day, etc.
 *
 * \param[in] e  The email to attach to a list of users.
 */
void sendmail::attach_email(email const & e)
{
    QString const to(e.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_TO)));

    // transform To: ... in a list of emails
    //
    tld_email_list list;
    if(list.parse(to.toUtf8().data(), 0) != TLD_RESULT_SUCCESS)
    {
        // Nothing we can do with those!? We should have erred when the
        // user specified this email address a long time ago.
        //
        return;
    }

    libdbproxy::table::pointer_t emails_table(get_emails_table());
    libdbproxy::row::pointer_t lists(emails_table->getRow(get_name(name_t::SNAP_NAME_SENDMAIL_LISTS)));

    // read all the emails
    //
    QString const & site_key(e.get_site_key());
    tld_email_list::tld_email_t m;
    bool is_list(false);
    while(list.next(m))
    {
        std::vector<tld_email_list::tld_email_t> emails;
        if(!m.f_email_only.empty())
        {
            QString const list_key(site_key + ": " + m.f_email_only.c_str());
            if(lists->exists(list_key))
            {
                // if the email is a list, we do not directly send to it
                //
                is_list = true;
                libdbproxy::value list_value(lists->getCell(list_key)->getValue());
                tld_email_list user_list;
                if(user_list.parse(to.toUtf8().data(), 0) == TLD_RESULT_SUCCESS)
                {
                    tld_email_list::tld_email_t um;
                    while(user_list.next(um))
                    {
                        // TODO
                        // what if um is the name of a list? We would
                        // have to add that to a list which itself
                        // gets processed (i.e. recursive adds)
                        //
                        emails.push_back(um);
                    }
                }
                // else ignore this error at this point...
            }
        }
        if(!is_list)
        {
            emails.push_back(m);
        }
        else
        {
            // reset for next iteration
            is_list = false;
        }
        if(!emails.empty())
        {
            // if the list is not empty, handle it!
            //
            for(auto const & it : emails)
            {
                // if groups are specified then the email address can be empty
                //
                if(!it.f_email_only.empty())
                {
                    email copy(e);
                    copy.add_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_TO), it.f_canonicalized_email.c_str());
                    attach_user_email(copy);
                }
            }
        }
    }
}


/** \brief Attach the specified email to the specified user.
 *
 * The specified email has an email address which is expected to be the
 * final destination (i.e. a user). This email is added to the user's email
 * account. It is then added to an index of emails that need to actually
 * be sent unless the user frequency parameter says that the email is only
 * to be registered in the system.
 *
 * \param[in] e  The email to attach to a user's account.
 */
void sendmail::attach_user_email(email const & e)
{
    // TODO the sendmail plugin cannot directly access the user's
    //      plugin if the user's plugin is to use the sendmail plugin
    //      (it may be easier to implement events to send emails
    //      from anyway and use that from the user plugin and avoid
    //      the sendmail reference in the user plugin)
    //
    //      this may have changed a bit since we now have a usersui
    //      plugin as well (TBD)

    // TBD: would we need to have a lock to test whether the user
    //      exists? since we are not about to add it ourselves, I
    //      do not think it is necessary
    //
    QString const to(e.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_TO)));
    tld_email_list list;
    if(list.parse(to.toUtf8().data(), 0) != TLD_RESULT_SUCCESS)
    {
        // this should never happen here
        //
        throw sendmail_exception_invalid_argument("To: field is not a valid email");
    }
    tld_email_list::tld_email_t m;
    if(!list.next(m))
    {
        throw sendmail_exception_invalid_argument("To: field does not include at least one email");
    }
    // Note: here the list of emails is always 1 item
    //
    users::users * users_plugin(users::users::instance());
    users::users::user_info_t user_info( users_plugin->get_user_info_by_email(m.f_email_only.c_str()) );

    // EX-164: if the email does not belong to a registered user, then create the key
    // based on the email only. Otherwise, we get a throw here.
    //
    QString const user_key( user_info.exists()
                            ? user_info.get_user_key()
                            : user_info.get_user_key(m.f_email_only.c_str())
                          );
#if 1
SNAP_LOG_TRACE  ("sendmail::attach_user_email(): email=")(m.f_email_only)
                (", user_info.get_identifier()=")(user_info.get_identifier())
                (", the user_key=")(user_key);
#endif
    libdbproxy::table::pointer_t emails_table(get_emails_table());
    libdbproxy::row::pointer_t row(emails_table->getRow(user_key)); // TODO: convert to using user identifier
    if( !user_info.exists() )
    {
        // the user does not yet exist, we only email people who have some
        // sort of account because otherwise we could not easily track
        // people's wishes (i.e. whether they do not want to receive our
        // emails); this system allows us to block all emails
        //
        QString reason;
        users::users::status_t status(users_plugin->register_user(m.f_email_only.c_str(), "!", reason));
        switch(status)
        {
        case users::users::status_t::STATUS_NEW:
            // TODO: Since we automatically created this account, change the
            //       status from NEW to AUTO...
            //
            break;

        // these are considered valid, but they should not occur since if
        // the account already had such a status we should not be in
        // this if() block in the first place...
        //
        case users::users::status_t::STATUS_VALID:
        case users::users::status_t::STATUS_AUTO:
        case users::users::status_t::STATUS_PASSWORD:
            break;

        default:
            // the email is not attached to a valid account, we cannot
            // send anything to anyone...
            //
            SNAP_LOG_ERROR("Could not create a new account for email \"")(m.f_email_only)("\" (")(reason)("). No email will be sent to that user.");
            return;

        }

        user_info = users_plugin->get_user_info_by_email(m.f_email_only.c_str());
    }

    // TODO: if the user is a placeholder (i.e. user changed his email
    //       address) then we need to get the new email address...

    // save the email for that user
    // (i.e. emails can be read from within the website)
    //
    QString const serialized_email(e.serialize());
    libdbproxy::value email_value;
    email_value.setStringValue(serialized_email);
    QString const unique_key(e.get_email_key());
    row->getCell(unique_key + "::" + get_name(name_t::SNAP_NAME_SENDMAIL_EMAIL))->setValue(email_value);
    libdbproxy::value status_value;
    status_value.setStringValue(get_name(name_t::SNAP_NAME_SENDMAIL_STATUS_NEW));
    row->getCell(unique_key + "::" + get_name(name_t::SNAP_NAME_SENDMAIL_STATUS))->setValue(status_value);
    libdbproxy::value sent_value;
    sent_value.setStringValue(get_name(name_t::SNAP_NAME_SENDMAIL_STATUS_NEW));
    row->getCell(unique_key + "::" + get_name(name_t::SNAP_NAME_SENDMAIL_SENDING_STATUS))->setValue(sent_value);
    int64_t const start_date(f_snap->get_start_date());
    row->getCell(unique_key + "::" + get_name(name_t::SNAP_NAME_SENDMAIL_CREATED))->setValue(start_date);

    // try to retrieve the mail frequency the user likes, but first check
    // whether this email address was assign one because if so it overrides
    // the user's choice; also the programmer can assign one to the email,
    // but that will be ignored if the user defined his own frequency
    //
    libdbproxy::value freq_value(row->getCell(get_name(name_t::SNAP_NAME_SENDMAIL_FREQUENCY))->getValue());
    if(freq_value.nullValue())
    {
        freq_value = user_info.get_value(get_name(name_t::SNAP_NAME_SENDMAIL_FREQUENCY));
        if(freq_value.nullValue())
        {
            // programmer defined a frequency parameter in the email?
            // (this is NOT a header because we do not want to forward
            // that in the email itself)
            //
            QString const email_freq(e.get_parameter(get_name(name_t::SNAP_NAME_SENDMAIL_EMAIL_FREQUENCY)));
            freq_value.setStringValue(email_freq);
        }
    }

    char const * immediate(get_name(name_t::SNAP_NAME_SENDMAIL_FREQUENCY_IMMEDIATE));
    QString frequency(immediate);
    if(!freq_value.nullValue())
    {
        frequency = freq_value.stringValue();
    }

    // default date for immediate emails
    //
    time_t unix_date(time(nullptr));

    // programmer may have added an offset to the default date
    //
    QString const minimum_time(e.get_parameter(get_name(name_t::SNAP_NAME_SENDMAIL_MINIMUM_TIME)));
    if(!minimum_time.isEmpty())
    {
        bool ok(false);
        int const time_offset(minimum_time.toInt(&ok, 10));
        if(ok && time_offset >= 0 && time_offset <= 366 * 24 * 60 * 60)
        {
            unix_date += time_offset;
        }
        else
        {
            SNAP_LOG_ERROR("Minimum time \"")(minimum_time)("\" is not a valid offset. It has to be a positive integer or be undefined (default is 0).");
        }
    }
    int const minimum_date(unix_date);

    // calculate the maximum time
    //
    QString const maximum_time(e.get_parameter(get_name(name_t::SNAP_NAME_SENDMAIL_MAXIMUM_TIME)));
    int time_limit(unix_date + 366 * 24 * 60 * 60); // 1 year max. by default
    if(!maximum_time.isEmpty())
    {
        bool ok(false);
        int const limit(maximum_time.toInt(&ok, 10));
        if(ok && limit >= 0)
        {
            time_limit = unix_date + limit;
        }
        else
        {
            SNAP_LOG_ERROR("Maximum time \"")(maximum_time)("\" is not a valid offset. It has to be a positive integer or be undefined (default is 1 year).");
        }
    }
    if(minimum_date > time_limit)
    {
        time_limit = minimum_date;
        SNAP_LOG_ERROR("Minimum time \"")(minimum_date)("\" is larger than maximum time \"")(time_limit)("\". Using minimum as both, minimum and maximum.");
    }

    // TODO: add user's timezone adjustment or the following math is wrong
    if(frequency != immediate)
    {
        struct tm t;
        gmtime_r(&unix_date, &t);
        t.tm_sec = 0;
        t.tm_min = 0;
        t.tm_hour = 10;
        if(frequency == get_name(name_t::SNAP_NAME_SENDMAIL_FREQUENCY_DAILY))
        {
            // tomorrow at 10am
            ++t.tm_mday;
        }
        else if(frequency == get_name(name_t::SNAP_NAME_SENDMAIL_FREQUENCY_WEEKLY))
        {
            // next Sunday at 10am
            t.tm_mday += 7 - t.tm_wday;
            // TODO: allow users to select the day of the week they prefer
        }
        else if(frequency == get_name(name_t::SNAP_NAME_SENDMAIL_FREQUENCY_MONTHLY))
        {
            // 1st of next month
            t.tm_mday = 1;
            t.tm_mday = 1;
            t.tm_mon += 1;
        }
        else
        {
            // TODO: warn about invalid value
            SNAP_LOG_WARNING("Unknown email frequency \"")(frequency)("\" for user \"")(user_key)("\", using daily.");
            ++t.tm_mday; // as DAILY
        }
        t.tm_isdst = 0; // mkgmtime() ignores DST... (i.e. UTC is not affected)
        unix_date = mkgmtime(&t);

        // TODO: apply user's locale
        //
        // There is some code that is used in other places to tweak the
        // locale. We may want to look at the having a way to specify
        // the user and setup the locale for that specific user. But
        // I'm not too sure right now how we can change our UTC date
        // to a user locale, but we'll have to test all of that.
        //
        //  QLocale us_locale(QLocale::English, QLocale::UnitedStates);
        //  QString date(us_locale.toString(expires, "ddd, dd MMM yyyy hh:mm:ss' GMT'"));
        //
        //{
        //    content::content * content_plugin(content::content::instance());
        //    libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());

        //    content::path_info_t user_ipath;
        //    user_ipath.set_path(user_path);

        //    libdbproxy::row::pointer_t revision_row(revision_table->getRow(user_ipath.get_revision_key()));
        //    QString const user_locale(revision_row->getCell(get_name(name_t::SNAP_NAME_USERS_LOCALE))->getValue().stringValue());
        //    if(!user_locale.isEmpty())
        //    {
        //        locale::locale::instance()->set_current_locale(user_locale);
        //    }
        //}
    }

    // no matter what we cannot go over the time_limit
    if(unix_date > time_limit)
    {
        unix_date = time_limit;
    }

    QString const index_key(QString("%1::%2").arg(unix_date, 16, 16, QLatin1Char('0')).arg(user_key));

    libdbproxy::value index_value;
    char const * index(get_name(name_t::SNAP_NAME_SENDMAIL_INDEX));
    if(emails_table->exists(index))
    {
        // the index already exists, check to see whether that cell exists
        if(emails_table->getRow(index)->exists(index_key))
        {
            // it exists, we need to concatenate the values
            index_value = emails_table->getRow(index)->getCell(index_key)->getValue();
        }
    }
    if(!index_value.nullValue())
    {
        index_value.setStringValue(index_value.stringValue() + "," + unique_key);
    }
    else
    {
        index_value.setStringValue(unique_key);
    }
    emails_table->getRow(index)->getCell(index_key)->setValue(index_value);
}


/** \brief Go through the list of emails to send.
 *
 * This function goes through the '*index*' of emails that are ready to
 * be sent to end users. When emails are posted to the sendmail plugin,
 * they are added to a list with a date when they should be sent.
 */
void sendmail::run_emails()
{
    libdbproxy::table::pointer_t emails_table(get_emails_table());
    const char * index(get_name(name_t::SNAP_NAME_SENDMAIL_INDEX));
    libdbproxy::row::pointer_t row(emails_table->getRow(index));
    row->clearCache();
    auto column_predicate = std::make_shared<libdbproxy::cell_range_predicate>();
    column_predicate->setStartCellKey("0");
    // we use +1 otherwise immediate emails are sent 5 min. later!
    time_t const unix_date(time(nullptr) + 1);
    QString const end(QString("%1").arg(unix_date, 16, 16, QLatin1Char('0')));
    column_predicate->setEndCellKey(end);
    column_predicate->setCount(100); // should this be a parameter?
    column_predicate->setIndex(); // behave like an index
    for(;;)
    {
        row->readCells(column_predicate);
        libdbproxy::cells const cells(row->getCells());
        if(cells.isEmpty())
        {
            break;
        }
        // handle one batch
        for(libdbproxy::cells::const_iterator c(cells.begin());
                c != cells.end();
                ++c)
        {
            // get the email from the database
            //
            // we expect empty values once in a while because a dropCell() is
            // not exactly instantaneous in Cassandra
            //
            libdbproxy::cell::pointer_t cell(*c);
            libdbproxy::value const value(cell->getValue());
            QString const column_key(cell->columnKey());
            if(!value.nullValue())
            {
                QString const key(column_key.mid(18));
                QString const unique_keys(value.stringValue());
                snap_string_list const list(unique_keys.split(","));
                int const max_emails(list.size());
                for(int i(0); i < max_emails; ++i)
                {
                    sendemail(key, list[i]);
                }
            }

            // we are done with that email, get rid of it from the index
            //
            row->dropCell(column_key);
        }
    }
}


///** \brief Copy the filename if defined.
// *
// * Check whether the filename is defined in the Content-Disposition
// * or the Content-Type fields and make sure to duplicate it in
// * both fields. This ensures that most email systems have access
// * to the filename.
// *
// * \note
// * The valid location of the filename is the Content-Disposition,
// * but it has been saved in the 'name' sub-field of the Content-Type
// * field and some tools only check that field.
// *
// * \param[in,out] attachment_headers  The headers to be checked for
// *                                    a filename.
// */
//void sendmail::copy_filename_to_content_type(email::header_map_t & attachment_headers)
//{
//    if(attachment_headers.contains(get_name(name_t::SNAP_NAME_SENDMAIL_CONTENT_DISPOSITION))
//    && attachment_headers.contains(snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)))
//    {
//        // both fields are defined, copy the filename as required
//        QString content_disposition(attachment_headers[get_name(name_t::SNAP_NAME_SENDMAIL_CONTENT_DISPOSITION)]);
//        QString content_type(attachment_headers[snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)]);
//
//        http_strings::WeightedHttpString content_disposition_subfields(content_disposition);
//        http_strings::WeightedHttpString content_type_subfields(content_type);
//
//        http_strings::WeightedHttpString::part_t::vector_t & content_disposition_parts(content_disposition_subfields.get_parts());
//        http_strings::WeightedHttpString::part_t::vector_t & content_type_parts(content_type_subfields.get_parts());
//
//        if(content_disposition_parts.size() > 0
//        && content_type_parts.size() > 0)
//        {
//            // we only use part 1 (there should not be more than one though)
//            //
//            QString const filename(content_disposition_parts[0].get_parameter("filename"));
//            if(!filename.isEmpty())
//            {
//                // okay, we found the filename in the Content-Disposition,
//                // copy that to the Content-Type
//                //
//                // Note: we always force the name parameter so if it was
//                //       already defined, we make sure it is the same as
//                //       in the Content-Disposition field
//                //
//                content_type_parts[0].add_parameter("name", filename);
//                attachment_headers[snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)] = content_type_subfields.to_string();
//            }
//            else
//            {
//                QString const name(content_type_parts[0].get_parameter("name"));
//                if(!name.isEmpty())
//                {
//                    // Somehow the filename is defined in the Content-Type field
//                    // so copy it to the Content-Disposition too (where it should be)
//                    //
//                    content_disposition_parts[0].add_parameter("filename", name);
//                    attachment_headers[get_name(name_t::SNAP_NAME_SENDMAIL_CONTENT_DISPOSITION)] = content_disposition_subfields.to_string();
//                }
//            }
//        }
//    }
//}


/** \brief This function actually sends the email.
 *
 * This function is the one that actually takes the email and sends it to
 * the destination. At this point it makes use of the sendmail tool.
 *
 * \todo
 * Compute all the main headers and then the whole body in memory first
 * then compute the signature (DKIM and maybe a DomainKey-Signature) so
 * that way we can confirm to the receiver that we sent that email.
 * Only we'd need to know how to offer the public key via DNS... Hmmm!
 *
 * \param[in] key  The key of the email being sent; i.e. the email in the To:
 *                 header
 * \param[in] unique_key  The email unique key.
 */
void sendmail::sendemail(QString const & key, QString const & unique_key)
{
    QString const sending_status(QString("%1::%2")
                                    .arg(unique_key)
                                    .arg(get_name(name_t::SNAP_NAME_SENDMAIL_SENDING_STATUS)));

    // first check the status to make sure it is to be sent
    libdbproxy::table::pointer_t emails_table(get_emails_table());
    libdbproxy::row::pointer_t row(emails_table->getRow(key));
    libdbproxy::value const sent_value(row->getCell(sending_status)->getValue());
    QString const sent_status(sent_value.stringValue());
    if(sent_status == get_name(name_t::SNAP_NAME_SENDMAIL_STATUS_SENT)
    || sent_status == get_name(name_t::SNAP_NAME_SENDMAIL_STATUS_FAILED)
    || sent_status == get_name(name_t::SNAP_NAME_SENDMAIL_STATUS_DELETED)
    || sent_status == get_name(name_t::SNAP_NAME_SENDMAIL_STATUS_UNSUBSCRIBED))
    {
        // email was already sent, not too sure why we are being called,
        // just ignore to avoid bothering the destination owner...
        //
        // also ignore if it failed (it probably would fail again) and
        // if it were deleted by a user
        //
        // TODO: a failed email may be reset to new by a user if so they
        //       wish so that way this process can try again (we will have
        //       to also put it in the list of new emails...); also in
        //       general it may be easier to just create a new email
        return;
    }

    // mark that the email is being processed, the LOADING status is
    // used to allow for a retry; if a well defined failure happens,
    // however, the status will change to FAILED and at that point
    // the system stops trying to send the email
    //
    libdbproxy::value sending_value;
    sending_value.setStringValue(get_name(name_t::SNAP_NAME_SENDMAIL_STATUS_LOADING));
    row->getCell(sending_status)->setValue(sending_value);

    libdbproxy::value email_data(row->getCell(unique_key + "::" + get_name(name_t::SNAP_NAME_SENDMAIL_EMAIL))->getValue());

    // we use f_email so that way we can generate the XML data
    // in the on_generate_main_content() function
    //
    f_email = email(); // reset f_email
    f_email.unserialize(email_data.stringValue());

    // force the Content-Type header
    //
    // TODO: long term this is probably wrong...
    //
    f_email.add_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER), "text/html; charset=\"utf-8\"");

    // although we could send emails to unsubscribe in clear (many do it)
    // it can be a privacy issue, so better encrypt the email! however,
    // we do not want to deal with encryption/decryption so instead we
    // use a session ID; the email "unsubscribe" feature will therefore
    // die after about 1 year
    //
    QString const to(f_email.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_TO)));

    {
        if(!validate_email(to, &f_email))
        {
            // marked as "invalid" from this or all websites
            // so we absolutely never send email to that user...
            //
            // note that example domains reach this case too... and those are
            // really not considered to be errors or invalid emails... but
            // as far as we are concerned here it looks invalid.
            //
            sending_value.setStringValue(get_name(name_t::SNAP_NAME_SENDMAIL_STATUS_INVALID));
            row->getCell(sending_status)->setValue(sending_value);
            SNAP_LOG_INFO("User \"")
                         (to)
                         ("\" has an email address which returned an unrecoverable 5XX error code or was blacklisted in some way. Email with key \"")
                         (unique_key)
                         ("\" will not be sent.");
            return;
        }
    }

    // TODO: look into whether we should have a way to setup the locale
    //       and timezone of a user without having to log the user in
    //       as we do here...
    //
    {
        // create a fake session so we can temporarily log this user
        // which means the locale and timezone can be setup for that user!
        // (i.e. destination user since that's whom we are sending the
        // email and thus his preferences apply)
        //
        sessions::sessions::session_info info;
        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_SECURE);
        info.set_session_id(1);
        info.set_plugin_owner(get_plugin_name()); // ourselves
        //info.set_page_path(); -- no path for emails
        info.set_object_path("/email-session/" + to); // save the email address; this is not a real path though
        info.set_user_agent(get_name(name_t::SNAP_NAME_SENDMAIL_USER_AGENT));
        info.set_time_limit(f_snap->get_start_time() + 86400);  // now + 1 day
        users::users * users_plugin(users::users::instance());
        bool ok(false);
        users::users::identifier_t const id( to.toLongLong(&ok) );
        if(!ok
        || !users_plugin->authenticated_user(id, &info))
        {
            SNAP_LOG_WARNING("User \"")(to)("\" could not be authenticated. The locale information will be set to the website locale.");
        }
    }

    locale::locale * locale_plugin(locale::locale::instance());
    locale_plugin->set_locale();
    locale_plugin->set_timezone();

    {
        sessions::sessions::session_info info;
        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_SECURE);
        info.set_session_id(SENDMAIL_SESSION_EMAIL_ENCRYPTION);
        info.set_plugin_owner(get_plugin_name()); // ourselves
        //info.set_page_path(); -- no path for emails
        info.set_object_path("/email-session/" + to); // save the email address; this is not a real path though
        info.set_user_agent(get_name(name_t::SNAP_NAME_SENDMAIL_USER_AGENT));
        info.set_time_to_live(86400 * 370);  // about 1 year
        f_email.add_parameter(get_name(name_t::SNAP_NAME_SENDMAIL_EMAIL_ENCRYPTION), sessions::sessions::instance()->create_session(info));
    }

    QString const & path(f_email.get_email_path());
    if(!path.isEmpty())
    {
        // TODO look how we want to setup the email: either all inline or
        //      with links back to the website (for images, CSS...) to
        //      inline, use the sub-attachment feature to embed the extra
        //      content in the email

        content::path_info_t ipath;
        ipath.set_path(path);
        if(ipath.has_revision())
        {
            QString const html_body(layout::layout::instance()->apply_layout(ipath, this));

            // the output only includes valid ASCII (controls + ' ' to '~')
            std::string const encoded_body(quoted_printable::encode(html_body.toUtf8().data(), quoted_printable::QUOTED_PRINTABLE_FLAG_LFONLY | quoted_printable::QUOTED_PRINTABLE_FLAG_NO_LONE_PERIOD));

            email::attachment html_body_attachment;
            QByteArray body_data;
            body_data.append(encoded_body.c_str(), static_cast<int>(encoded_body.length()));
            html_body_attachment.set_data(body_data, "text/html; charset=\"utf-8\"");
            html_body_attachment.add_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_CONTENT_TRANSFER_ENCODING), "quoted-printable");
            f_email.set_body_attachment(html_body_attachment);

            // Use the page title as the subject
            // (TBD: should the page title always overwrite the subject?)
            if(f_email.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_SUBJECT)).isEmpty())
            {
                // TODO: apply safety filters on the subject
                content::content *c(content::content::instance());
                f_email.set_subject(c->get_content_parameter(ipath, content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE), content::content::param_revision_t::PARAM_REVISION_REVISION).stringValue());
            }
        }
        else
        {
            SNAP_LOG_ERROR("Page \"")(path)("\" was not found. Misspelled?");
        }
    }

    // verify that we have at least one attachment
    int const max_attachments(f_email.get_attachment_count());
    if(max_attachments < 1)
    {
        // this should never happen since this is tested in the post_email() function
        sending_value.setStringValue(get_name(name_t::SNAP_NAME_SENDMAIL_STATUS_FAILED));
        row->getCell(sending_status)->setValue(sending_value);
        SNAP_LOG_FATAL("No attachment, not even a body, so email ")(key)("/")(unique_key)(" cannot be sent");
        return;
    }

    // we want to transform the body from HTML to text ahead of time
    email::attachment body_attachment(f_email.get_attachment(0));
    // TODO: verify that the body is indeed HTML!
    //       although html2text works against plain text but that is a waste
    QString plain_text;
    QString const body_mime_type(body_attachment.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)));
    if(body_mime_type.mid(0, 9) == "text/html")
    {
        process p("html2text");
        p.set_mode(process::mode_t::PROCESS_MODE_INOUT);
        p.set_command("html2text");
        //p.add_argument("-help");
        p.add_argument("-nobs");
        p.add_argument("-utf8");
        p.add_argument("-style");
        p.add_argument("pretty");
        p.add_argument("-width");
        p.add_argument("70");
        std::string html_data;
        QByteArray data(body_attachment.get_data());
        // TODO: support other encoding, err if not supported
        if(body_attachment.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_CONTENT_TRANSFER_ENCODING)) == "quoted-printable")
        {
            // if it was quoted-printable encoded, we have to decode
            // (I know, we encode in this very function and could just
            // keep a copy of the original, HOWEVER, the end user could
            // build the whole email with this encoding already in place
            // and thus we anyway would have to decode... This being said,
            // we could have that as an optimization XXX)
            html_data = quoted_printable::decode(data.data());
        }
        else
        {
            html_data = data.data();
        }
        p.set_input(QString::fromUtf8(html_data.c_str()));
        int const r(p.run());
        if(r == 0)
        {
            plain_text = p.get_output();
        }
        else
        {
            SNAP_LOG_ERROR("An error occurred while executing html2text (exit code: ")(r)(")");
        }
    }

    tld_email_list list;
    if(list.parse(to.toUtf8().data(), 0) != TLD_RESULT_SUCCESS)
    {
        // this should never happen here
        sending_value.setStringValue(get_name(name_t::SNAP_NAME_SENDMAIL_STATUS_FAILED));
        row->getCell(sending_status)->setValue(sending_value);
        SNAP_LOG_FATAL("To: email address is considered invalid, email ")(key)("/")(unique_key)("  won't get sent");
        return;
    }
    tld_email_list::tld_email_t m;
    if(!list.next(m))
    {
        sending_value.setStringValue(get_name(name_t::SNAP_NAME_SENDMAIL_STATUS_FAILED));
        row->getCell(sending_status)->setValue(sending_value);
        SNAP_LOG_FATAL("To: email address does not return at least one email, email ")(key)("/")(unique_key)(" won't get sent");
        return;
    }

    // now we are starting to send the email to the system sendmail tool
    //
    sending_value.setStringValue(get_name(name_t::SNAP_NAME_SENDMAIL_STATUS_SENDING));
    row->getCell(sending_status)->setValue(sending_value);

    // XXX: capture the throw in case the pipe cannot be created?
    //
//    QString cmd("sendmail -f ");
//    cmd += f_email.get_header(get_name(name_t::SNAP_NAME_CORE_EMAIL_FROM));
//    cmd += " ";
//    cmd += m.f_email_only.c_str();
//    
//    SNAP_LOG_TRACE("sendmail command: [")(cmd)("]");
//
//    snap_pipe spipe(cmd, snap_pipe::mode_t::PIPE_MODE_IN);
//    std::ostream f(&spipe);

    // convert email data to text and send that to the sendmail command line
//    email::header_map_t headers(f_email.get_all_headers());

//    const bool body_only(max_attachments == 1 && plain_text.isEmpty());
//    QString boundary;
//    if(body_only)
//    {
//        // if the body is by itself, then its encoding needs to be transported
//        // to the main set of headers
//        if(body_attachment.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_CONTENT_TRANSFER_ENCODING)) == "quoted-printable")
//        {
//            headers[snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_CONTENT_TRANSFER_ENCODING)] = "quoted-printable";
//        }
//    }
//    else
//    {
//        // boundary      := 0*69<bchars> bcharsnospace
//        // bchars        := bcharsnospace / " "
//        // bcharsnospace := DIGIT / ALPHA / "'" / "(" / ")" /
//        //                  "+" / "_" / "," / "-" / "." /
//        //                  "/" / ":" / "=" / "?"
//        //
//        // Note: we generate boundaries without special characters
//        //       (and especially no spaces or dashes) to make it simpler
//        //
//        // Note: the boundary starts wity "=S" which is not a valid
//        //       quoted-printable sequence of characters (on purpose)
//        //
//        const char allowed[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"; //'()+_,./:=?";
//        boundary = "=Snap.Websites=";
//        for(int i(0); i < 20; ++i)
//        {
//            // this is just for boundaries, so rand() is more than enough
//            // it just needs to be unique
//            int const c(static_cast<int>(rand() % (sizeof(allowed) - 1)));
//            boundary += allowed[c];
//        }
//        headers[snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)] = "multipart/mixed;\n  boundary=\"" + boundary + "\"";
//        headers[snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_MIME_VERSION)] = "1.0";
//    }

    // we need the database and pluginsto generate the message ID so we
    // do it here
    //
    if(!f_email.has_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_MESSAGE_ID)))
    {
        // if the message identifier was not created by the user, we want
        // to create it ourselves for tracking purposes (in case we receive
        // a returned message with that same ID within the next few days)
        //
        // long term goal is to get postfix to tell us whenever an email is
        // returned and have Snap! Websites mark the conresponding emails
        // as not received; also that can allow us to stop sending emails to
        // the same address if it always fails...

        // we use a "secure" ID because they are bigger; a USER one would
        // probably be more than enough though
        sessions::sessions::session_info info;
        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_SECURE);
        info.set_session_id(SENDMAIL_SESSION_ID_MESSAGE);
        info.set_plugin_owner(get_plugin_name()); // ourselves
        info.set_page_path("/users/" + to); // save the user email address for bounces and to know whether the user opened his email or clicked on a link...
        info.set_object_path("/email/" + f_email.get_email_key()); // save the email key; this is not a real path though
        info.set_user_agent(get_name(name_t::SNAP_NAME_SENDMAIL_USER_AGENT));
        info.set_time_to_live(86400 * 30);  // 30 days
        QString const message_id(sessions::sessions::instance()->create_session(info));

        f_email.add_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_MESSAGE_ID),
                           QString("<%1.snapwebsites@%2>")
                                    .arg(message_id)
                                    .arg(f_snap->get_website_key()));
    }

    // add an unsubscribe link if there isn't one yet
    //
    // the email address defines "an account" so we always have a place
    // where we can write whether the email is unsubscribed or not
    //
    if(!f_email.has_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_LIST_UNSUBSCRIBE)))
    {
        f_email.add_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_LIST_UNSUBSCRIBE),
                           QString("%1unsubscribe/%2")
                                    .arg(f_snap->get_site_key_with_slash())
                                    .arg(f_email.get_parameter(get_name(name_t::SNAP_NAME_SENDMAIL_EMAIL_ENCRYPTION))));
    }

//    for(email::header_map_t::const_iterator it(headers.begin());
//                                            it != headers.end();
//                                            ++it)
//    {
//        // TODO: the it.value() needs to be URI encoded to be valid
//        //       in an email; if some characters appear that need
//        //       encoding, we should err (we probably want to
//        //       capture those in the add_header() though)
//        //
//        f << it.key() << ": " << it.value() << std::endl;
//    }
//
//    // XXX: allow administrators to change the `branding` flag?
//    //
//    f << "X-Generated-By: Snap! Websites C++ v" SNAPWEBSITES_VERSION_STRING " (https://snapwebsites.org/)" << std::endl
//      << "X-Mailer: Snap! Websites C++ v" SNAPWEBSITES_VERSION_STRING " (https://snapwebsites.org/)" << std::endl
//      << std::endl;
//
//    if(body_only)
//    {
//        // in this case we only have one entry, probably HTML, and thus we
//        // can avoid the multi-part headers and attachments
//        email::email_attachment attachment(f_email.get_attachment(0));
//        f << attachment.get_data().data() << std::endl;
//    }
//    else
//    {
//        int i(0);
//        if(!plain_text.isEmpty())
//        {
//            // if we have plain text then we have alternatives
//            f << "--" << boundary << std::endl
//              << "Content-Type: multipart/alternative;" << std::endl
//              << "  boundary=\"" << boundary << ".msg\"" << std::endl
//              << std::endl
//              << "--" << boundary << ".msg" << std::endl
//              << "Content-Type: text/plain; charset=\"utf-8\"" << std::endl
//              //<< "MIME-Version: 1.0" << std::endl -- only show this one in the main header
//              << "Content-Transfer-Encoding: quoted-printable" << std::endl
//              << "Content-Description: Mail message body" << std::endl
//              << std::endl
//              << quoted_printable::encode(plain_text.toUtf8().data(), quoted_printable::QUOTED_PRINTABLE_FLAG_NO_LONE_PERIOD) << std::endl;
//            if(i < max_attachments)
//            {
//                // now include the HTML
//                email::email_attachment html_attachment(f_email.get_attachment(0));
//                f << "--" << boundary << ".msg" << std::endl;
//                email::header_map_t attachment_headers(html_attachment.get_all_headers());
//                for(email::header_map_t::const_iterator it(attachment_headers.begin());
//                                                        it != attachment_headers.end();
//                                                        ++it)
//                {
//                    f << it.key() << ": " << it.value() << std::endl;
//                }
//
//                // one empty line before the contents
//                // here the data is already be encoded
//                f << std::endl
//                  << html_attachment.get_data().data() << std::endl
//                  << "--" << boundary << ".msg--" << std::endl
//                  << std::endl;
//
//                // we used "attachment" 0, so print the others starting at 1
//                i = 1;
//            }
//        }
//        // note that we send ALL the attachments, including attachment 0 since
//        // if we converted the HTML to plain text, we still want to send the
//        // HTML to the user
//        for(; i < max_attachments; ++i)
//        {
//            email::email_attachment attachment(f_email.get_attachment(i));
//            f << "--" << boundary << std::endl;
//            email::header_map_t & attachment_headers(attachment.get_all_headers());
//            copy_filename_to_content_type(attachment_headers);
//            for(email::header_map_t::const_iterator it(attachment_headers.begin());
//                                                    it != attachment_headers.end();
//                                                    ++it)
//            {
//                f << it.key() << ": " << it.value() << std::endl;
//            }
//
//            // one empty line before the contents
//            f << std::endl;
//
//            // here the data is already be encoded
//            f << attachment.get_data().data() << std::endl;
//        }
//        f << "--" << boundary << "--" << std::endl;
//    }
//
//    // end the message
//    f << std::endl
//      << "." << std::endl;

    // send the email now
    //
    if(!f_email.send())
    {
        sending_value.setStringValue(get_name(name_t::SNAP_NAME_SENDMAIL_STATUS_FAILED));
        row->getCell(sending_status)->setValue(sending_value);
        SNAP_LOG_FATAL("Pipe to sendmail failed, email ")(key)("/")(unique_key)(" will not get sent.");
        return;
    }

    // now it is marked as fully sent
    sending_value.setStringValue(get_name(name_t::SNAP_NAME_SENDMAIL_STATUS_SENT));
    row->getCell(sending_status)->setValue(sending_value);
}


/** \brief Add sendmail specific tags to the layout DOM.
 *
 * This function adds different sendmail specific tags to the layout page
 * and body XML documents. Especially, it will add all the parameters that
 * the user added to the email object before calling the post_email()
 * function.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void sendmail::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    locale::locale * locale_plugin(locale::locale::instance());

    // by default an email is just like a regular page
    output::output::instance()->on_generate_main_content(ipath, page, body);

    // but we also have email specific parameters we want to add
    QDomDocument doc(page.ownerDocument());

    {
        QDomElement sendmail_tag(doc.createElement("sendmail"));
        body.appendChild(sendmail_tag);

        // /snap/page/body/sendmail/from
        {
            QDomElement from(doc.createElement("from"));
            sendmail_tag.appendChild(from);
            QString const from_email(f_email.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_FROM)));
            QDomText from_text(doc.createTextNode(from_email));
            from.appendChild(from_text);
            // TODO: parse the email address with libtld and offer:
            //         sender-name
            //         sender-email
        }
        // /snap/page/body/sendmail/to
        {
            QDomElement to(doc.createElement("to"));
            sendmail_tag.appendChild(to);
            QString const to_email(f_email.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_TO)));
            QDomText to_text(doc.createTextNode(to_email));
            to.appendChild(to_text);
        }
        // /snap/page/body/sendmail/path
        {
            QDomElement path_tag(doc.createElement("path"));
            sendmail_tag.appendChild(path_tag);
            QDomText path_text(doc.createTextNode(f_email.get_email_path()));
            path_tag.appendChild(path_text);
        }
        // /snap/page/body/sendmail/key
        {
            QDomElement key(doc.createElement("key"));
            sendmail_tag.appendChild(key);
            QDomText key_text(doc.createTextNode(f_email.get_email_key()));
            key.appendChild(key_text);
        }
        // /snap/page/body/sendmail/created
        QString const created_date(locale_plugin->format_date(f_email.get_time()));
        QString const created_time(locale_plugin->format_time(f_email.get_time()));
        {
            QDomElement time_tag(doc.createElement("created"));
            sendmail_tag.appendChild(time_tag);
            QDomText time_text(doc.createTextNode(QString("%1 %2").arg(created_date).arg(created_time)));
            time_tag.appendChild(time_text);
        }
        // /snap/page/body/sendmail/date
        {
            QDomElement time_tag(doc.createElement("date"));
            sendmail_tag.appendChild(time_tag);
            QDomText time_text(doc.createTextNode(created_date));
            time_tag.appendChild(time_text);
        }
        // /snap/page/body/sendmail/time
        {
            QDomElement time_tag(doc.createElement("time"));
            sendmail_tag.appendChild(time_tag);
            QDomText time_text(doc.createTextNode(created_time));
            time_tag.appendChild(time_text);
        }
        // /snap/page/body/sendmail/attachment-count
        {
            QDomElement time_tag(doc.createElement("attachment-count"));
            sendmail_tag.appendChild(time_tag);
            QDomText time_text(doc.createTextNode(QString("%1").arg(f_email.get_attachment_count())));
            time_tag.appendChild(time_text);
        }
        // /snap/page/body/sendmail/important
        {
            // save the priority as a name
            QDomElement important(doc.createElement("important"));
            sendmail_tag.appendChild(important);
            const QString important_email(f_email.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_IMPORTANCE)));
            QDomText important_text(doc.createTextNode(important_email));
            important.appendChild(important_text);
        }
        // /snap/page/body/sendmail/x-priority
        const QString x_priority(f_email.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_EMAIL_X_PRIORITY)));
        {
            // save the priority as a value + name between parenthesis
            QDomElement priority(doc.createElement("x-priority"));
            sendmail_tag.appendChild(priority);
            QDomText priority_text(doc.createTextNode(x_priority));
            priority.appendChild(priority_text);
        }
        // /snap/page/body/sendmail/priority
        {
            // save the priority as a value
            QDomElement priority(doc.createElement("priority"));
            sendmail_tag.appendChild(priority);
            snap_string_list value_name(x_priority.split(" "));
            QDomText priority_text(doc.createTextNode(value_name[0]));
            priority.appendChild(priority_text);
        }
        // /snap/page/body/sendmail/parameters/param[name=...][value=...]
        const email::parameter_map_t& parameters(f_email.get_all_parameters());
        if(parameters.size() > 0)
        {
            QDomElement parameters_tag(doc.createElement("parameters"));
            sendmail_tag.appendChild(parameters_tag);
            //for(email::parameter_map_t::const_iterator it(parameters.begin());
            //                                        it != parameters.end();
            //                                        ++it)
            for(auto const it : parameters)
            {
                QDomElement param_tag(doc.createElement("param"));
                param_tag.setAttribute("name", it.first);
                param_tag.setAttribute("value", it.second);
                parameters_tag.appendChild(param_tag);
            }
        }
    }
}


/** \brief Replace a token with a corresponding value.
 *
 * This function replaces the user tokens with their value. For most
 * the values were already computed in the XML document, so all we have
 * to do is query the XML and return the corresponding value.
 *
 * The supported tokens are listed below. Parameters written between
 * double quotes are optional. The name of the parameter is not required
 * if written in order.
 *
 * \li [sendmail::forgot_password_link([text="anchor text"])]
 * \li [sendmail::unsubscribe_link([text="anchor text"])]
 * \li [sendmail::verify_link([text="anchor text"])]
 * \li [sendmail::from]
 * \li [sendmail::to]
 * \li [sendmail::path]
 * \li [sendmail::key]
 * \li [sendmail::created]
 * \li [sendmail::date]
 * \li [sendmail::time]
 * \li [sendmail::attachment_count]
 * \li [sendmail::priority]
 * \li [sendmail::parameter(name="parameter name")]
 *
 * \param[in,out] ipath  The path to the page being worked on.
 * \param[in,out] xml  The XML document used with the layout.
 * \param[in,out] token  The token object, with the token name and optional parameters.
 */
void sendmail::on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token)
{
    NOTUSED(ipath);

    if(!token.is_namespace("sendmail::"))
    {
        return;
    }

    if(token.is_token("sendmail::forgot_password_link"))
    {
        QString identifier;
        QDomXPath dom_xpath;
        dom_xpath.setXPath(QString("/snap/page/body/sendmail/parameters/param[@name=\"%1\"]/@value")
                           .arg(users::get_name(users::name_t::SNAP_NAME_USERS_FORGOT_PASSWORD_EMAIL)
                           ));
        QDomXPath::node_vector_t result(dom_xpath.apply(xml));
        if(result.size() > 0 && result[0].isAttr())
        {
            identifier = "/" + result[0].toAttr().value();
        }
        QString anchor_text("Click here to change your password");
        if(token.verify_args(0, 1) && token.f_parameters.size() >= 1)
        {
            filter::filter::parameter_t param(token.get_arg("text", 0, filter::filter::token_t::TOK_STRING));
            if(!token.f_error)
            {
                anchor_text = param.f_value;
            }
        }
        token.f_replacement = "<a href=\"" + f_snap->get_site_key_with_slash() + "new-password" + identifier + "\">" + anchor_text + "</a>";
    }
    else if(token.is_token("sendmail::unsubscribe_link"))
    {
        // this code is part of the low level unsubscript link handling
        // so it stays here instead of going to plugins/info/unsubscribe.cpp
        //
        QString user_email;
        QDomXPath dom_xpath;
        dom_xpath.setXPath(QString("/snap/page/body/sendmail/parameters/param[@name=\"%1\"]/@value")
                           .arg(get_name(name_t::SNAP_NAME_SENDMAIL_EMAIL_ENCRYPTION)));
        QDomXPath::node_vector_t result(dom_xpath.apply(xml));
        if(result.size() > 0 && result[0].isAttr())
        {
            // this is actually an "encrypted" user email
            user_email = "/" + result[0].toAttr().value();
        }
        // TODO: change "Snap! Websites" with the name of the website
        QString site_name(f_snap->get_site_parameter(get_name(snap::name_t::SNAP_NAME_CORE_SITE_NAME)).stringValue());
        if(site_name.isEmpty())
        {
            site_name = "Snap! Websites";
        }
        // TODO: translation
        QString anchor_text(QString("unsubscribe from %1 emails").arg(site_name));
        if(token.verify_args(0, 1) && token.f_parameters.size() >= 1)
        {
            filter::filter::parameter_t param(token.get_arg("text", 0, filter::filter::token_t::TOK_STRING));
            if(!token.f_error)
            {
                anchor_text = param.f_value;
            }
        }
        //
        // WARNING: "user_email" may be empty so we on purpose do NOT
        //          want a "/" after "unsubscribe"
        //
        token.f_replacement = "<a href=\"" + f_snap->get_site_key_with_slash() + "unsubscribe" + user_email + "\">" + anchor_text + "</a>";
    }
    else if(token.is_token("sendmail::verify_link"))
    {
        QString identifier;
        QDomXPath dom_xpath;
        dom_xpath.setXPath(QString("/snap/page/body/sendmail/parameters/param[@name=\"%1\"]/@value")
                           .arg(users::get_name(users::name_t::SNAP_NAME_USERS_VERIFY_EMAIL)));
        QDomXPath::node_vector_t result(dom_xpath.apply(xml));
        if(result.size() > 0 && result[0].isAttr())
        {
            identifier = "/" + result[0].toAttr().value();
        }
        QString anchor_text("Click here to verify your account");
        if(token.verify_args(0, 1) && token.f_parameters.size() >= 1)
        {
            filter::filter::parameter_t param(token.get_arg("text", 0, filter::filter::token_t::TOK_STRING));
            if(!token.f_error)
            {
                anchor_text = param.f_value;
            }
        }
        //
        // WARNING: "identifier" may be empty so we on purpose do NOT
        //          want a "/" after "verify"
        //
        token.f_replacement = "<a href=\"" + f_snap->get_site_key_with_slash() + "verify" + identifier + "\">" + anchor_text + "</a>";
    }
    else
    {
        QString xpath;
        if(token.is_token("sendmail::from"))
        {
            xpath = "/snap/page/body/sendmail/from";
        }
        else if(token.is_token("sendmail::to"))
        {
            xpath = "/snap/page/body/sendmail/to";
        }
        else if(token.is_token("sendmail::path"))
        {
            xpath = "/snap/page/body/sendmail/path";
        }
        else if(token.is_token("sendmail::key"))
        {
            xpath = "/snap/page/body/sendmail/key";
        }
        else if(token.is_token("sendmail::created"))
        {
            xpath = "/snap/page/body/sendmail/created";
        }
        else if(token.is_token("sendmail::date"))
        {
            xpath = "/snap/page/body/sendmail/date";
        }
        else if(token.is_token("sendmail::time"))
        {
            xpath = "/snap/page/body/sendmail/time";
        }
        else if(token.is_token("sendmail::attachment_count"))
        {
            xpath = "/snap/page/body/sendmail/attachment-count";
        }
        else if(token.is_token("sendmail::priority"))
        {
            xpath = "/snap/page/body/sendmail/x-priority";
        }
        else if(token.is_token("sendmail::parameter"))
        {
            if(token.verify_args(1, 1))
            {
                filter::filter::parameter_t param(token.get_arg("name", 0, filter::filter::token_t::TOK_STRING));
                if(!token.f_error)
                {
                    xpath = "/snap/page/body/sendmail/parameters/param[@name=\"" + param.f_value + "\"]/@value";
                }
            }
        }
        if(!xpath.isEmpty())
        {
            QDomXPath dom_xpath;
            dom_xpath.setXPath(xpath);
            QDomXPath::node_vector_t result(dom_xpath.apply(xml));
            if(result.size() > 0)
            {
                // apply the replacement
                if(result[0].isElement())
                {
                    // get the value between the tags
                    QDomDocument document;
                    QDomNode copy(document.importNode(result[0], true));
                    document.appendChild(copy);
                    token.f_replacement = document.toString(-1);
                }
                else if(result[0].isAttr())
                {
                    // get an attribute
                    token.f_replacement = result[0].toAttr().value();
                }
            }
        }
    }
}


void sendmail::on_token_help(filter::filter::token_help_t & help)
{
    help.add_token("sendmail::forgot_password_link",
        "Generate a link that can be used to go to the \"forgot"
        " password\" form. The anchor text can be defined using"
        " the first token parameter [text].");

    help.add_token("sendmail::unsubscribe_link",
        "Generate a link that can be used to go to the \"unsubscribe\" form."
        " The anchor text can be defined using the first token parameter"
        " [text].");

    help.add_token("sendmail::verify_link",
        "Generate a link that can be used to verify a user's email address."
        " The anchor text can be defined using the first token parameter"
        " [text].");

    help.add_token("sendmail::from",
        "Return the 'from' parameter of the email.");

    help.add_token("sendmail::to",
        "Return the 'to' parameter of the email.");

    help.add_token("sendmail::path",
        "Return the 'path' parameter of the email.");

    help.add_token("sendmail::key",
        "Return the 'key' parameter of the email.");

    help.add_token("sendmail::created",
        "Return the 'created' parameter of the email (The date and"
        " time when the email was created).");

    help.add_token("sendmail::date",
        "Return the 'date' parameter of the email (The date when the"
        " email was created).");

    help.add_token("sendmail::time",
        "Return the 'time' parameter of the email (The time when the"
        " email was created).");

    help.add_token("sendmail::attachment_count",
        "Return the 'attachment_count' parameter of the email which"
        " represents the total number of attachments (may be zero).");

    help.add_token("sendmail::priority",
        "Return the 'priority' parameter of the email.");

    help.add_token("sendmail::parameter",
        "Return the specified parameter of the email [name]. The parameter name is case sensitive.");
}



/** \brief Parse an email from plain text to an email object.
 *
 * This function transforms an email from a string to a
 * snap::snapsendmail::email object.
 *
 * The email may include a bounce header that we add in snapbounce.
 * In that case, make sure to set the \p bounce_email to true.
 * The bounce fields (appearing before the email) are added as
 * parameters to the \p e email passed in.
 *
 * The parser returns false if it finds something it does not
 * understand. In most cases an error message will be logged
 * when that happens.
 *
 * At this point we support several types of emails as follow:
 *
 * \li Plain text email
 *
 * This type of email has one header with Content-Type set to
 * text/plain or text/html. If Content-Type was not specified,
 * text/plain is assumed.
 *
 * This creates one attachment with the body of the email and
 * returns.
 *
 * \li Multipart email
 *
 * This type of email is certainly one of the most used email
 * now a day. It includes multiple parts describing the data
 * defined in the email.
 *
 * The structure of the input email looks like this:
 *
 * \code
 * - multipart/mixed
 *   - multipart/alternative
 *     - text/plain
 *     - multipart/related
 *       - text/html
 *       - image/jpg (Images used in text/html)
 *       - image/png
 *       - image/gif
 *       - text/css (the CSS used by the HTML)
 *   - application/pdf (PDF attachment)
 * \endcode
 *
 * We save both, the plain text and HTML content as an attachment. You may
 * want to drop the plain text if there is HTML...
 *
 * The resulting structure would be something like this:
 *
 * \code
 * - email
 *   - text attachment
 *   - HTML attachment
 *     - image/jpg related
 *     - image/png related
 *     - image/gif related
 *     - text/css related
 *   - application/pdf attachment
 * \endcode
 *
 * \li Report email -- from a bounced email
 *
 * In this case, the email buffer is a report. This means the Content-Type
 * of the main header is expected to be "multipart/report". Bounce emails
 * must be parsed by setting the \p bounce_email parameter to true.
 *
 * In this case we expected a very specific set of blocks that get parsed
 * in a very specific way. The resulting structure is expected to include
 * exactly three parts:
 *
 * 1) the notification, which is expected to be a human readable body of
 *    plain text.
 *
 * 2) the delivery report, which is expected to be an email header with
 *    information that the computer can analyze (i.e. the Final-Recipient
 *    field, for example.)
 *
 * 3) the original header, which is the header we had in our oringinal
 *    email, including our Message-ID field which references a Snap
 *    session linked to the user to whom we sent that email.
 *
 * The content of the delivery report and original header are parsed
 * before the function returned and saved in a related object. This
 * makes it very easy to retrieve the data later.
 *
 * \code
 * - email
 *   - notification attachment
 *   - delivery report attachment
 *     - Reporting-MTA
 *     - Final-Recipient
 *   - original header attachment
 *     - original header fields
 * \endcode
 *
 * The notification, delivery report, and original header attachment
 * may appear in any order. However, the Reporting-MTA and
 * Final-Recipient related parts are always expected to be in that
 * order. If you are looking for a specific field, you could still
 * search in both of these sets of header fields.
 *
 * \todo
 * The email is NOT cleared on entry. So if you loop over a set of
 * emails, make sure you create a new email object each time before
 * calling this function.
 *
 * \param[in] email_data  The string representing a complete email,
 *                        including attachments
 * \param[in,out] e  The email where the data is read.
 * \param[bool] bounce_email  Whether we are parsing a bounced email.
 *
 * \return true if the parser succeeded, false otherwise. If false is
 *         return, problem(s) with the email should have been logged.
 */
bool sendmail::parse_email(QString const & email_data, email & e, bool bounce_email)
{
    class parse_t
    {
    public:
        parse_t(QString const & email_data, email & e, bool bounce_email)
            : f_lines(email_data.split('\n'))
            , f_max_lines(f_lines.size())
            , f_email(e)
            , f_bounce_email(bounce_email)
        {
        }

        bool parse()
        {
            if(f_bounce_email)
            {
                // read all the fields ahead of the real thing, these
                // were added by our snapbounce utility
                //
                for(; f_line < f_max_lines && !f_lines[f_line].isEmpty(); ++f_line)
                {
                    QString name;
                    QString value;
                    if(!parse_one_field(name, value))
                    {
                        SNAP_LOG_ERROR("field parsing failed on bounced email");
                        return false;
                    }
                    // we save those as parameters to make sure we do not
                    // get them mixed with the email fields
                    //
                    // WARNING: parameters are case sensitive
                    //
                    f_email.add_parameter(name, value);
                }
                if(f_line < f_max_lines
                && f_lines[f_line].isEmpty())
                {
                    // skip the empty line
                    ++f_line;
                }
            }

            // the very first line of the email must be "From <address> <date>"
            // TODO: verify the format closer
            //
            if(f_line + 1 >= f_max_lines
            || !f_lines[f_line].startsWith("From "))
            {
                SNAP_LOG_ERROR("email does not start with \"From <email@address> <date>\", found \"")
                                (f_line >= f_max_lines ? "<empty>" : f_lines[f_line])("\" instead.");
                return false;
            }
            ++f_line;

            // read the main header
            //
            for(; f_line < f_max_lines && !f_lines[f_line].isEmpty(); ++f_line)
            {
                QString name;
                QString value;
                if(!parse_one_field(name, value))
                {
                    SNAP_LOG_ERROR("field parsing failed on main header");
                    return false;
                }
                f_email.add_header(name, value);
            }

            // determine the type of message from Content-Type
            //
            QString const content_type(f_email.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)));
            f_content_type_parameters = split_parameters(content_type);

            if(f_bounce_email)
            {
                return read_bounce_email();
            }
            else if(f_content_type_parameters[0] == "multipart/mixed")
            {
                // we support mixed emails: text and/or HTML and attachment(s)
                //
                return read_mixed_email();
            }
            else if(f_content_type_parameters[0] == "text/plain"
                 || f_content_type_parameters[0] == "text/html"
                 || f_content_type_parameters[0].isEmpty())
            {
                // direct text or HTML is fine too
                //
                return read_simple_email();
            }

            // anything else, we have no clue what to do at this time
            //
            SNAP_LOG_ERROR("unknown content type... \"")(f_content_type_parameters[0])("\"");
            return false;
        }

        bool parse_one_field(QString & name, QString & value)
        {
            if(f_line >= f_max_lines)
            {
                SNAP_LOG_ERROR("called parse_one_field() with f_line too large.");
                return false;
            }

            int const pos(f_lines[f_line].indexOf(':'));
            if(pos <= 0)
            {
                // we also see the case where the line start with ':'
                // as an error because in that case the name is empty
                //
                SNAP_LOG_ERROR("called parse_one_field() on a line without a ':' character: \"")(f_lines[f_line])("\".");
                return false;
            }

            // field names are case insensitive, which is taken
            // care of in the header map already
            //
            name = f_lines[f_line].mid(0, pos).trimmed();
            value = f_lines[f_line].mid(pos + 1).trimmed();

            // long line?
            //
            while(f_line + 1 < f_max_lines && !f_lines[f_line + 1].isEmpty() && f_lines[f_line + 1][0].isSpace())
            {
                // it is a long line, merge the data in one single long value
                //
                ++f_line;
                value += " ";
                value += f_lines[f_line].trimmed();
            }

            return true;
        }

        bool read_bounce_email()
        {
            // bounce emails must to be a report
            //
            if(f_content_type_parameters[0] != "multipart/report")
            {
                SNAP_LOG_ERROR("called read_bounce_email() but Content-Type is not \"multipart/report\", it is \"")(f_content_type_parameters[0])("\".");
                return false;
            }

            // check the report-type parameter
            int const max_content_type_parameters(f_content_type_parameters.size());
            for(int idx(1); idx < max_content_type_parameters; ++idx)
            {
                f_content_type_parameters[idx] = f_content_type_parameters[idx].trimmed();
                QString const report_type(f_content_type_parameters[idx].toLower());
                if(report_type.startsWith("report-type=delivery-status"))
                {
                    // retrieve the boundary
                    //
                    QString const boundary(get_boundary(f_content_type_parameters));
                    if(boundary.isEmpty())
                    {
                        SNAP_LOG_ERROR("boundary not found in the delivery-status section");
                        return false;
                    }

                    QString const end_boundary(boundary + "--");
                    do
                    {
                        email::attachment report;

                        // all good, go on with checking the report information
                        //
                        // read one part, no sub-part expected although we
                        // can parse the content
                        //
                        get_part_header(boundary, report);
                        QString const part_type(report.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)));
                        QVector<QCaseInsensitiveString> part_type_parameters(split_parameters(part_type));
                        part_type_parameters[0] = part_type_parameters[0].toLower();
                        // TBD: should we check Content-Description instead of Content-Type?
                        if(part_type_parameters[0] == "message/delivery-status"
                        || part_type_parameters[0] == "text/rfc822-headers")
                        {
                            // the data of the message delivery status
                            // is represented as two blocks of fields
                            //
                            // first we skip one line (empty line between header and content)
                            //
                            for(++f_line; f_line < f_max_lines && f_lines[f_line] != boundary && f_lines[f_line] != end_boundary;)
                            {
                                // the MTA report are just headers pre-parsed
                                email::attachment mta_report;
                                if(!get_part_data(boundary, mta_report))
                                {
                                    SNAP_LOG_ERROR("reading MTA report data fields failed");
                                    return false;
                                }
                                report.add_related(mta_report);
                            }
                        }
                        else //if(part_type_parameters[0] == "text/plain") -- any other part is read as is
                        {
                            // this is the human readable part of the message;
                            // text that explains why the email was returned;
                            // we save that data as the main body of the report
                            //
                            snap_string_list data;
                            if(!get_part_data(boundary, data))
                            {
                                SNAP_LOG_ERROR("reading MTA report notification failed");
                                return false;
                            }
                            QByteArray const body(data.join("\n").toUtf8());
                            report.set_data(body, part_type);
                        }

                        if(f_line >= f_max_lines)
                        {
                            SNAP_LOG_ERROR("reach end of report before the end boundary");
                            return false;
                        }

                        f_email.add_attachment(report);
                    }
                    while(f_lines[f_line] != end_boundary);

                    return true;
                }
            }

            SNAP_LOG_ERROR("delivery-status not found in this report");
            return false;
        }

        bool read_mixed_email()
        {
            // get the mixed boundary
            //
            QString const boundary(get_boundary(f_content_type_parameters));
            if(boundary.isEmpty())
            {
                SNAP_LOG_ERROR("no boundary defined in a mixed email");
                return false;
            }

            QString const end_boundary(boundary + "--");
            do
            {
                email::attachment a;
                if(!get_part_header(boundary, a))
                {
                    SNAP_LOG_ERROR("mixed email attachment header failed");
                    return false;
                }
                QString const attachment_type(a.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)));
                QVector<QCaseInsensitiveString> const attachment_type_parameters(split_parameters(attachment_type));

                // mixed is most often coming with alternatives (text and HTML)
                if(attachment_type_parameters[0].toLower() == "multipart/alternative")
                {
                    QString const alternative_boundary(get_boundary(attachment_type_parameters));
                    if(alternative_boundary.isEmpty())
                    {
                        SNAP_LOG_ERROR("alternative boundary count not be determined");
                        return false;
                    }

                    // read
                    QString const end_alternative_boundary(alternative_boundary + "--");
                    do
                    {
                        email::attachment alternative_attachment;
                        if(!get_part_header(alternative_boundary, alternative_attachment))
                        {
                            SNAP_LOG_ERROR("alternative attachment header failed");
                            return false;
                        }
                        QString const alternative_type(alternative_attachment.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)));
                        QVector<QCaseInsensitiveString> const alternative_type_parameters(split_parameters(alternative_type));
                        if(alternative_type_parameters[0].toLower() == "multipart/related")
                        {
                            // the text or html is the attachment
                            //
                            QString const related_boundary(get_boundary(alternative_type_parameters));
                            if(related_boundary.isEmpty())
                            {
                                SNAP_LOG_ERROR("boundary for related multipart failed");
                                return false;
                            }

                            QString const end_related_boundary(related_boundary + "--");
                            do
                            {
                                email::attachment related;
                                if(!get_part_header(related_boundary, related))
                                {
                                    SNAP_LOG_ERROR("related header could not be read");
                                    return false;
                                }
                                snap_string_list data;
                                if(!get_part_data(related_boundary, data))
                                {
                                    SNAP_LOG_ERROR("related data could not be read");
                                    return false;
                                }
                                QByteArray const body(data.join("\n").toUtf8());
                                related.set_data(body, related.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)));
                                alternative_attachment.add_related(related);

                                if(f_line >= f_max_lines)
                                {
                                    // end boundary missing
                                    SNAP_LOG_ERROR("related alternative not ending with the end boundary");
                                    return false;
                                }
                            }
                            while(f_lines[f_line] != end_related_boundary);
                        }
                        else
                        {
                            snap_string_list data;
                            if(!get_part_data(alternative_boundary, data))
                            {
                                SNAP_LOG_ERROR("alternative data not ended properly");
                                return false;
                            }
                            QByteArray const body(data.join("\n").toUtf8());
                            alternative_attachment.set_data(body, alternative_type);
                        }

                        if(f_line >= f_max_lines)
                        {
                            // end boundary missing
                            SNAP_LOG_ERROR("end alternative boundary not found");
                            return false;
                        }

                        f_email.add_attachment(alternative_attachment);
                    }
                    while(f_lines[f_line] != end_alternative_boundary);

                    // skip the end_alternative_boundary and move the
                    // cursor to the next boundary
                    //
                    for(++f_line; f_line < f_max_lines; ++f_line)
                    {
                        if(f_lines[f_line] == boundary
                        || f_lines[f_line] == end_boundary)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    // a regular attachment load it as is
                    //
                    snap_string_list data;
                    if(!get_part_data(boundary, data))
                    {
                        SNAP_LOG_ERROR("end boundary not found in attachment");
                        return false;
                    }
                    QByteArray const body(data.join("\n").toUtf8());
                    a.set_data(body, attachment_type);
                    f_email.add_attachment(a);
                }

                if(f_line >= f_max_lines)
                {
                    // end boundary missing
                    SNAP_LOG_ERROR("end boundary of mixed email not found");
                    return false;
                }
            }
            while(f_lines[f_line] != end_boundary);

            return true;
        }

        bool read_simple_email()
        {
            QString const content_type(f_email.get_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)));
            email::attachment a;
            QByteArray const body(static_cast<QStringList>(f_lines.mid(f_line)).join("\n").toUtf8());
            a.set_data(body, content_type);
            f_email.add_attachment(a);
            return true;
        }

        QString get_boundary(QVector<QCaseInsensitiveString> content_type_parameters)
        {
            // search the Content-Type field for a parameter named "boundary"
            //
            int const max_content_type_parameters(content_type_parameters.size());
            for(int idx(1); idx < max_content_type_parameters; ++idx)
            {
                if(content_type_parameters[idx].toLower().startsWith("boundary="))
                {
                    // got it, return that with the additional "--"
                    //
                    QString boundary(content_type_parameters[idx].mid(9));
                    if(boundary.isEmpty())
                    {
                        return QString();
                    }
                    if(boundary[0] == '"'
                    && boundary.at(boundary.length() - 1) == '"')
                    {
                        boundary = boundary.mid(1, boundary.length() - 2);
                    }
                    boundary = "--" + boundary;

                    // move the "cursor" to the first boundary; anything
                    // between here and the first boundary is ignored
                    //
                    for(; f_line < f_max_lines; ++f_line)
                    {
                        if(f_lines[f_line] == boundary)
                        {
                            return boundary;
                        }
                    }

                    // not even one boundary?!
                    return QString();
                }
            }

            // multi-part message without a boundary is considered invalid
            return QString();
        }

        bool get_part_header(QString const & boundary, email::attachment & a)
        {
            // make sure we are on a boundary (the get_boundary() moves the
            // cursor to that location for us)
            //
            if(f_line >= f_max_lines
            || f_lines[f_line] != boundary)
            {
                SNAP_LOG_ERROR("trying to read a mixed header without boundary \"")(boundary)("\" on line ")(f_line)(", but \"")(f_lines[f_line])("\".");
                return false;
            }

            // retrieve the header
            //
            QString const end_boundary(boundary + "--");
            for(++f_line; f_line < f_max_lines && !f_lines[f_line].isEmpty(); ++f_line)
            {
                if(f_lines[f_line] == boundary
                || f_lines[f_line] == end_boundary)
                {
                    // this is incorrect, we need to have at least one empty
                    // line to end the header
                    //
                    SNAP_LOG_ERROR("header ends with a boundary instead of an empty line");
                    return false;
                }

                QString name;
                QString value;
                parse_one_field(name, value);
                a.add_header(name, value);
            }

            return true;
        }

        bool get_part_data(QString const & boundary, email::attachment & sub_attachment)
        {
            QString const end_boundary(boundary + "--");
            for(; f_line < f_max_lines; ++f_line)
            {
                if(f_lines[f_line] == boundary
                || f_lines[f_line] == end_boundary)
                {
                    // this is the end of this sub-header!
                    //
                    return true;
                }
                if(f_lines[f_line].isEmpty())
                {
                    // skip all empty lines
                    //
                    for(++f_line;; ++f_line)
                    {
                        if(f_line >= f_max_lines)
                        {
                            // boundary missing
                            SNAP_LOG_ERROR("reached end of email before boundary or end boundary");
                            return false;
                        }
                        if(!f_lines[f_line].isEmpty())
                        {
                            break;
                        }
                    }
                    return true;
                }
                QString name;
                QString value;
                parse_one_field(name, value);
                sub_attachment.add_header(name, value);
            }

            // the data block was not ended by boundaries or an empty line...
            //
            SNAP_LOG_ERROR("sub-header did not end with a boundary limit");
            return false;
        }

        bool get_part_data(QString const & boundary, snap_string_list & data)
        {
            QString const end_boundary(boundary + "--");
            for(++f_line; f_line < f_max_lines; ++f_line)
            {
                if(f_lines[f_line] == boundary
                || f_lines[f_line] == end_boundary)
                {
                    // this is the end of this message!
                    //
                    if(data.last().isEmpty())
                    {
                        // remove the last line, it is there to make sure
                        // all systems can properly process a message
                        //
                        data.removeLast();
                    }
                    return true;
                }
                data << f_lines[f_line];
            }

            // the data block was not ended by boundaries...
            //
            SNAP_LOG_ERROR("end of file reached before data block end boundary");
            return false;
        }

        QVector<QCaseInsensitiveString> split_parameters(QString const & s)
        {
            QVector<QCaseInsensitiveString> result;
            int pos(0);
            for(;;)
            {
                int const next(s.indexOf(';', pos));
                if(next < 0)
                {
                    result << s.mid(pos).trimmed();
                    return result;
                }

                result << s.mid(pos, next - pos).trimmed();
                pos = next + 1;
            }
        }

    private:
        snap_string_list                f_lines = snap_string_list();
        int                             f_max_lines = 0;
        int                             f_line = 0;
        email &                         f_email;
        bool                            f_bounce_email = false;
        QVector<QCaseInsensitiveString> f_content_type_parameters = QVector<QCaseInsensitiveString>();
    };

    parse_t p(email_data, e, bounce_email);

    return p.parse();
}



SNAP_PLUGIN_END()

// Multipart emails are documented here
// http://tools.ietf.org/html/rfc2557

// There is an example of SMTP
// Actually we're under Linux and want to use sendmail instead (much easier!)
// http://stackoverflow.com/questions/9317305/sending-an-email-from-a-c-c-program-in-linux
//
// http://curl.haxx.se/libcurl/c/smtp-tls.html
// telnet mail.m2osw.com 25
// Trying 69.55.233.23...
// Connected to mail.m2osw.com.
// Escape character is '^]'.
// 220 mail.m2osw.com ESMTP Postfix (Made to Order Software Corporation)
// HELO mail.m2osw.com
// 250 mail.m2osw.com
// MAIL FROM: <alexis@m2osw.com>
// 250 2.1.0 Ok
// RCPT TO: <alexis_wilke@yahoo.com>
// 250 2.1.5 Ok
// DATA
// 354 End data with <CR><LF>.<CR><LF>
// From: <alexis@m2osw.com>
// To: <alexis_wilke@yahoo.com>
// Subject: Hello!
// 
// Testing SMTP really quick. We need to understand how to get the necessary info so it works.
// 
// .
// 250 2.0.0 Ok: queued as 9652742A0FC
// QUIT
// 221 2.0.0 Bye
// Connection closed by foreign host.


// vim: ts=4 sw=4 et
