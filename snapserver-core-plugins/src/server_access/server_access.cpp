// Snap Websites Server -- handle AJAX responses
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

#include "server_access.h"

#include "../messages/messages.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/snap_utf8.h>

#include <iostream>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(server_access, 1, 0)


/* \brief Get a fixed server_access name.
 *
 * The server_access plugin makes use of different names in the database.
 * This function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    // Note: <branch>.<revision> are actually replaced by a full version
    //       when dealing with JavaScript and CSS files (Version: field)
    switch(name)
    {
    case name_t::SNAP_NAME_SERVER_ACCESS_AJAX_FIELD:
        return "_ajax";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_PATH_...");

    }
    NOTREACHED();
}



/** \brief Initialize the server_access plugin.
 *
 * This function is used to initialize the server_access plugin object.
 */
server_access::server_access()
    //: f_snap(nullptr) -- auto-init
    : f_ajax("snap")
    //, f_ajax_initialized(false) -- auto-init
    //, f_success(false) -- auto-init
    //, f_ajax_redirect("") -- auto-init
    //, f_ajax_target("") -- auto-init
{
}


/** \brief Clean up the server_access plugin.
 *
 * Ensure the server_access object is clean before it is gone.
 */
server_access::~server_access()
{
}


/** \brief Get a pointer to the server_access plugin.
 *
 * This function returns an instance pointer to the server_access plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the server_access plugin.
 */
