// Snap Websites Server -- JavaScript WYSIWYG form widgets
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

#include "editor.h"

#include "../output/output.h"
#include "../attachment/attachment.h"
#include "../locale/snap_locale.h"
#include "../messages/messages.h"
#include "../mimetype/mimetype.h"
#include "../permissions/permissions.h"

#include <snapwebsites/dbutils.h>
#include <snapwebsites/log.h>
#include <snapwebsites/mkgmtime.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qdomreceiver.h>
#include <snapwebsites/qdomxpath.h>
#include <snapwebsites/qxmlmessagehandler.h>
#include <snapwebsites/snap_image.h>
#include <snapwebsites/snap_lock.h>
#include <snapwebsites/xslt.h>

#include <libtld/tld.h>

#include <iostream>
#include <cmath>

#include <QTextDocument>
#include <QFile>
#include <QFileInfo>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(editor, 1, 0)



namespace
{

/** \brief Default timeout in minutes.
 *
 * A form is attached to a session. That way we make sure that a client
 * does not send us a form which content is days, weeks, months old,
 * or worst, a client who never accessed the server to retrieve a valid
 * form (i.e. web form spam where robots send data without first having
 * to load a form from a website.)
 *
 * This timeout represents the number of minutes an editor session is
 * created for. At this time we set it up to 24 hours (1 whole day.)
 *
 * \todo
 * At some point we want to add a way for the client browser to
 * auto-submit. At that point, the client will not lose his data
 * to a session that times out.
 */
int const g_default_timeout = 1440;

} // no name namespace


/** \brief Get a fixed editor plugin name.
 *
 * The editor plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_EDITOR_AUTO_RESET:
        return "editor::auto_reset";

    case name_t::SNAP_NAME_EDITOR_DRAFTS_PATH:
        return "admin/drafts";

    case name_t::SNAP_NAME_EDITOR_LAYOUT:
        return "editor::layout";

    case name_t::SNAP_NAME_EDITOR_PAGE:
        return "editor::page";

    case name_t::SNAP_NAME_EDITOR_PAGE_TYPE:
        return "editor::page_type";

    case name_t::SNAP_NAME_EDITOR_SESSION:
        return "editor::session";

    case name_t::SNAP_NAME_EDITOR_TIMEOUT:
        return "editor::timeout";

    case name_t::SNAP_NAME_EDITOR_TYPE_FORMAT_PATH: // a format to generate the path of a page
        return "editor::type_format_path";

    case name_t::SNAP_NAME_EDITOR_TYPE_EXTENDED_FORMAT_PATH:
        return "editor::type_extended_format_path";

    default:
        // invalid index
        throw snap_logic_exception("Invalid name_t::SNAP_NAME_EDITOR_...");

    }
    NOTREACHED();
}


editor::value_to_string_info_t::value_to_string_info_t(content::path_info_t & ipath, QDomElement widget, QtCassandra::QCassandraValue const & value)
    : f_ipath(ipath)
    , f_widget(widget)
    , f_value(value)
{
}


bool editor::value_to_string_info_t::is_done() const
{
    return f_status != status_t::WORKING;
}


bool editor::value_to_string_info_t::is_valid() const
{
    return f_status == status_t::DONE;
}


void editor::value_to_string_info_t::set_status(status_t const status)
{
    f_status = status;
}


content::path_info_t & editor::value_to_string_info_t::get_ipath() const
{
    return f_ipath;
}


QDomElement editor::value_to_string_info_t::get_widget() const
{
    return f_widget;
}


QtCassandra::QCassandraValue const & editor::value_to_string_info_t::get_value() const
{
    return f_value;
}


QString const & editor::value_to_string_info_t::get_widget_type() const
{
    if(f_widget_type.isEmpty())
    {
        f_widget_type = f_widget.attribute("type");
        // emptiness (i.e. invalidity) is checked before this event
        // is used so we should be just fine here
    }
    return f_widget_type;
}


QString const & editor::value_to_string_info_t::get_data_type() const
{
    if(f_data_type.isEmpty())
    {
        f_data_type = f_widget.attribute("auto-save");
        // emptiness (i.e. invalidity) is checked before this event
        // is used so we should be just fine here
    }
    return f_data_type;
}


QString const & editor::value_to_string_info_t::get_type_name() const
{
    return f_type_name;
}


void editor::value_to_string_info_t::set_type_name(QString const & new_type_name)
{
    f_type_name = new_type_name;
}


QString & editor::value_to_string_info_t::result()
{
    return f_result;
}



editor::string_to_value_info_t::string_to_value_info_t(content::path_info_t & ipath, QDomElement widget, QString const & data)
    : f_ipath(ipath)
    , f_widget(widget)
    , f_data(data)
{
}


/** \brief Check whether the value was already handled.
 *
 * Since all signal functions will be called, you need a way to know
 * whether you still need to do some work on the data. This can be done
 * using the is_done() function as in:
 *
 * \code
 *      my_plugin::string_to_value(value_info_t & info)
 *      {
 *          if(info.is_done())
 *          {
 *              return;
 *          }
 *
 *          // manage your own types
 *          if(info.get_type() == "my_ype")
 *          {
 *              ...
 *          }
 *      }
 * \endcode
 *
 * \return true if the value was already handled, which means you should
 *         not process this signal any further.
 */
bool editor::string_to_value_info_t::is_done() const
{
    return f_status != status_t::WORKING;
}


/** \brief Check whether the value info is valid.
 *
 * On return of the string_to_value() signal, the is_valid() function can
 * be used to know whether the value was considered valid.
 *
 * Note that the function returns false if the status is still WORKING
 * because that's considered invalid since no one handled the value.
 *
 * \return true if the value entered by the user is considered valid.
 */
bool editor::string_to_value_info_t::is_valid() const
{
    return f_status == status_t::DONE;
}


void editor::string_to_value_info_t::set_status(status_t const status)
{
    f_status = status;
}


content::path_info_t & editor::string_to_value_info_t::get_ipath() const
{
    return f_ipath;
}


QDomElement editor::string_to_value_info_t::get_widget() const
{
    return f_widget;
}


QString const & editor::string_to_value_info_t::get_data() const
{
    return f_data;
}


QString const & editor::string_to_value_info_t::get_widget_type() const
{
    if(f_widget_type.isEmpty())
    {
        f_widget_type = f_widget.attribute("type");
        // emptiness (i.e. invalidity) is checked before this event
        // is used so we should be just fine here
    }
    return f_widget_type;
}


QString const & editor::string_to_value_info_t::get_data_type() const
{
    if(f_data_type.isEmpty())
    {
        f_data_type = f_widget.attribute("auto-save");
        // emptiness (i.e. invalidity) is checked before this event
        // is used so we should be just fine here
    }
    return f_data_type;
}


QString const & editor::string_to_value_info_t::get_type_name() const
{
    return f_type_name;
}


void editor::string_to_value_info_t::set_type_name(QString const & new_type_name)
{
    f_type_name = new_type_name;
}


QtCassandra::QCassandraValue & editor::string_to_value_info_t::result()
{
    return f_result;
}






/** \brief Initialize the editor plugin.
 *
 * This function is used to initialize the editor plugin object.
 */
editor::editor()
    //: f_snap(nullptr) -- auto-init
    //, f_editor_form() -- auto-init
{
}


/** \brief Clean up the editor plugin.
 *
 * Ensure the editor object is clean before it is gone.
 */
editor::~editor()
{
}


/** \brief Get a pointer to the editor plugin.
 *
 * This function returns an instance pointer to the editor plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the editor plugin.
 */
editor * editor::instance()
{
    return g_plugin_editor_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString editor::settings_path() const
{
    return "/admin/settings/editor";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString editor::icon() const
{
    return "/images/editor/editor-logo-64x64.png";
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
QString editor::description() const
{
    return "Offer a WYSIWYG* editor to people using the website."
        " The editor appears wherever a plugin creates a div tag with"
        " the contenteditable attribute set to true."
        "\n(*) WYSIWYG: What You See Is What You Get.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString editor::dependencies() const
{
    return "|attachment|locale|messages|output|server_access|sessions|";
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
 *            updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t editor::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2017, 6, 18, 18, 58, 30, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our administration pages, etc.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables
 *            added to the database by this update (in micro-seconds).
 */
void editor::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize editor.
 *
 * This function terminates the initialization of the editor plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void editor::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(editor, "server", server,         process_post,              _1);
    SNAP_LISTEN(editor, "layout", layout::layout, generate_header_content,   _1,  _2, _3);
    SNAP_LISTEN(editor, "layout", layout::layout, generate_page_content,     _1,  _2, _3);
    SNAP_LISTEN(editor, "layout", layout::layout, add_layout_from_resources, _1);
    SNAP_LISTEN(editor, "form",   form::form,     validate_post_for_widget,  _1,  _2, _3,  _4, _5, _6);
    SNAP_LISTEN(editor, "path",   path::path,     check_for_redirect,        _1);
}


/** \brief Listen to the check_for_redirect() event, and do a soft redirect if applicable.
 *
 * When the path plugin fires, it will call this first, to see if we are doing a redirect.
 * If so, then we take the target url and use it as the path. Think of this as a "soft redirect."
 *
 * \note This addresses EX-175
 *
 * \param[in,out] ipath  The referring path.
 */
void editor::on_check_for_redirect(content::path_info_t & ipath)
{
    if( f_snap->has_post() )
    {
        QString const editor_full_session(f_snap->postenv("_editor_session"));
        if(editor_full_session.isEmpty())
        {
            // if the _editor_session variable does not exist, do not consider this
            // POST as an Editor POST
            SNAP_LOG_WARNING("***** POST is not for editor plugin");
            return;
        }

        snap_string_list const session_data(editor_full_session.split("/"));
        if(session_data.size() == 2)
        {
            sessions::sessions::session_info info;
            sessions::sessions::instance()->load_session(session_data[0], info, false);

            // verify that the path is correct
            content::path_info_t main_ipath; // at this point main_ipath == ipath but that should get fixed one day
            main_ipath.set_path(f_snap->get_uri().path());
            if(info.get_page_path() != main_ipath.get_key()
                    || info.get_user_agent() != f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT))
                    || info.get_plugin_owner() != get_plugin_name())
            {
                // the path was tempered with? the agent changes between hits?
                f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_ACCEPTABLE, "Not Acceptable",
                            "The POST request does not correspond to the editor it was defined for.",
                            QString("User POSTed a request against \"%1\" with an incompatible page path (%2) or a different plugin (%3).")
                            .arg(ipath.get_key())
                            .arg(info.get_page_path())
                            .arg(info.get_plugin_owner()));
                NOTREACHED();
            }

//SNAP_LOG_TRACE("**** setting ipath to ")(info.get_object_path());
            ipath.set_path( info.get_object_path() );
        }
    }
}


/** \brief Add editor specific tags to the layout DOM.
 *
 * This function adds different editor specific tags to the layout page
 * and body XML documents.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void editor::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    // a regular page
    output::output::instance()->on_generate_main_content(ipath, page, body);
}


/** \brief Setup page for the editor.
 *
 * The editor has a set of dynamic parameters that the users are offered
 * to setup. These parameters need to be sent to the user and we use this
 * function for that purpose.
 *
 * \todo
 * Look for a way to generate the editor data only if necessary (too
 * complex for now.)
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] header  The header being generated.
 * \param[in,out] metadata  The metadata being generated.
 */
void editor::on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    NOTUSED(ipath);

    QDomDocument doc(header.ownerDocument());

    // TODO: find a way to include the editor only if required
    //       (it may already be done! search on add_javascript() for info.)
    //
    content::content::instance()->add_javascript(doc, "editor");
    content::content::instance()->add_css(doc, "editor");

    // The following creates a session for editing the page.
    // This code is NOT used if the page is an editor form (i.e.
    // when the editor has widgets on a page).
    //
    // TODO: change the following behavior to allow editing in various
    //       other ways than when the action is edit or administer
    //
    // TODO: change the way the session ID gets in the page?
    //       (i.e. it would be better to have it go there
    //       using an AJAX request)
    //
    QDomDocument editor_widgets(get_editor_widgets(ipath));
    if(editor_widgets.isNull())
    {
        QString const action(f_snap->get_action());
        if(action == "edit" || action == "administer")
        {
            sessions::sessions::session_info info;
            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_FORM);
            info.set_session_id(EDITOR_SESSION_ID_EDIT);
            info.set_plugin_owner(get_plugin_name()); // ourselves
            content::path_info_t main_ipath;
            main_ipath.set_path(f_snap->get_uri().path());
            info.set_page_path(main_ipath.get_key());
            info.set_object_path(ipath.get_key());
            info.set_user_agent(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)));
            info.set_time_to_live(86400);  // 24 hours
            QString const session(sessions::sessions::instance()->create_session(info));
            int32_t const random(info.get_session_random());

            // /metadata/page_session
            QString const session_identification(QString("%1/%2").arg(session).arg(random));
            QDomElement session_tag(doc.createElement("page_session"));
            QDomText session_text(doc.createTextNode(session_identification));
            session_tag.appendChild(session_text);
            metadata.appendChild(session_tag);
        }
    }
}


/* \brief Check whether \p cpath matches our introducers.
 *
 * This function checks that cpath matches our introducer and if
 * so we tell the path plugin that we're taking control to
 * manage this path.
 *
 * We understand "user" as in list of users.
 *
 * We understand "user/<name>" as in display that user information
 * (this may be turned off on a per user or for the entire website.)
 * Websites that only use an email address for the user identification
 * do not present these pages publicly.
 *
 * We understand "profile" which displays the current user profile
 * information in detail and allow for editing of what can be changed.
 *
 * We understand "login" which displays a form for the user to log in.
 *
 * We understand "verify-credentials" which is very similar to "login"
 * albeit simpler and only appears if the user is currently logged in
 * but not recently logged in (i.e. administration rights.)
 *
 * We understand "logout" to allow users to log out of Snap! C++.
 *
 * We understand "register" to display a registration form to users.
 *
 * We understand "verify" to check a session that is being returned
 * as the user clicks on the link we sent on registration.
 *
 * We understand "forgot-password" to let users request a password reset
 * via a simple form.
 *
 * \todo
 * If we cannot find a global way to check the Origin HTTP header
 * sent by the user agent, we probably want to check it here in
 * pages where the referrer should not be a "weird" 3rd party
 * website.
 *
 * \param[in,out] ipath  The path being handled dynamically.
 * \param[in,out] plugin_info  If you understand that cpath, set yourself here.
 */
//void editor::on_can_handle_dynamic_path(content::path_info_t& ipath, path::dynamic_plugin_t& plugin_info)
//{
//    if(ipath.get_cpath() == "admin/drafts/new")
//    {
//        // tell the path plugin that this is ours
//        plugin_info.set_plugin(this);
//    }
//}


/** \brief Execute the specified path.
 *
 * This is a dynamic page which the editor plugin knows how to handle.
 *
 * \param[in,out] ipath  The canonicalized path.
 *
 * \return true if the processing worked as expected, false if the page
 *         cannot be created ("Page Not Present" results on false)
 */
bool editor::on_path_execute(content::path_info_t & ipath)
{
    // the editor forms are generated using token replacements
    f_snap->output(layout::layout::instance()->apply_layout(ipath, this));

    return true;
}


void editor::on_validate_post_for_widget(
          content::path_info_t & ipath
        , sessions::sessions::session_info & info
        , QDomElement const & widget
        , QString const & widget_name
        , QString const & widget_type
        , bool const is_secret)
{
    NOTUSED(widget);
    NOTUSED(widget_type);
    NOTUSED(is_secret);

    messages::messages * messages(messages::messages::instance());

    // we are only interested by our widgets
    QString const cpath(ipath.get_cpath());
    if(cpath == "admin/drafts/new")
    {
        // verify the type of the new page
        if(widget_name == "type")
        {
            // get the value
            QString const type(f_snap->postenv(widget_name));

            QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
            QString const site_key(f_snap->get_site_key_with_slash());
            QString const type_key(site_key + "types/taxonomy/system/content-types/" + type);
            if(!content_table->exists(type_key))
            {
                // TODO: test whether the user could create a new type, if so
                //       then do not err at all here
                messages->set_error(
                    "Unknown Type",
                    QString("Type \"%1\" is not yet defined and you do not have permission to create a new type of pages at this point.").arg(type),
                    "type does not exist and we do not yet offer a way to auto-create a content type",
                    false
                ).set_widget_name(widget_name);
                info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
            }
        }
    }
}


/** \brief Process a post from one of the editor forms.
 *
 * This function processes the post of an editor form. The function uses the
 * \p ipath parameter in order to determine which form is being processed.
 *
 * See the plugins/editor/new-draft.xml file.
 *
 * \param[in,out] ipath  The path the user is accessing now.
 * \param[in] session_info  The user session being processed.
 */
void editor::on_process_form_post(content::path_info_t & ipath, sessions::sessions::session_info const & session_info)
{
    NOTUSED(session_info);

    QString const cpath(ipath.get_cpath());
    if(cpath == "admin/drafts/new")
    {
        process_new_draft();
    }
    else
    {
        // this should not happen because invalid paths will not pass the
        // session validation process
        throw editor_exception_invalid_path("editor::on_process_form_post() was called with an unsupported path: \"" + ipath.get_key() + "\"");
    }
}


/** \brief Finish the processing of a new draft.
 *
 * This function ends the processing of a new draft by saving the information
 * the user entered in the new draft form. This function creates a draft
 * under the admin/draft path under the user publishes the page. This allows
 * for the path of the new page to be better defined than if we were creating
 * the page at once.
 *
 * The path used under admin/draft simply makes use of the Unix time value.
 * If two or more users create a new draft simultaneously (within the same
 * second) then an additional .1 to .99 is added to the path. If more than
 * 100 users create a page simultaneously, the 101 and further fail saving
 * the new draft and will have to test again later.
 */
