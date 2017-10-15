// Snap Websites Server -- OAuth2 handling
// Copyright (C) 2012-2017  Made to Order Software Corp.
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

/** \file
 * \brief OAuth2 handling.
 *
 * This plugin handles authentication via OAuth2 by application that
 * want to access private features of a Snap! Website.
 *
 * This plugin does not offer any REST API by itself. Only an authentication
 * process.
 *
 * At this point we do not support OAuth2 as described in RFC 6749. We may
 * want to get closer to the reference with time. The fact is that the
 * current version works pretty well as it is.
 *
 * Not yet official OAuth2 reference:
 * https://tools.ietf.org/html/rfc6749
 */

#include "oauth2.h"

#include "../users/users.h"

#include <snapwebsites/http_strings.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(oauth2, 1, 0)



namespace
{

char const * g_oauth2_error_names[] =
{
    "invalid_request",
    "invalid_client",
    "invalid_grant",
    "unauthorized_client",
    "unauthorized_grant_type",
    "invalid_scope"
};

} // no name namespace


/** \brief Get a fixed oauth2 plugin name.
 *
 * The oauth2 plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_OAUTH2_EMAIL:
        return "oauth2::email";

    case name_t::SNAP_NAME_OAUTH2_ENABLE:
        return "oauth2::enable";

    case name_t::SNAP_NAME_OAUTH2_IDENTIFIER:
        return "oauth2::identifier";

    case name_t::SNAP_NAME_OAUTH2_IDENTIFIERS:
        return "*oauth2::identifier*";

    case name_t::SNAP_NAME_OAUTH2_SECRET:
        return "oauth2::secret";

    case name_t::SNAP_NAME_OAUTH2_USER_ENABLE:
        return "oauth2::user_enable";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_OAUTH2_...");

    }
    NOTREACHED();
}


/** \class oauth2
 * \brief The oauth2 plugin handles application authentication.
 *
 * Any Snap! website can be setup to accept application authentication.
 *
 * The website generates a token that can be used to log you in.
 */


/** \brief Initialize the oauth2 plugin.
 *
 * This function initializes the oauth2 plugin.
 */
oauth2::oauth2()
    //: f_snap(nullptr) -- auto-init
{
}

/** \brief Destroy the oauth2 plugin.
 *
 * This function cleans up the oauth2 plugin.
 */
oauth2::~oauth2()
{
}


/** \brief Get a pointer to the oauth2 plugin.
 *
 * This function returns an instance pointer to the oauth2 plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the oauth2 plugin.
 */