server_access * server_access::instance()
{
    return g_plugin_server_access_factory.instance();
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString server_access::icon() const
{
    return "/images/server-access/server-access-logo-64x64.png";
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
QString server_access::description() const
{
    return "Intercept default output and transform it for AJAX responses."
        " Handle AJAX responses for functions that do it right.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString server_access::dependencies() const
{
    return "|content|";
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
int64_t server_access::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2017, 1, 7, 20, 0, 32, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update (in
 *                                 micro-seconds).
 */
void server_access::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the server_access.
 *
 * This function terminates the initialization of the server_access plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void server_access::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(server_access, "server", server, output_result, _1, _2);

    SNAP_TEST_PLUGIN_SUITE_LISTEN(server_access);
}


/** \brief Check whether the POST was an AJAX request.
 *
 * This function returns true if the POST request was an AJAX request.
 *
 * \return true if the request is an AJAX request.
 */
bool server_access::is_ajax_request() const
{
    return f_snap->postenv_exists(get_name(name_t::SNAP_NAME_SERVER_ACCESS_AJAX_FIELD));
}


/** \brief Process the final result.
 *
 * This function checks whether the request was an AJAX request. All AJAX
 * requests must be answered with an XML file so this function intercepts
 * regular answers and transforms them as AJAX responses if the main
 * plugin concerned did not know how to do that itself using the
 * other functions of this plugin.
 *
 * \param[in] uri_path  The path of the page being worked on.
 * \param[in,out] result  The result about to be sent to the end user.
 */
void server_access::on_output_result(QString const & uri_path, QByteArray & result)
{
    // AJAX request?
    if(!is_ajax_request()  // already an AJAX response
    || f_ajax_output)
    {
        return;
    }

    // remove the Location header if present!
    f_snap->set_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_LOCATION_HEADER), "", snap_child::HEADER_MODE_EVERYWHERE);

    // if we arrive here, we suppose that the AJAX answer is a failure
    if(!f_ajax_initialized)
    {
        content::path_info_t ipath;
        ipath.set_path(uri_path);

        create_ajax_result(ipath, false);
    }
    else
    {
        // make sure it shows up as a failure
        ajax_failure();
    }
    ajax_append_data("default-response", result);
    ajax_output();
    result = f_snap->get_output();
}


/** \brief Create an AJAX response.
 *
 * This function creates an AJAX response. All plugins that make use of
 * AJAX to communicate with the server want to use this function and
 * the call the ajax_output() to save the resulting XML file in the
 * output buffer and finally return back to the snap_child::execute()
 * function.
 *
 * \note
 * If errors occured while generated your content, you may want to set
 * the success flag to false. This is not currently mandatory though.
 * Thus you may still return success even if errors were generated.
 * This won't prevent the JavaScript code from displaying the resulting
 * error messages to the client.
 *
 * \param[in,out] ipath  The path of the page being generated.
 * \param[in] success  Whether the request is considered successful.
 */
void server_access::create_ajax_result(content::path_info_t & ipath, bool success)
{
    if(f_ajax_initialized)
    {
        throw server_access_exception_create_called_twice("the server_access::create_ajax_result() function cannot be called more than once.");
    }
    f_ajax_initialized = true;

    // create the root
    QDomElement snap_tag(f_ajax.createElement("snap"));
    f_ajax.appendChild(snap_tag);

    // TODO: should we always return an error if there are error messages?
    //       (even if the 'success' variable is true)

    // add the result
    QDomElement result(f_ajax.createElement("result"));
    snap_tag.appendChild(result);
    QDomText text(f_ajax.createTextNode(success ? "success" : "failure"));
    result.appendChild(text);

    f_success = success;

    // if a redirect had been added before the create function was called
    // make sure it is saved in the XML
    ajax_redirect(f_ajax_redirect, f_ajax_target);

    while(f_ajax_data.size() > 0)
    {
        ajax_append_data(f_ajax_data.begin().key(), f_ajax_data.begin().value());
        f_ajax_data.erase(f_ajax_data.begin());
    }

    process_ajax_result(ipath, f_success);
}


/** \brief Generate the AJAX output buffer.
 *
 * This function generates the output of an AJAX request.
 *
 * After this call, the caller should return to the snap_child execute
 * function which then takes over by sending the results to the client.
 */
void server_access::ajax_output()
{
    if(!f_ajax_initialized || f_ajax_output)
    {
        throw snap_logic_exception("server_access::ajax_output() called before server_access::create_ajax_result() or called a second time");
    }
    f_ajax_output = true;

    QDomElement snap_tag(f_ajax.documentElement());

    // if any messages were generated, add them to the AJAX message
    messages::messages * messages(messages::messages::instance());
    int const max_messages(messages->get_message_count());
    if(max_messages > 0)
    {
        // /snap/messages[errcnt=...][warncnt=...]
        QDomElement messages_tag(f_ajax.createElement("messages"));
        int const errcnt(messages->get_error_count());
        messages_tag.setAttribute("error-count", errcnt);
        messages_tag.setAttribute("warning-count", messages->get_warning_count());
        snap_tag.appendChild(messages_tag);

        for(int i(0); i < max_messages; ++i)
        {
            QString type;
            messages::messages::message const & msg(messages->get_message(i));
            switch(msg.get_type())
            {
            case messages::messages::message::message_type_t::MESSAGE_TYPE_ERROR:
                type = "error";
                if(f_success)
                {
                    // convert to a failure if there is an error
                    // (in some cases we may not correctly catch the fact
                    // early enough but if an error was generated we must
                    // ignore the redirect and this is the way...)
                    throw server_access_exception_success_with_errors("f_success is true when an error was generated, this is a contradictory AJAX reply, do not send.");
                }
                break;

            case messages::messages::message::message_type_t::MESSAGE_TYPE_WARNING:
                type = "warning";
                break;

            case messages::messages::message::message_type_t::MESSAGE_TYPE_INFO:
                type = "info";
                break;

            case messages::messages::message::message_type_t::MESSAGE_TYPE_DEBUG:
                type = "debug";
                break;

            // no default, compiler knows if one missing
            }
            {
                // /snap/messages/message[id=...][msg-id=...][type=...]
                // create the message tag with its type
                QDomElement msg_tag(f_ajax.createElement("message"));
                QString const widget_name(msg.get_widget_name());
                if(!widget_name.isEmpty())
                {
                    msg_tag.setAttribute("id", widget_name);
                }
                msg_tag.setAttribute("msg-id", msg.get_id());
                msg_tag.setAttribute("type", type);
                messages_tag.appendChild(msg_tag);

                // there is always a title
                {
                    QDomElement title_tag(f_ajax.createElement("title"));
                    msg_tag.appendChild(title_tag);
                    QDomElement span_tag(f_ajax.createElement("span"));
                    span_tag.setAttribute("class", "message-title");
                    title_tag.appendChild(span_tag);
                    snap_dom::insert_html_string_to_xml_doc(span_tag, msg.get_title());
                }

                // do not create the body if empty
                if(!msg.get_body().isEmpty())
                {
                    QDomElement body_tag(f_ajax.createElement("body"));
                    msg_tag.appendChild(body_tag);
                    // TODO: use <div> rather than <span> here?
                    QDomElement span_tag(f_ajax.createElement("span"));
                    span_tag.setAttribute("class", "message-body");
                    body_tag.appendChild(span_tag);
                    snap_dom::insert_html_string_to_xml_doc(span_tag, msg.get_body());
                }
            }
        }
        // assume user gets the messages so we can clear them from Snap!
        messages->clear_messages();

        if(errcnt != 0)
        {
            // on errors generate a warning in the header
            f_snap->set_header(
                    messages::get_name(messages::name_t::SNAP_NAME_MESSAGES_WARNING_HEADER),
                    QString("This page generated %1 error%2")
                            .arg(errcnt)
                            .arg(errcnt == 1 ? "" : "s"),
                    snap_child::HEADER_MODE_EVERYWHERE);
        }
    }

    // the type in this case is XML
    f_snap->set_header(snap::get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER),
                       "text/xml; charset=utf-8",
                       snap_child::HEADER_MODE_EVERYWHERE);

    // This is an XML file, so put the XML header, just in case
    // encoding is UTF-8 and that is the default so do not specify in header
    f_snap->output("<?xml version=\"1.0\"?>" + f_ajax.toString(-1));
}