void editor::process_new_draft()
{
    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());

    // get the 3 parameters entered by the user to get the new page started
    QString const type(f_snap->postenv("type"));
    QString const sibling(f_snap->postenv("sibling"));
    QString const title(f_snap->postenv("title"));
    QString const page_description(f_snap->postenv("description"));

    // TODO: test that "type" exists and if not creating it (if the user
    //       has engouh rights); we already checked whether the type
    //       existed and the user had engouh rights, but we want to test
    //       again; that being said, until we support creating new types
    //       we don't have to do anything here

    // now create the new page as a pure draft (opposed to an unpublised set
    // of changes on a page which is also called a draft, but is directly
    // linked to that one page.)
    time_t const start_time(f_snap->get_start_time());
    int64_t const start_date(f_snap->get_start_date());
    char const * drafts_path(get_name(name_t::SNAP_NAME_EDITOR_DRAFTS_PATH));
    QString const site_key(f_snap->get_site_key_with_slash());
    QString new_draft_key(QString("%1%2/%3").arg(site_key).arg(drafts_path).arg(start_time));

    // we got as much as we could ready before locking
    {
        // make sure this draft key is unique
        // lock the parent briefly
        snap_lock lock(QByteArray(drafts_path));
        for(int extra(1); content_table->exists(new_draft_key); ++extra)
        {
            // TBD: Could it really ever happen that a website would have over
            //      100 people (i.e. not robots) create a page all at once?
            //      Should we offer to make this number a variable that
            //      administrators could bump up to be "safe"?
            if(extra >= 100) // 100 excluded since we start with zero (.0 is not included in the very first name)
            {
                // TODO: this error needs to be reported to the administrator(s)
                //       (especially if it happens often because that means
                //       robots are working on the website!)
                f_snap->die(snap_child::http_code_t::HTTP_CODE_CONFLICT,
                    "Conflict Error", "We could not create a new draft entry for you. Too many other drafts existed already. Please try again later.",
                    "Somehow the server was not able to generated another draft entry.");
                NOTREACHED();
            }
            new_draft_key = QString("%1%2/%3.%4").arg(site_key).arg(drafts_path).arg(start_time).arg(extra);
        }
        // create that row so the next user will detect it as existing
        // and we can then unlock the parent row
        content_table->row(new_draft_key)->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->setValue(start_date);
    }

    // before we go further officially create the content
    //
    // TODO: fix the locale; it should come from the favorite locale of that
    //       user and we should offer the user to select another locale if
    //       he/she has more than one in his account
    //
    QString const locale("xx");
    QString const owner(output::output::instance()->get_plugin_name());
    content::path_info_t draft_ipath;
    draft_ipath.set_path(new_draft_key);
    draft_ipath.force_branch(content_plugin->get_current_user_branch(new_draft_key, locale, true));
    draft_ipath.force_revision(snap_version::SPECIAL_VERSION_FIRST_REVISION);
    draft_ipath.force_locale(locale);
    content_plugin->create_content(draft_ipath, owner, "page/draft");

    // save the title, description, and link to the type as a "draft type"
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(draft_ipath.get_revision_key()));
    revision_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->setValue(start_date);
    revision_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->setValue(title);
    revision_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_DESCRIPTION))->setValue(page_description);
    revision_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_BODY))->setValue(QString("enter page content here ([year])"));

    // link to the type, but not as the official type yet since this page
    // has to have a "draft page" type for a while
    {
        QString const link_name(get_name(name_t::SNAP_NAME_EDITOR_PAGE_TYPE));
        bool const source_unique(true);
        QString const link_to(get_name(name_t::SNAP_NAME_EDITOR_PAGE));
        bool const destination_unique(false);
        content::path_info_t type_ipath;
        QString const type_key(site_key + "types/taxonomy/system/content-types/" + type);
        type_ipath.set_path(type_key);
        links::link_info source(link_name, source_unique, draft_ipath.get_key(), draft_ipath.get_branch());
        links::link_info destination(link_to, destination_unique, type_ipath.get_key(), type_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    // give edit permission of the draft
    // <link name="permissions::view" to="permissions::view" mode="*:*">/types/permissions/rights/view/page/for-spammers</link>
    {
        QString const link_name(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_ACTION_EDIT));
        bool const source_unique(false);
        QString const link_to(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_EDIT));
        bool const destination_unique(false);
        content::path_info_t type_ipath;
        // TBD -- should this includes the type of page?
        QString const type_key(site_key + "types/permissions/rights/edit/page");
        type_ipath.set_path(type_key);
        links::link_info source(link_name, source_unique, draft_ipath.get_key(), draft_ipath.get_branch());
        links::link_info destination(link_to, destination_unique, type_ipath.get_key(), type_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    // redirect the user to the new page so he can edit it
    QString const qs_action(f_snap->get_server_parameter("qs_action"));
    f_snap->page_redirect(QString("%1?%2=edit").arg(draft_ipath.get_key()).arg(qs_action), snap_child::http_code_t::HTTP_CODE_FOUND,
            "Page was created successfully",
            "Sending you to your new page so that way you can edit it and ultimately publish it.");
    NOTREACHED();
}


/** \brief Check the URL and process the POST data accordingly.
 *
 * This function manages the data sent back by the editor.js script
 * and save the new values as reuired.
 *
 * The function verifies that the "editor_session" variable is set, if
 * not it ignores the POST since another plugin may be the owner.
 *
 * \note
 * This function is a server signal generated by the snap_child
 * execute() function.
 *
 * \param[in] uri_path  The path received from the HTTP server.
 */
void editor::on_process_post(QString const & uri_path)
{
    content::path_info_t ipath;
    ipath.set_path(uri_path);
    ipath.set_main_page(true);
    ipath.force_locale("xx");

    QString const editor_request_original_data(f_snap->postenv("_editor_request_original_data"));
    if(!editor_request_original_data.isEmpty())
    {
        // the client is asking for the original content of a field
        //
        // TODO: make sure to get the _editor_session checked too!!!
        //
        retrieve_original_field(ipath);
        return;
    }

    QString const editor_full_session(f_snap->postenv("_editor_session"));
//SNAP_LOG_WARNING("process post of [") << uri_path << "] [" << editor_full_session << "]";
    if(editor_full_session.isEmpty())
    {
        // if the _editor_session variable does not exist, do not consider this
        // POST as an Editor POST
        return;
    }

    save_mode_t editor_save_mode(string_to_save_mode(f_snap->postenv("_editor_save_mode")));
    if(editor_save_mode == save_mode_t::EDITOR_SAVE_MODE_UNKNOWN)
    {
        // this could happen between versions (i.e. newer version wants to
        // use a new mode which we did not yet implement in the
        // string_to_save_mode() function.) -- it could be a problem between
        // a server that has a newer version and a server that does not...
        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_ACCEPTABLE, "Not Acceptable",
                "Somehow the editor does not understand the Save command sent to the server.",
                QString("User gave us an unknown save mode (%1).").arg(f_snap->postenv("_editor_save_mode")));
        NOTREACHED();
    }

//SNAP_LOG_WARNING("save mode [")(static_cast<int>(editor_save_mode))("]");
    // [0] -- session Id, [1] -- random number
    snap_string_list const session_data(editor_full_session.split("/"));
    if(session_data.size() != 2)
    {
        // should never happen on a valid user
        // TBD: lose the data in this case? The user browser may have
        //      inadvertedly deleted the session cookie?
        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_ACCEPTABLE, "Not Acceptable",
                "The session identification is not valid.",
                QString("User gave us an unknown session identifier (%1).").arg(editor_full_session));
        NOTREACHED();
    }

    messages::messages * messages(messages::messages::instance());

    // First we verify the editor form session information
    // <div id="content" form_name="..." class="editor-form ..." session="session_id/random_number">...</div>
    sessions::sessions::session_info info;
    sessions::sessions::instance()->load_session(session_data[0], info, false);
    switch(info.get_session_type())
    {
    case sessions::sessions::session_info::session_info_type_t::SESSION_INFO_VALID:
        // unless we get this value we've got a problem with the session itself
        break;

    case sessions::sessions::session_info::session_info_type_t::SESSION_INFO_MISSING:
        // TBD: We may have a special "trash like draft area" where we can
        // save such data, although someone who waits that long... plus if
        // we have an auto-close, this would not happen anyway
        f_snap->die(snap_child::http_code_t::HTTP_CODE_GONE,
                    "Editor Session Gone",
                    "It looks like you attempted to submit editor content without first loading it.",
                    "User sent editor content with a session identifier that is not available.");
        NOTREACHED();
        return;

    case sessions::sessions::session_info::session_info_type_t::SESSION_INFO_OUT_OF_DATE:
        // TODO:
        // this is a harsh one! We need to save that data as a Draft, whatever
        // the Save mode we got. That way if the user wanted to keep his
        // data he will be able to do so from the draft (update the message to
        // correspond to the new mode/possibilities!)
        messages->set_http_error(snap_child::http_code_t::HTTP_CODE_GONE,
                                 "Editor Timeout",
                                 "Sorry! You sent this request back to Snap! way too late. It timed out. Please re-enter your information and re-submit.",
                                 "User did not click the submit button soon enough, the server session timed out.",
                                 true);
        if(editor_save_mode == save_mode_t::EDITOR_SAVE_MODE_ATTACHMENT)
        {
            editor_save_mode = save_mode_t::EDITOR_SAVE_MODE_AUTO_ATTACHMENT;
        }
        else
        {
            editor_save_mode = save_mode_t::EDITOR_SAVE_MODE_AUTO_DRAFT;
        }
        break;

    case sessions::sessions::session_info::session_info_type_t::SESSION_INFO_USED_UP:
        // this should not happen because we do not mark editor sessions
        // for one time use
        messages->set_http_error(snap_child::http_code_t::HTTP_CODE_CONFLICT,
                                 "Editor Already Submitted",
                                 "This editor session was already processed.",
                                 "The user submitted the same session more than once.",
                                 true);
        if(editor_save_mode == save_mode_t::EDITOR_SAVE_MODE_ATTACHMENT)
        {
            editor_save_mode = save_mode_t::EDITOR_SAVE_MODE_AUTO_ATTACHMENT;
        }
        else
        {
            editor_save_mode = save_mode_t::EDITOR_SAVE_MODE_AUTO_DRAFT;
        }
        break;

    default:
        throw snap_logic_exception("load_session() returned an unexpected SESSION_INFO_... value in editor::on_process_post()");

    }

    server_access::server_access * server_access_plugin(server_access::server_access::instance());

    content::path_info_t real_ipath;
    QString const object_path(info.get_object_path());
    if(object_path.isEmpty())
    {
        real_ipath.set_path(ipath.get_key());
    }
    else
    {
        real_ipath.set_path(object_path);
        ipath.set_real_path(object_path);
    }

    // TODO: if we generated an error, we do not even get a way to save
    //       the data to a draft
    if(messages->get_error_count() == 0)
    {
        // verify that the session random number is compatible
        if(info.get_session_random() != session_data[1].toInt())
        {
            f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_ACCEPTABLE, "Not Acceptable",
                    "The POST request does not correspond to the session that the editor generated.",
                    QString("User POSTed a request with random number %1, but we expected %2.")
                            .arg(info.get_session_random())
                            .arg(session_data[1]));
            NOTREACHED();
        }

#if 0
        // EX-175: moved this test to on_check_redirect() above.
        // verify that the path is correct
        content::path_info_t main_ipath; // at this point main_ipath == ipath but that should get fixed one day
        main_ipath.set_path(f_snap->get_uri().path());
        if(info.get_page_path() != main_ipath.get_key()
        || info.get_user_agent() != f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT))
        || info.get_plugin_owner() != get_plugin_name())
        {
            // the path was tempered with? the agent changes between hits?
            f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_ACCEPTABLE, "Not Acceptable",
                    "The POST request does not correspond to the editor it was defined for.",
                    QString("User POSTed a request against \"%1\" with an incompatible page path (%2) or a different plugin (%3).")
                            .arg(ipath.get_key())
                            .arg(info.get_page_path())
                            .arg(info.get_plugin_owner()));
            NOTREACHED();
        }
#endif

        // editing a draft?
        if(real_ipath.get_cpath().startsWith("admin/drafts/"))
        {
            // adjust the mode for drafts are "special" content
            switch(editor_save_mode)
            {
            case save_mode_t::EDITOR_SAVE_MODE_DRAFT:
                editor_save_mode = save_mode_t::EDITOR_SAVE_MODE_SAVE;
            case save_mode_t::EDITOR_SAVE_MODE_SAVE:
                break;

            case save_mode_t::EDITOR_SAVE_MODE_PUBLISH:
                editor_save_mode = save_mode_t::EDITOR_SAVE_MODE_NEW_BRANCH;
            case save_mode_t::EDITOR_SAVE_MODE_NEW_BRANCH: // should not be accessible
                break;

            case save_mode_t::EDITOR_SAVE_MODE_AUTO_DRAFT: // TBD
                break;

            case save_mode_t::EDITOR_SAVE_MODE_ATTACHMENT: // no change
            case save_mode_t::EDITOR_SAVE_MODE_AUTO_ATTACHMENT: // TBD
                break;

            case save_mode_t::EDITOR_SAVE_MODE_UNKNOWN:
                // this should never happen
                throw snap_logic_exception("The UNKNOWN save mode was ignore, yet we have an edit_save_mode set to UNKNOWN.");

            }
        }

        // act on the data as per the user's specified mode
        switch(editor_save_mode)
        {
        case save_mode_t::EDITOR_SAVE_MODE_DRAFT:
            break;

        case save_mode_t::EDITOR_SAVE_MODE_NEW_BRANCH:
            editor_create_new_branch(real_ipath);
            break;

        case save_mode_t::EDITOR_SAVE_MODE_SAVE:
            editor_save(real_ipath, info);
            break;

        case save_mode_t::EDITOR_SAVE_MODE_PUBLISH:
            //editor_save(real_ipath, info); -- this will most certainly call the same function with a flag
            break;

        case save_mode_t::EDITOR_SAVE_MODE_AUTO_DRAFT:
            break;

        case save_mode_t::EDITOR_SAVE_MODE_ATTACHMENT:
            editor_save_attachment(real_ipath, info, server_access_plugin);
            break;

        case save_mode_t::EDITOR_SAVE_MODE_AUTO_ATTACHMENT:
            //editor_save_attachment(real_ipath, info, server_access_plugin); -- we need to save the attachment as a "draft"
            break;

        case save_mode_t::EDITOR_SAVE_MODE_UNKNOWN:
            // this should never happen
            throw snap_logic_exception("The UNKNOWN save mode was ignore, yet we have an edit_save_mode set to UNKNOWN.");

        }
    }

    // for forms that are not automatically saved by the editor, further
    // processing may be required
    bool succeeded(messages->get_error_count() == 0);
    finish_editor_form_processing(ipath, succeeded);
    succeeded = succeeded && messages->get_error_count() == 0;

    // create the AJAX response
    server_access_plugin->create_ajax_result(ipath, succeeded);
    server_access_plugin->ajax_output();
}


void editor::retrieve_original_field(content::path_info_t & ipath)
{
    server_access::server_access * server_access_plugin(server_access::server_access::instance());
    messages::messages * messages(messages::messages::instance());

    QString const field_name(f_snap->postenv("field_name"));

    // the name we are given in "field_name" is not the name of
    // the field in the database, we have to do a conversion
    //
    QDomDocument editor_widgets(get_editor_widgets(ipath, true));
    if(editor_widgets.isNull())
    {
        // problem...
        messages->set_error(
            "Field Not Found",
            "The system encountered a problem as it could not determine which field is required by the editor.",
            QString("Search for field named \"%1\" was cut short as the editor widgets could not even be loaded.").arg(field_name),
            false
        );
    }
    else
    {
        if(f_snap->postenv_exists("page_language"))
        {
            ipath.force_locale(f_snap->postenv("page_language"));
        }

        QDomNodeList widgets(editor_widgets.elementsByTagName("widget"));
        int const max_widgets(widgets.size());
        for(int i(0); i < max_widgets; ++i)
        {
            QDomElement widget(widgets.at(i).toElement());

            QString const widget_name(widget.attribute("id"));
            if(widget_name == field_name)
            {
                QString const database_field_name(widget.attribute("field"));

                content::content * content_plugin(content::content::instance());
                QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
                dbutils du("revision", database_field_name);
                QtCassandra::QCassandraCell::pointer_t c(revision_table->row(ipath.get_revision_key())->cell(database_field_name));
                dbutils::column_type_t const ct( du.get_column_type( c ) );
                QString field_data;
                if(ct == dbutils::column_type_t::CT_string_value)
                {
                    // in this case we do not use the dbutils which
                    // replaces "\n" characters with "\\n"...
                    //
                    // TODO:
                    // we certainly should have another function in the
                    // dbutils to load and save data for the editor...
                    // so we do not need to have special cases like this
                    //
                    field_data = c->value().stringValue();
                }
                else
                {
                    field_data = du.get_column_value(c, false);
                }
                server_access_plugin->create_ajax_result(ipath, true);
                server_access_plugin->ajax_append_data("field_data", field_data.toUtf8());
                server_access_plugin->ajax_output();
                return;
            }
        }

        messages->set_error(
            "Field Not Found",
            "The system encountered a problem as it could not determine which field is required by the editor.",
            QString("Searched field named \"%1\" in the default set of widget and it was not found.").arg(field_name),
            false
        );
    }

    server_access_plugin->create_ajax_result(ipath, false);
    server_access_plugin->ajax_output();
}


/** \brief Transform the editor save mode to a number.
 *
 * This function transforms \p mode into a number representing the
 * save mode used with a POST.
 *
 * If the mode is not known, then EDITOR_SAVE_MODE_UNKNOWN is returned.
 * If your function cannot manage any mode, it should die() with a
 * corresponding error.
 *
 * \param[in] mode  The mode to be transformed.
 *
 * \return One of the EDITOR_SAVE_MODE_...
 */
editor::save_mode_t editor::string_to_save_mode(QString const & mode)
{
    if(mode == "draft")
    {
        return save_mode_t::EDITOR_SAVE_MODE_DRAFT;
    }
    if(mode == "publish")
    {
        return save_mode_t::EDITOR_SAVE_MODE_PUBLISH;
    }
    if(mode == "save")
    {
        return save_mode_t::EDITOR_SAVE_MODE_SAVE;
    }
    if(mode == "new_branch")
    {
        return save_mode_t::EDITOR_SAVE_MODE_NEW_BRANCH;
    }
    if(mode == "auto_draft")
    {
        return save_mode_t::EDITOR_SAVE_MODE_AUTO_DRAFT;
    }
    if(mode == "attachment")
    {
        return save_mode_t::EDITOR_SAVE_MODE_ATTACHMENT;
    }

    return save_mode_t::EDITOR_SAVE_MODE_UNKNOWN;
}


/** \brief Transform a database value to a string for display.
 *
 * This function transforms a database value back to a string as displayed
 * to end users.
 *
 * The value must be valid. Invalid values do not make it in the result
 * string. In other words, the result string remains unchanged if the
 * input value is considered invalid.
 *
 * \param[in] value_info  A value_to_string_info_t object.
 *
 * \return true if the data_type is not known internally, false when the type
 *         was managed by this very function
 */
bool editor::value_to_string_impl(value_to_string_info_t & value_info)
{
    if(value_info.get_value().nullValue())
    {
        // no value, ignore, do NOT change the result string
        value_info.set_status(value_to_string_info_t::status_t::ERROR);
        return false;
    }

    if(value_info.get_data_type() == "int8")
    {
        value_info.set_type_name("decimal integer");

        int const v(value_info.get_value().safeSignedCharValue());
        if(value_info.get_widget_type() == "checkmark")
        {
            value_info.result() = v == 0 ? "0" : "1";
        }
        else
        {
            value_info.result() = QString("%1").arg(v);
        }
        value_info.set_status(value_to_string_info_t::status_t::DONE);
        return false;
    }

    if(value_info.get_data_type() == "int64")
    {
        value_info.set_type_name("decimal integer");

        int64_t const v(value_info.get_value().safeInt64Value());
        value_info.result() = QString("%1").arg(v);
        value_info.set_status(value_to_string_info_t::status_t::DONE);
        return false;
    }

    if(value_info.get_data_type() == "double"
    || value_info.get_data_type() == "float64")
    {
        value_info.set_type_name("decimal number");

        double const v(value_info.get_value().safeDoubleValue());
        value_info.result() = QString("%1").arg(v);
        value_info.set_status(value_to_string_info_t::status_t::DONE);
        return false;
    }

    if(value_info.get_data_type() == "percent64")
    {
        value_info.set_type_name("percent number");

        double const v(value_info.get_value().safeDoubleValue());
        value_info.result() = QString("%1%").arg(v * 100.0);
        value_info.set_status(value_to_string_info_t::status_t::DONE);
        return false;
    }

    if(value_info.get_data_type() == "plain")
    {
        value_info.set_type_name("string");

        // characters such as <, >, and & have to be re-escaped here
        value_info.result() = snap_dom::escape(value_info.get_value().stringValue());
        value_info.set_status(value_to_string_info_t::status_t::DONE);
        return false;
    }

    if(value_info.get_data_type() == "string"
    || value_info.get_data_type() == "html")
    {
        value_info.set_type_name("string");

        // data is already as expected, copy as is
        value_info.result() = value_info.get_value().stringValue();
        value_info.set_status(value_to_string_info_t::status_t::DONE);
        return false;
    }

    if(value_info.get_data_type() == "ms-date-us")
    {
        value_info.set_type_name("date");

        value_info.result() = f_snap->date_to_string(value_info.get_value().safeInt64Value(), snap_child::date_format_t::DATE_FORMAT_SHORT_US);
        value_info.set_status(value_to_string_info_t::status_t::DONE);
        return false;
    }

    if(value_info.get_data_type() == "date")
    {
        value_info.set_type_name("date");

        int64_t const date(value_info.get_value().safeInt64Value());
        if(date != 0)
        {
            value_info.result() = locale::locale::instance()->format_date(date / 1000000);
        }
        value_info.set_status(value_to_string_info_t::status_t::DONE);
        return false;
    }

    if(value_info.get_data_type() == "time")
    {
        value_info.set_type_name("time");

        value_info.result() = locale::locale::instance()->format_time(value_info.get_value().safeInt64Value() / 1000000);
        value_info.set_status(value_to_string_info_t::status_t::DONE);
        return false;
    }

    return true;
}


/** \brief Transform data to a QCassandraValue.
 *
 * This function transforms a value received from a POST into a
 * QCassandraValue to be saved in the database.
 *
 * \param[in] value_info  Information about the widget to be checked.
 *
 * \return false if the data_type is not known internally, true when the type
 *         was managed by this very function
 */