oauth2 * oauth2::instance()
{
    return g_plugin_oauth2_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString oauth2::settings_path() const
{
    return "/admin/settings/oauth2";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString oauth2::icon() const
{
    return "/images/oauth2/oauth2-logo-64x64.png";
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
QString oauth2::description() const
{
    return "The OAuth2 plugin offers an authentication mechanism to"
          " be used by all the other plugins that support a REST API."
          " The administrator of a website can decide whether to authorize"
          " such access or not.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString oauth2::dependencies() const
{
    return "|editor|layout|path|users|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last
 *                          updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t oauth2::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 12, 25, 0, 7, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the oauth2 plugin content.
 *
 * This function updates the contents in the database using the
 * system update settings found in the resources.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update
 *                                 (in micro-seconds).
 */
void oauth2::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Bootstrap the oauth2.
 *
 * This function adds the events the oauth2 plugin is listening for.
 *
 * \param[in] snap  The child handling this request.
 */
void oauth2::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN0(oauth2, "server", server, process_cookies);
    SNAP_LISTEN(oauth2, "content", content::content, create_content, _1, _2, _3);
}


/** \brief Called each time a page gets created.
 *
 * We use this signal to make sure that the OAuth2 identifier and secret
 * are defined. This will always happens after the settings page is created.
 */
void oauth2::on_create_content(content::path_info_t & ipath, QString const & owner, QString const & type)
{
    NOTUSED(type);

    if(owner != "output"
    || ipath.get_cpath() != "admin/settings/oauth2")
    {
        return;
    }

    struct secret_t
    {
        static void create(name_t n)
        {
            content::content * content_plugin(content::content::instance());
            QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());

            // make sure the secret does not include a ':' which is not
            // compatible with Basic Auth
            QString secret(users::users::create_password());
            for(;;)
            {
                secret.remove(':');
                if(secret.length() > 64)
                {
                    break;
                }
                QString const extra(users::users::create_password());
                secret += extra;
            }

            content::path_info_t ipath;
            ipath.set_path("admin/settings/oauth2");
            secret_table->row(ipath.get_key())->cell(get_name(n))->setValue(secret);
        }
    };

    secret_t::create(name_t::SNAP_NAME_OAUTH2_IDENTIFIER);
    secret_t::create(name_t::SNAP_NAME_OAUTH2_SECRET);
}


/** \brief Check for the "/user/oauth2" path.
 *
 * This function ensures that the URL is /user/oauth2 and if so write
 * checks that the application knows the identifier and secret of this
 * website and if so, return a session identifier that can be used to
 * further access the server including private pages.
 *
 * \param[in] ipath  The URL being managed.
 *
 * \return true if the authentication parameters were properly defined,
 *         an error is generated otherwise.
 */
bool oauth2::on_path_execute(content::path_info_t & ipath)
{
    if(ipath.get_cpath() != "user/oauth2")
    {
        return false;
    }

    SNAP_LOG_INFO("OAuth2 authorization request");

    f_snap->set_ignore_cookies();

    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    content::path_info_t settings_ipath;
    settings_ipath.set_path("admin/settings/oauth2");
    QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(settings_ipath.get_revision_key()));
    int8_t const enable(revision_row->cell(get_name(name_t::SNAP_NAME_OAUTH2_ENABLE))->value().safeSignedCharValue());
    if(!enable)
    {
        die(snap_child::http_code_t::HTTP_CODE_UNAUTHORIZED,
                    oauth2_error_t::OAUTH2_INVALID_REQUEST,
                    "Unauthorized Authentication",
                    "This website does not authorize OAuth2 authentications at the moment.",
                    "The OAuth2 system is currently disabled.",
                    "http://snapwebsites.org/implementation/feature-requirements/oauth2-core");
        NOTREACHED();
    }

    QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());
    QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(settings_ipath.get_key()));
    QString email(secret_row->cell(get_name(name_t::SNAP_NAME_OAUTH2_EMAIL))->value().stringValue());
    if(email.isEmpty())
    {
        die(snap_child::http_code_t::HTTP_CODE_UNAUTHORIZED,
                    oauth2_error_t::OAUTH2_INVALID_REQUEST,
                    "Invalid Settings",
                    "Your OAuth2 settings do not include a user email for us to log your application in.",
                    "The OAuth2 system is currently \"disabled\" because no user email was specified.",
                    "http://snapwebsites.org/implementation/feature-requirements/oauth2-core");
        NOTREACHED();
    }

    // Retrieve the Snap-Authorization Field
    //
    // Note:
    // We do not use the Authorization field because that field is
    // removed by Apache2 (at least when you run mod_auth_basic and
    // similar modules)
    QString const authorization(f_snap->snapenv("HTTP_SNAP_AUTHORIZATION"));
    snap_string_list const snap_base64(authorization.simplified().split(" "));
    if(snap_base64.size() != 2
    || snap_base64[0].toUpper() != "SNAP")
    {
        require_oauth2_login();
        die(snap_child::http_code_t::HTTP_CODE_UNAUTHORIZED,
                    oauth2_error_t::OAUTH2_INVALID_REQUEST,
                    "Unauthorized Method of Authentication",
                    "We only support the Snap authentication method.",
                    QString("The authorization did not have 2 parts (Snap and Secret) or the first is not \"Snap\" (\"%1\")")
                            .arg(snap_base64.size() == 2 ? snap_base64[0] : "undefined"),
                    "http://snapwebsites.org/implementation/feature-requirements/oauth2-core");
        NOTREACHED();
    }

    // Decrypt the buffer
    QByteArray const base64_buffer(QByteArray::fromBase64(snap_base64[1].toUtf8()));
    snap_string_list const identifier_secret(QString::fromUtf8(base64_buffer.data()).split(":"));
    if(identifier_secret.size() != 2)
    {
        require_oauth2_login();
        die(snap_child::http_code_t::HTTP_CODE_BAD_REQUEST,
                    oauth2_error_t::OAUTH2_INVALID_REQUEST,
                    "Invalid Authentication",
                    "The authentication identifier and secret codes are expected to include only one colon character.",
                    "The expected authorization \"id:secret\" not available.",
                    "http://snapwebsites.org/implementation/feature-requirements/oauth2-core");
        NOTREACHED();
    }

    users::users * users_plugin(users::users::instance());

    // Check validity (i.e. is the application logged in?)
    QString identifier(secret_row->cell(get_name(name_t::SNAP_NAME_OAUTH2_IDENTIFIER))->value().stringValue());
    QString secret(secret_row->cell(get_name(name_t::SNAP_NAME_OAUTH2_SECRET))->value().stringValue());

    if(identifier != identifier_secret[0]
    || secret     != identifier_secret[1])
    {
        users::users::user_info_t oauth2_user_info( users_plugin->get_user_info_by_name(get_name(name_t::SNAP_NAME_OAUTH2_IDENTIFIERS)) );
        //
        // XXX: should the oauth2_user_info be the '*identifier*' row or the one
        //      linked by the specified 'email'? (it is particularly
        //      important when we call the invalid_password() signal below)
        //
        bool invalid(true);
        int8_t const user_enable(revision_row->cell(get_name(name_t::SNAP_NAME_OAUTH2_USER_ENABLE))->value().safeSignedCharValue());
        if(user_enable)
        {
            // in this case we need to determine the secret from the user
            // account which is identifier by "identifier"
            //if(users_table->exists(get_name(name_t::SNAP_NAME_OAUTH2_IDENTIFIERS))
            if(oauth2_user_info.exists()
            && oauth2_user_info.value_exists(identifier_secret[0]))
            {
                // change the email to that user's email
                email = oauth2_user_info.get_value(identifier_secret[0]).stringValue(); //user_row->cell(identifier_secret[0])->value().stringValue();
                users::users::user_info_t user_info( users_plugin->get_user_info_by_email(email) );
                if(user_info.exists())
                {
                    // make sure user is currently valid otherwise it would
                    // be a way for a user to bypass being blocked!
                    QString ignore_status_key;
                    users::users::status_t const status(users_plugin->user_status_from_email(email, ignore_status_key));
                    if(status == users::users::status_t::STATUS_VALID
                    || status == users::users::status_t::STATUS_PASSWORD)
                    {
                        identifier = oauth2_user_info.get_value( get_name(name_t::SNAP_NAME_OAUTH2_IDENTIFIER)).stringValue();
                        secret     = oauth2_user_info.get_value( get_name(name_t::SNAP_NAME_OAUTH2_SECRET    )).stringValue();
                        invalid = identifier != identifier_secret[0]
                               || secret     != identifier_secret[1];
                    }
                }
            }
        }

        // if still not equal, the user credentials are not 100% valid
        if(invalid)
        {
            // TODO: determine whether using the same 'user_row' for the global and
            //       the user accounts is fine (i.e. I would imagine that this means
            //       any bad system can end up blocking all systems; a form a DoS
            //       for the API... which I think is better than not blocking anyone
            //       ever when a hacker is attempting to break our systems.)
            //
            users_plugin->invalid_password(oauth2_user_info, "oauth2");
            require_oauth2_login();
            die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN,
                        oauth2_error_t::OAUTH2_INVALID_REQUEST,
                        "Forbidden Authentication",
                        "Your OAuth2 identifier and secret do not match this website OAuth2 information.",
                        QString("Invalid%1%2.")
                                .arg(identifier != identifier_secret[0] ? " identifier" : "")
                                .arg(secret     != identifier_secret[1] ? " secret"     : ""),
                        "http://snapwebsites.org/implementation/feature-requirements/oauth2-core");
            NOTREACHED();
        }
    }

    // create a new user session since the username and password matched
    time_t login_limit(0);
    bool validation_required(false);
    QString const details(users_plugin->login_user(email, "", validation_required, users::users::login_mode_t::LOGIN_MODE_FULL, "oauth2"));
    QString session_id;
    if(details.isEmpty())
    {
        sessions::sessions::session_info const & info(users_plugin->get_session());
        session_id = QString("%1/%2")
                    .arg(info.get_session_key())
                    .arg(info.get_session_random());
        login_limit = info.get_administrative_login_limit();
    }
    else
    {
        SNAP_LOG_ERROR("Could not log in this application because the user attached to this website OAuth2 was not accepted. Details: ")(details);
    }

    // generate the result, an OAuth2 session
    // accept XML and JSON
    http_strings::WeightedHttpString encodings(f_snap->snapenv("HTTP_ACCEPT"));
    float const xml_level(encodings.get_level("application/xml"));
    float const json_level(encodings.get_level("application/json"));
    if(json_level > xml_level)
    {
        f_snap->output(QString(
            "{"
            "\"version\":\"" SNAPWEBSITES_VERSION_STRING "\","
            "\"oauth2\":\"%1.%2\","
            "\"result\":\"%3\""
            "%4"
            "%5"
            "%6"
            "}")
                .arg(get_major_version()).arg(get_minor_version())
                .arg(details.isEmpty() ? "success" : "failure")
                .arg(session_id.isEmpty() ? "" : QString(",\"session\":\"%1\",\"session_type\":\"Bearer\"").arg(session_id))
                .arg(details.isEmpty() ? "" : QString(",\"error\":\"%1\"")
                        .arg(validation_required ? "The account you chose as the OAuth2 account was not yet validated."
                                                 : "Your OAuth2 credentials were incorrect."))
                .arg(login_limit == 0 ? "" : QString(",\"timeout\":%1").arg(login_limit))
            );
    }
    else
    {
        f_snap->output(QString(
            "<?xml version=\"1.0\"?>"
            "<snap version=\"" SNAPWEBSITES_VERSION_STRING "\" oauth2=\"%1.%2\">"
                "<result>%3</result>"
                "%4"
                "%5"
                "%6"
            "</snap>")
                .arg(get_major_version()).arg(get_minor_version())
                .arg(details.isEmpty() ? "success" : "failure")
                .arg(session_id.isEmpty() ? "" : QString("<oauth2-session type=\"Bearer\">%1</oauth2-session>").arg(session_id))
                .arg(details.isEmpty() ? "" : QString("<error>%1</error>")
                        .arg(validation_required ? "The account you chose as the OAuth2 account was not yet validated."
                                                 : "Your OAuth2 credentials were incorrect."))
                .arg(login_limit == 0 ? "" : QString("<timeout>%1</timeout>").arg(login_limit))
            );
    }

    return true;
}


