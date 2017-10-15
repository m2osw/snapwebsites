// Snap Websites Server -- handle the basic display of the website content
// Copyright (C) 2011-2017  Made to Order Software Corp.
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

#include "output.h"

#include "../locale/snap_locale.h"
#include "../messages/messages.h"
#include "../server_access/server_access.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>

#include <iostream>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(output, 1, 0)

// using the name_t::SNAP_NAME_CONTENT_... for this one.
/* \brief Get a fixed output name.
 *
 * The output plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
//char const * get_name(name_t name)
//{
//    switch(name)
//    {
//    case name_t::SNAP_NAME_OUTPUT_ACCEPTED:
//        return "output::accepted";
//
//    default:
//        // invalid index
//        throw snap_logic_exception("invalid name_t::SNAP_NAME_OUTPUT_...");
//
//    }
//    NOTREACHED();
//}









/** \brief Initialize the output plugin.
 *
 * This function is used to initialize the output plugin object.
 */
output::output()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the output plugin.
 *
 * Ensure the output object is clean before it is gone.
 */
output::~output()
{
}


/** \brief Get a pointer to the output plugin.
 *
 * This function returns an instance pointer to the output plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the output plugin.
 */
output * output::instance()
{
    return g_plugin_output_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString output::settings_path() const
{
    return "/admin/settings/info";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString output::icon() const
{
    return "/images/snap/snap-logo-64x64.png";
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
QString output::description() const
{
    return "Output nearly all the content of your website. This plugin handles"
        " the transformation of you pages to HTML, PDF, text, etc.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString output::dependencies() const
{
    return "|content|filter|layout|locale|path|server_access|";
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
int64_t output::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2017, 6, 19, 0, 13, 58, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                        to the database by this update (in micro-seconds).
 */
void output::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());

    layout::layout::instance()->add_layout_from_resources(content::get_name(content::name_t::SNAP_NAME_CONTENT_MINIMAL_LAYOUT_NAME));
}


/** \brief Initialize the output.
 *
 * This function terminates the initialization of the output plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void output::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(output, "layout", layout::layout, generate_page_content, _1, _2, _3);
    SNAP_LISTEN(output, "filter", filter::filter, replace_token, _1, _2, _3);
    SNAP_LISTEN(output, "filter", filter::filter, token_help, _1);
}


/** \brief Execute a page: generate the complete output of that page.
 *
 * This function displays the page that the user is trying to view. It is
 * supposed that the page permissions were already checked and thus that
 * its contents can be displayed to the current user.
 *
 * Note that the path was canonicalized by the path plugin and thus it does
 * not require any further corrections.
 *
 * \param[in,out] ipath  The canonicalized path being managed.
 *
 * \return true if the content is properly generated, false otherwise.
 */
bool output::on_path_execute(content::path_info_t & ipath)
{
    QString const action(ipath.get_parameter("action"));

    if(action == "view"
    || action == "edit"
    || action == "administer")
    {
        f_snap->output(layout::layout::instance()->apply_layout(ipath, this));
        return true;
    }
    else if(action == "delete")
    {
        // actually delete the page
        //
        // TODO: put that in the background and return a 202
        //       (this is especially important if someone wants to delete
        //       a large tree!)
        //
        content::content::instance()->trash_page(ipath);

        // if the command was sent with AJAX, make sure to answer
        // using AJAX
        //
        server_access::server_access * server_access_plugin(server_access::server_access::instance());
        if(server_access_plugin->is_ajax_request())
        {
//f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
//                    "Page Deleted",
//                    "This page was deleted.",
//                    QString("User accessed already deleted page \"%1\" with action \"delete\".")
//                            .arg(ipath.get_key()));
//NOTREACHED();
            messages::messages::instance()->set_info(
                "Page Deleted",
                QString("Page \"%1\" was successfully deleted.").arg(ipath.get_key())
            );

            server_access_plugin->create_ajax_result(ipath, true);
            server_access_plugin->ajax_output();
            return true;
        }

        // TBD: should we NOT use the die() function? (Especially with the OK
        //      "error" code.)
        //
        // no AJAX, use the die() function because there is no content to
        // return (unless we decide to use HTTP_CODE_NO_CONTENT? but then
        // we cannot get the "restore page" link)
        path::path::instance()->add_restore_link_to_signature_for(ipath.get_cpath());
        f_snap->die(snap_child::http_code_t::HTTP_CODE_OK,
                    "Page Deleted",
                    "This page was deleted.",
                    QString("User accessed already deleted page \"%1\" with action \"delete\".")
                            .arg(ipath.get_key()));
        NOTREACHED();
    }

    // we did not handle the page, so return false
    return false;
}


/** \brief Generate the page main content.
 *
 * This function generates the main output of the page. Other
 * plugins will also have the event called if they subscribed and
 * thus will be given a chance to add their own content to the
 * main page. This part is the one that (in most cases) appears
 * as the main content on the page although the content of some
 * columns may be interleaved with this content.
 *
 * Note that this is NOT the HTML output. It is the \<page\> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void output::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    NOTUSED(page);

    content::content * content(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content->get_content_table());
    QString const language(ipath.get_locale());

    // if the content is the main page then define the titles and body here
    //
    // titles are defined as HTML; you can output them as plain text
    // using "value-of" instead of "copy-of" in your .xsl files
    //
    FIELD_SEARCH
        (content::field_search::command_t::COMMAND_MODE, content::field_search::mode_t::SEARCH_MODE_EACH)
        (content::field_search::command_t::COMMAND_ELEMENT, body)
        (content::field_search::command_t::COMMAND_PATH_INFO_REVISION, ipath)

        // /snap/page/body/titles
        (content::field_search::command_t::COMMAND_CHILD_ELEMENT, "titles")

            // /snap/page/body/titles/title
            (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))
            (content::field_search::command_t::COMMAND_SELF)
            (content::field_search::command_t::COMMAND_SAVE_XML, "title")
            // /snap/page/body/titles/short-title
            (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_SHORT_TITLE))
            (content::field_search::command_t::COMMAND_SELF)
            (content::field_search::command_t::COMMAND_SAVE_XML, "short-title")
            // /snap/page/body/titles/long-title
            (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_LONG_TITLE))
            (content::field_search::command_t::COMMAND_SELF)
            (content::field_search::command_t::COMMAND_SAVE_XML, "long-title")

        (content::field_search::command_t::COMMAND_PARENT_ELEMENT)

        // /snap/page/body/content
        (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_BODY))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_SAVE_XML, "content")

        // /snap/page/body/description
        (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_DESCRIPTION))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_SAVE_XML, "description")

        // /snap/page/body/lang
        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, language)
        (content::field_search::command_t::COMMAND_SAVE_XML, "lang")

        // generate!
        ;

    // to get alternate translations we have to gather the list of
    // available translastions which is the list of revisions that
    // match the revision key in the content except the language
    //
    // TODO: at this point I only check the "current revision" when
    //       we may have to check the list of "current working revision"
    //
    // TODO: determining the list of available languages for a page
    //       should be something in content and not code here...
    //
    // TODO: cache the list of languages; it will only change if the
    //       administrator edits a page, change branch/revision, etc.
    //       so it can be cached and retrieved quickly
    //
    {
        QtCassandra::QCassandraRow::pointer_t page_row(content_table->row(ipath.get_key()));
        page_row->clearCache();
        snap_version::version_number_t const branch(ipath.get_branch());
        QString const revision_key(QString("%1::%2::%3::")
                .arg(content::get_name(content::name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(content::get_name(content::name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION))
                .arg(branch));
        int const revision_key_length(revision_key.length());
        auto column_predicate(std::make_shared<QtCassandra::QCassandraCellRangePredicate>());
        column_predicate->setCount(100);
        column_predicate->setIndex(); // behave like an index
        column_predicate->setStartCellKey(revision_key + "@");
        column_predicate->setEndCellKey(revision_key + "~");
        bool first(true);
        for(;;)
        {
            page_row->readCells(column_predicate);
            QtCassandra::QCassandraCells const cells(page_row->cells());
            if(cells.isEmpty())
            {
                // no script found, error appears at the end of the function
                break;
            }
            for(QtCassandra::QCassandraCells::const_iterator dc(cells.begin());
                    dc != cells.end();
                    ++dc)
            {
                QtCassandra::QCassandraCell::pointer_t c(*dc);

                QString const key(c->columnName());
                QString const lang(key.mid(revision_key_length));
                if(lang != language) // skip this page language, it is not a translation for itself
                {
                    if(first)
                    {
                        first = false;

                        // TODO: get mode (see below)
                    }

                    content::path_info_t translated_ipath;
                    translated_ipath.set_path(ipath.get_cpath());
                    translated_ipath.force_locale(lang);

                    FIELD_SEARCH
                        (content::field_search::command_t::COMMAND_MODE, content::field_search::mode_t::SEARCH_MODE_EACH)
                        (content::field_search::command_t::COMMAND_ELEMENT, body)
                        (content::field_search::command_t::COMMAND_PATH_INFO_REVISION, translated_ipath)

                        // /snap/page/body/translations[@mode="path"]
                        (content::field_search::command_t::COMMAND_CHILD_ELEMENT, "translations")
                        (content::field_search::command_t::COMMAND_ELEMENT_ATTR, "mode=query-string")  // TODO: need to be defined in the database

                            // /snap/page/body/translations[@mode="path"]/l
                            (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))
                            (content::field_search::command_t::COMMAND_SELF)
                            (content::field_search::command_t::COMMAND_SAVE_XML, "l")

                            // /snap/page/body/translations[@mode="path"]/l[@lang="..."]
                            (content::field_search::command_t::COMMAND_CHILD_ELEMENT, "l")
                            (content::field_search::command_t::COMMAND_ELEMENT_ATTR, QString("lang=%1").arg(lang))  // TODO: need to be defined in the database

                        // generate!
                        ;
                }
            }
        }
    }
}


/** \brief Generate boxes marked as owned by the output plugin.
 *
 * \param[in] page_cpath  The page where the box appears.
 * \param[in] ipath  The box being worked on.
 * \param[in] page  The page element.
 * \param[in] box  The box element.
 */
void output::on_generate_boxes_content(content::path_info_t & page_cpath, content::path_info_t & ipath, QDomElement & page, QDomElement & box)
{
    NOTUSED(page_cpath);

    on_generate_main_content(ipath, page, box);
}


/** \brief Generate the page common content.
 *
 * This function generates some content that is expected in a page
 * by default.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void output::on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    // create information mainly used in the HTML <head> tag
    QString up;
    int const p(ipath.get_cpath().lastIndexOf('/'));
    if(p == -1)
    {
        // in this case it is an equivalent to top
        up = f_snap->get_site_key();
    }
    else
    {
        up = f_snap->get_site_key_with_slash() + ipath.get_cpath().mid(0, p);
    }

    FIELD_SEARCH
        (content::field_search::command_t::COMMAND_MODE, content::field_search::mode_t::SEARCH_MODE_EACH)
        (content::field_search::command_t::COMMAND_ELEMENT, body)
        (content::field_search::command_t::COMMAND_PATH_INFO_GLOBAL, ipath)

        // /snap/page/body/page-created
        (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_SAVE_INT64_DATE, "page-created")
        (content::field_search::command_t::COMMAND_WARNING, "field missing")

        // /snap/page/body/created
        (content::field_search::command_t::COMMAND_PATH_INFO_BRANCH, ipath)
        (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_SAVE_INT64_DATE, "created")
        (content::field_search::command_t::COMMAND_WARNING, "field missing")

        // /snap/page/body/created-precise
        (content::field_search::command_t::COMMAND_PATH_INFO_BRANCH, ipath)
        (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_SAVE_INT64_DATE_AND_TIME, "created-precise")
        (content::field_search::command_t::COMMAND_WARNING, "field missing")

        // /snap/page/body/updated
        // XXX should it be mandatory or just use "created" as the default?
        // modified in the branch "converted" to updated
        (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_MODIFIED))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_SAVE_INT64_DATE, "updated")
        (content::field_search::command_t::COMMAND_WARNING, "field missing")

        // /snap/page/body/updated-precise
        // XXX should it be mandatory or just use "created" as the default?
        // modified in the branch "converted" to updated
        (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_MODIFIED))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_SAVE_INT64_DATE_AND_TIME, "updated-precise")
        (content::field_search::command_t::COMMAND_WARNING, "field missing")

        // /snap/page/body/accepted
        (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_ACCEPTED))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_SAVE_INT64_DATE, "accepted")

        // /snap/page/body/submitted
        (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_SUBMITTED))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_SAVE_INT64_DATE, "submitted")

        // /snap/page/body/since
        (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_SINCE))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_SAVE_INT64_DATE, "since")

        // /snap/page/body/until
        (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_UNTIL))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_SAVE_INT64_DATE, "until")

        // /snap/page/body/copyrighted
        (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_COPYRIGHTED))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_SAVE_INT64_DATE, "copyrighted")

        // /snap/page/body/issued
        (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_ISSUED))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_SAVE_INT64_DATE, "issued")

        // /snap/page/body/modified
        // XXX should it be mandatory or just use "created" as the default?
        // modified is from the revision (opposed to updated that comes from the branch)
        (content::field_search::command_t::COMMAND_PATH_INFO_REVISION, ipath)
        (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_MODIFIED))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_SAVE_INT64_DATE, "modified")
        (content::field_search::command_t::COMMAND_WARNING, "field missing")

        // /snap/page/body/modified-precise
        // XXX should it be mandatory or just use "created" as the default?
        // modified is from the revision (opposed to updated that comes from the branch)
        (content::field_search::command_t::COMMAND_PATH_INFO_REVISION, ipath)
        (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_MODIFIED))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_SAVE_INT64_DATE_AND_TIME, "modified-precise")
        (content::field_search::command_t::COMMAND_WARNING, "field missing")

        // test whether we're dealing with the home page, if not add these links:
        // /snap/page/body/navigation/link[@rel="top"][@title="Index"][@href="<site key>"]
        // /snap/page/body/navigation/link[@rel="up"][@title="Up"][@href="<path/..>"]
        (content::field_search::command_t::COMMAND_DEFAULT_VALUE_OR_NULL, ipath.get_cpath())
        (content::field_search::command_t::COMMAND_IF_NOT_FOUND, 1)
            //(content::field_search::COMMAND_RESET) -- uncomment if we go on with other things
            (content::field_search::command_t::COMMAND_CHILD_ELEMENT, "navigation")

            // Index
            (content::field_search::command_t::COMMAND_NEW_CHILD_ELEMENT, "link")
            (content::field_search::command_t::COMMAND_ELEMENT_ATTR, "rel=top")
            (content::field_search::command_t::COMMAND_ELEMENT_ATTR, "title=Index") // TODO: translate
            (content::field_search::command_t::COMMAND_ELEMENT_ATTR, "href=" + f_snap->get_site_key())
            (content::field_search::command_t::COMMAND_PARENT_ELEMENT)

            // Up
            (content::field_search::command_t::COMMAND_NEW_CHILD_ELEMENT, "link")
            (content::field_search::command_t::COMMAND_ELEMENT_ATTR, "rel=up")
            (content::field_search::command_t::COMMAND_ELEMENT_ATTR, "title=Up") // TODO: translate
            (content::field_search::command_t::COMMAND_ELEMENT_ATTR, "href=" + up)
            //(content::field_search::command_t::COMMAND_PARENT_ELEMENT) -- uncomment if we go on with other things

            //(content::field_search::command_t::COMMAND_PARENT_ELEMENT) -- uncomment if we go on with other things
        (content::field_search::command_t::COMMAND_LABEL, 1)

        // generate!
        ;

    // go through the list of messages and append them to the body
    //
    // IMPORTANT NOTE: we handle the output of the messages in the output
    //                 plugin because the messages cannot depend on the
    //                 layout plugin (circular dependencies)
    QDomDocument doc(page.ownerDocument());
    messages::messages * messages_plugin(messages::messages::instance());
    int const max_messages(messages_plugin->get_message_count());
    if(max_messages > 0)
    {
        QDomElement messages_tag(doc.createElement("messages"));
        int const errcnt(messages_plugin->get_error_count());
        messages_tag.setAttribute("error-count", errcnt);
        messages_tag.setAttribute("warning-count", messages_plugin->get_warning_count());
        body.appendChild(messages_tag);

        for(int i(0); i < max_messages; ++i)
        {
            QString type;
            messages::messages::message const & msg(messages_plugin->get_message(i));
            switch(msg.get_type())
            {
            case messages::messages::message::message_type_t::MESSAGE_TYPE_ERROR:
                type = "error";
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
                // create the message tag with its type
                QDomElement msg_tag(doc.createElement("message"));
                msg_tag.setAttribute("id", QString("messages_message_%1").arg(msg.get_id()));
                msg_tag.setAttribute("type", type);
                messages_tag.appendChild(msg_tag);

                // there is always a title
                {
                    QDomDocument message_doc("snap");
                    message_doc.setContent("<title><span class=\"message-title\">" + msg.get_title() + "</span></title>");
                    QDomNode message_title(doc.importNode(message_doc.documentElement(), true));
                    msg_tag.appendChild(message_title);
                }

                // do not create the body if empty
                if(!msg.get_body().isEmpty())
                {
                    QDomDocument message_doc("snap");
                    message_doc.setContent("<body><span class=\"message-body\">" + msg.get_body() + "</span></body>");
                    QDomNode message_body(doc.importNode(message_doc.documentElement(), true));
                    msg_tag.appendChild(message_body);
                }
            }
        }
        messages_plugin->clear_messages();

        if(errcnt != 0)
        {
            // on errors generate a warning in the header
            f_snap->set_header(messages::get_name(messages::name_t::SNAP_NAME_MESSAGES_WARNING_HEADER),
                    QString("This page generated %1 error%2")
                            .arg(errcnt).arg(errcnt == 1 ? "" : "s"));
        }

        content::content::instance()->add_javascript(page.ownerDocument(), "output");
    }

    {
        QDomElement breadcrumb_tag(doc.createElement("breadcrumb"));
        body.appendChild(breadcrumb_tag);
        breadcrumb(ipath, breadcrumb_tag);
    }
}


/** \brief Replace a token with a corresponding value.
 *
 * This function replaces the content tokens with their value. In some cases
 * the values were already computed in the XML document, so all we have to
 * do is query the XML and return the corresponding value.
 *
 * The supported tokens are:
 *
 * \li content::created -- the date when this page was created
 * \li content::last_updated -- the date when this page was last updated
 * \li content::page(path[, action]) -- the content (body) of another page
 *
 * \param[in,out] ipath  The path to the page being worked on.
 * \param[in,out] xml  The XML document used with the layout.
 * \param[in,out] token  The token object, with the token name and optional parameters.
 */
void output::on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token)
{
    NOTUSED(xml);

    if(!token.is_namespace("content::"))
    {
        // not a users plugin token
        return;
    }

    if(token.is_token("content::created"))
    {
        if(token.verify_args(0, 1))
        {
            content::content * content(content::content::instance());
            QtCassandra::QCassandraTable::pointer_t content_table(content->get_content_table());
            int64_t const created_date(content_table->row(ipath.get_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->value().safeInt64Value());
            time_t const unix_time(created_date / 1000000); // transform to seconds
            QString date_format;
            if(token.has_arg("format", 0))
            {
                filter::filter::parameter_t param(token.get_arg("format", 0, filter::filter::token_t::TOK_STRING));
                date_format = param.f_value;
            }
            token.f_replacement = locale::locale::instance()->format_date(unix_time, date_format, true);
        }
        return;
    }

    if(token.is_token("content::last_updated"))
    {
        if(token.verify_args(0, 1))
        {
            // last updated is the date when the last revision was created
            content::content * content(content::content::instance());
            QtCassandra::QCassandraTable::pointer_t revision_table(content->get_revision_table());
            int64_t const created_date(revision_table->row(ipath.get_revision_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->value().safeInt64Value());
            time_t const unix_time(created_date / 1000000); // transform to seconds
            QString date_format;
            if(token.has_arg("format", 0))
            {
                filter::filter::parameter_t param(token.get_arg("format", 0, filter::filter::token_t::TOK_STRING));
                date_format = param.f_value;
            }
            token.f_replacement = locale::locale::instance()->format_date(unix_time, date_format, true);
        }
        return;
    }

    if(token.is_token("content::page"))
    {
        if(token.verify_args(1, 2))
        {
            filter::filter::parameter_t param(token.get_arg("path", 0, filter::filter::token_t::TOK_STRING));

            content::path_info_t page_ipath;
            page_ipath.set_path(param.f_value);

            // user can specify the action to use on this one
            //
            if(token.has_arg("action", 1))
            {
                filter::filter::parameter_t action_param(token.get_arg("action", 0, filter::filter::token_t::TOK_STRING));
                page_ipath.set_parameter("action", action_param.f_value);
            }

            // WARNING: here we have to allocate the error callback,
            //          otherwise the plugin_load() fails
            //
            path::path * path_plugin(path::path::instance());
            std::shared_ptr<path::path_error_callback> main_page_error_callback(std::make_shared<path::path_error_callback>(f_snap, page_ipath));
            plugins::plugin * owner_plugin(path_plugin->get_plugin(page_ipath, *main_page_error_callback));
            layout::layout_content * body_plugin(dynamic_cast<layout::layout_content *>(owner_plugin));

            if(body_plugin != nullptr)
            {

                // before we can add the output to the token,
                // we MUST verify the permission of this user to that other page
                //
                quiet_error_callback page_error_callback(f_snap, true);
                path_plugin->verify_permissions(page_ipath, page_error_callback);
                if(!page_error_callback.has_error())
                {
                    token.f_replacement = layout::layout::instance()->create_body_string(token.f_xml, page_ipath, body_plugin);
                }
            }
        }
    }

    // For now breadcrumbs are created as a DOM so we skip this part
    // since anyway it should never be too useful
    //if(token.is_token("content::breadcrumb"))
    //{
    //    token.f_replacement = breadcrumb(ipath);
    //    return;
    //}
}


void output::on_token_help(filter::filter::token_help_t & help)
{
    help.add_token("content::created",
        "The date and time when this page was created. The token accepts"
        " one parameter to define the date and time format [format].");

    help.add_token("content::last_updated",
        "The date and time when this page was last updated (i.e. when the"
        " last revision was created). The token accepts one parameter to"
        " define the date and time format [format].");
}


void output::breadcrumb(content::path_info_t & ipath, QDomElement parent)
{
    content::content * content(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t revision_table(content->get_revision_table());

    QDomDocument doc(parent.ownerDocument());

    QDomElement ol(doc.createElement("ol"));
    ol.setAttribute("vocab", "http://schema.org/");
    ol.setAttribute("typeOf", "BreadcrumbList");
    parent.appendChild(ol);

    content::path_info_t info_ipath;
    info_ipath.set_path("admin/settings/info");

    QtCassandra::QCassandraRow::pointer_t info_row(revision_table->row(info_ipath.get_revision_key()));

    QString home_label(info_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_BREADCRUMBS_HOME_LABEL))->value().stringValue());
    if(home_label.isEmpty())
    {
        // translation is taken in account by the settings since we
        // expect the right language selection to happen before we
        // reach this function
        //
        home_label = "Home";
    }

    QtCassandra::QCassandraValue value(info_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_BREADCRUMBS_SHOW_HOME))->value());
    bool const show_home(value.nullValue() || value.safeSignedCharValue() != 0);

    value = info_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_BREADCRUMBS_SHOW_CURRENT_PAGE))->value();
    bool show_current_page(value.nullValue() || value.safeSignedCharValue() != 0);

    // the breadcrumb is a list of paths from this page back to
    // the home:
    QDomElement previous_li;
    snap_string_list segments = ipath.get_segments();
    int max_segments(ipath.get_cpath() == "" ? 0 : segments.size());
    int first(show_home ? 0 : 1);
    bool has_last(false);
    for(int i(max_segments); i >= 0; --i)
    {
        // ol/li
        QDomElement li(doc.createElement("li"));
        snap_string_list classes;
        if((!show_home && i == 0)
        || (!show_current_page && i == max_segments))
        {
            classes << "hide";
        }
        if((show_home && i == 0)
        || (!show_home && i == 1))
        {
            classes << "first";
        }
        if(!has_last
        && ((show_current_page && i == max_segments)
        || (!show_current_page && i == max_segments - 1)))
        {
            has_last = true;
            classes << "last";
        }
        // we expected "odd" for the very first item which is not hidden
        if((i & 1) == first)
        {
            classes << "odd";
        }
        else
        {
            classes << "even";
        }
        if(!classes.isEmpty())
        {
            li.setAttribute("class", classes.join(" "));
        }
        li.setAttribute("typeOf", "ListItem");
        li.setAttribute("property", "itemListElement");

        if(previous_li.isNull())
        {
            ol.appendChild(li);
        }
        else
        {
            ol.insertBefore(li, previous_li);
        }
        previous_li = li;

        // ol/li/a
        // (for Google, it is better to have <a> for ALL entries, including
        // the current page, although you could hide the current page.)
        QDomElement anchor(doc.createElement("a"));
        anchor.setAttribute("typeof", "WebPage");
        anchor.setAttribute("property", "item");
        li.appendChild(anchor);

        // ol/li/a/span
        QDomElement span(doc.createElement("span"));
        span.setAttribute("property", "name");
        anchor.appendChild(span);

        content::path_info_t page_ipath;
        QString label;
        if(i == 0)
        {
            // special case for the Home page
            anchor.setAttribute("href", "/");

            // Note: although there is a title in the home page and
            //       we could use that name, it is likely the name
            //       of the website and it may not be appropriate
            //       here. You can edit this label in "/admin/settings/info"
            //
            label = home_label;
        }
        else
        {
            QString const path(static_cast<QStringList>(segments.mid(0, i)).join("/"));
            page_ipath.set_path(path);

            // Google says we should use full paths... that is easy for us
            anchor.setAttribute("href", "/" + page_ipath.get_cpath());

            // by default try to use the short title if available
            label = revision_table->row(page_ipath.get_revision_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_SHORT_TITLE))->value().stringValue();
            if(label.isEmpty())
            {
                label = revision_table->row(page_ipath.get_revision_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->value().stringValue();
            }
        }

        // ol/li/a/span/text
        // Text may include HTML in which case we cannot use a QDomText
        //QDomText text(doc.createTextNode(label));
        //span.appendChild(text);
        snap::snap_dom::insert_html_string_to_xml_doc(span, label);

        // ol/li/meta
        QDomElement position(doc.createElement("meta"));
        position.setAttribute("property", "position");
        position.setAttribute("content", QString("%1").arg(i + 1)); // position starts at 1
        li.appendChild(position);

        // the page may know better than us what its parent is (i.e. we can
        // build breadcrumbs that are not truely following the natural
        // parent/child path)
        if(i != 0
        && revision_table->row(page_ipath.get_revision_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_BREADCRUMBS_PARENT)))
        {
            // if the page defines its own breadcrumb parent, then replace
            // the current path information with that new info.
            QString const breadcrumbs_parent(revision_table->row(page_ipath.get_revision_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_BREADCRUMBS_PARENT))->value().stringValue());

            // canonicalize
            content::path_info_t parent_ipath;
            parent_ipath.set_path(breadcrumbs_parent);

            // replace segments and index
            segments = parent_ipath.get_segments();
            if(parent_ipath.get_cpath() == "")
            {
                // special case... (because "" or "one-segment" is the same)
                i = 1;
            }
            else
            {
                i = segments.size() + 1;
            }
            max_segments = segments.size();
            show_current_page = true;
        }
    }
}