bool editor::string_to_value_impl(string_to_value_info_t & value_info)
{
    // the default type name is the raw (technical) data type
    // it may be changed so as to make it clearer to end users
    if(value_info.get_data_type() == "no")
    {
        return false;
    }

    value_info.set_type_name(value_info.get_data_type());

    // integer of 8 bits
    if(value_info.get_data_type() == "int8")
    {
        value_info.set_type_name("decimal integer");

        signed char c;
        if(value_info.get_widget_type() == "checkmark")
        {
            c = value_info.get_data() == "0" ? 0 : 1;
        }
        else
        {
            bool ok(false);
            int const r(value_info.get_data().toInt(&ok, 10));
            if(!ok || r < 0 || r > 255)
            {
                value_info.set_status(string_to_value_info_t::status_t::ERROR);
                return false;
            }
            c = static_cast<signed char>(r);
        }

        value_info.result().setSignedCharValue(c);
        value_info.set_status(string_to_value_info_t::status_t::DONE);
        return false;
    }

    // integer of 64 bits
    if(value_info.get_data_type() == "int64")
    {
        value_info.set_type_name("decimal integer");

        int64_t v;
        bool ok(false);
        v = value_info.get_data().toLongLong(&ok);
        if(!ok)
        {
            value_info.set_status(string_to_value_info_t::status_t::ERROR);
            return false;
        }

        value_info.result().setInt64Value(v);
        value_info.set_status(string_to_value_info_t::status_t::DONE);
        return false;
    }

    // floating points of 64 bits
    if(value_info.get_data_type() == "double"
    || value_info.get_data_type() == "float64")
    {
        value_info.set_type_name("decimal number");

        double dbl;
        bool ok(false);
        dbl = value_info.get_data().toDouble(&ok);
        if(!ok)
        {
            value_info.set_status(string_to_value_info_t::status_t::ERROR);
            return false;
        }

        value_info.result().setDoubleValue(dbl);
        value_info.set_status(string_to_value_info_t::status_t::DONE);
        return false;
    }

    // floating point of 64 bits followed by "%"
    if(value_info.get_data_type() == "percent64")
    {
        value_info.set_type_name("percent number");

        QString percent(value_info.get_data());
        if(percent.at(percent.length() - 1) != '%')
        {
            value_info.set_status(string_to_value_info_t::status_t::ERROR);
            return false;
        }
        percent = percent.mid(0, percent.length() - 1);

        double dbl;
        bool ok(false);
        dbl = percent.toDouble(&ok);
        if(!ok)
        {
            value_info.set_status(string_to_value_info_t::status_t::ERROR);
            return false;
        }

        value_info.result().setDoubleValue(dbl / 100.0);
        value_info.set_status(string_to_value_info_t::status_t::DONE);
        return false;
    }

    // simple US date for now (MM-DD-YYYY), needs to be extended
    // (format NOT even checked properly!!!)
    //
    if(value_info.get_data_type() == "ms-date-us")
    {
        value_info.set_type_name("date");

        // convert a US date to 64 bit value in micro seconds
        //
        // TODO: verify that the date is valid and has a
        //       proper format for the locale.
        //
        //       See the locale plugin...
        //
        //       Also we want to have a function in the
        //       library to do this conversion because many
        //       different people may end up doing similar
        //       conversions...
        //
        struct tm time_info;
        memset(&time_info, 0, sizeof(time_info));
        time_info.tm_mon = value_info.get_data().mid(0, 2).toInt() - 1;
        time_info.tm_mday = value_info.get_data().mid(3, 2).toInt();
        time_info.tm_year = value_info.get_data().mid(6, 4).toInt() - 1900;
        time_t const t(mkgmtime(&time_info));
        value_info.result().setInt64Value(t * 1000000); // seconds to microseconds
        value_info.set_status(string_to_value_info_t::status_t::DONE);
        return false;
    }

    // convert a date using the current locale which we expect is
    // specific to the current user (if the user is not logged in
    // then we should fallback to the website default.)
    //
    // the result is in microseconds like most other dates we use in Snap!
    //
    if(value_info.get_data_type() == "date")
    {
        value_info.set_type_name("date");

        // convert a date to 64 bit value in micro seconds
        //
        // TODO: verify that this works as expected for various
        //       user of various locales and timezones.
        //
        // Note that the date should already have been verified so we
        // should not get an error code here.
        //
        locale::locale::parse_error_t errcode;
        time_t const t(locale::locale::instance()->parse_date(value_info.get_data(), errcode));
        if(errcode == locale::locale::parse_error_t::PARSE_NO_ERROR)
        {
            value_info.result().setInt64Value(t * 1000000); // seconds to microseconds
        }
        else
        {
            // use 0 on failure
            value_info.result().setInt64Value(0);
        }
        value_info.set_status(string_to_value_info_t::status_t::DONE);
        return false;
    }

    // convert a string representing a time using the current locale
    // which we expect is specific to the current user (if the user is
    // not logged in then we should fallback to the website default.)
    //
    // the result is in microseconds like most other dates we use in Snap!
    //
    if(value_info.get_data_type() == "time")
    {
        value_info.set_type_name("time");

        // convert a time to 64 bit value in micro seconds
        //
        // TODO: verify that this works as expected for various
        //       user of various locales and timezones.
        //
        // Note that the time should already have been verified so we
        // should not get an error code here.
        //
        locale::locale::parse_error_t errcode;
        time_t const t(locale::locale::instance()->parse_time(value_info.get_data(), errcode));
        if(errcode == locale::locale::parse_error_t::PARSE_NO_ERROR)
        {
            value_info.result().setInt64Value(t * 1000000); // seconds to microseconds
        }
        else
        {
            // use 0 on failure
            value_info.result().setInt64Value(0);
        }
        value_info.set_status(string_to_value_info_t::status_t::DONE);
        return false;
    }

    // a standard string (remember we use UTF-8 everywhere)
    //
    if(value_info.get_data_type() == "string")
    {
        value_info.set_type_name("string");

        // no special handling for strings
        value_info.result().setStringValue(value_info.get_data());
        value_info.set_status(string_to_value_info_t::status_t::DONE);
        return false;
    }

    // full HTML, we do one special trick on that data: we convert
    // inline images into attachment and replace the href with the
    // new URI
    //
    if(value_info.get_data_type() == "html")
    {
        value_info.set_type_name("HTML");

        QString value(value_info.get_data());
        value = verify_html_validity(value);

        // like a string, but convert inline images too
        //
        // TODO: verify that the HTML code is indeed valid HTML
        //       (valid XML like code and all tags are known)
        //
        parse_out_inline_img(value_info.get_ipath(), value, value_info.get_widget());
        value_info.result().setStringValue(value);
        value_info.set_status(string_to_value_info_t::status_t::DONE);
        return false;
    }

    // plain text is easy
    //
    if(value_info.get_data_type() == "plain")
    {
        value_info.set_type_name("plain text");

        // in case of plain text we want to remove all
        // tags if any and then unescape entities which
        // the remove_tags() function does all at once
        //
        value_info.result().setStringValue(snap_dom::remove_tags(value_info.get_data()));
        value_info.set_status(string_to_value_info_t::status_t::DONE);
        return false;
    }

    // not an internal data type, let other plugins handle this one
    //
    return true;
}


/** \brief Save the fields in a new revision.
 *
 * This function ensures that the current revision is copied in a new
 * revision and overwritten with the new data that the editor just
 * received (i.e. the user may just have changed his page title.)
 *
 * \param[in,out] ipath  The path to the page being updated.
 * \param[in,out] info  The session information, for the validation, just in case.
 */
void editor::editor_save(content::path_info_t & ipath, sessions::sessions::session_info & info)
{

//
// TODO -- the verification phase needs to be moved to a separate function
//         that gets called whatever the "process post" function was called
//         (at this point drafts and such will not work right)
//
//         Unfortunately the saving of the data is intricately intermingled
//         from what I can tell... although if we could extract the
//         loop that validates and saves the data that could be enough
//         because then we could call it last with the revision row where
//         the data is to be saved.
//
//         Plus, we have to verify that the Save happens only after
//         validation (for obvious security reasons.) However, drafts are a
//         potential problem in that arena...
//

    content::content * content_plugin(content::content::instance());
    messages::messages * messages(messages::messages::instance());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());

    snap_version::version_number_t branch_number(ipath.get_branch());
    bool const switch_branch(snap_version::SPECIAL_VERSION_SYSTEM_BRANCH == branch_number);
    if(switch_branch)
    {
        // force a user branch if that page still uses a system branch!
        branch_number = snap_version::SPECIAL_VERSION_USER_FIRST_BRANCH;
    }
    QString const key(ipath.get_key());
    QString const locale(ipath.get_locale());

    // get the widgets
    QDomDocument editor_widgets(get_editor_widgets(ipath, true));

    // check whether auto-save is ON
    QDomElement on_save(snap_dom::get_element(editor_widgets, "on-save", false));
    bool const auto_save(on_save.isNull() ? true : on_save.attribute("auto-save", "yes") == "yes");

    QString const draft_key(ipath.get_draft_key(users::users::instance()->get_user_info().get_identifier()));

    // these pointers are used to load existing data
    // and save new data
    // it is also shared with various signals
    QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(ipath.get_revision_key()));
    QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(ipath.get_key())); // same key as the content table
    QtCassandra::QCassandraRow::pointer_t draft_row(revision_table->row(draft_key));

    // the data_row will get initialized as required
    QtCassandra::QCassandraRow::pointer_t data_row;

    save_info_t save_info(ipath, editor_widgets, revision_row, secret_row, draft_row);

    // first load the XML code representing the editor widgets for this page
    if(!editor_widgets.isNull())
    {
        // a default (data driven) redirect to apply when saving an editor form
        //
        // we do that early so other plugins may change the value when they
        // get called
        //
        if(!on_save.isNull() && on_save.hasAttribute("redirect"))
        {
            QString const redirect(on_save.attribute("redirect"));
            //if(redirect == "...") { ... } -- support some semi-dynamic redirects? (i.e. parent)
            server_access::server_access::instance()->ajax_redirect(redirect, on_save.attribute("target"));
        }

        locale::locale * locale_plugin(locale::locale::instance());

        // make sure dates and times are properly handled
        locale_plugin->set_timezone();
        locale_plugin->set_locale();

        // make sure we do not have any spurious data in there
        // (other plugins may have forced a read of some fields which
        // is not actually defined in the database and we could thus
        // end up with an empty value and exists() returning true!)
        revision_row->clearCache();
        secret_row->clearCache();
        draft_row->clearCache();

        // now go through all the widgets checking out their path, if the
        // path exists in doc then save the data in Cassandra
        QDomNodeList widgets(editor_widgets.elementsByTagName("widget"));
        int const max_widgets(widgets.size());

        // ************ 1.
        //
        // first create maps of all the values available
        //
        // * those the user just sent us
        // * those in the draft if the user has one
        // * those in the revision table
        //
        f_post_values.clear();
        f_current_values.clear();
        f_draft_values.clear();
        f_default_values.clear();
        for(int i(0); i < max_widgets; ++i)
        {
            QDomElement widget(widgets.at(i).toElement());

            bool const is_secret(widget_is_secret(widget));

            QString const widget_name(widget.attribute("id"));
            QString const field_name(widget.attribute("field"));
            QString const widget_type(widget.attribute("type"));
            QString const widget_auto_save(widget.attribute("auto-save", "string")); // this one is #IMPLIED

            // TODO: the following XML validation should be done ONCE with
            //       an external tool at compile time
            if(widget_name.isEmpty())
            {
                throw snap_logic_exception(QString("ID of a widget on line %1 found in an editor XML document is missing.").arg(widget.lineNumber()));
            }
            if(widget_type.isEmpty())
            {
                throw snap_logic_exception(QString("TYPE of a widget on line %1 found in an editor XML document is missing.").arg(widget.lineNumber()));
            }
            if(field_name.isEmpty() && widget_auto_save != "no")
            {
                throw snap_logic_exception(QString("The \"field\" attribute of a widget on line %1 found in an editor XML document is missing. It is required when auto-save is ON.").arg(widget.lineNumber()));
            }

            if(f_snap->postenv_exists(widget_name))
            {
                f_post_values[widget_name] = clean_post_value(widget_type, f_snap->postenv(widget_name));
            }

            if(!field_name.isEmpty())
            {
                // validation fails if we do not have the default value
                // and there is one defined so we have to get such now
                QDomElement default_tag(widget.firstChildElement("default"));
                if(!default_tag.isNull())
                {
                    // we have a default value
                    f_default_values[widget_name] = snap_dom::xml_children_to_string(default_tag);
                }
                else
                {
                    QDomElement preset_tag(widget.firstChildElement("preset"));
                    if(!preset_tag.isNull())
                    {
                        for(QDomElement e(preset_tag.firstChildElement("item"));
                                        !e.isNull();
                                        e = e.nextSiblingElement())
                        {
                            if(e.hasAttribute("default"))
                            {
                                // the value of the attribute is not important
                                // the default value is either the value="..."
                                // or the child XML data from this item tag
                                if(e.hasAttribute("value"))
                                {
                                    f_default_values[widget_name] = e.attribute("value");
                                }
                                else
                                {
                                    f_default_values[widget_name] = snap_dom::xml_children_to_string(e);
                                }
                                break;
                            }
                        }
                    }
                }

                // secret values do not get saved in the draft, it would not be safe
                if(!is_secret && draft_row->exists(field_name))
                {
                    // get the draft value from the database
                    //
                    // note that was not converted, we only use strings in
                    // this row! (dbutils will not work right on these rows!)
                    f_draft_values[widget_name] = draft_row->cell(field_name)->value().stringValue();
                }

                if(auto_save || widget_auto_save != "no")
                {
                    if(is_secret)
                    {
                        data_row = secret_row;
                    }
                    else
                    {
                        data_row = revision_row;
                    }

                    // get the current value from the database
                    if(data_row->exists(field_name))
                    {
                        value_to_string_info_t value_info(ipath, widget, data_row->cell(field_name)->value());
                        value_to_string(value_info);
                        if(value_info.is_valid())
                        {
                            f_current_values[widget_name] = value_info.result();
                        }
                    }
                }
            }
        }

        // ************ 2.
        //
        // Second check all the values, if one or more errors occur, we save
        // the values in the draft row instead of the normal secret/revision
        // rows; this allows us to reload the data later from the draft instead
        // of the current revision (we will use the dates to know what to load)
        //
        for(int i(0); i < max_widgets; ++i)
        {
            QDomElement widget(widgets.at(i).toElement());

            bool const is_secret(widget_is_secret(widget));

            QString const widget_name(widget.attribute("id"));
            QString const field_name(widget.attribute("field"));
            QString const widget_type(widget.attribute("type"));
            QString const widget_auto_save(widget.attribute("auto-save", "string")); // this one is #IMPLIED

            // ignore the session identifier in this case
            if(field_name == get_name(name_t::SNAP_NAME_EDITOR_SESSION))
            {
                continue;
            }

            // note: the auto-save may not be turned on, we can still copy
            //       empty pointers around, it is fast enough
            if(is_secret)
            {
                data_row = secret_row;
            }
            else
            {
                data_row = revision_row;
            }

            // now validate using a signal so any plugin can take over
            // the validation process
            sessions::sessions::session_info::session_info_type_t const session_type(info.get_session_type());
            // pretend that everything is fine so far...
            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_VALID);

            QString current_value;

            // the priority is:
            //
            // * POST data
            // * Draft data (TODO: implement the date test)
            // * Current data (from database, also called current data)
            // * Default data (from the XML form)
            //
            if(f_post_values.contains(widget_name))
            {
                current_value = f_post_values[widget_name];
            }
            else if(f_draft_values.contains(widget_name))
            {
                current_value = f_draft_values[widget_name];
            }
            else if(f_current_values.contains(widget_name))
            {
                current_value = f_current_values[widget_name];
            }
            else if(f_default_values.contains(widget_name))
            {
                // We do not check the default value because the check
                // may actually fail on the default value! but the fact
                // that it is defined proves that we do not have to worry
                //current_value = f_default_Values[widget_name];
                continue;
            }
            //else -- currently undefined value, if it is required, it will generate an error

            int const errcnt(messages->get_error_count());
            int const warncnt(messages->get_warning_count());

            //
            // first do a validation, if that fails, we avoid the
            // conversion to a QtCassandraValue below
            //
            // TODO: change the parameters with a structure?
            //
            validate_editor_post_for_widget(ipath, info, widget, widget_name, widget_type, current_value, is_secret);

            //
            // if no errors occurred in the validation process, then attempt
            // a conversion
            //
            // note that there is no conversion necessary for widgets that
            // do not specify a field
            //
            if(info.get_session_type() == sessions::sessions::session_info::session_info_type_t::SESSION_INFO_VALID
            && !field_name.isEmpty()        // no field name, no access to the database at all
            && widget_auto_save != "no")    // no known data type when auto-save="no", so nothing to convert...
            {
                if(current_value.isEmpty())    // emptiness invalidity is check by validate_editor_post_for_widget()
                {
                    if(!save_info.has_errors())
                    {
                        // save the empty string as the result
                        f_converted_values[widget_name] = QString();
                    }
                }
                else
                {
                    string_to_value_info_t value_info(ipath, widget, current_value);
                    string_to_value(value_info);
                    if(value_info.is_valid())
                    {
                        // on errors we are not going to make use of these
                        // values so avoid wasting time on them
                        if(!save_info.has_errors())
                        {
                            // keep a copy of the result on success
                            f_converted_values[widget_name] = value_info.result();
                        }
                    }
                    else
                    {
                        QString label(widget.firstChildElement("label").text());
                        if(label.isEmpty())
                        {
                            label = widget_name;
                        }
                        messages->set_error(
                            "Type Conflict",
                            QString("Field \"%1\" must be a valid %2, \"%3\" is not acceptable.")
                                    .arg(label)
                                    .arg(value_info.get_type_name())
                                    .arg(form::form::html_64max(current_value, is_secret)),
                            "This could be a hacker unless the JavaScript does not check the value properly, assuming the JavaScript is implemented.",
                            false
                        ).set_widget_name(widget_name);
                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                    }
                }
            }

            if(info.get_session_type() != sessions::sessions::session_info::session_info_type_t::SESSION_INFO_VALID)
            {
                // it was not valid so mark the widgets as errorneous (i.e. so we
                // can display it with an error message)
                if(messages->get_error_count() == errcnt
                && messages->get_warning_count() == warncnt)
                {
                    // the plugin marked that it found an error but did not
                    // generate an actual error, do so here with a generic
                    // error message
                    QString label(widget.firstChildElement("label").text());
                    if(label.isEmpty())
                    {
                        label = widget_name;
                    }
                    messages->set_error(
                        "Invalid Content",
                        QString("\"%1\" is not valid for \"%2\".")
                                .arg(form::form::html_64max(current_value, is_secret))
                                .arg(label),
                        "unspecified error for widget",
                        false
                    ).set_widget_name(widget_name);
                }
                messages::messages::message const & msg(messages->get_last_message());

                // Add the following to the widget so we can display the
                // widget as having an error and show the error on request
                //
                // <error>
                //   <title>$title</title>
                //   <message>$message</message>
                // </error>

                QDomElement err_tag(editor_widgets.createElement("error"));
                err_tag.setAttribute("idref", QString("messages_message_%1").arg(msg.get_id()));
                widget.appendChild(err_tag);
                QDomElement title_tag(editor_widgets.createElement("title"));
                err_tag.appendChild(title_tag);
                QDomText title_text(editor_widgets.createTextNode(msg.get_title()));
                title_tag.appendChild(title_text);
                QDomElement message_tag(editor_widgets.createElement("message"));
                err_tag.appendChild(message_tag);
                QDomText message_text(editor_widgets.createTextNode(msg.get_body()));
                message_tag.appendChild(message_text);

                save_info.mark_as_having_errors();
                f_converted_values.clear(); // these are not going to be used
            }
            else
            {
                // restore the last type
                info.set_session_type(session_type);

                // TODO support for attachment so they do not just disappear on
                //      errors is required here; i.e. we need a way to be able
                //      to save all the valid attachments in a temporary place
                //      and then "move" them to their final location once the
                //      form validates properly
            }
        }
        // prevent further modification of various flags
        // (f_has_error at time of writing)
        //
        save_info.lock();

        // now we switch to a new revision in the event the data was not
        // considered erroneous
        //
        if(!save_info.has_errors() && auto_save)
        {
            // create the new revision and make it current
            //
            // TODO: if multiple users approval is required, we cannot make this
            //       new revision the current revision except if that's the very
            //       first (although the very first is not created here)
            //

            // make this newer revision the current one
            //
            if(switch_branch)
            {
                // TODO: test whether that branch already exists (it should not!)
                //
                content_plugin->copy_branch(key, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, branch_number);

                // working branch cannot really stay as the system branch
                // so force both branches in this case
                //
                content_plugin->set_branch(key, branch_number, false);
                content_plugin->set_branch(key, branch_number, true);
                content_plugin->set_branch_key(key, branch_number, true);
                content_plugin->set_branch_key(key, branch_number, false);
            }

            // get the revision number only AFTER the branch was created
            //
            // TODO: once we have a "save branch" the old_branch parameter needs
            //       to be corrected (another function anyway?)
            //
            snap_version::version_number_t revision_number(content_plugin->get_new_revision(key, branch_number, locale, true, switch_branch ? static_cast<snap_version::version_number_t>(snap_version::SPECIAL_VERSION_SYSTEM_BRANCH) : branch_number));

// TODO: add revision manager
//       the current/working revisions are not correctly handled yet...
//       we should not force to the latest every time, but for now it's
//       the way it is
            if(switch_branch || true)
            {
                // in that case we also need to save the new revision accordingly
                content_plugin->set_current_revision(key, branch_number, revision_number, locale, false);
                content_plugin->set_revision_key(key, branch_number, revision_number, locale, false);
            }
            content_plugin->set_current_revision(key, branch_number, revision_number, locale, true);
            content_plugin->set_revision_key(key, branch_number, revision_number, locale, true);

            // now save the new data
            ipath.force_branch(branch_number);
            ipath.force_revision(revision_number);

            // make sure the revision row is using the new key
            revision_row = revision_table->row(ipath.get_revision_key());

            // the draft and secret rows are not affected
        }

        // ************ 3.
        //
        // Third we save the data to either the secret and revision
        // rows or the draft row depending on the results of the
        // previous loop: if an error occurred save in the draft row,
        // except secret values that get dropped.
        //
        for(int i(0); i < max_widgets; ++i)
        {
            QDomElement widget(widgets.at(i).toElement());

            bool const is_secret(widget_is_secret(widget));

            QString const widget_name(widget.attribute("id"));
            QString const field_name(widget.attribute("field"));
            QString const widget_type(widget.attribute("type"));
            QString const widget_auto_save(widget.attribute("auto-save", "string")); // this one is #IMPLIED

            // if not in the post, totally ignore the value in the save process
            // (TBD: we most certainly need to support the draft values!!!
            //       because the editor is not sending them back!!!)
            //
            if(!f_post_values.contains(widget_name) || widget_auto_save == "no")
            {
                continue;
            }

            // note: the auto-save may not be turned on, we can still copy
            //       empty pointers around, it is fast enough
            if(save_info.has_errors())
            {
                if(is_secret)
                {
                    // drop secret rows on error because we cannot securely
                    // save them in the revision table; plus it does not
                    // make sense to memorize secrets that could then be
                    // sent back to a user years later
                    continue;
                }

                // in the draft row we save post data as is (as strings)
                draft_row->cell(field_name)->setValue(f_post_values[widget_name]);
            }
            else
            {
                if(is_secret)
                {
                    data_row = secret_row;
                }
                else
                {
                    data_row = revision_row;
                }

                if(!f_converted_values.contains(widget_name))
                {
                    // This is an internal error, all the values should
                    // have been converted if we reach this line!
                    throw snap_logic_exception(QString("value for widget named \"%1\" is missing.").arg(widget_name));
                }

                data_row->cell(field_name)->setValue(f_converted_values[widget_name]);
            }

            save_info.mark_as_modified();
        }
    }

    //
    // allow each plugin to save special fields (i.e. no auto-save)
    //
    // TBD: should we add the draft entry (and whether data was drafted
    //      or saved as normal)?
    //
    // TODO: determine whether the save_editor_fields() should NOT be called
    //       if no save took place
    //
    save_editor_fields(save_info);

    if(save_info.modified())
    {
        if(save_info.has_errors())
        {
            int64_t const start_date(f_snap->get_start_date());
            draft_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_MODIFIED))->setValue(start_date);
        }
        else
        {
            // if there was a draft, it got saved so drop it now
            revision_table->dropRow(draft_key);
        }

        // save the modification date in the branch
        content_plugin->modified_content(ipath);
    }
}