/** \brief An application may need to be logged in.
 *
 * This function checks whether the application is logged in or not.
 *
 * The login makes use of the session identifier and random number
 * defined in the Snap-Authorization field. The random number is
 * currently ignored because it would otherwise require applications
 * to support changing the random number on their next access which
 * is "complicated" to do.
 *
 * The function returns only if the user (application) is properly
 * logged in. In all other cases the application is not logged in
 * and the process calls die() with a 401 or a 403 error.
 */
void oauth2::application_login()
{
    // prevent login in with the "wrong" methods
    //
    QString const method(f_snap->snapenv(get_name(snap::name_t::SNAP_NAME_CORE_REQUEST_METHOD)));
    if(method == "HEAD"
    || method == "TRACE")
    {
        require_oauth2_login();
        die(snap_child::http_code_t::HTTP_CODE_METHOD_NOT_ALLOWED,
                    oauth2_error_t::OAUTH2_INVALID_REQUEST,
                    "Method Not Allowed",
                    "Applications do not accept method HEAD or TRACE.",
                    "Invalid method to access an application page.",
                    "http://snapwebsites.org/implementation/feature-requirements/oauth2-core");
        NOTREACHED();
    }

    // if the user is not accessing the OAuth2 log in feature
    // we check whether a Snap-Authorization field exists with
    // type named Bearer and if so verify the session identifier
    // and random
    QString const authorization(f_snap->snapenv("HTTP_SNAP_AUTHORIZATION"));

    snap_string_list const session_id(authorization.simplified().split(" "));
    if(session_id.size() != 2
    || session_id[0].toUpper() != "BEARER")
    {
        require_oauth2_login();
        die(snap_child::http_code_t::HTTP_CODE_UNAUTHORIZED,
                oauth2_error_t::OAUTH2_INVALID_REQUEST,
                "Permission Denied",
                "This page requires a Snap-Authorization.",
                QString("An API page was accessed with any invalid Snap-Authorization field (%1).").arg(authorization),
                "http://snapwebsites.org/implementation/feature-requirements/oauth2-core");
        NOTREACHED();
    }

    users::users * users_plugin(users::users::instance());

    // is that session a valid "user" (application) session?
    sessions::sessions::session_info info;
    if(users_plugin->load_login_session(session_id[1], info, false) == users::users::LOGIN_STATUS_OK)
    {
        QString const path(info.get_object_path());
        // Note: this call returns false if the path matches "|/logout(/.*)?|"
        //
        bool ok(false);
        users::users::identifier_t const id( path.mid(6).toLongLong(&ok) );
        if(ok && users_plugin->authenticated_user(id, &info))
        {
            // this session qualifies as a log in session
            // accept this session and go on by executing this path
            //
            return;
        }
    }

    // did the application use the /logout path to delete its session?
    //
    // Note that the authenticated_user() function will return false
    // so we automatically reach here if the path was "logout".
    // Also, the authenticated_user() function clears the session so
    // we do not have to do any of that work here.
    //
    content::path_info_t main_ipath;
    main_ipath.set_path(f_snap->get_uri().path());
    if(main_ipath.get_cpath() == "logout"
    || main_ipath.get_cpath() == "logout/")
    {
        // it was a log out, there is nothing more to do, but there is no
        // error in logging out from a website
        //
        // generate the result, an OAuth2 session
        // accept XML and JSON
        QString buffer;
        http_strings::WeightedHttpString encodings(f_snap->snapenv("HTTP_ACCEPT"));
        float const xml_level(encodings.get_level("application/xml"));
        float const json_level(encodings.get_level("application/json"));
        if(json_level > xml_level)
        {
            buffer = QString(
                "{"
                "\"version\":\"" SNAPWEBSITES_VERSION_STRING "\","
                "\"oauth2\":\"%1.%2\","
                "\"result\":\"logged out\""
                "}")
                    .arg(get_major_version()).arg(get_minor_version())
                ;
        }
        else
        {
            buffer = QString(
                "<?xml version=\"1.0\"?>"
                "<snap version=\"" SNAPWEBSITES_VERSION_STRING "\" oauth2=\"%1.%2\">"
                    "<result>logged out</result>"
                "</snap>")
                    .arg(get_major_version()).arg(get_minor_version())
                ;
        }
        // we are in an odd location and to end the child now
        // we need to do all the work ourselves
        f_snap->output_result(snap_child::HEADER_MODE_NO_ERROR, buffer.toUtf8());

        // IMPORTANT NOTE:
        // We are still inside the process_cookies() signal and thus the
        // detach_from_session() signal was not yet emitted so we do not
        // have to call the attach_to_session() signal before exiting.
        exit(0);
        NOTREACHED();
    }

    require_oauth2_login();
    die(snap_child::http_code_t::HTTP_CODE_BAD_REQUEST,
            oauth2_error_t::OAUTH2_INVALID_REQUEST,
            "Unauthorized",
            "This page requires a valid Snap-Authorization. If you had such, it may have timed out.",
            "The application session information was not valid and the user could not be authenticated properly.",
            "http://snapwebsites.org/implementation/feature-requirements/oauth2-core");
    NOTREACHED();
}