/** \brief Change the request in a failure.
 *
 * This function changes an AJAX request in a failure. It does nothing
 * if the request is already marked as such.
 *
 * This function is particularly useful when the create_ajax_result() is
 * called and the on_process_ajax_result() signal generates an error.
 * From the on_process_ajax_result() you cannot call create_ajax_result(),
 * since it would generate an infinite loop.
 *
 * \note
 * At this point there is no function to go the other way around.
 *
 * \note
 * If the ajax_redirect() function was called earlier, then the redirect
 * gets removed by this ajax_failure() function.
 *
 * \note
 * The create_ajax_result() function can only be called once. The second
 * time it throws an error.
 */
void server_access::ajax_failure()
{
    if(f_success)
    {
        // it is currently a success, change it to a failure
        //
        f_success = false;

        // make sure no redirect is in place
        //
        QDomElement snap_tag(f_ajax.documentElement());
        QDomElement redirect_tag(snap_dom::get_child_element(snap_tag, "redirect"));
        if(!redirect_tag.isNull())
        {
            snap_tag.removeChild(redirect_tag);
        }

        // in case the result was already defined, make sure to mark it as false
        //
        QDomText text(f_ajax.createTextNode("failure"));
        QDomElement result(snap_dom::get_child_element(snap_tag, "result"));
        if(result.isNull())
        {
            result = f_ajax.createElement("result");
            snap_tag.appendChild(result);
        }
        else
        {
            snap_dom::remove_all_children(result);
        }
        result.appendChild(text);
    }
}


/** \brief Setup a redirect.
 *
 * This function defines an AJAX redirect (i.e. a redirect which cannot
 * just use a Location: ... header.)
 *
 * The AJAX plugin ignores the redirect request if the AJAX response is not
 * a success.
 *
 * If \p uri is an empty string, then no redirect is defined.
 *
 * The \p target parameter is optional. It may be set to the name of
 * a frame (although these are being deprecated...) or one of the
 * following values:
 *
 * \li _blank -- open the URI in a new window
 * \li _parent -- redirect in the parent window
 * \li _self -- redirect to self (this is the default if target is not defined)
 * \li _top -- redirect in the top window
 *
 * \note
 * This function can be called before the create_ajax_result() function.
 *
 * \todo
 * Offer a way to remove the current redirect request. Probably by setting
 * the \p uri parameter to "". At this time that special case is just
 * ignored.
 *
 * \param[in] uri  The URI of the page to send the user to.
 * \param[in] target  The target for the redirect (see above).
 */