void editor::on_add_layout_from_resources(QString const & name)
{
    QtCassandra::QCassandraTable::pointer_t layout_table(layout::layout::instance()->get_layout_table());

    {
        QString const body(QString(":/xml/layout/%1-page.xml").arg(name));
        QFile file(body);
        if(file.open(QIODevice::ReadOnly))
        {
            QByteArray data(file.readAll());
            layout_table->row(name)->cell(QString("%1-page.xml").arg(name))->setValue(data);
        }
    }
}


/** \brief This function cleans the tainted data from a POST.
 *
 * This function attempts to clean a value that was just posted to us from
 * a client. The checks depend on the type of widget we are dealing with.
 *
 * \todo
 * Complete the function.
 *
 * \param[in] widget_type  The type of widget.
 * \param[in] value  The value to be cleaned up.
 *
 * \return The cleaned up value of the widget.
 */
QString editor::clean_post_value(QString const & widget_type, QString value)
{
    // first trim the value and remove the starting/ending <br> because those
    // are most often improperly added by editors.

    // trim at the start
    {
        QRegExp start_re("^(<br */?>| |\t|\n|\r|\v|\f|&nbsp;|&#160;|&#xA0;)+", Qt::CaseInsensitive, QRegExp::RegExp2);
        if(start_re.indexIn(value) != 0)
        {
            value.remove(0, start_re.matchedLength());
        }
    }

    // trim at the end
    {
        QRegExp end_re("(<br */?>| |\t|\n|\r|\v|\f|&nbsp;|&#160;|&#xA0;)+$", Qt::CaseInsensitive, QRegExp::RegExp2);
        int const p(end_re.indexIn(value));
        if(p > 0) // here it cannot be zero or we already removed all the characters
        {
            value.remove(p, end_re.matchedLength());
        }
    }

    // a line edit cannot include new line characters
    if(widget_type == "line-edit")
    {
        value.replace("\n", " ").replace("\r", " ");
        QRegExp break_line("<br */?>", Qt::CaseInsensitive, QRegExp::RegExp2);
        for(;;)
        {
            int const p(break_line.indexIn(value));
            if(p == -1)
            {
                // done removing all those enries
                break;
            }
            value.remove(p, break_line.matchedLength());
        }

        // TODO: check for any tag that represents a block (i.e. <div>)
    }

    // TODO: apply XSS filter as required for this user

    // TODO: offer other plugins to do their own clean up

    return value;
}


/** \brief Instant save attachment function.
 *
 * Attachment can be made to be saved instantaneously. If that feature is
 * used, then this function gets called at some point. The save is very
 * simply a normal create attachment to this page.
 *
 * \todo
 * We should put such attachments in a list of temporary attachments because
 * if the user cancels their upload, then we want to delete the attachment
 * otherwise we'd end up with many left overs...
 *
 * \param[in,out] ipath  The path to the page being updated.
 * \param[in,out] info  The session information, for the validation, just in case.
 * \param[in,out] server_access_plugin  The plugin used to build the output data for the AJAX request.
 */
void editor::editor_save_attachment(content::path_info_t & ipath, sessions::sessions::session_info & info, server_access::server_access * server_access_plugin)
{
    NOTUSED(info);

    mimetype::mimetype * mimetype_plugin(mimetype::mimetype::instance());

    // get the editor widgets and save them in a map
    typedef std::map<QString, QDomElement> widget_map_t;
    widget_map_t widgets_by_name;
    QDomDocument editor_widgets(get_editor_widgets(ipath));
    QDomNodeList widgets(editor_widgets.elementsByTagName("widget"));
    int const max_widgets(widgets.size());
    for(int i(0); i < max_widgets; ++i)
    {
        QDomElement widget(widgets.at(i).toElement());
        //bool const is_secret(widget_is_secret(widget));
        QString const widget_name(widget.attribute("id"));
        widgets_by_name[widget_name] = widget;
        //QString const field_name(widget.attribute("field"));
        //QString const widget_type(widget.attribute("type"));
        //QString const widget_auto_save(widget.attribute("auto-save", "string")); // this one is #IMPLIED
    }

    // by default let the attachment plugin handle attachments
    QString const default_attachment_owner(attachment::attachment::instance()->get_plugin_name());

    QString const widget_names(f_snap->postenv("_editor_widget_names"));
//std::cerr << "***\n*** Editor Processing POST... [" << ipath.get_key() << "::" << widget_names << "]\n***\n";

    snap_string_list const names(widget_names.split(","));
    for(int i(0); i < names.size(); ++i)
    {
        widget_map_t::const_iterator w(widgets_by_name.find(names[i]));
        if(w == widgets_by_name.end())
        {
            // TBD: should we check each field name BEFORE saving anything?
            f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_ACCEPTABLE, "Field Name Not Acceptable",
                QString("Editor widget named \"%1\" is not valid.").arg(names[i]),
                "Somehow the client sent us a reply with an invalid widget name.");
            NOTREACHED();
        }
        QDomNodeList attachment_tags(w->second.elementsByTagName("attachment"));
        int const max_attachments(attachment_tags.size());
        if(max_attachments >= 2)
        {
            throw editor_exception_too_many_tags(QString("you can have 0 or 1 attachment tag in a widget, you have %1 right now.").arg(max_attachments));
        }
        QString attachment_type("attachment"); // extremely restrained by default (i.e. visible by a "root" user only)
        QString attachment_owner(default_attachment_owner);
        QString force_filename;
        QString force_path("#");
        QDomElement attachment_tag;
        if(max_attachments == 1)
        {
            attachment_tag = attachment_tags.at(0).toElement();
            if(!attachment_tag.isNull())
            {
                attachment_type = attachment_tag.attribute("identification", "attachment");
                attachment_owner = attachment_tag.attribute("owner", default_attachment_owner);

                force_filename = attachment_tag.attribute("force-filename", ""); // this one is #IMPLIED
                force_path = attachment_tag.attribute("force-path", "#"); // this one is #IMPLIED
            }
        }

        content::path_info_t attachment_ipath;
        if(force_path == "#")
        {
            attachment_ipath = ipath;
        }
        else
        {
            attachment_ipath.set_path(force_path);
        }

        content::attachment_file the_attachment(f_snap, f_snap->postfile(names[i]));
        the_attachment.set_multiple(false);
        the_attachment.set_parent_cpath(attachment_ipath.get_cpath());
        the_attachment.set_field_name(names[i]);
        the_attachment.set_attachment_owner(attachment_owner);
        the_attachment.set_attachment_type(attachment_type);

        QString const mime_type(the_attachment.get_file().get_mime_type());

        // make sure the filename is all proper for our system
        //
        QString ext(mimetype_plugin->mimetype_to_extension(mime_type));
        QString filename(force_filename.isEmpty() ? the_attachment.get_file().get_filename() : force_filename);
        if(!filter::filter::filter_filename(filename, ext))
        {
            // user supplied filename is not considered valid, use a default name
            //
            filename = QString("attachment.%1").arg(ext);
        }
        the_attachment.set_file_filename(filename);

        // TBD: give others the opportunity to tweak the attachment and
        //      its parameters before it gets saved in the database
        //      (i.e. you may want to dynamically define the type)
        //blah();

        // TODO: define the locale in some ways... for now we use "", i.e. neutral
        //
        // TBD: we may want to follow the "secret" attribute, although
        //      attachments are saved in another table altogether anyway...
        //      and we do not (currently) offer scripts that can access
        //      attachment directly.
        content::content::instance()->create_attachment(the_attachment, ipath.get_branch(), "");

        QString const attachment_cpath(the_attachment.get_attachment_cpath());
        if(!attachment_cpath.isEmpty())
        {
            content::path_info_t final_attachment_ipath;
            final_attachment_ipath.set_path(attachment_cpath);
            server_access_plugin->ajax_append_data("attachment-path", final_attachment_ipath.get_key().toUtf8());
            QString const mime_type_icon(mimetype_plugin->mimetype_to_icon(mime_type));
            server_access_plugin->ajax_append_data("attachment-icon", mime_type_icon.toUtf8());
        }

        new_attachment_saved(the_attachment, w->second, attachment_tag);
    }
}


/** \brief This function reads the editor widgets.
 *
 * This function is used to read the editor widgets. The function caches
 * the editor form in memory so that way we can put errors in it and thus
 * when we generate the page we can put the errors linked to each widgets.
 *
 * \param[in,out] ipath  The path for which we look for an editor form.
 * \param[in] saving  Whether we are loading or saving.
 *
 * \return The QDomDocument representing the editor form, may be null.
 */
QDomDocument editor::get_editor_widgets(content::path_info_t & ipath, bool const saving)
{
    static QMap<QString, QDomDocument> g_cached_form;

    QString const cpath(ipath.get_cpath());
    if(!g_cached_form.contains(cpath))
    {
        QDomDocument editor_widgets;
        layout::layout * layout_plugin(layout::layout::instance());
        QString script(layout_plugin->get_layout(ipath, get_name(name_t::SNAP_NAME_EDITOR_LAYOUT), true));
        snap_string_list const script_parts(script.split("/"));
        if(script_parts.size() == 2)
        {
            if(script_parts[0].isEmpty()
            || script_parts[1].isEmpty())
            {
                f_snap->die(snap_child::http_code_t::HTTP_CODE_CONFLICT, "Conflict Error",
                    QString("Editor layout name \"%1\" is not valid. Names on both sides of the slash (/) must be defined.").arg(script),
                    "The editor layout name is not composed of two valid names separated by a slash (/) but it does contain a slash.");
                NOTREACHED();
            }
            script = script_parts[1];
        }
        else if(script_parts.size() != 1)
        {
            // the script parts cannot be empty even if we start with an
            // empty string so this code is unreachable
            //
            f_snap->die(snap_child::http_code_t::HTTP_CODE_CONFLICT, "Conflict Error",
                QString("Editor layout name \"%1\" is not valid.").arg(script),
                "The editor layout name is not composed of exactly one or two names.");
            NOTREACHED();
        }

        // if empty then there is nothing else to do, there is no editor form
        //
        if(!script_parts.isEmpty())
        {
            if(script == "default")
            {
                if(saving)
                {
                    // the default starts with our hard coded file from the resources
                    // other plugins can add to it whenever their
                    // dynamic_editor_widget() signal implementation is called.
                    //
                    QFile rc_widgets(":/xml/editor/default-page.xml");
                    if(!rc_widgets.open(QIODevice::ReadOnly))
                    {
                        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND, "Missing File",
                            "Editor default layout for a standard page could not be opened.",
                            "The editor \"default-page.xml\" layout file could not be opened.");
                        NOTREACHED();
                    }

                    QByteArray const data(rc_widgets.readAll());
                    if(data.isEmpty())
                    {
                        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND, "Missing File",
                            "Editor default layout for a standard page could not be read.",
                            "The editor \"default-page.xml\" layout file could not be read.");
                        NOTREACHED();
                    }

                    QString const widgets_xml(QString::fromUtf8(data.data(), data.size()));
                    if(widgets_xml.isEmpty())
                    {
                        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND, "Missing File",
                            "Editor default layout is empty.",
                            "The editor \"default-page.xml\" layout file is empty?");
                        NOTREACHED();
                    }

                    QDomDocument named_editor_widgets("editor-form");
                    editor_widgets = named_editor_widgets;
                    editor_widgets.setContent(widgets_xml);

                    dynamic_editor_widget(ipath, script, editor_widgets);
                }
            }
            else
            {
                // in this case we totally ignore the query string because it would
                // most certainly not correspond to the right theme (the one that
                // links us to the editor layout)
                //
                QString const theme_name(script_parts.size() == 2
                            ? script_parts[0] // force the layout::layout from the editor::layout
                            : layout_plugin->get_layout(ipath, layout::get_name(layout::name_t::SNAP_NAME_LAYOUT_THEME), false));
                QString widgets_xml;
                if(!theme_name.isEmpty())
                {
                    // always test for the data in the layout table first
                    //
                    QtCassandra::QCassandraTable::pointer_t layout_table(layout_plugin->get_layout_table());
                    widgets_xml = layout_table->row(theme_name)->cell(script + ".xml")->value().stringValue();
                }

                if(widgets_xml.isEmpty())
                {
                    // check for a file in the resources instead...
                    //
                    QFile rc_widgets(QString(":/xml/editor/%1.xml").arg(script));
                    if(rc_widgets.open(QIODevice::ReadOnly))
                    {
                        QByteArray const data(rc_widgets.readAll());
                        if(!data.isEmpty())
                        {
                            widgets_xml = QString::fromUtf8(data.data(), data.size());
                        }
                    }
                }

                if(widgets_xml.isEmpty())
                {
                    SNAP_LOG_WARNING("Could not find an editor layout parser file named \"")
                            (script)("\". We checked the row \"")
                            (theme_name)("\" in the \"layout\" table, then in Qt resources with filename \":/xml/editor/")
                            (script)(".xml\".");
                }
                else
                {
                    QDomDocument named_editor_widgets("editor-form");
                    editor_widgets = named_editor_widgets;
                    editor_widgets.setContent(widgets_xml);

                    dynamic_editor_widget(ipath, script, editor_widgets);
                }
            }
        }
        g_cached_form[cpath] = editor_widgets;
    }

    return g_cached_form[cpath];
}


/** \fn void editor::dynamic_editor_widget(content::path_info_t& ipath, QString const& name, QDomDocument& editor_widgets)
 * \brief Allow other plugins to dynamically add widgets.
 *
 * This message is sent to the plugins to give them a chance to dynamically
 * add dynamic widgets to a list of editor widgets.
 *
 * \param[in,out] ipath  The path to the page being handled.
 * \param[in] name  The name of the editor layout being loaded for this page.
 * \param[in,out] editor_widgets  The DOM with the editor widgets.
 */


/** \brief Check a widget to know whether its content is secret.
 *
 * This function simplifies checking whether a widget is secret or not.
 * This is whether the contents of the widget are to be saved in the
 * "secret" table or not.
 *
 * One can explicitly mark a field as secret in the XML declaration
 * of the widget.
 *
 * \code
 * <widget ... secret="secret" ...>
 * \endcode
 *
 * The function sends the editor_widget_type_is_secret() signal which
 * is thus given a chance to modify the widget just before it gets
 * used.
 *
 * \param[in,out] widget  The widget to be checked.
 *
 * \return true if the widget data is to be saved in the secret table.
 */
bool editor::widget_is_secret(QDomElement widget)
{
    content::permission_flag is_public;
    editor_widget_type_is_secret(widget, is_public);
    return !is_public.allowed();
}


/** \brief Check the widget type to know whether it is secret.
 *
 * Some widget types may be secret. This signal allows you to set
 * the is_public to "not permitted" depending on the type.
 *
 * The widget parameter is in/out so you may change it. For example,
 * widgets of type "password" have there "field_name" attribute
 * removed. This makes it a lot safer for such fields.
 *
 * At some point, we will have a tool to check files before adding
 * them to the Qt resources or a layout. That way we can check
 * everything. Until then but even after we want to keep security
 * checks at the time we check everything.
 *
 * \param[in,out] widget  The widget being checked.
 * \param[in] is_public  Whether the widget type is public or private.
 */
bool editor::editor_widget_type_is_secret_impl(QDomElement widget, content::permission_flag & is_public)
{
    // true if not "public" which is #IMPLIED
    if(widget.attribute("secret") == "secret")
    {
        is_public.not_permitted();
    }

    // now check the type
    QString const widget_type(widget.attribute("type"));
    if(widget_type == "password")
    {
        is_public.not_permitted();
        widget.removeAttribute("field");
        widget.setAttribute("auto-save", "no");
    }

    return true;
}


/** \brief Start a widget validation.
 *
 * This function prepares the validation of the specified widget by
 * applying common core validations proposed by the editor.
 *
 * The \p info parameter is used for the result. If something is wrong,
 * then the type of the session is changed from SESSION_INFO_VALID to
 * one of the SESSION_INFO_... that represent an error, in most cases we
 * use SESSION_INFO_INCOMPATIBLE.
 *
 * The supported validations are described on the website. There is a
 * brief list here:
 *
 * \li sizes -- minimum / maximum sizes, number of characters, number of
 *              lines, number of pixels (width x height)
 * \li required -- the data is required
 * \li duplicate-of -- verify that this is equal to another widget
 * \li filters -- validate using a filter: regex, name, date, datetime,
 *                decimal, email, emails, integer, time, min-date, max-date,
 *                min-time, max-time, uri, extensions, validate
 *
 * The filters/validate makes use of a JavaScript to know whether the value
 * is valid. The script is given the value and you can access with:
 *
 * \code
 *      plugins.editor.value
 *
 *      // values sent via the AJAX post
 *      plugins.editor.post_<name>     // the <name> is the id="..." value
 *
 *      // current values read from the database
 *      plugins.editor.current_<name>  // the <name> is the id="..." value
 *
 *      // for example
 *      var a = ParseInt(plugins.editor.value);
 *      return a >= -100 && a <= 100;
 * \endcode
 *
 * \warning
 * The \p value parameter represents HTML and not plain text even if in
 * many cases it will show up as plain text when this function gets called.
 * Most importantly, if you expect the string to be plain text (i.e. no
 * tags) special characters such as \<, >, and & will be encoded so you
 * want to call snap_dom::unescape() on such values. If the value may
 * include tags, it is more complicated. You may call snap_dom::remove_tags()
 * if you do not need to check the tags, though. Of course, if the value
 * expected cannot otherwise include those characters (i.e. an integer) then
 * there is no need for such drastic measures.
 *
 * \param[in,out] ipath  The path where the form is defined
 * \param[in,out] info  The information linked with this form (loaded
 *                      from the session)
 * \param[in] widget  The widget being tested
 * \param[in] widget_name  The name of the widget
 *                         (i.e. the id="..." attribute value)
 * \param[in] widget_type  The type of the widget
 *                         (i.e. the type="..." attribute value)
 * \param[in] value  The value being validated, it is an HTML value, even if
 *                   in many cases it will look like plain text.
 * \param[in] is_secret  If true, the field is considered a secret
 *                       field (i.e. a password.)
 *
 * \return Always return true so other plugins have a chance to validate too.
 */