/** \brief This function is based on RFC-3966.
 *
 * This function formats a phone number so it can be used as a URI in
 * an anchor (i.e. the href attribute.) The function removes all the
 * characters other than the digits (0-9) and letters (a-z).
 *
 * The letters are all forced to lowercase.
 *
 * The string is returned with the "tel:" introducer. Note that the
 * input may start with "tel:" or "callto:". The "callto:" is changed
 * to "tel:" which is the standard ("callto:" is used by Skpye only.)
 *
 * Source: http://tools.ietf.org/html/rfc3966#page-6
 *
 * \param[in] phone  The phone number to transform.
 * \param[in] type  The type of phone number to transform.
 *
 * \return The phone number to put in the href of an anchor.
 */
QString output::phone_to_uri(QString const phone, phone_number_type_t const type)
{
    QString number(phone.toLower());

    if(number.startsWith("tel:") || number.startsWith("fax:"))
    {
        number.remove(0, 4);
    }
    else if(number.startsWith("callto:"))
    {
        number.remove(0, 7);
    }

    // remove any character that does not represent a phone number
    number.replace(QRegExp("[^0-9a-z]+"), "");

    // if the number is empty, make sure to return empty and not "tel:"
    if(number.isEmpty())
    {
        return QString();
    }

    QString prefix;
    switch(type)
    {
    case phone_number_type_t::PHONE_NUMBER_TYPE_FAX:
        prefix = "fax";
        break;
        
    case phone_number_type_t::PHONE_NUMBER_TYPE_SKYPE:
        prefix = "callto";
        break;

    case phone_number_type_t::PHONE_NUMBER_TYPE_TELEPHONE:
        prefix = "tel";
        break;

    default:
        throw snap_logic_exception(QString("unknown phone number type (%1) in output::phone_to_uri()").arg(static_cast<int>(type)));

    }

    return QString("%1:%2").arg(prefix).arg(number);
}



// javascript can depend on content, not the other way around
// so this plugin has to define the default content support

// right now we do not use the QScript anymore anywhere, so we
// are cancelling these functions (instead we have snap_expr)
//int output::js_property_count() const
//{
//    return 1;
//}
//
//
//QVariant output::js_property_get(QString const& name) const
//{
//    if(name == "modified")
//    {
//        return "content::modified";
//    }
//    return QVariant();
//}
//
//
//QString output::js_property_name(int index) const
//{
//    if(index == 0)
//    {
//        return "modified";
//    }
//    return "";
//}
//
//
//QVariant output::js_property_get(int index) const
//{
//    if(index == 0)
//    {
//        return "content::modified";
//    }
//    return QVariant();
//}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