void server_access::ajax_redirect(QString const & uri, QString const & target)
{
    if(!uri.isEmpty())
    {
        // make sure it does not include '\n' nor '\r'
        if(uri.contains('\n') || uri.contains('\r'))
        {
            throw server_access_exception_invalid_uri("server_access::ajax_redirect(): called with a URI including \\n or \\r characters.");
        }

        // now validate the syntax as a whole
        snap_uri canonicalize_uri;
        if(!canonicalize_uri.set_uri(uri))
        {
            // in most cases it fails because the protocol is missing
            QString local_path(uri);
            f_snap->canonicalize_path(local_path);
            if(!canonicalize_uri.set_uri(f_snap->get_site_key_with_slash() + local_path))
            {
                throw server_access_exception_invalid_uri("server_access::ajax_redirect(): called with a URI it could not understand.");
            }
        }

        // if initialize, directly deal with the DOM
        if(f_ajax_initialized)
        {
            if(f_success)
            {
                // redirect only successful requests
                QDomElement snap_tag(f_ajax.documentElement());
                QDomElement redirect_tag(snap_dom::create_element(snap_tag, "redirect"));
                if(!target.isEmpty())
                {
                    redirect_tag.setAttribute("target", target);
                }
                QDomText redirect_uri(f_ajax.createTextNode(uri));
                QDomNode child(redirect_tag.firstChild());
                if(child.isNull())
                {
                    redirect_tag.appendChild(redirect_uri);
                }
                else
                {
                    redirect_tag.replaceChild(redirect_uri, child);
                }
            }
        }
        else
        {
            // request not yet generated, keep the data in our cache
            f_ajax_redirect = uri;
            f_ajax_target = target;
        }
    }
}


/** \brief Transform "raw" data in an XML chunk.
 *
 * This function saves the \p data buffer as plain text in the AJAX
 * XML document.
 *
 * \note
 * The data is saved in the AJAX result only if it is valid UTF-8 data.
 * Otherwise it would not fit in the XML document by itself and encoded
 * (uuencode, base64, etc.) data is not terribly useful for the client.
 *
 * Multiple data blocks with the same or different names can be added
 * with each AJAX request. Different blocks can be recognized using
 * different name parameter.
 *
 * \note
 * The data is saved as an XML Text DOM object. This means the text
 * is parsed and any XML special character, such as the \<, are
 * tranformed to an entity. Also control characters are NOT welcome
 * in XML (even though they are valid UTF-8 characters.)
 *
 * \param[in] name  Give a name to that data block.
 * \param[in] data  Data that is to be sent to the client "as is."
 */
void server_access::ajax_append_data(QString const& name, QByteArray const& data)
{
    if(!f_ajax_initialized)
    {
        f_ajax_data[name] = data;
        return;
    }

    if(is_valid_utf8(data.data()))
    {
        QDomElement snap_tag(f_ajax.documentElement());
        QDomElement data_tag(f_ajax.createElement("data"));
        data_tag.setAttribute("name", name);
        snap_tag.appendChild(data_tag);
        // send it escaped... whatever it is
        QDomText data_text(f_ajax.createTextNode(data.data()));
        data_tag.appendChild(data_text);
    }
    // else -- TBD: we silently refuse those at this point
}


/** \fn void server_access::process_ajax_result(content::path_info_t& ipath, bool const succeeded)
 * \brief Inform plugins that an AJAX reply is about to happen.
 *
 * This signal is sent whenever an AJAX result is processed. This gives
 * you a changes to modify the AJAX result. For example, you may want to
 * redirect the user to a different page.
 *
 * This signal happens right after the AJAX reply is created. The plugin
 * taht creates the request then has a chance to overwrite all the changes
 * made by other plugins before generating the output.
 *
 * \param[in,out] ipath  The ipath on which the POST was processed.
 * \param[in] succeeded  Whether the saving process succeeded (true) or not (false).
 */


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