bool editor::validate_editor_post_for_widget_impl(
            content::path_info_t & ipath,
            sessions::sessions::session_info & info,
            QDomElement const & widget,
            QString const & widget_name,
            QString const & widget_type,
            QString const & value,
            bool const is_secret)
{
    // TODO: we want to move that class to the editor class and make it
    //       public and use it to make the validate_editor_post_for_widget()
    //       signal call, that way we can have this code available to
    //       all plugins; we could even have all sorts of things available
    //       like ways to generate the error messages in an editor
    //       consistent way
    //
    class value_handler_t
    {
    public:
        value_handler_t(QString const & value)
            : f_value(value)
        {
        }

        QString const & get_value() const
        {
            return f_value;
        }

        int get_value_length() const
        {
            return f_value.length();
        }

        QString get_stripped_value() const
        {
            if(!f_stripped_value_defined)
            {
                f_stripped_value_defined = true;
                f_stripped_value = snap_dom::remove_tags(f_value);
            }
            return f_stripped_value;
        }

        int get_stripped_value_length()
        {
            return get_stripped_value().length();
        }

    private:
        QString const &         f_value;
        mutable bool            f_stripped_value_defined = false;
        mutable QString         f_stripped_value;
    };
    value_handler_t value_handler(value);

    messages::messages * messages(messages::messages::instance());
    locale::locale * locale_plugin(locale::locale::instance());

    bool has_minimum(false);

//std::cerr << "value [" << value << "] for [" << widget_name << "]\n";

    QString label(widget.firstChildElement("label").text());
    if(label.isEmpty())
    {
        label = widget_name;
    }

    {
        // Check the minimum and maximum length / sizes / dimensions
        QDomElement sizes(widget.firstChildElement("sizes"));
        if(!sizes.isNull())
        {
            // minimum number of characters, for images minimum width and height
            QDomElement absolute_min_element(sizes.firstChildElement("absolute-min"));
            if(!absolute_min_element.isNull())
            {
                has_minimum = true;
                QString const m(absolute_min_element.text());
                bool ok;
                int const l(m.toInt(&ok));
                if(!ok)
                {
                    throw editor_exception_invalid_editor_form_xml(QString("the absolute minimum size \"%1\" must be a valid decimal integer").arg(m));
                }
                if(value_handler.get_value_length() < l)
                {
                    // length too small
                    messages->set_error(
                        "Absolute Length Too Small",
                        QString("\"%1\" is too small in \"%2\". The widget requires at least %3 characters of any type.")
                                .arg(form::form::html_64max(value, is_secret)).arg(label).arg(m),
                        QString("not enough characters in \"%1\"").arg(widget_name),
                        is_secret
                    ).set_widget_name(widget_name);
                    info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                }
            }

            // minimum number of VISIBLE characters, for images minimum width and height
            QDomElement min_element(sizes.firstChildElement("min"));
            if(!min_element.isNull())
            {
                has_minimum = true;
                QString const m(min_element.text());
                if(widget_type == "image-box"
                || widget_type == "dropped-file"
                || widget_type == "dropped-file-with-preview"
                || widget_type == "dropped-image-with-preview"
                || widget_type == "dropped-any-with-preview")
                {
                    int width, height;
                    if(!form::form::parse_width_height(m, width, height))
                    {
                        // invalid width 'x' height
                        messages->set_error(
                            "Invalid Sizes",
                            QString("minimum size \"%1\" is not a valid \"width 'x' height\" definition for image widget \"%2\".")
                                .arg(form::form::html_64max(m, false)).arg(label),
                            QString("incorrect sizes for \"%1\"").arg(widget_name),
                            false
                        ).set_widget_name(widget_name);
                        // TODO add another type of error for setup ("programmer") data?
                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                    }
                    else if(f_snap->postfile_exists(widget_name))
                    {
                        snap_child::post_file_t const& image(f_snap->postfile(widget_name));
                        int image_width(image.get_image_width());
                        int image_height(image.get_image_height());
                        if(width == 0 || height == 0)
                        {
                            messages->set_error(
                                "Incompatible Image File",
                                QString("The image \"%1\" was not recognized as a supported image file format.").arg(label),
                                QString("the system did not recognize the image as such (width/height are not valid), cannot verify the minimum size in \"%1\"").arg(widget_name),
                                is_secret
                            ).set_widget_name(widget_name);
                            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                        }
                        else if(image_width < width || image_height < height)
                        {
                            messages->set_error(
                                "Image Too Small",
                                QString("The image \"%1\" you uploaded is too small (your image is %2x%3, the minimum required is %4x%5).")
                                        .arg(label).arg(image_width).arg(image_height).arg(width).arg(height),
                                "the user uploaded an image that is too small",
                                is_secret
                            ).set_widget_name(widget_name);
                            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                        }
                    }
                }
                else
                {
                    bool ok;
                    int const l(m.toInt(&ok));
                    if(!ok)
                    {
                        throw editor_exception_invalid_editor_form_xml(QString("the minimum size \"%1\" must be a valid decimal integer").arg(m));
                    }
                    if(value_handler.get_stripped_value_length() < l)
                    {
                        // length too small
                        messages->set_error(
                            "Length Too Small",
                            QString("\"%1\" is too small in \"%2\". The widget requires at least %3 characters.")
                                    .arg(form::form::html_64max(value, is_secret)).arg(label).arg(m),
                            QString("not enough characters in \"%1\"").arg(widget_name),
                            is_secret
                        ).set_widget_name(widget_name);
                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                    }
                }
            }

            // maximum number of characters, for images maximum width and height
            QDomElement absolute_max_element(sizes.firstChildElement("absolute-max"));
            if(!absolute_max_element.isNull())
            {
                QString const m(absolute_max_element.text());
                bool ok;
                int const l(m.toInt(&ok));
                if(!ok)
                {
                    throw editor_exception_invalid_editor_form_xml(QString("the maximum size \"%1\" must be a valid decimal integer").arg(m));
                }
                if(value_handler.get_value_length() > l)
                {
                    // length too large
                    messages->set_error(
                        "Length Too Long",
                        QString("\"%1\" is too long in \"%2\". The widget requires at most %3 characters.")
                                .arg(form::form::html_64max(value, is_secret)).arg(label).arg(m),
                        QString("too many characters in \"%1\"").arg(widget_name),
                        is_secret
                    ).set_widget_name(widget_name);
                    info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                }
            }

            // maximum number of characters, for images maximum width and height
            QDomElement max_element(sizes.firstChildElement("max"));
            if(!max_element.isNull())
            {
                QString const m(max_element.text());
                if(widget_type == "image-box"
                || widget_type == "dropped-file"
                || widget_type == "dropped-file-with-preview"
                || widget_type == "dropped-image-with-preview"
                || widget_type == "dropped-any-with-preview")
                {
                    int width, height;
                    if(!form::form::parse_width_height(m, width, height))
                    {
                        // invalid width 'x' height
                        messages->set_error(
                            "Invalid Sizes",
                            QString("maximum size \"%1\" is not a valid \"width 'x' height\" definition for this image widget.")
                                    .arg(form::form::html_64max(m, false)),
                            "incorrect sizes for " + widget_name,
                            false
                        ).set_widget_name(widget_name);
                        // TODO add another type of error for setup ("programmer") data?
                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                    }
                    else if(f_snap->postfile_exists(widget_name))
                    {
                        snap_child::post_file_t const & image(f_snap->postfile(widget_name));
                        int image_width(image.get_image_width());
                        int image_height(image.get_image_height());
                        if(width == 0 || height == 0)
                        {
                            // TODO avoid error a 2nd time if done in minimum case
                            messages->set_error(
                                "Incompatible Image File",
                                QString("The image \"%1\" was not recognized as a supported image file format.").arg(label),
                                QString("the system did not recognize the image as such (width/height are not valid), cannot verify the minimum size of \"%1\"").arg(widget_name),
                                is_secret
                            ).set_widget_name(widget_name);
                            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                        }
                        else if(image_width > width || image_height > height)
                        {
                            messages->set_error(
                                "Image Too Large",
                                QString("The image \"%1\" you uploaded is too large (your image is %2x%3, the maximum allowed is %4x%5).")
                                    .arg(label).arg(image_width).arg(image_height).arg(width).arg(height),
                                QString("the user uploaded an image that is too large for \"%1\"").arg(widget_name),
                                is_secret
                            ).set_widget_name(widget_name);
                            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                        }
                    }
                }
                else
                {
                    bool ok;
                    int const l(m.toInt(&ok));
                    if(!ok)
                    {
                        throw editor_exception_invalid_editor_form_xml(QString("the maximum size \"%1\" must be a valid decimal integer").arg(m));
                    }
                    if(value_handler.get_stripped_value_length() > l)
                    {
                        // length too large
                        messages->set_error(
                            "Length Too Long",
                            QString("\"%1\" is too long in \"%2\". The widget requires at most %3 characters.")
                                    .arg(form::form::html_64max(value, is_secret)).arg(label).arg(m),
                            QString("too many characters in \"%1\"").arg(widget_name),
                            is_secret
                        ).set_widget_name(widget_name);
                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                    }
                }
            }

            // maximum number of lines
            QDomElement min_lines(sizes.firstChildElement("min-lines"));
            QDomElement max_lines(sizes.firstChildElement("max-lines"));
            if(!min_lines.isNull()
            || !max_lines.isNull())
            {
                QString min_str("-1");
                QString max_str("-1");
                int min_value(-1);
                int max_value(-1);
                bool ok(false);

                // minimum defined?
                if(!min_lines.isNull())
                {
                    min_str = min_lines.text();
                    min_value = min_str.toInt(&ok);
                    if(!ok || min_value < 0)
                    {
                        throw editor_exception_invalid_editor_form_xml(QString("the number of min-lines \"%1\" must be a valid and positive decimal integer").arg(min_str));
                    }
                }

                // maximum defined?
                if(!max_lines.isNull())
                {
                    max_str = max_lines.text();
                    max_value = max_str.toInt(&ok);
                    if(!ok || max_value < 0)
                    {
                        throw editor_exception_invalid_editor_form_xml(QString("the number of max-lines \"%1\" must be a valid and positive decimal integer").arg(max_str));
                    }
                }

                // sorted properly?
                if(min_value != -1 && max_value != -1 && max_value < min_value)
                {
                    throw editor_exception_invalid_editor_form_xml(QString("the number of min-lines \"%1\" is smaller than max-lines \"%2\"").arg(min_str).arg(max_str));
                }

                if(widget_type == "text-edit"
                || widget_type == "html-edit")
                {
                    // calculate the number of lines in value
                    int const lines(form::form::count_text_lines(value));
                    if(min_value != -1 && lines < min_value)
                    {
                        // not enough lines (text)
                        messages->set_error(
                            "Not Enough Lines",
                            QString("\"%1\" does not include enough lines in \"%2\". The widget requires at least %3 lines.")
                                    .arg(form::form::html_64max(value, is_secret)).arg(label).arg(min_str),
                            QString("not enough lines in \"%1\"").arg(widget_name),
                            is_secret
                        ).set_widget_name(widget_name);
                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                    }
                    if(max_value != -1 && lines > max_value)
                    {
                        // too many lines (text)
                        messages->set_error(
                            "Too Many Lines",
                            QString("\"%1\" has too many lines in \"%2\". The widget accepts at most %3 lines.")
                                    .arg(form::form::html_64max(value, is_secret)).arg(label).arg(max_str),
                            QString("not enough lines in \"%1\"").arg(widget_name),
                            is_secret
                        ).set_widget_name(widget_name);
                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                    }
                }
            }
        }
    }

    {
        // check whether the field is required, in case of a checkbox required
        // means that the user selects the checkbox ("on")
        if(widget_type == "line-edit"
        //|| widget_type == "password" -- not yet implemented
        || widget_type == "checkbox"
        || widget_type == "radio"
        || widget_type == "image-box"
        || widget_type == "dropped-file"
        || widget_type == "dropped-file-with-preview"
        || widget_type == "dropped-image-with-preview"
        || widget_type == "dropped-any-with-preview")
        {
            QDomElement required(widget.firstChildElement("required"));
            if(!required.isNull())
            {
                QString const required_text(required.text());
                if(required_text == "required")
                {
                    // It is required!
                    if(widget_type == "dropped-file"
                    || widget_type == "dropped-file-with-preview")
                    {
                        content::path_info_t file_ipath;
                        file_ipath.set_path(f_snap->postenv(widget_name));
                        content::path_info_t attachment_ipath;
                        file_ipath.get_parent(attachment_ipath);
                        //if(!f_snap->postfile_exists(widget_name)) // the field is just a string (path) -- the editor sends files at the time they get dropped
                        {
                            QString const name(QString("%1::%2::%3")
                                    .arg(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT))
                                    .arg(widget_name)
                                    .arg(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT_PATH_END)));
                            QtCassandra::QCassandraValue cassandra_value(content::content::instance()->get_content_parameter(attachment_ipath, name, content::content::param_revision_t::PARAM_REVISION_GLOBAL));
//SNAP_LOG_WARNING("widget_name -- [")(widget_name)
//                ("] -> [")(f_snap->postenv(widget_name))
//                ("] for [")(attachment_ipath.get_key())
//                ("] / name = [")(name)
//                ("] and result [")(cassandra_value.stringValue())
//                ("]");
                            if(cassandra_value.nullValue())
                            {
                                // not defined!
                                messages->set_error(
                                        "Invalid Value",
                                        QString("\"%1\" is a required field.").arg(label),
                                        QString("no file attached by user in widget \"%1\"").arg(widget_name),
                                        is_secret
                                    ).set_widget_name(widget_name);
                                info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                            }
                            // TBD: should we verify that the URI is exactly the same?
                            //      (i.e. I am thinking it should be)
                            //if(cassandra_value.stringValue() != f_snap->postenv(widget_name))
                        }
                    }
                    else if(widget_type == "image-box"
                         || widget_type == "dropped-image-with-preview"
                         || widget_type == "dropped-any-with-preview")
                    {
                        // here whether has_minimum is set does not matter
                        if(!f_snap->postfile_exists(widget_name)) // TBD <- this test is not logical if widget_type cannot be a FILE type...
                        {
                            if(value.isEmpty())
                            {
                                messages->set_error(
                                        "Value is Invalid",
                                        QString("\"%1\" is a required field.").arg(label),
                                        QString("no data dropped in widget \"%1\" by user").arg(widget_name),
                                        is_secret
                                    ).set_widget_name(widget_name);
                                info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                            }
                        }
                    }
                    else
                    {
                        // not an additional error if the minimum error was
                        // already generated
                        if(!has_minimum && value.isEmpty())
                        {
                            messages->set_error(
                                    "Value is Invalid",
                                    QString("\"%1\" is a required field.").arg(label),
                                    QString("no data entered in widget \"%1\" by user").arg(widget_name),
                                    is_secret
                                ).set_widget_name(widget_name);
                            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                        }
                    }
                }
            }
        }
    }

    {
        // check whether the widget has a "duplicate-of" attribute, if so
        // then it must be equal to that other widget's value
        QString duplicate_of(widget.attribute("duplicate-of"));
        if(!duplicate_of.isEmpty())
        {
            // What we need is the name of the widget so we can get its
            // current value and the duplicate-of attribute is just that!
            QString const duplicate_value(f_snap->postenv(duplicate_of));
            if(duplicate_value != value)
            {
                QString dup_label(duplicate_of);
                QDomXPath dom_xpath;
                dom_xpath.setXPath(QString("/snap-form//widget[@id=\"%1\"]/@id").arg(duplicate_of));
                QDomXPath::node_vector_t result(dom_xpath.apply(widget));
                if(result.size() > 0 && result[0].isElement())
                {
                    // we found the widget, display its label instead
                    dup_label = result[0].toElement().text();
                }
                messages->set_error(
                  "Value is Invalid",
                  QString("\"%1\" must be an exact copy of \"%2\". Please try again.")
                        .arg(label).arg(dup_label),
                  QString("confirmation widget \"%1\" is not equal to the original \"%2\" (i.e. most likely a password confirmation)")
                        .arg(widget_name).arg(duplicate_of),
                  is_secret
                ).set_widget_name(widget_name);
                info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
            }
        }
    }

    {
        QDomElement filters(widget.firstChildElement("filters"));
        if(!filters.isNull()
        && !value.isEmpty()) // emptiness was checked with the "required" test
        {
            // regular expression
            {
                QDomElement regex_tag(filters.firstChildElement("regex"));
                if(!regex_tag.isNull())
                {
                    QString re;

                    // not an email address by default; -1 any number, 1+ max. number
                    int email(0);
                    // not a date by default
                    enum class date_t
                    {
                        NO_DATE,
                        DATE_ONLY,
                        TIME_ONLY,
                        DATE_AND_TIME
                    };
                    date_t date(date_t::NO_DATE);

                    QString const regex_name(regex_tag.attribute("name"));
                    if(!regex_name.isEmpty())
                    {
                        switch(regex_name[0].unicode())
                        {
                        case 'd':
                            if(regex_name == "date")
                            {
                                date = date_t::DATE_ONLY;
                            }
                            else if(regex_name == "datetime")
                            {
                                date = date_t::DATE_AND_TIME;
                            }
                            else if(regex_name == "decimal")
                            {
                                re = "^[0-9]+(?:\\.[0-9]+)?$";
                            }
                            break;

                        case 'e':
                            if(regex_name.startsWith("email("))
                            {
                                int const pos(regex_name.lastIndexOf(")"));
                                if(pos > 6)
                                {
                                    QString const count(regex_name.mid(6, pos - 6));
                                    bool ok(false);
                                    email = count.toInt(&ok);
                                    if(!ok)
                                    {
                                        // it did not work...
                                        email = 0;
                                    }
                                }
                                if(email == 0)
                                {
                                    f_snap->die(
                                        snap_child::http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR,
                                        "Internal Server Error",
                                        QString("The server could not parse the email filter in \"%1\".").arg(regex_name),
                                        "The email format could not properly be parsed.");
                                    NOTREACHED();
                                }
                            }
                            else if(regex_name == "email")
                            {
                                // one email address
                                email = 1;
                            }
                            else if(regex_name == "emails")
                            {
                                // unlimited number of email addresses
                                email = -1;
                            }
                            break;

                        case 'f':
                            if(regex_name == "float")
                            {
                                re = "^[0-9]+(?:\\.[0-9]+)?(?:[eE][-+]?[0-9]+)?$";
                            }
                            break;

                        case 'i':
                            if(regex_name == "integer")
                            {
                                re = "^[0-9]+$";
                            }
                            break;

                        case 'p':
                            if(regex_name == "percent")
                            {
                                // 0.00% where one set of digits before or
                                // after the decimal point are optional
                                //
                                re = "^[-+]?(?:(?:[0-9]+(?:\\.[0-9]+)?)|(?:[0-9]*\\.[0-9]+))%$";
                            }
                            break;

                        case 't':
                            if(regex_name == "time")
                            {
                                date = date_t::TIME_ONLY;
                            }
                            break;

                        case 's':
                            if(regex_name == "signed-decimal")
                            {
                                re = "^[-+]?[0-9]+(?:\\.[0-9]+)?$";
                            }
                            else if(regex_name == "signed-integer")
                            {
                                re = "^[-+]?[0-9]+$";
                            }
                            break;

                        }
                        // We need to have a better check of the XML so we can
                        // make sure that this is an error, however, this is
                        // not considered an error here because another plugin
                        // may be able to understand a named regex...
                        //
                        //if(re.isEmpty() && email == 0 && date == date_t::NO_DATE)
                        //{
                        //    // TBD: this can be a problem if we remove a plugin that
                        //    //      adds some regexes (although right now we do not
                        //    //      have such a signal...)
                        //    throw editor_exception_invalid_editor_form_xml(QString("the regular expression named \"%1\" is not supported.").arg(regex_name));
                        //}
                    }
                    else
                    {
                        // Note:
                        // We do not test whether there is some text here to avoid
                        // wasting time; we should have such a test in a tool of
                        // ours used to verify that the editor form is well defined.
                        re = regex_tag.text();
                    }

                    if(email != 0)
                    {
                        tld_email_list emails;
                        if(emails.parse(snap_dom::unescape(value).toUtf8().data(), 0) != TLD_RESULT_SUCCESS)
                        {
                            messages->set_error(
                                "Invalid Value",
                                QString("\"%1\" is not a valid email address for field \"%2\".")
                                        .arg(form::form::html_64max(value, is_secret)).arg(label),
                                QString("failed to check the label value for \"%1\"")
                                        .arg(widget_name),
                                is_secret
                            ).set_widget_name(widget_name);
                            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                        }
                        else if(email != -1 && emails.count() > email) // if email is -1 then any number is fine
                        {
                            messages->set_error(
                                "Invalid Value",
                                QString("\"%1\" includes too many emails, \"%2\" expected at most %3 %4.")
                                        .arg(form::form::html_64max(value, is_secret))
                                        .arg(label)
                                        .arg(email)
                                        .arg(email == 1 ? "address" : "addresses"),
                                QString("failed because \"%1\" expects only one email address")
                                        .arg(widget_name),
                                is_secret
                            ).set_widget_name(widget_name);
                            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                        }
                    }
                    else if(date != date_t::NO_DATE)
                    {
                        // break parts date / time
                        snap_string_list parts;
                        if(date == date_t::DATE_AND_TIME)
                        {
                            // TODO: look at create a parse_date_and_time()
                            //       function instead
                            //
                            parts = value.split(" ");

                            // remove empty entries (i.e. multiple spaces)
                            for(int i(parts.size() - 1); i >= 0; --i)
                            {
                                if(parts[i].isEmpty())
                                {
                                    parts.removeAt(i);
                                }
                            }
                        }
                        else
                        {
                            parts << value;
                        }

                        if(((date == date_t::DATE_ONLY || date == date_t::TIME_ONLY) && parts.size() != 1)
                        || (date == date_t::DATE_AND_TIME && parts.size() != 2))
                        {
                            messages->set_error(
                                "Invalid Value",
                                QString("\"%1\" is not valid for \"%2\".")
                                        .arg(form::form::html_64max(value, is_secret)).arg(label),
                                QString("widget \"%1\" does not represent a valid date and/or time")
                                        .arg(widget_name),
                                is_secret
                            ).set_widget_name(widget_name);
                            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                        }
                        else
                        {
                            // check date?
                            if(date == date_t::DATE_ONLY || date == date_t::DATE_AND_TIME)
                            {
                                // use the locale to make sure we get a check depending on the
                                // user locale; also the separator varies depending on the locale
                                // (i.e. dashes (-), slashes (/), periods (.), etc.)
                                //
                                // TBD: we were testing the validity of the date below (i.e. so
                                //      as to avoid certain days in 1752) although that really
                                //      depends on the locale and if a function has to take
                                //      care of that test it will be the parse_date()
                                //
                                locale::locale::parse_error_t errcode;
                                locale_plugin->parse_date(parts[0], errcode);
                                if(errcode != locale::locale::parse_error_t::PARSE_NO_ERROR)
                                {
                                    messages->set_error(
                                        "Invalid Value",
                                        QString("\"%1\" is not a valid date for \"%2\".")
                                                .arg(form::form::html_64max(value, is_secret)).arg(label),
                                        QString("the date did not validate for \"%1\"")
                                                .arg(widget_name),
                                        is_secret
                                    ).set_widget_name(widget_name);
                                    info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                                }
                            }
                            // check time?
                            if(date == date_t::TIME_ONLY || date == date_t::DATE_AND_TIME)
                            {
                                // get part 1 if we had a date (date == date_t::DATE_AND_TIME)
                                // accept : or . as separator
                                int const index(date == date_t::TIME_ONLY ? 0 : 1);
                                locale::locale::parse_error_t errcode;
                                locale_plugin->parse_time(parts[index], errcode);
                                if(errcode != locale::locale::parse_error_t::PARSE_NO_ERROR)
                                {
                                    messages->set_error(
                                        "Invalid Value",
                                        QString("\"%1\" is not a valid time for \"%2\".")
                                                .arg(form::form::html_64max(value, is_secret)).arg(label),
                                        QString("the time did not validate for \"%1\"")
                                                .arg(widget_name),
                                        is_secret
                                    ).set_widget_name(widget_name);
                                    info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                                }
                            }
                        }
                    }
                    else
                    {
                        Qt::CaseSensitivity cs(Qt::CaseSensitive);
                        if(!re.isEmpty() && re[0] == '/')
                        {
                            re = re.mid(1);
                            int const p(re.lastIndexOf('/'));
                            if(p >= 0)
                            {
                                QString const flags(re.mid(p + 1));
                                re = re.mid(0, p);
                                for(auto s(flags.data()); s->unicode() != '\0'; ++s)
                                {
                                    switch(s->unicode())
                                    {
                                    case 'i':
                                        cs = Qt::CaseInsensitive;
                                        break;

                                    default:
                                        throw editor_exception_invalid_editor_form_xml(QString("\"%1\" is not a supported regex flag").arg(*s));

                                    }
                                }
                            }
                        }
                        QRegExp reg_expr(re, cs, QRegExp::RegExp2);
                        if(!reg_expr.isValid())
                        {
                            throw editor_exception_invalid_editor_form_xml(QString("\"%1\" regular expression is invalid.").arg(re));
                        }
                        bool const inverse_match(regex_tag.attribute("match").toLower() == "no");
                        if((reg_expr.indexIn(value) == -1) ^ inverse_match)
                        {
                            messages->set_error(
                                "Invalid Value",
                                QString("\"%1\" is not valid for \"%2\".")
                                        .arg(form::form::html_64max(value, is_secret)).arg(label),
                                QString("the value did %1match the filter regular expression of \"%2\"")
                                        .arg(inverse_match ? "" : "not ")
                                        .arg(widget_name),
                                is_secret
                            ).set_widget_name(widget_name);
                            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                        }
                    }
                }
            }

            // minimum/maximum value (integers / floats)
            {
                QDomElement min_value(filters.firstChildElement("min-value"));
                QDomElement max_value(filters.firstChildElement("max-value"));
                if(!min_value.isNull()
                || !max_value.isNull())
                {
                    // first test whether the user entry was valid, if not
                    // just skip this test 100% -- here we assume double
                    // numbers; to force integers, use the integer regex
                    //
                    bool ok(false);
                    double const v(value.toDouble(&ok));
                    if(ok)
                    {
                        QString min_str("-1");
                        QString max_str("-1");
                        double min_bound(std::numeric_limits<double>::quiet_NaN());
                        double max_bound(std::numeric_limits<double>::quiet_NaN());

                        if(!min_value.isNull())
                        {
                            min_str = min_value.text();
                            min_bound = min_str.toDouble(&ok);
                            if(!ok)
                            {
                                throw editor_exception_invalid_editor_form_xml(QString("the minimum value \"%1\" must be a valid number").arg(min_str));
                            }
                        }

                        if(!max_value.isNull())
                        {
                            max_str = max_value.text();
                            max_bound = max_str.toDouble(&ok);
                            if(!ok)
                            {
                                throw editor_exception_invalid_editor_form_xml(QString("the maximum value \"%1\" must be a valid number").arg(max_str));
                            }
                        }

                        if(!std::isnan(min_bound)
                        && !std::isnan(max_bound)
                        && max_bound < min_bound)
                        {
                            throw editor_exception_invalid_editor_form_xml(QString("the minimum number \"%1\" is not smaller than the maximum number \"%2\"").arg(min_str).arg(max_str));
                        }

                        // Note: if 'value' is not a valid date, we ignore the error
                        //       at this point, we catch it below if the user asked
                        //       for the format to be checked with a regex filter
                        //       named 'date' or 'datetime'.
                        //  
                        if(!std::isnan(min_bound) && v < min_bound)
                        {
                            // number is too small
                            messages->set_error(
                                "Too Small",
                                QString("\"%1\" is too small for \"%2\". The widget requires a minimum value of \"%3\".")
                                        .arg(form::form::html_64max(value, is_secret)).arg(label).arg(min_str),
                                QString("unexpected number in \"%1\"").arg(widget_name),
                                is_secret
                            ).set_widget_name(widget_name);
                            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                        }

                        if(!std::isnan(max_bound) && v > max_bound)
                        {
                            // number is too large
                            messages->set_error(
                                "Too Large",
                                QString("\"%1\" is too large for \"%2\". The widget requires a maximum value of \"%3\".")
                                        .arg(form::form::html_64max(value, is_secret)).arg(label).arg(max_str),
                                QString("unexpected number in \"%1\"").arg(widget_name),
                                is_secret
                            ).set_widget_name(widget_name);
                            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                        }
                    }
                }
            }

            // minimum/maximum date
            {
                QDomElement min_date(filters.firstChildElement("min-date"));
                QDomElement max_date(filters.firstChildElement("max-date"));
                if(!min_date.isNull()
                || !max_date.isNull())
                {
                    // first test whether the user entry was valid, if not
                    // just skip this test 100% -- if the programmer wants
                    // a valid date every time, he has to use the regex
                    // tag with the name attribute set to date or datetime:
                    //
                    //     <regex name="date"/>
                    //
                    locale::locale::parse_error_t errcode(locale::locale::parse_error_t::PARSE_NO_ERROR);
                    time_t date_value(0);
                    // first try a conversion of the value using the
                    // string_to_value() signal, if that returns a date
                    // use that date (which is expected to be in microseconds)
                    //
                    // TODO: look into whether we could call the
                    //       string_to_value() before the validation
                    //       and pass the results to the function;
                    //       that way we could call it once instead
                    //       of twice
                    //
                    string_to_value_info_t value_info(ipath, widget, value);
                    string_to_value(value_info);
                    if(value_info.is_valid()
                    && value_info.get_type_name() == "date")
                    {
                        date_value = value_info.result().safeInt64Value() / 1000000LL;
                    }
                    else
                    {
                        date_value = locale_plugin->parse_date(value, errcode);
                    }
                    if(errcode == locale::locale::parse_error_t::PARSE_NO_ERROR)
                    {
                        QString min_str("-1");
                        QString max_str("-1");
                        time_t min_time(-1);
                        time_t max_time(-1);

                        if(!min_date.isNull())
                        {
                            min_str = min_date.text();
                            min_time = locale_plugin->parse_date(min_str, errcode);
                            if(errcode != locale::locale::parse_error_t::PARSE_NO_ERROR)
                            {
                                throw editor_exception_invalid_editor_form_xml(QString("the minimum date \"%1\" must be a valid date").arg(min_str));
                            }
                        }

                        if(!max_date.isNull())
                        {
                            max_str = max_date.text();
                            max_time = locale_plugin->parse_date(max_str, errcode);
                            if(errcode != locale::locale::parse_error_t::PARSE_NO_ERROR)
                            {
                                throw editor_exception_invalid_editor_form_xml(QString("the maximum date \"%1\" must be a valid date").arg(max_str));
                            }
                        }

                        if(min_time != -1 && max_time != -1 && max_time < min_time)
                        {
                            throw editor_exception_invalid_editor_form_xml(QString("the minimum date \"%1\" is not smaller than the maximum date \"%2\"").arg(min_str).arg(max_str));
                        }

                        // Note: if 'value' is not a valid date, we ignore the error
                        //       at this point, we catch it below if the user asked
                        //       for the format to be checked with a regex filter
                        //       named 'date'.
                        //  
                        if(min_time != -1 && date_value < min_time)
                        {
                            // date is too small
                            messages->set_error(
                                "Too Old",
                                QString("\"%1\" is too far in the past for \"%2\". The widget requires a date starting on \"%3\".")
                                        .arg(form::form::html_64max(value, is_secret)).arg(label).arg(min_str),
                                QString("unexpected date in \"%1\"").arg(widget_name),
                                is_secret
                            ).set_widget_name(widget_name);
                            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                        }

                        if(max_time != -1 && date_value > max_time)
                        {
                            // date is too large
                            messages->set_error(
                                "Too Recent",
                                QString("\"%1\" is too far in the future for \"%2\". The widget requires a date ending on \"%3\".")
                                        .arg(form::form::html_64max(value, is_secret)).arg(label).arg(max_str),
                                QString("unexpected date in \"%1\"").arg(widget_name),
                                is_secret
                            ).set_widget_name(widget_name);
                            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                        }
                    }
                }
            }

            // minimum/maximum time
            {
                QDomElement min_time(filters.firstChildElement("min-time"));
                QDomElement max_time(filters.firstChildElement("max-time"));
                if(!min_time.isNull()
                || !max_time.isNull())
                {
                    // first test whether the user entry was valid, if not
                    // just skip this test 100% -- if the programmer wants
                    // a valid time every time, he has to use the regex
                    // tag with the name attribute set to time or datetime:
                    //
                    //     <regex name="time"/>
                    //
                    locale::locale::parse_error_t errcode;
                    time_t const time_value(locale_plugin->parse_time(value, errcode));
                    if(errcode == locale::locale::parse_error_t::PARSE_NO_ERROR)
                    {
                        QString min_str("-1");
                        QString max_str("-1");
                        time_t min_time_value(-1);
                        time_t max_time_value(-1);

                        if(!min_time.isNull())
                        {
                            min_str = min_time.text();
                            min_time_value = locale_plugin->parse_time(min_str, errcode);
                            if(errcode != locale::locale::parse_error_t::PARSE_NO_ERROR)
                            {
                                throw editor_exception_invalid_editor_form_xml(QString("the minimum time \"%1\" must be a valid time").arg(min_str));
                            }
                        }

                        if(!max_time.isNull())
                        {
                            max_str = max_time.text();
                            max_time_value = locale_plugin->parse_time(max_str, errcode);
                            if(errcode != locale::locale::parse_error_t::PARSE_NO_ERROR)
                            {
                                throw editor_exception_invalid_editor_form_xml(QString("the maximum time \"%1\" must be a valid time").arg(max_str));
                            }
                        }

                        if(min_time_value != -1 && max_time_value != -1 && max_time_value < min_time_value)
                        {
                            // here we have a special case, the time loops so the min/max have to be
                            // tested slightly differently
                            if(time_value < max_time_value || time_value > min_time_value)
                            {
                                // time is too large or too small... out of range for sure
                                messages->set_error(
                                    "Time Out of Range",
                                    QString("\"%1\" is out of range for \"%2\". The widget requires a time starting on \"%3\" and ending on \"%4\".")
                                            .arg(form::form::html_64max(value, is_secret)).arg(label).arg(max_str).arg(min_str),
                                    QString("unexpected time in \"%1\"").arg(widget_name),
                                    is_secret
                                ).set_widget_name(widget_name);
                                info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                            }
                        }
                        else
                        {
                            // Note: if 'value' is not a valid time, we ignore the error
                            //       at this point, we catch it below if the user asked
                            //       for the format to be checked with a regex filter
                            //       named 'time' or 'datetime'.
                            //  
                            if(min_time_value != -1 && time_value < min_time_value)
                            {
                                // time is too small
                                messages->set_error(
                                    "Too Old",
                                    QString("\"%1\" is too far in the past for \"%2\". The widget requires a time starting on \"%3\".")
                                            .arg(form::form::html_64max(value, is_secret)).arg(label).arg(min_str),
                                    QString("unexpected time in \"%1\"").arg(widget_name),
                                    is_secret
                                ).set_widget_name(widget_name);
                                info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                            }

                            if(max_time_value != -1 && time_value > max_time_value)
                            {
                                // time is too large
                                messages->set_error(
                                    "Too Recent",
                                    QString("\"%1\" is too far in the future for \"%2\". The widget requires a time ending on \"%3\".")
                                            .arg(form::form::html_64max(value, is_secret)).arg(label).arg(max_str),
                                    QString("unexpected time in \"%1\"").arg(widget_name),
                                    is_secret
                                ).set_widget_name(widget_name);
                                info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                            }
                        }
                    }
                }
            }

            // verify that a field is a valid URI
            {
                QDomElement uri_tag(filters.firstChildElement("uri"));
                if(!uri_tag.isNull())
                {
                    // the text may include allowed or forbidden TLDs
                    QString const uri_tlds(uri_tag.text());
                    snap_string_list tld_list(uri_tlds.split(",", QString::SkipEmptyParts));
                    bool const match(uri_tag.attribute("match") != "no");
                    snap_uri uri;
                    bool valid(uri.set_uri(value));
                    if(!valid)
                    {
                        // try again adding a default protocol
                        valid = uri.set_uri("http://" + value);
                    }
                    if(!valid)
                    {
                        messages->set_error(
                            "URL is Invalid",
                            QString("\"%1\" is not a valid URL as expected by \"%2\".")
                                    .arg(form::form::html_64max(value, is_secret)).arg(label),
                            QString("widget \"%1\" included a URL which is invalid")
                                    .arg(widget_name),
                            is_secret
                        ).set_widget_name(widget_name);
                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                    }
                    else
                    {
                        QString const tld(uri.top_level_domain());
                        int const max_tld(tld_list.size());
                        bool found(false);
                        for(int i(0); i < max_tld; ++i)
                        {
                            QString const item(tld_list[i].trimmed());
                            if(item.isEmpty())
                            {
                                // skip empty entries (this can happen if the trimmed()
                                // call removed all spaces and it was only spaces!)
                                continue;
                            }
                            if(item == tld)
                            {
                                found = true;
                                break;
                            }
                            tld_list[i] = item; // save the trimmed version back for errors
                        }
                        // if all extensions were checked and none accepted, error
                        if(!found ^ match)
                        {
                            messages->set_error(
                                "URL is Invalid",
                                QString("\"%1\" is not a valid URL as expected by \"%2\".")
                                        .arg(form::form::html_64max(value, is_secret)).arg(label),
                                QString("widget \"%1\" included a URL which is not allowed")
                                        .arg(widget_name),
                                is_secret
                            ).set_widget_name(widget_name);
                            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                        }
                    }
                }
            }

            // force extensions on file names
            {
                QDomElement extensions_tag(filters.firstChildElement("extensions"));
                if(!extensions_tag.isNull())
                {
                    QString const extensions(extensions_tag.text());
                    snap_string_list ext_list(extensions.split(",", QString::SkipEmptyParts));
                    int const max_ext(ext_list.size());
                    QFileInfo const file_info(value);
                    QString const file_ext(file_info.suffix());
                    int i;
                    for(i = 0; i < max_ext; ++i)
                    {
                        QString const ext(ext_list[i].trimmed());
                        if(ext.isEmpty())
                        {
                            // skip empty entries (this can happen if the trimmed()
                            // call removed all spaces and it was only spaces!)
                            continue;
                        }
                        if(file_ext == ext)
                        {
                            break;
                        }
                        ext_list[i] = ext; // save the trimmed version back for errors
                    }
                    // if all extensions were checked and none accepted, error
                    if(i >= max_ext)
                    {
                        messages->set_error(
                            "Filename Extension is Invalid",
                            QString("\"%1\" must end with one of \"%2\" in \"%3\". Please try again.")
                                    .arg(form::form::html_64max(value, is_secret)).arg(ext_list.join(", ")).arg(label),
                            QString("widget \"%1\" included a filename with an invalid extension")
                                    .arg(widget_name),
                            is_secret
                        ).set_widget_name(widget_name);
                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                    }
                }
            }

            // run JavaScript validate script
            {
                QDomElement validate_tag(filters.firstChildElement("validate"));
                if(!validate_tag.isNull())
                {
                    // save so the JavaScript script can access the value
                    // through the callbacks
                    f_value_to_validate = value;

                    // TODO: convert the use of javascript->evaluate_scrtip()
                    //       to using snap_expr so we can eliminate the
                    //       dependency completely
                    //
                    javascript::javascript::instance()->register_dynamic_plugin(this);
                    QString const validate_script(validate_tag.text());
                    QVariant v(javascript::javascript::instance()->evaluate_script(validate_script));
                    bool const result(v.toBool());
                    if(!result)
                    {
                        messages->set_error(
                            "Validation Failed",
                            QString("\"%1\" did not validate in \"%3\".")
                                    .arg(form::form::html_64max(value, is_secret)).arg(label),
                            QString("widget \"%1\" included a filename with an invalid extension")
                                    .arg(widget_name),
                            is_secret
                        ).set_widget_name(widget_name);
                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                    }
                }
            }
        }
    }

    return true;
}