/** \brief Simulate the snap_child::die() for oauth2.
 *
 * The OAuth2 specification clearly says that we have to return a buffer
 * equivalent to what the user requested. For example, the user may have
 * requested JSON, in which case we cannot return HTML.
 *
 * This function returns JSON or XML as the reply.
 *
 * \warning
 * The error_Code can be set to either 401 when the authentication itself
 * fails or 400 in all other cases. You are responsible to use the correct
 * code whenever you call this function.
 *
 * \param[in] err_code  The HTTP error code being returned.
 * \param[in] err_oauth2  The OAuth2 error code.
 * \param[in] err_name  The textual name of the error (instead of just
 *            "Bad Request")
 * \param[in] err_description  The error description, sent back to client.
 * \param[in] err_details  Details not for the preying eyes, used in our
 *            private logs only.
 * \param[in] err_help_uri  A URI to a page explaining this specific error.
 */
void oauth2::die(snap_child::http_code_t err_code, oauth2_error_t err_oauth2, QString err_name, QString const & err_description, QString const & err_details, QString const & err_help_uri)
{
    // see snap_child::die() for comments and details about this code
    //

    if(static_cast<size_t>(err_oauth2) >= sizeof(g_oauth2_error_names) / sizeof(g_oauth2_error_names[0]))
    {
        throw snap_logic_exception(QString("OAuth2 die() called with an invalid err_oauth2 (%1)")
                    .arg(static_cast<int>(err_oauth2)));
    }

    QRegExp const regex_with_space("[ !#-[\\]-~]+");
    QRegExp const regex_no_spaces("[!#-[\\]-~]+");

    if(!regex_with_space.exactMatch(err_description))
    {
        throw snap_logic_exception(QString("OAuth2 error description \"%1\" includes characters that are not acceptable in the error_description tag (accepted characters are x20-x21 | x23-x5B | x5D-x7E)")
                    .arg(err_description));
    }

    if(!regex_no_spaces.exactMatch(err_help_uri))
    {
        throw snap_logic_exception(QString("OAuth2 error help URI \"%1\" includes characters that are not acceptable in the error_description tag (accepted characters are x21 | x23-x5B | x5D-x7E)")
                    .arg(err_help_uri));
    }

    snap_child::define_http_name(err_code, err_name);

    SNAP_LOG_FATAL("snap child process: ")(err_details)(" (")(static_cast<int>(err_code))(": ")(err_description)(")");

    f_snap->set_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_STATUS_HEADER),
                       QString("%1 %2").arg(static_cast<int>(err_code)).arg(err_name),
                       snap_child::HEADER_MODE_ERROR);

    // attach_to_session() -- missing because we do not have access to the
    //                        server; but I do not think it is required for
    //                        an API?

    QString buffer;

    http_strings::WeightedHttpString encodings(f_snap->snapenv("HTTP_ACCEPT"));
    float const xml_level(encodings.get_level("application/xml"));
    float const json_level(encodings.get_level("application/json"));
    if(json_level > xml_level)
    {
        f_snap->set_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER),
                           "text/json; charset=utf8",
                           snap_child::HEADER_MODE_EVERYWHERE);

        buffer = QString(
            "{"
            "\"error\":\"%1\","
            "\"error_description\":\"%2\","
            "\"error_uri\":\"%3\""
            "}")
                .arg(g_oauth2_error_names[static_cast<int>(err_oauth2)])
                .arg(err_description)
                .arg(err_help_uri)
            ;
    }
    else
    {
        f_snap->set_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER),
                           "text/xml; charset=utf8",
                           snap_child::HEADER_MODE_EVERYWHERE);

        buffer = QString(
            "<?xml version=\"1.0\"?>"
            "<snap version=\"" SNAPWEBSITES_VERSION_STRING "\" oauth2=\"%1.%2\">"
                "<error>%3</error>"
                "<error_description>%4</error_description>"
                "<error_uri>%5</error_uri>"
            "</snap>")
                .arg(get_major_version()).arg(get_minor_version())
                .arg(g_oauth2_error_names[static_cast<int>(err_oauth2)])
                .arg(snap_dom::escape(err_description))
                .arg(snap_dom::escape(err_help_uri))
            ;
    }

    f_snap->output_result(snap_child::HEADER_MODE_ERROR, buffer.toUtf8());

    exit(1);
}