/** \brief Publish the page, making it the current page.
 *
 * This function saves the page in a new revision and makes it the current
 * revision. If the page does not exist yet, then it gets created (i.e.
 * saving from the admin/drafts area to a real page.)
 *
 * The page type as defined when creating the draft is used as the type of
 * this new page. This generally defines the permissions, so we do not
 * worry about that here.
 *
 * \param[in,out] ipath  The path to the page being updated.
 */
void editor::editor_create_new_branch(content::path_info_t & ipath)
{
    messages::messages * messages(messages::messages::instance());
    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    QString const site_key(f_snap->get_site_key_with_slash());

    // although we expect the URI sent by the editor to be safe, we filter it
    // again here really quick because the client sends this to us and thus
    // the data can be tainted
    QString page_uri(f_snap->postenv("_editor_uri"));
    filter::filter::filter_uri(page_uri);

    // if the ipath is admin/drafts/<date> then we're dealing with a brand
    // new page; the URI we just filtered has to be unique
    bool const is_draft(ipath.get_cpath().startsWith("admin/drafts/"));

    // we got to retrieve the type used on the draft to create the full
    // page; the type is also used to define the path to the page
    //
    // IMPORTANT: it is different here from the normal case because
    //            we check the EDITOR page type and not the CONTENT
    //            page type...
    //
    QString type_name;
    links::link_info info(is_draft ? content::get_name(content::name_t::SNAP_NAME_CONTENT_PAGE_TYPE)
                                   : get_name(name_t::SNAP_NAME_EDITOR_PAGE_TYPE),
                          false, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info type_info;
    if(link_ctxt->next_link(type_info))
    {
        QString const type(type_info.key());
        if(type.startsWith(site_key + "types/taxonomy/system/content-types/"))
        {
            type_name = type.mid(site_key.length() + 36);
        }
    }
    if(type_name.isEmpty())
    {
        // this should never happen, but we need a default in case the
        // type selected at the time the user created the draft is not
        // valid somehow; at this point the most secure without making
        // the page totally innaccessible is as follow
        //
        // TBD: should we use page/private instead?
        // TODO: offer the administrator to define a default
        type_name = "page/secure";
    }

    // now that we have the type, we can get the path definition for that
    // type of pages; it is always important because when editing a page
    // you "lose" the path and "regain" it when you save
    //
    QString type_format("[page-uri]"); // default is just the page URI computed from the title
    QString const type_key(QString("%1types/taxonomy/system/content-types/%2").arg(site_key).arg(type_name));
    if(content_table->row(type_key)->exists(get_name(name_t::SNAP_NAME_EDITOR_TYPE_FORMAT_PATH)))
    {
        type_format = content_table->row(type_key)->cell(get_name(name_t::SNAP_NAME_EDITOR_TYPE_FORMAT_PATH))->value().stringValue();
    }

    params_map_t params;
    QString key(format_uri(type_format, ipath, page_uri, params));
    if(is_draft)
    {
        // TBD: we probably should have a lock, but what would we lock in
        //      this case? (also it is rather unlikely that two people try
        //      to create a page with the exact same URI at the same time)
        //
        QString extended_type_format;
        QString new_key;
        for(int i(0);; ++i)
        {
            // page already exists?
            if(i == 0)
            {
                new_key = key;
            }
            else
            {
                if(extended_type_format.isEmpty())
                {
                    if(content_table->row(type_key)->cell(get_name(name_t::SNAP_NAME_EDITOR_TYPE_EXTENDED_FORMAT_PATH)))
                    {
                        extended_type_format = content_table->row(type_key)->cell(get_name(name_t::SNAP_NAME_EDITOR_TYPE_EXTENDED_FORMAT_PATH))->value().stringValue();
                    }
                    if(extended_type_format.isEmpty()
                    || extended_type_format == type_format)
                    {
                        extended_type_format = QString("%1-[param(counter)]").arg(type_format);
                    }
                }
                new_key = format_uri(type_format, ipath, page_uri, params);
            }
            if(!content_table->exists(new_key)
            || !content_table->row(new_key)->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
            {
                if(key != new_key)
                {
                    messages->set_warning(
                        "Editor Already Submitted",
                        QString("The URL \"<a href=\"%1\">%1</a>\" for your new page is already used by another page and was changed to \"%2\" for this new page.")
                                            .arg(key)
                                            .arg(new_key),
                        "Changed URL because another page already used that one.");
                    key = new_key;
                }
                break;
            }
        }

        // this is a new page, create it now
        //
        // TODO: language "xx" is totally wrong, plus we actually need to
        //       publish ALL those languages present in the draft
        //
        QString const locale("xx");
        QString const owner(output::output::instance()->get_plugin_name());
        content::path_info_t page_ipath;
        page_ipath.set_path(key);
        page_ipath.force_branch(content_plugin->get_current_user_branch(key, locale, true));
        page_ipath.force_revision(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));
        page_ipath.force_locale(locale);
        content_plugin->create_content(page_ipath, owner, type_name);

        // it was created at the time the draft was created
        int64_t const created_on(content_table->row(ipath.get_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->value().int64Value());
        content_table->row(page_ipath.get_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->setValue(created_on);

        // it is being issued now
        branch_table->row(page_ipath.get_branch_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_ISSUED))->setValue(f_snap->get_start_date());

        // copy the last revision
        dbutils::copy_row(revision_table, ipath.get_revision_key(), revision_table, page_ipath.get_revision_key());

        // TODO: copy links too...
    }
}


/** \brief Use a format string to generate a path.
 *
 * This function uses a format string to transform different parameters
 * available in a page to create its path (URI path.)
 *
 * The format uses tokens written between square brackets. The brackets
 * are used to clearly delimit the start and end of the tokens. The tokens
 * to not take any parameters. Instead, we decided to make it one simple
 * word per token. There is no recursivity support nor possibility to
 * add parameters to tokens. Instead, each and every token is a separate
 * keyword. More keywords can be added as more features are added.
 *
 * The keywords are transformed using the signal.
 *
 * \li [title] -- the title of the page filtered
 * \li [date] -- the date the page was issued (YMD)
 * \li [year] -- the year the page was issued
 * \li [month] -- the month the page was issued
 * \li [day] -- the day the page was issued
 * \li [time] -- the time the page was issued (HMS)
 * \li [hour] -- the hour the page was issued
 * \li [minute] -- the minute the page was issued
 * \li [second] -- the second the page was issued
 * \li [now] -- the date right now (YMD)
 * \li [now-year] -- the year right now
 * \li [now-month] -- the month right now
 * \li [now-day] -- the day right now
 * \li [now-time] -- the time the page was issued (HMS)
 * \li [now-hour] -- the hour right now
 * \li [now-minute] -- the minute right now
 * \li [now-second] -- the second right now
 * \li [mod] -- the modification date when the branch was last modified (YMD)
 * \li [mod-year] -- the year when the branch was last modified
 * \li [mod-month] -- the month when the branch was last modified
 * \li [mod-day] -- the day when the branch was last modified
 * \li [mod-time] -- the time the page was issued (HMS)
 * \li [mod-hour] -- the hour when the branch was last modified
 * \li [mod-minute] -- the minute when the branch was last modified
 * \li [mod-second] -- the second when the branch was last modified
 *
 * \todo
 * Look into ways to allow for extensions.
 *
 * \param[in] format  The format used to generate the string.
 * \param[in,out] ipath  The ipath of the page we're working on.
 * \param[in] page_name  The name of the page (i.e. the title transformed
 *                       to fit Snap! paths limitations)
 * \param[in] params  An array of parameters.
 *
 * \return The formatted path.
 */
QString editor::format_uri(QString const & format, content::path_info_t & ipath, QString const & page_name, params_map_t const & params)
{
    class parser
    {
    public:
        typedef ushort char_t;

        parser(editor * e, QString const & format, content::path_info_t & ipath, QString const & page_name, params_map_t const & params)
            : f_editor(e)
            , f_format(format)
            //, f_pos(0)
            , f_token_info(ipath, page_name, params)
            //, f_result("")
        {
        }

        void parse()
        {
            for(;;)
            {
                char_t c(getc());
                if(c == static_cast<char_t>(EOF))
                {
                    // done
                    break;
                }
                if(c == '[')
                {
                    if(!parse_token())
                    {
                        // TBD?
                    }
                }
                else
                {
                    f_result += QChar(c);
                }
            }
        }

        bool parse_token()
        {
            f_token_info.f_token.clear();
            for(;;)
            {
                char_t c(getc());
                if(c == static_cast<char_t>(EOF) || isspace(c))
                {
                    return false;
                }
                if(c == ']')
                {
                    break;
                }
                f_token_info.f_token += QChar(c);
            }
            f_token_info.f_result.clear();
            f_editor->replace_uri_token(f_token_info);
            f_result += f_token_info.f_result;
            return true;
        }

        char_t getc()
        {
            char_t c(static_cast<char_t>(EOF));
            if(f_pos < f_format.length())
            {
                c = f_format[f_pos].unicode();
                ++f_pos;
            }
            return c;
        }

        QString result()
        {
            return f_result;
        }

    private:
        editor *                    f_editor = nullptr;
        QString const &             f_format;
        int32_t                     f_pos = 0;
        editor_uri_token            f_token_info;
        QString                     f_result;
    };
    parser result(this, format, ipath, page_name, params);
    result.parse();
    return result.result();
}


/** \brief Replace the specified token with data to generate a URI.
 *
 * This signal is used to transform tokens from URI format strings to
 * values. If your function doesn't know about the token, then just
 * return without doing anything. The main function returns false
 * if it understands the token and thus no other plugins receive the
 * signal in that case.
 *
 * The ipath represents the path to the page being saved. It may be
 * the page draft (under "admin/drafts".)
 *
 * The page_name parameter is computed from the page title. It is the title
 * all in lowercase, with dashes instead of spaces, and removal of
 * characters that are not generally welcome in a URI.
 *
 * The params map defines additional parameters tha are available at the
 * time the signal is called.
 *
 * The token is the keyword parsed our of the input format. For example, it
 * may be the word "year" to be replaced by the current year.
 *
 * \note
 * This function transforms the "editor" known tokens, this includes
 * all the tokens known by the editor and any plugin that cannot include
 * the editor without creating a circular dependency.
 *
 * \param[in,out] token_info  The information about this token.
 *
 * \return true if the token was not an editor basic token, false otherwise
 *         so other plugins get a chance to transform the token themselves
 */
bool editor::replace_uri_token_impl(editor_uri_token & token_info)
{
    //
    // TITLE
    //
    if(token_info.f_token == "page-uri")
    {
        token_info.f_result = token_info.f_page_name;
        return false;
    }

    QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
    QtCassandra::QCassandraTable::pointer_t branch_table(content::content::instance()->get_branch_table());

    //
    // TIME / DATE
    //
    enum class type_t
    {
        TIME_SOURCE_UNKNOWN,
        TIME_SOURCE_NOW,
        TIME_SOURCE_CREATION_DATE,
        TIME_SOURCE_MODIFICATION_DATE
    };
    std::string time_format;
    type_t type(type_t::TIME_SOURCE_UNKNOWN);
    if(token_info.f_token == "date")
    {
        time_format = "%Y%m%d";
        type = type_t::TIME_SOURCE_CREATION_DATE;
    }
    else if(token_info.f_token == "year")
    {
        time_format = "%Y";
        type = type_t::TIME_SOURCE_CREATION_DATE;
    }
    else if(token_info.f_token == "month")
    {
        time_format = "%m";
        type = type_t::TIME_SOURCE_CREATION_DATE;
    }
    else if(token_info.f_token == "day")
    {
        time_format = "%d";
        type = type_t::TIME_SOURCE_CREATION_DATE;
    }
    else if(token_info.f_token == "time")
    {
        time_format = "%H%M%S";
        type = type_t::TIME_SOURCE_CREATION_DATE;
    }
    else if(token_info.f_token == "hour")
    {
        time_format = "%H";
        type = type_t::TIME_SOURCE_CREATION_DATE;
    }
    else if(token_info.f_token == "minute")
    {
        time_format = "%M";
        type = type_t::TIME_SOURCE_CREATION_DATE;
    }
    else if(token_info.f_token == "second")
    {
        time_format = "%S";
        type = type_t::TIME_SOURCE_CREATION_DATE;
    }
    else if(token_info.f_token == "now")
    {
        time_format = "%Y%m%d";
        type = type_t::TIME_SOURCE_NOW;
    }
    else if(token_info.f_token == "now-year")
    {
        time_format = "%Y";
        type = type_t::TIME_SOURCE_NOW;
    }
    else if(token_info.f_token == "now-month")
    {
        time_format = "%m";
        type = type_t::TIME_SOURCE_NOW;
    }
    else if(token_info.f_token == "now-day")
    {
        time_format = "%d";
        type = type_t::TIME_SOURCE_NOW;
    }
    else if(token_info.f_token == "now-time")
    {
        time_format = "%H%M%S";
        type = type_t::TIME_SOURCE_NOW;
    }
    else if(token_info.f_token == "now-hour")
    {
        time_format = "%H";
        type = type_t::TIME_SOURCE_NOW;
    }
    else if(token_info.f_token == "now-hour")
    {
        time_format = "%H";
        type = type_t::TIME_SOURCE_NOW;
    }
    else if(token_info.f_token == "now-minute")
    {
        time_format = "%M";
        type = type_t::TIME_SOURCE_NOW;
    }
    else if(token_info.f_token == "now-second")
    {
        time_format = "%S";
        type = type_t::TIME_SOURCE_NOW;
    }
    else if(token_info.f_token == "mod")
    {
        time_format = "%Y%m%d";
        type = type_t::TIME_SOURCE_MODIFICATION_DATE;
    }
    else if(token_info.f_token == "mod-year")
    {
        time_format = "%Y";
        type = type_t::TIME_SOURCE_MODIFICATION_DATE;
    }
    else if(token_info.f_token == "mod-month")
    {
        time_format = "%m";
        type = type_t::TIME_SOURCE_MODIFICATION_DATE;
    }
    else if(token_info.f_token == "mod-day")
    {
        time_format = "%d";
        type = type_t::TIME_SOURCE_MODIFICATION_DATE;
    }
    else if(token_info.f_token == "mod-time")
    {
        time_format = "%H%M%S";
        type = type_t::TIME_SOURCE_MODIFICATION_DATE;
    }
    else if(token_info.f_token == "mod-hour")
    {
        time_format = "%H";
        type = type_t::TIME_SOURCE_MODIFICATION_DATE;
    }
    else if(token_info.f_token == "mod-minute")
    {
        time_format = "%M";
        type = type_t::TIME_SOURCE_MODIFICATION_DATE;
    }
    else if(token_info.f_token == "mod-second")
    {
        time_format = "%S";
        type = type_t::TIME_SOURCE_MODIFICATION_DATE;
    }

    if(type != type_t::TIME_SOURCE_UNKNOWN)
    {
        time_t seconds;
        switch(type)
        {
        case type_t::TIME_SOURCE_CREATION_DATE:
            {
                QString cell_name;

                if(token_info.f_ipath.get_cpath().startsWith("admin/drafts/"))
                {
                    cell_name = content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED);
                }
                else
                {
                    cell_name = content::get_name(content::name_t::SNAP_NAME_CONTENT_ISSUED);
                }
                seconds = content_table->row(token_info.f_ipath.get_key())->cell(cell_name)->value().int64Value() / 1000000;
            }
            break;

        case type_t::TIME_SOURCE_MODIFICATION_DATE:
            seconds = branch_table->row(token_info.f_ipath.get_branch_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_MODIFIED))->value().int64Value() / 1000000;
            break;

        case type_t::TIME_SOURCE_NOW:
            seconds = f_snap->get_start_date() / 1000000;
            break;

        //case TIME_SOURCE_UNKNOWN: -- this is not possible, really! look at the if()
        default:
            throw snap_logic_exception("somehow the time parameter was set to an unknown value");

        }
        struct tm time_info;
        // TODO: allow for gmtime or localtime ...
        gmtime_r(&seconds, &time_info);
        char buf[256];
        buf[0] = '\0';
        strftime(buf, sizeof(buf), time_format.c_str(), &time_info);
        return false;
    }

    return true;
}


/** \brief Save fields that the editor and other plugins manage.
 *
 * This signal can be overridden by other plugins to save the fields that
 * they add to the editor manager.
 *
 * The row parameter passed down to this function is the revision row in
 * the data table. If you need to save data in another location (i.e. the
 * branch or even in the content table) then you want to look into generating
 * a key for that content and get the corresponding row. In most cases, though
 * saving your data in the revision row is the way to go.
 *
 * Note that the ipath parameter has its revision number set to the new
 * revision number that was allocated to save this data.
 *
 * \param[in] info  The various information while saving these fields.
 */
bool editor::save_editor_fields_impl(save_info_t & info)
{
    // Page Title
    //
    if(f_snap->postenv_exists("title"))
    {
        QString title(f_snap->postenv("title"));
        title = verify_html_validity(title);
        // TODO: XSS filter title
        info.revision_row()->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->setValue(title);
    }

    // Page Body
    //
    if(f_snap->postenv_exists("body"))
    {
        QString body(f_snap->postenv("body"));
        body = verify_html_validity(body);
        // TODO: find a way to detect whether images are allowed in this
        //       field and if not make sure that if we find some err
        //
        // body may include images, transform the <img src="inline-data"/>
        // to an <img src="/images/..."/> link instead
        //
        QDomDocument doc;
        QDomElement body_widget(doc.createElement("widget"));

        // add stuff as required by the parse_out_inline_img() -- nothing for now for the body
        //
        // XXX: check whether the body HTML is already checked in 
        //      editor::string_to_value_impl() to avoid parsing the
        //      data twice...
        //
        parse_out_inline_img(info.ipath(), body, body_widget);

        // TODO: XSS filter body
        info.revision_row()->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_BODY))->setValue(body);
    }

    return true;
}


/** \brief Verify HTML data and make sure it is valid XML.
 *
 * This function takes a string representing HTML that comes from a client.
 * The string may not represent valid XML data so here we change a few tags
 * so they work as expected.
 *
 * Since we expect the data to come from a decent browser and user, we do
 * not check everything in detail. We just make fix the few tags we know
 * are at times sent to us improperly.
 *
 * \todo
 * Should we make this a filter function instead?
 *
 * \param[in] body  The HTML to fix.
 *
 * \return The fixed HTML.
 */
QString editor::verify_html_validity(QString body)
{
    // any starting spaces
    //
    QRegExp const re_start("^(<br */?>| |\\t|\\n|\\r|\\v|\\f|&nbsp;|&#160;|&#xA0;)+");
    body.replace(re_start, "");

    // any ending spaces
    //
    QRegExp const re_end("(<br */?>| |\\t|\\n|\\r|\\v|\\f|&nbsp;|&#160;|&#xA0;)+$");
    body.replace(re_end, "");

    // replace <br> with <br/>
    //
    QRegExp const br_without_slash("<br */?>");
    body.replace(br_without_slash, "<br/>");

    // replace <hr> with <hr/>
    //
    QRegExp const hr_without_slash("<hr */?>");
    body.replace(hr_without_slash, "<hr/>");

    // replace any entity other than &amp;, &lt;, and &gt; to their Unicode
    // value (very important because QXmlQuery does not like any of the
    // other entities)
    //
    body = xslt::filter_entities_out(body);

    // return the result
    //
    return body;
}


/** \brief Transform inline images into links.
 *
 * This function takes a value that was posted by the user of an editor
 * input field and transforms the <img> tags that have inline data into
 * images saved as files attachment to the current page and replace the
 * src="..." with the corresponding path.
 *
 * \param[in,out] ipath  The ipath to the page being modified.
 * \param[in,out] body  The HTML to be parsed and "fixed."
 * \param[in] widget  The tag representing the widget being saved.
 */
void editor::parse_out_inline_img(
          content::path_info_t & ipath
        , QString & body
        , QDomElement widget)
{
    QDomDocument doc;
    doc.setContent(QString("<element>%1</element>").arg(body));
    QDomNodeList imgs(doc.elementsByTagName("img"));

    // we check for a force-filename here because of the counter
    // below which requires a name
    //
    QDomNodeList attachment_tags(widget.elementsByTagName("attachment"));
    int const max_attachments(attachment_tags.size());
    if(max_attachments >= 2)
    {
        throw editor_exception_too_many_tags(QString("you can have 0 or 1 attachment tag in a widget, you have %1 right now.").arg(max_attachments));
    }
    QString force_filename; // this one is #IMPLIED
    QString force_path("#"); // this one is #IMPLIED
    if(max_attachments == 1)
    {
        QDomElement attachment_tag(attachment_tags.at(0).toElement());
        if(!attachment_tag.isNull())
        {
            force_filename = attachment_tag.attribute("force-filename", ""); // this one is #IMPLIED
            force_path = attachment_tag.attribute("force-path", "#"); // this one is #IMPLIED
        }
    }

    snap_string_list used_filenames;
    int count(0);
    bool has_changed(false);
    int const max_images(imgs.size());
    for(int i(0); i < max_images; ++i)
    {
        QDomElement img(imgs.at(i).toElement());
        if(img.isNull())
        {
            continue;
        }

        // data:image/jpeg;base64,...
        QString const src(img.attribute("src"));
        if( !src.startsWith("data:") )
        {
            continue;
        }

        // TBD: should multi-image + force_filename be an error?
        //if(has_changed && !force_filename.isEmpty()) ...error...

        has_changed = true;

        // TODO: we need to extract the function from save_inline_image()
        //       to "calculate" the proper filename, especially because
        //       we need to force the correct extension and the current
        //       version does not do it 100% correctly
        QString ff(force_filename);
        if(ff.isEmpty())
        {
            ff = img.attribute("filename");
            if(ff.isEmpty())
            {
                ff = "image";
            }
        }
        if(used_filenames.contains(ff))
        {
            // add "-<count>" to the filename just before the
            // extension; note that the parameter 'count' is
            // always unique and incremented on each iteration
            // which means it may not be incremented one by
            // one when it comes to saving the files to the
            // database (i.e. if 2 has a different filename
            // then 3 has the same as 1, not you have 1 saved
            // as is, and 3 saved with "-2" and not "-1".)
            //
            int const p1(ff.lastIndexOf('.'));
            int const p2(ff.lastIndexOf('/'));
            if(p1 > p2)
            {
                // make sure to remove the extension
                ff = QString("%1-%2%3").arg(ff.mid(0, p1)).arg(count).arg(ff.mid(p1));
            }
            else
            {
                // no valid extension it looks like
                ff = QString("%1-%2").arg(ff).arg(count);
            }
        }
        //else -- although we should be able to do that, a hacker could send us a matching filename of a name with -<number>...
        {
            used_filenames.push_back(ff);
        }
        content::path_info_t attachment_ipath;
        if(force_path == "#")
        {
            attachment_ipath = ipath;
        }
        else
        {
            attachment_ipath.set_path(force_path);
            // the locale is defined in the call to
            // create_attachment() (last parameter at this time)
        }
        bool const valid(save_inline_image(attachment_ipath, img, src, ff, widget));
        if(valid)
        {
            // XXX: the counter may need to be incremented any time
            //      it gets used rather than here?
            //
            ++count;

            // TODO: check whether the img tag has a width/height
            //       which are (way) smaller than the image, and
            //       if so create a script to have a resized version
            //       and use that version instead (i.e. will be a
            //       lot faster to load)
        }
        else
        {
            // remove that tag, it is not considered valid so it
            // may cause harm, who knows...
            //
            // TODO: let the user know what we have just done
            //
            img.parentNode().removeChild(img);
        }
    }

    // if any image was switched, change the body with the new img tags
    if(has_changed)
    {
        // get the document back in the form of a string (unfortunate...)
        //
        body = doc.toString(-1);

        // the <element/> happens if the widget was just an image and
        // thus the result becomes empty
        //
        body.remove("<element>").remove("</element>").remove("<element/>");
    }
}


/** \brief Save the inline image as an attachment.
 *
 * This function retrieves an inline image and transforms it in an
 * attachment to the specified path.
 *
 * \param[in,out] ipath  The path of the page with the image.
 * \param[in] img  The image element being saved.
 * \param[in] src  The src attribute of the image element.
 * \param[in] filename  The name to used to save the attachment, if empty
 *                      save as "image.<type>"
 * \param[in] widget  The widget being saved.
 */
bool editor::save_inline_image(
          content::path_info_t & ipath
        , QDomElement img
        , QString const & src
        , QString filename
        , QDomElement widget)
{
    static uint32_t g_index = 0;

    // we only support images so the MIME type has to start with "image/"
    if(!src.startsWith("data:image/"))
    {
        SNAP_LOG_DEBUG("refused inline image because it does not start with \"data:image/\".");
        return false;
    }

    // verify that it is base64 encoded, that is the only encoding we
    // support (and browsers too I would think?)
    int const p(src.indexOf(';', 11));
    if(p < 0
    || p > 64
    || src.mid(p, 8) != ";base64,")
    {
        SNAP_LOG_DEBUG("refused inline image because it is not base64 encoded.");
        return false;
    }

    // TODO: add the necessary to allow extensions defined by the
    //       administrator.
    //
    // TODO: it seems to me that the MIME type we receive here could
    //       very be wrong; we should check it (TBD)
    //
    // the type of image (i.e. "png", "jpeg", "gif"...)
    // we set that up so we know that it is "jpeg" and not "jpg"
    //
    // also define the extension for each type, especially for image
    // types that have a type which is completely different than what
    // the general extension is expected to be
    //
    QString const type(src.mid(11, p - 11));
    QString ext(type);
    if(type == "jpeg")
    {
        ext = "jpg";
    }
    else if(type == "x-icon")
    {
        ext = "ico";
    }
    else if(type != "png"
         && type != "gif")
    {
        // not one of the image format that our JavaScript supports, so
        // ignore at once
        //
        SNAP_LOG_DEBUG("refused image of type \"")(type)("\" because at this point we do not accept such.");
        return false;
    }

    // this is an inline image
    //
    QByteArray const base64(src.mid(p + 8).toUtf8());
    QByteArray const data(QByteArray::fromBase64(base64));

    // verify the image magic
    //
    snap_image image;
    if(!image.get_info(data))
    {
        SNAP_LOG_WARNING("image.get_info() failed for image of type \"")(type)("\".");
        return false;
    }
    int const max_frames(image.get_size());
    if(max_frames == 0)
    {
        // a "valid" image file without actual frames?!
        //
        SNAP_LOG_WARNING("image.get_info() returned an image with 0 frames, image type \"")(type)("\".");
        return false;
    }
    for(int i(0); i < max_frames; ++i)
    {
        smart_snap_image_buffer_t ibuf(image.get_buffer(i));
        if(ibuf->get_mime_type().mid(6) != type)
        {
            // mime types do not match!?
            //
            SNAP_LOG_WARNING("image defined MIME type returned by sever is \"")
                            (ibuf->get_mime_type().mid(6))
                            ("\" an image with 0 frames, image type \"")
                            (type)
                            ("\".");
            return false;
        }
    }

    if(!filter::filter::filter_filename(filename, ext))
    {
        // user supplied filename is not considered valid, use a default name
        //
        filename = QString("image.%1").arg(ext);
    }

    QString identification;
    QDomNodeList attachment_tags(widget.elementsByTagName("attachment"));
    int const max_attachments(attachment_tags.size());

    // NOTE: This max_attachments test is already done in the
    //       parse_out_inline_img() function
    //
    if(max_attachments >= 2)
    {
        throw editor_exception_too_many_tags(QString("you can have 0 or 1 attachment tag in a widget, you have %1 right now.").arg(max_attachments));
    }

    //QString widget_identification; // this one is #IMPLIED
    QDomElement attachment_tag;
    if(max_attachments == 1)
    {
        attachment_tag = attachment_tags.at(0).toElement();
        if(!attachment_tag.isNull())
        {
            identification = attachment_tag.attribute("identification", ""); // this one is #IMPLIED
        }
    }

    if(identification.isEmpty())
    {
        // TODO: should we default to attachment/private instead?
        identification = "attachment/public";
    }

    QString field_name("image");
    if(widget.hasAttribute("field"))
    {
        field_name = widget.attribute("field");
    }

    snap_child::post_file_t postfile;
    postfile.set_name(field_name);
    postfile.set_filename(filename);
    postfile.set_original_mime_type(type);
    postfile.set_creation_time(f_snap->get_start_time());
    postfile.set_modification_time(f_snap->get_start_time());
    postfile.set_data(data);
    postfile.set_image_width(image.get_buffer(0)->get_width());
    postfile.set_image_height(image.get_buffer(0)->get_height());
    ++g_index;
    postfile.set_index(g_index);

    content::attachment_file the_attachment(f_snap, postfile);
    the_attachment.set_multiple(false);
    the_attachment.set_parent_cpath(ipath.get_cpath());
    the_attachment.set_field_name(field_name);
    the_attachment.set_attachment_owner(attachment::attachment::instance()->get_plugin_name());
    // TODO: determine the correct attachment permission (public by default is probably wrong!)
    the_attachment.set_attachment_type(identification);
    // TODO: define the locale in some ways... for now we use "neutral"
    content::content::instance()->create_attachment(the_attachment, ipath.get_branch(), "");

    // replace the inline image data block with a local (albeit full) URI
    //
    // TODO: this most certainly won't work if the website definition
    //       uses a path
    //
    // TODO: get a function to fix a path like this, because it is rather
    //       complex when considering path right under the root...
    //
    QString result_src;
    if(ipath.get_cpath() != "")
    {
        // this is important because otherwise we end up with "//favicon.ico"
        // or similar invalid paths since "//" references a domain name
        //
        result_src  = QString("/%1").arg(ipath.get_cpath());
    }

    // EX-167: transform the image so that it contains standard revisioning information,
    // since it is an asset in our system. Also, we want to make sure we overcome the
    // browser's caching ability should the image change (but not the filename).
    //
    result_src = QString("[images::inline_uri('%1/%2')]")
                .arg(result_src)
                .arg(filename)
                ;
    img.setAttribute("src", result_src);

    new_attachment_saved(the_attachment, widget, attachment_tag);

    return true;
}