/** \brief Send the authorization mechanism to the client.
 *
 * This function is used by plugins that implement an API and
 * find out that the page being accessed requires more permissions.
 *
 * The function sends the client an additional header with
 * the authentication type and realm.
 */
void oauth2::require_oauth2_login()
{
    f_snap->set_header("WWW-Snap-Authenticate", "Snap realm=\"Snap OAuth2\"", snap_child::HEADER_MODE_ERROR);
}


/** \brief Check whether we have a Snap-Authorization field.
 *
 * This signal is raised pretty early on and we use it here to try to
 * avoid redirects to /login on permission problems.
 */
void oauth2::on_process_cookies()
{
    QString const authorization(f_snap->snapenv("HTTP_SNAP_AUTHORIZATION"));
    if(!authorization.isEmpty())
    {
        f_snap->set_ignore_cookies();

        snap_string_list const auth(authorization.simplified().split(" "));
        if(auth.size() == 2
        && auth[0].toUpper() != "SNAP")
        {
            // we have to log in right now otherwise permissions will
            // prevent access to the other plugin pages before they
            // get a chance to do anything
            application_login();
        }
    }
}


/*

telnet csnap.m2osw.com 80
GET /user/oauth2 HTTP 1.1
Host: csnap.m2osw.com
User-Agent: telnet 0.17-36build2
Accept: application/json;q=0.7,application/xml;q=0.9
Snap-Authorization: ...Snap <application identifier ':' application secret> in base64, one line, see snap_uuencode_basic_auth...

telnet csnap.m2osw.com 80
GET /admin/settings/oauth2 HTTP 1.1
Host: csnap.m2osw.com
User-Agent: telnet 0.17-36build2
Accept: application/json;q=0.7,application/xml;q=1.0
Snap-Authorization: Bearer 38e81b746237c816/897095972

telnet csnap.m2osw.com 80
GET /logout HTTP 1.1
Host: csnap.m2osw.com
User-Agent: telnet 0.17-36build2
Accept: application/json;q=0.7,application/xml;q=0.5
Snap-Authorization: Bearer 231749675e79d6ae/1651269099

*/


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