/** \brief Setup for editor.
 *
 * The editor transforms all the fields added to the XML and that the user
 * is expected to be able to edit in a way that gives the user the ability
 * to click "Edit this field". More or less, this means adding a couple of
 * \<div\> tags around the data of those fields.
 *
 * In order to allow field editing, you need one \<div\> with class
 * "snap-editor". This field will also be given the attribute "field_name"
 * with the name of the field. Within that first \<div\> you want another
 * \<div\> with class "editor-content".
 *
 * \todo
 * We need to know whether the editor is only inserted if the action is
 * set to edit or even in view mode. At this point we need ot it for
 * a customer and only the edit mode requires the editor. This may also
 * be a setting in the database (per page, type, global...).
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The XML element named "page".
 * \param[in,out] body  The XML element named "body".
 */
void editor::on_generate_page_content(
          content::path_info_t & ipath
        , QDomElement & page
        , QDomElement & body)
{
#ifdef _DEBUG
    //SNAP_LOG_DEBUG("editor::on_generate_page_content()");
#endif
    enum class added_form_file_support_t
    {
        ADDED_FORM_FILE_NONE,
        ADDED_FORM_FILE_NOT_YET,
        ADDED_FORM_FILE_YES
    };
    static added_form_file_support_t g_added_editor_form_js_css(added_form_file_support_t::ADDED_FORM_FILE_NONE);

    content::content * content_plugin(content::content::instance());

    QDomDocument editor_widgets(get_editor_widgets(ipath));
    if(editor_widgets.isNull())
    {
        // no editor specified for this page, skip on it (no editing allowed)
        return;
    }

    QDomNodeList widgets(editor_widgets.elementsByTagName("widget"));
    int const max_widgets(widgets.size());
//SNAP_LOG_WARNING("generating editor form: ")(max_widgets)(" widgets for ")(ipath.get_key());
    if(max_widgets == 0)
    {
        // no editor we we do not at least have one widget
        // TBD -- this happens, not too sure why at this point
        return;
    }

    QDomDocument doc(page.ownerDocument());

    {
        QDomElement metadata(snap_dom::get_element(doc, "metadata", true));
        QDomElement editor_tag(snap_dom::create_element(metadata, "editor"));
        metadata.appendChild(editor_tag);

        QDomElement on_save(snap_dom::get_element(editor_widgets, "on-save", false));
        if(on_save.attribute("allow-edit", "yes") == "no")
        {
            // /snap/head/metadata/editor[@darken-on-save='yes']
            editor_tag.setAttribute("darken-on-save", "yes");
        }

        QDomElement root(editor_widgets.documentElement());
        if(!root.isNull())
        {
            QString const owner_name(root.attribute("owner"));
            if(!owner_name.isEmpty())
            {
                // /snap/head/metadata/editor[@owner='...']
                editor_tag.setAttribute("owner", owner_name);
            }
            QString const layout_name(root.attribute("layout"));
            if(!layout_name.isEmpty() || !owner_name.isEmpty())
            {
                // /snap/head/metadata/editor[@layout='...']
                editor_tag.setAttribute("layout", layout_name.isEmpty() ? owner_name : layout_name);
            }
            QString const form_id(root.attribute("id"));
            if(!form_id.isEmpty())
            {
                // /snap/head/metadata/editor[@id='...']
                editor_tag.setAttribute("id", form_id);
            }
        }
    }

    QString redirect_on_timeout;
    int timeout_int(g_default_timeout); // 24h in minutes
    {
        QDomElement timeout_tag(snap_dom::get_element(editor_widgets, "timeout", false));
        QString const timeout_str(timeout_tag.attribute("minutes", "-1"));
        bool ok;
        int const timeout_temp(timeout_str.toInt(&ok));
        if(ok && timeout_temp > 0)
        {
            // save user defined value
            timeout_int = timeout_temp;

            // if we use the minutes defined in this tag, we also have
            // to use the redirect if defined
            redirect_on_timeout = timeout_tag.attribute("redirect", "");
        }

        // TODO: limit this timing to the user session; there is no need
        //       to keep a form accessible for 24 hours (the usual default)
        //       if the user cannot be logged in for more than 1 hour at a
        //       time!
    }

    QString auto_reset; // no default value
    {
        QDomElement auto_reset_tag(snap_dom::get_element(editor_widgets, "auto-reset", false));
        auto_reset = auto_reset_tag.attribute("minutes", "-1");
        bool ok;
        int auto_reset_int(auto_reset.toInt(&ok));
        if(!ok || auto_reset_int < 1)
        {
            // ignore invalid entries
            auto_reset.clear();
        }
    }

    // Define a session identifier (one per form)
    QString session_identification;
    {
        sessions::sessions::session_info info;
        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_FORM);
        info.set_session_id(EDITOR_SESSION_ID_EDIT);
        info.set_plugin_owner(get_plugin_name()); // ourselves
        content::path_info_t main_ipath;
        main_ipath.set_path(f_snap->get_uri().path());
        info.set_page_path(main_ipath.get_key());
        info.set_object_path(ipath.get_key());
        info.set_user_agent(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)));
        info.set_time_to_live(timeout_int * 60);  // minutes to seconds
        QString const session(sessions::sessions::instance()->create_session(info));
        int32_t const random(info.get_session_random());

        session_identification = QString("%1/%2").arg(session).arg(random);
    }

    QString const draft_key(ipath.get_draft_key(users::users::instance()->get_user_info().get_identifier()));

    // now go through all the widgets checking out their path, if the
    // path exists in doc then copy the data somewhere in the doc
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());

    QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(ipath.get_revision_key()));
    QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(ipath.get_key()));
    QtCassandra::QCassandraRow::pointer_t draft_row(revision_table->row(draft_key));
    QtCassandra::QCassandraRow::pointer_t data_row;

    revision_row->clearCache();
    secret_row->clearCache();
    draft_row->clearCache();

    // make sure dates and times are properly handled
    locale::locale * locale_plugin(locale::locale::instance());
    locale_plugin->set_timezone();
    locale_plugin->set_locale();

    QString action;
    QDomElement form_mode(snap_dom::get_element(editor_widgets, "mode", false));
    if(form_mode.hasAttribute("action"))
    {
        action = form_mode.attribute("action");
    }
    else
    {
        QString const qs_action(f_snap->get_server_parameter("qs_action"));
        snap_uri const& uri(f_snap->get_uri());
        action = uri.query_option(qs_action);
    }

    int64_t revision_created(0);
    if(revision_row->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
    {
        revision_created = revision_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->value().safeInt64Value();
    }

    int64_t draft_modified(0);
    if(draft_row->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_MODIFIED)))
    {
        draft_modified = draft_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_MODIFIED))->value().safeInt64Value();
    }

    // use the draft if it was modified more recently than the revision
    bool const use_draft(draft_modified > revision_created && action == "edit");

    // check a few things and setup the <value> or <post> and a few other
    // tags in each widget
    bool found_timeout_widget(false);
    for(int i(0); i < max_widgets; ++i)
    {
        QDomElement w(widgets.at(i).toElement());

        bool const is_secret(widget_is_secret(w));

        QString const field_name(w.attribute("field"));
        QString const field_id(w.attribute("id"));
        QString const field_type(w.attribute("type"));
        //QString const widget_auto_save(w.attribute("auto-save", "string")); // this one is #IMPLIED

        // note: the auto-save may not be turned on, we can still copy
        //       empty pointers around, it is fast enough
        bool draft_value(false);
        if(is_secret)
        {
            // secret data is never saved in the draft, so use the secret
            // row in this case
            data_row = secret_row;
        }
        else if(use_draft && !field_name.isEmpty() && draft_row->exists(field_name))
        {
            // the draft value exists, use that one because it is more
            // recent than the revision row data
            //
            // note that the POST data still have higher priority, the draft
            // data is especially if the user closes his browser and comes
            // back (much) later
            //
            data_row = draft_row;
            draft_value = true;
        }
        else
        {
            // last chance is the revision row data... if this is not
            // defined, the system tries with defaults defined in the XML
            // widget definitions
            data_row = revision_row;
        }

        // get the current value from the database if it exists
        bool const is_editor_session_field(field_name == get_name(name_t::SNAP_NAME_EDITOR_SESSION));
        bool const is_editor_timeout(field_name == get_name(name_t::SNAP_NAME_EDITOR_TIMEOUT));
        bool const is_editor_auto_reset(field_name == get_name(name_t::SNAP_NAME_EDITOR_AUTO_RESET));
        if(!field_name.isEmpty()
        && (is_editor_session_field || is_editor_timeout || is_editor_auto_reset || draft_value || data_row->exists(field_name)))
        {
            QtCassandra::QCassandraValue const value(data_row->cell(field_name)->value());
            QString current_value;
            bool set_value(true);
            if(is_editor_session_field)
            {
                // special case of the "editor::session" value
                current_value = session_identification;
            }
            else if(is_editor_timeout)
            {
                found_timeout_widget = true;
                if(redirect_on_timeout.isEmpty())
                {
                    current_value = QString("%1").arg(timeout_int);
                }
                else
                {
                    current_value = QString("%1,%2").arg(timeout_int).arg(redirect_on_timeout);
                }
            }
            else if(is_editor_auto_reset)
            {
                current_value = auto_reset;
            }
            else if(draft_value)
            {
                // all draft values are saved as is as strings
                current_value = value.stringValue();
            }
            else
            {
                value_to_string_info_t value_info(ipath, w, value);
                value_to_string(value_info);
                if(value_info.is_valid())
                {
                    current_value = value_info.result();
                }
                else
                {
                    // TODO: make sure this is correct... I noticed
                    //       that I was not setting set_value to false
                    //       anywhere anymore since I use the
                    //       value_to_string() signal
                    //
                    set_value = false;
                }
            }

            if(set_value)
            {
                QDomElement value_tag;
                value_tag = w.firstChildElement("value");
                if(value_tag.isNull())
                {
                    // no <value> tag, create one
                    value_tag = editor_widgets.createElement("value");
                    w.appendChild(value_tag);
                }
                else
                {
                    snap_dom::remove_all_children(value_tag);
                }
                snap_dom::insert_html_string_to_xml_doc(value_tag, current_value);
            }
        }
        init_editor_widget(ipath, field_id, field_type, w, data_row);
    }

    // a form without a timeout widget, but a declaration of a timeout
    // other than the default is not considered valid
    if(!found_timeout_widget
    && timeout_int != g_default_timeout)
    {
        throw editor_exception_invalid_argument(QString("Editor form \"%1\" includes a timeout tag, but no timeout widget").arg(ipath.get_key()));
    }

    // now process the XML data with the plugin specialized data for
    // each field through the editor XSLT
    prepare_editor_form(this);

    // check whether the user has edit rights
    content::permission_flag can_edit;
    path::path::instance()->access_allowed(
            users::users::instance()->get_user_info().get_user_path(false),
            ipath,
            "edit",
            permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED),
            can_edit);
    QString const can_edit_page(can_edit.allowed() ? "yes" : "");

    // transforms the widgets to HTML
    //
    xslt x;
    x.set_xsl(f_editor_form);
    x.set_document(editor_widgets);
    x.add_variable("editor_session", QVariant(session_identification));
    x.add_variable("action",         QVariant(action));
    x.add_variable("tabindex_base",  QVariant(form::form::current_tab_id()));
    x.add_variable("can_edit",       QVariant(can_edit_page));
    QDomDocument doc_output("widgets");
    x.evaluate_to_document(doc_output);

#ifdef _DEBUG
    //SNAP_LOG_DEBUG("Editor Form [")(f_editor_form.toString(-1))("]");
    //SNAP_LOG_DEBUG("Editor Focus [")(editor_widgets.toString(-1))("]");
    //SNAP_LOG_DEBUG("Editor Output [")(doc_output.toString(-1))("]");
#endif

    QDomNodeList result_widgets(doc_output.elementsByTagName("widget"));
    int const max_results(result_widgets.size());
    for(int i(0); i < max_results; ++i)
    {
        QDomElement w(result_widgets.at(i).toElement());
        QString const path(w.attribute("path"));

        QDomElement field_tag(snap_dom::create_element(body, path));
        snap_dom::insert_node_to_xml_doc(field_tag, w);

        if(g_added_editor_form_js_css == added_form_file_support_t::ADDED_FORM_FILE_NONE)
        {
            g_added_editor_form_js_css = added_form_file_support_t::ADDED_FORM_FILE_NOT_YET;
        }
    }
//SNAP_LOG_WARNING("Editor XML [")(doc.toString(-1))("]");

    if(g_added_editor_form_js_css == added_form_file_support_t::ADDED_FORM_FILE_NOT_YET)
    {
        g_added_editor_form_js_css = added_form_file_support_t::ADDED_FORM_FILE_YES;

        content::content::instance()->add_javascript(doc, "editor");
        content::content::instance()->add_css(doc, "editor");
    }

    // the count includes all the widgets even those that do not make
    // use of the tab index so we will get some gaps, but that is a very
    // small price to pay for this cool feature
    form::form::used_tab_id(max_widgets);
}


void editor::add_editor_widget_templates(QDomDocument doc)
{
    QDomNode const node(doc.documentElement());
    QDomNode child(f_editor_form.documentElement());
    snap_dom::insert_node_to_xml_doc(child, node);
}


void editor::add_editor_widget_templates(QString const & xslt)
{
    if(f_editor_form.documentElement().isNull())
    {
        // this is easier because the copy would otherwise not
        // copy the stylesheet attributes without specialized
        // code... this means the other documents do not need
        // valid XSLT attributes.
        f_editor_form.setContent(xslt);
    }
    else
    {
        QDomDocument doc;
        doc.setContent(xslt);
        add_editor_widget_templates(doc);
    }
}


void editor::add_editor_widget_templates_from_file(QString const & filename)
{
    QFile editor_xsl_file(filename);
    if(!editor_xsl_file.open(QIODevice::ReadOnly))
    {
        throw snap_logic_exception(QString("Could not open resource file \"%1\".").arg(filename));
    }
    QByteArray const data(editor_xsl_file.readAll());
    if(data.isEmpty())
    {
        throw snap_logic_exception(QString("Could not read resource file \"%1\".").arg(filename));
    }
    add_editor_widget_templates(QString::fromUtf8(data.data(), data.size()));
}


bool editor::prepare_editor_form_impl(editor * e)
{
    // no need to use 'e' in this implementation,
    // it is useful in other plugins though
    NOTUSED(e);

    // if we already computed that document, return false immediately
    if(!f_editor_form.documentElement().isNull())
    {
        return false;
    }

    // add the core XSL document
    add_editor_widget_templates_from_file(":/xsl/editor/editor-form.xsl");

    return true;
}


void editor::on_generate_boxes_content(
          content::path_info_t & page_cpath
        , content::path_info_t & ipath
        , QDomElement & page
        , QDomElement & box)
{
    NOTUSED(page_cpath);

    // generate the editor content
    //
    // TODO: see if there would not be a cleaner way to do this
    //       because this requires the data to be owned by the editor
    //
    QDomDocument doc(page.ownerDocument());
    QDomElement body(snap_dom::get_element(doc, "body"));
    on_generate_page_content(ipath, page, body);

    // use the output generate main content in the end
    output::output::instance()->on_generate_main_content(ipath, page, box);
}


/** \brief Repair the editor links.
 *
 * When cloning a page, the editor plugin may create an editor page type,
 * which is used once a draft is saved as a full page. This type has to
 * be duplicated here.
 */
void editor::repair_link_of_cloned_page(
          QString const & clone
        , snap_version::version_number_t branch_number
        , links::link_info const & source
        , links::link_info const & destination
        , bool const cloning)
{
    NOTUSED(cloning);

    links::link_info src(source.name(), source.is_unique(), clone, branch_number);
    links::links::instance()->create_link(src, destination);
}


// TODO: add support to return ALL the widget values instead of just
//       the one being checked right now
int editor::js_property_count() const
{
    return 1;
}


QVariant editor::js_property_get(QString const & name) const
{
    // the current value
    if(name == "value")
    {
        // this is one of the post_..., draft_..., or current_... too
        return f_value_to_validate;
    }

    // any one post we received?
    if(name.startsWith("post_"))
    {
        QString const post_name(name.mid(5));
        if(f_post_values.contains(post_name))
        {
            return f_post_values[post_name];
        }
        return QVariant();
    }

    // any one post we received?
    if(name.startsWith("draft_"))
    {
        QString const draft_name(name.mid(6));
        if(f_draft_values.contains(draft_name))
        {
            return f_draft_values[draft_name];
        }
        return QVariant();
    }

    // any one database variable?
    if(name.startsWith("current_"))
    {
        QString const current_name(name.mid(8));
        if(f_current_values.contains(current_name))
        {
            return f_current_values[current_name];
        }
        return QVariant();
    }

    // any one XML default value?
    if(name.startsWith("default_"))
    {
        QString const default_name(name.mid(8));
        if(f_default_values.contains(default_name))
        {
            return f_default_values[default_name];
        }
        return QVariant();
    }

    return QVariant();
}


QString editor::js_property_name(int index) const
{
    if(index == 0)
    {
        return "value";
    }
    --index;

    // try posts
    if(index < f_post_values.size())
    {
        return f_post_values.keys()[index];
    }
    index -= f_post_values.size();

    // try drafts
    if(index < f_draft_values.size())
    {
        return f_draft_values.keys()[index];
    }
    index -= f_draft_values.size();

    // try current values
    if(index < f_current_values.size())
    {
        return f_current_values.keys()[index];
    }
    index -= f_current_values.size();

    // try default values
    if(index < f_default_values.size())
    {
        return f_default_values.keys()[index];
    }
    //index -= f_default_values.size();

    return "";
}


QVariant editor::js_property_get(int index) const
{
    if(index == 0)
    {
        return f_value_to_validate;
    }
    --index;

    // try posts
    if(index < f_post_values.size())
    {
        return f_post_values.values()[index];
    }
    index -= f_post_values.size();

    // try drafts
    if(index < f_draft_values.size())
    {
        return f_draft_values.values()[index];
    }
    index -= f_draft_values.size();

    // try current values
    if(index < f_current_values.size())
    {
        return f_current_values.values()[index];
    }
    index -= f_current_values.size();

    // try default values
    if(index < f_default_values.size())
    {
        return f_default_values.values()[index];
    }
    //index -= f_default_values.size();

    return QVariant();
}


bool editor::has_post_value( QString const & name ) const
{
    return f_post_values.contains(name);
}


QString editor::get_post_value( QString const & name ) const
{
    if( !f_post_values.contains(name) )
    {
        throw editor_exception_invalid_argument( QString("name '%1' not found in editor post values list!").arg(name) );
    }

    return f_post_values[name];
}


bool editor::has_value( QString const & name ) const
{
    return f_converted_values.contains(name);
}


QtCassandra::QCassandraValue editor::get_value( QString const & name ) const
{
    if( !f_converted_values.contains(name) )
    {
        throw editor_exception_invalid_argument( QString("name '%1' not found in editor converted values list!").arg(name) );
    }

    return f_converted_values[name];
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
