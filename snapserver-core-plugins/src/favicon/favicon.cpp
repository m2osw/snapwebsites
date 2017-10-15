// Snap Websites Server -- favicon generator and settings
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

#include "favicon.h"

#include "../attachment/attachment.h"
#include "../messages/messages.h"
#include "../permissions/permissions.h"
#include "../output/output.h"
#include "../server_access/server_access.h"
#include "../users/users.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/snap_image.h>

#include <QFile>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(favicon, 1, 0)


/** \brief Get a fixed favicon name.
 *
 * The favicon plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_FAVICON_ICON: // icon is in Cassandra
        return "icon";

    case name_t::SNAP_NAME_FAVICON_ICON_PATH:
        return "favicon::icon";

    case name_t::SNAP_NAME_FAVICON_IMAGE: // specific image for this page or type
        return "content::attachment::favicon::icon::path";

    case name_t::SNAP_NAME_FAVICON_SETTINGS:
        return "admin/settings/favicon";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_FAVICON_...");

    }
    NOTREACHED();
}


/** \class favicon
 * \brief Support for the favicon (favorite icon) of a website.
 *
 * The favorite icon plugin adds a small icon in your browser tab,
 * location, or some other location depending on the browser.
 *
 * With Snap! C++ the favicon.ico file must be in the Cassandra database.
 * We first check the page being accessed, its type and the parents of
 * that type up to and including content-types. If no favicon.ico is
 * defined in these, try the site parameter favicon::image. If still
 * not defined, we return the default Snap! resource file. (the blue "S").
 *
 * The following shows the existing support by browser. The file
 * format is .ico by default (old media type image/x-icon, new
 * media type: image/vnd.microsoft.icon).
 *
 * \code
 *     Support by browser versus format
 *  
 *   Browser   .ico  PNG  GIF  AGIF  JPEG  APNG  SVG
 *   Chrome      1    1    4    4      4    --    --
 *   Firefox     1    1    1    1      1     3    --
 *   IE          5   11   11   --     --    --    --
 *   Opera       7    7    7    7      7   9.5   9.6
 *   Safari      1    4    4   --      4    --    --
 * \endcode
 *
 * The plugin allows any page, theme, content type, etc. to have a
 * different favicon. Note, however, that it is very unlikely that the
 * browser will read each different icon for each different page.
 * (i.e. you are expected to have one favicon per website.)
 *
 * In most cases website owners should only define the site wide favicon.
 * The settings should allow for the module not to search the page and
 * type so as to save processing time.
 *
 * \note
 * To refresh your site's favicon you can force browsers to download a
 * new version using the link tag and a querystring on your filename. This
 * is especially helpful in production environments to make sure your users
 * get the update.
 *
 * \code
 * <link rel="shortcut icon" href="http://www.yoursite.com/favicon.ico?v=2"/>
 * \endcode
 *
 * Source: http://stackoverflow.com/questions/2208933/how-do-i-force-a-favicon-refresh
 */


/** \brief Initialize the favicon plugin.
 *
 * This function is used to initialize the favicon plugin object.
 */
favicon::favicon()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the favicon plugin.
 *
 * Ensure the favicon object is clean before it is gone.
 */
favicon::~favicon()
{
}


/** \brief Get a pointer to the favicon plugin.
 *
 *
 * This function returns an instance pointer to the favicon plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the favicon plugin.
 */
favicon * favicon::instance()
{
    return g_plugin_favicon_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString favicon::settings_path() const
{
    return "/admin/settings/favicon";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString favicon::icon() const
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
QString favicon::description() const
{
    return "Handling of the favicon.ico file(s).";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString favicon::dependencies() const
{
    return "|form|messages|output|permissions|";
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
int64_t favicon::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 4, 7, 1, 45, 1, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our favicon references.
 *
 * Send our favicon to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void favicon::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the favicon.
 *
 * This function terminates the initialization of the favicon plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void favicon::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(favicon, "server", server, improve_signature, _1, _2, _3);
    SNAP_LISTEN(favicon, "layout", layout::layout, generate_header_content, _1, _2, _3);
    SNAP_LISTEN(favicon, "layout", layout::layout, generate_page_content, _1, _2, _3);
    SNAP_LISTEN(favicon, "path", path::path, can_handle_dynamic_path, _1, _2);
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
bool favicon::on_path_execute(content::path_info_t & ipath)
{
    // favicon.ico happens all the time so it is much faster to test here
    // like this...
    QString const cpath(ipath.get_cpath());
    if(cpath == "favicon.ico"
    || cpath == "default-favicon.ico"
    || cpath.endsWith("/favicon.ico"))
    {
        // got to use the master favorite icon or a page specific icon
        // either way we search using the get_icon() function
        output(ipath);
        return true;
    }

    // in case of the settings, we handle that special case, which we
    // use to handle the default favicon of the settings
    //
    if(cpath == "admin/settings/favicon")
    {
        if(f_snap->postenv_exists("icon"))
        {
            save_clicked_icon();

            server_access::server_access * server_access_plugin(server_access::server_access::instance());
            server_access_plugin->create_ajax_result(ipath, true);
            server_access_plugin->ajax_output();
            return true;
        }

        output::output::instance()->on_path_execute(ipath);
        return true;
    }

    return false;
}


void favicon::save_clicked_icon()
{
    QString const icon_name(f_snap->postenv("icon"));

    QFile file(QString(":/images/favicon/%1.ico").arg(icon_name));
    if(!file.open(QIODevice::ReadOnly))
    {
        f_snap->die(
                snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
                "Predefined Icon Not Found",
                QString("The system could not read favorite icon \"%1.ico\".").arg(icon_name),
                QString("Could not load the default resource favicon \":/images/favicon/%1.ico\".").arg(icon_name));
        NOTREACHED();
    }
    QByteArray data(file.readAll());

    // verify the image magic
    snap_image image;
    if(!image.get_info(data))
    {
        f_snap->die(
                snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
                "Predefined Icon Incompatible",
                QString("The system could not load favorite icon \"%1.ico\".").arg(icon_name),
                QString("The load of resource favicon \":/images/favicon/%1.ico\" failed.").arg(icon_name));
        NOTREACHED();
    }

    // verify the number of frames in this .ico
    int const max_frames(image.get_size());
    if(max_frames == 0)
    {
        // a "valid" image file without actual frames?!
        f_snap->die(
                snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
                "Predefined Icon Incompatible",
                QString("The system could not load at least one frame from favorite icon \"%1.ico\".").arg(icon_name),
                QString("The load of resource favicon \":/images/favicon/%1.ico\" failed: no frames available.").arg(icon_name));
        NOTREACHED();
    }

    smart_snap_image_buffer_t ibuf(image.get_buffer(0));
    if(ibuf->get_mime_type().mid(6) != "x-icon")
    {
        // this is a "warning" to the developer who maybe one day will see
        // it and fix the problem...
        //
        SNAP_LOG_ERROR("the image \":/images/favicon/")(icon_name)(".ico\" is not an x-icon image.");
    }

    content::path_info_t root_ipath;
    root_ipath.set_path("");

    snap_child::post_file_t postfile;
    postfile.set_name("image");
    postfile.set_filename("favicon.ico");
    postfile.set_original_mime_type("image/x-icon"); // should be "image/vnd.microsoft.icon", but x-icon still works in (many) more cases
    postfile.set_creation_time(f_snap->get_start_time());
    postfile.set_modification_time(f_snap->get_start_time());
    postfile.set_data(data);
    postfile.set_image_width(image.get_buffer(0)->get_width());
    postfile.set_image_height(image.get_buffer(0)->get_height());
    postfile.set_index(1);

    content::attachment_file the_attachment(f_snap, postfile);
    the_attachment.set_multiple(false);
    the_attachment.set_parent_cpath(""); // root (/)
    the_attachment.set_field_name("image");
    the_attachment.set_attachment_owner(attachment::attachment::instance()->get_plugin_name());
    the_attachment.set_attachment_type("attachment/public");
    // TODO: define the locale in some ways... for now we use "neutral"
    content::content::instance()->create_attachment(the_attachment, root_ipath.get_branch(), "");
}


/** \brief Add a CSS file for the settings.
 *
 * When the path is to the favicon settings, add the favicon.css file
 * so we can tweak the display of the editor form. The CSS file is
 * added only on the favicon settings since it is useless anywhere else.
 *
 * \param[in] ipath  The path of the page being displayed.
 * \param[in] header  The header element.
 * \param[in] metadata  The metadata element.
 */
void favicon::on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    NOTUSED(metadata);

    if(ipath.get_cpath() == "admin/settings/favicon")
    {
        QDomDocument doc(header.ownerDocument());

        content::content::instance()->add_css(doc, "favicon");
    }
}


/** \brief Retrieve the favicon.ico image and return it to the client.
 *
 * This function is the one retrieving the favicon file and sending it
 * to the client. The function uses various tests to know which file
 * is to be returned.
 *
 * \param[in] ipath  The ipath used by the client to retrieve the favicon.
 */
void favicon::output(content::path_info_t & ipath)
{
    QByteArray image;
    content::field_search::search_result_t result;

    // set a default Content-Type, although we should always
    // set this properly below, to make it safe...
    //
    QString content_type("image/x-icon");

    // check for a favicon.ico on this very page and then its type tree
    //
    bool const default_icon(ipath.get_cpath() == "default-favicon.ico");
    if(!default_icon)
    {
        // if the user tried with "default-favicon.ico" then it cannot
        // be an attachment at the top so skip on that get_icon()
        //
        int const pos(ipath.get_cpath().indexOf('/'));
        if(pos > 0)
        {
            // this is not the top default icon
            //
            get_icon(ipath, result);
        }
    }

    // keep user defined .ico file?
    //
    if(result.isEmpty())
    {
        // attempt loading the /favicon.ico attachment directly
        //
        if(!default_icon)
        {
            content::attachment_file file(f_snap);
            content::content::instance()->load_attachment(ipath.get_key(), file);
            image = file.get_file().get_data();
            content_type = file.get_file().get_mime_type();
        }
    }
    else
    {
        // load a user favicon.ico file from somewhere else than the
        // root folder
        //
        content::attachment_file file(f_snap);
        content::content::instance()->load_attachment(ipath.get_key(), file);
        image = file.get_file().get_data();
        content_type = file.get_file().get_mime_type();
    }

    // if the load_attachment() fails (or does not happen because the
    // user wants the default icon), we want to load the default
    // snap-favicon.ico instead, directly from the resources
    //
    // Note: the load_attachment() fails until the user adds his own
    //       icon (because the content.xml cannot properly add an
    //       image at that location for now... I think. We may want
    //       to completely change this scheme anyway once we have
    //       a fix and put the snap-favicon.ico as /favicon.ico so
    //       that way it works as expected.)
    //
    if(image.isEmpty())
    {
        // last resort we use the version saved in our resources
        QFile file(":/images/favicon/snap-favicon.ico");
        if(!file.open(QIODevice::ReadOnly))
        {
            f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND, "Icon Not Found",
                    "This website does not have a favorite icon.",
                    "Could not load the default resource favicon \":/images/favicon/snap-favicon.ico\".");
            NOTREACHED();
        }
        image = file.readAll();

        // we know that this image is an ICO, although if someone changes
        // it to something else (PNG, GIF...) the agent could fail
        // the newer media type is image/vnd.microsoft.icon
        // the old media type was image/x-icon and it works better for our purpose
        //content_type = "image/vnd.microsoft.icon";
        content_type = "image/x-icon";
    }

    // Note: since IE v11.x PNG and GIF are supported.
    //       support varies between browsers
    //
    f_snap->set_header("Content-Type", content_type);
    f_snap->set_header("Content-Transfer-Encoding", "binary");

    f_snap->output(image);

    // make sure that the session time limit does not get updated on
    // an attachment
    //
    users::users::instance()->transparent_hit();
}


/** \brief Generate the header common content.
 *
 * This function generates some content that is expected in a page
 * by default.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void favicon::on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    NOTUSED(page);

    content::field_search::search_result_t result;

    get_icon(ipath, result);

    // add the favicon.ico name at the end of the path we have found
    QString icon_path;
    if(result.isEmpty())
    {
        // use the default if no other entries was found
        icon_path = f_snap->get_site_key_with_slash() + "favicon.ico";
    }
    else
    {
        icon_path = result[0].stringValue();
    }

    FIELD_SEARCH
        (content::field_search::command_t::COMMAND_ELEMENT, body)
        (content::field_search::command_t::COMMAND_CHILD_ELEMENT, "image")
        (content::field_search::command_t::COMMAND_CHILD_ELEMENT, "shortcut")
        (content::field_search::command_t::COMMAND_ELEMENT_ATTR, "type=image/x-icon") // should be vnd.microsoft.icon but that's not supported everywhere yet
        (content::field_search::command_t::COMMAND_ELEMENT_ATTR, "href=" + icon_path)
        // TODO retrieve the image sizes from the database so we can
        //      use the real sizes here
        (content::field_search::command_t::COMMAND_ELEMENT_ATTR, "width=16")
        (content::field_search::command_t::COMMAND_ELEMENT_ATTR, "height=16")

        // generate
        ;
}


/** \brief Search for the favorite icon for a given page.
 *
 * This function searches for the favorite icon for a given page. If not
 * found anywhere, then the default can be used (i.e. favicon.ico in the
 * root.)
 *
 * \param[in] ipath  The page for which we are searching the icon
 * \param[out] result  The result is saved in this array.
 */
void favicon::get_icon(content::path_info_t & ipath, content::field_search::search_result_t & result)
{
    result.clear();

// *** WARNING WARNING WARNING ***
//
// This function is crap now, we will not be doing things this way at all
// instead we always want to save favicon images as attachments and just
// reference that attachment; the code below assumes we load an image
// from a "special field" instead which is way too complicated to
// implement with the editor when attachments are 100% automatic!
//
// Only we would need to have a UI for pages to test this feature properly.
//
// *** WARNING WARNING WARNING ***



    FIELD_SEARCH
        (content::field_search::command_t::COMMAND_MODE, content::field_search::mode_t::SEARCH_MODE_EACH)
        (content::field_search::command_t::COMMAND_PATH_INFO_GLOBAL, ipath)

        // /snap/head/metadata/desc[@type="favicon"]/data
        (content::field_search::command_t::COMMAND_FIELD_NAME, get_name(name_t::SNAP_NAME_FAVICON_IMAGE))
        (content::field_search::command_t::COMMAND_SELF)
        (content::field_search::command_t::COMMAND_IF_FOUND, 1)
            (content::field_search::command_t::COMMAND_LINK, content::get_name(content::name_t::SNAP_NAME_CONTENT_PAGE_TYPE))
            (content::field_search::command_t::COMMAND_SELF)
            (content::field_search::command_t::COMMAND_IF_FOUND, 1)
            (content::field_search::command_t::COMMAND_PARENTS, "types/taxonomy/system/content-types")
            // we cannot check the default here because it
            // cannot be accessed by anonymous visitors
        (content::field_search::command_t::COMMAND_LABEL, 1)
        (content::field_search::command_t::COMMAND_RESULT, result)

        // retrieve!
        ;
}


/** \brief Check whether \p cpath matches our introducer.
 *
 * This function checks that cpath matches the favicon introducer which
 * is "/s/" by default.
 *
 * \param[in,out] ipath  The path being handled dynamically.
 * \param[in,out] plugin_info  If you understand that cpath, set yourself here.
 */
void favicon::on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info)
{
    // for favicon.ico we already know since it is defined in the content.xml
    if(ipath.get_cpath().endsWith("/favicon.ico")
    || ipath.get_cpath() == "favicon.ico"
    || ipath.get_cpath() == "default-favicon.ico")
    {
        // tell the path plugin that this is ours
        plugin_info.set_plugin(this);
    }
}


/** \brief Improves the error signature.
 *
 * This function adds the favicon link to the header.
 *
 * \param[in] path  The path to the page that generated the error.
 * \param[in] doc  The DOM document.
 * \param[in,out] signature_tag  The DOM element where signature anchors are added.
 */
void favicon::on_improve_signature(QString const & path, QDomDocument doc, QDomElement signature_tag)
{
    NOTUSED(path);
    NOTUSED(signature_tag);

    // check whether a favicon is defined
    content::path_info_t ipath;
    content::field_search::search_result_t result;
    get_icon(ipath, result);

    QString icon_path;

    if(result.isEmpty())
    {
        icon_path = f_snap->get_site_key_with_slash() + "favicon.ico";
    }
    else
    {
        icon_path = result[0].stringValue();
    }

    QDomElement head;
    QDomElement root(doc.documentElement());
    if(!snap_dom::get_tag("head", root, head, false))
    {
        throw snap_logic_exception("favicon::on_improve_signature(): get_tag() of \"head\" failed.");
    }

    FIELD_SEARCH
        (content::field_search::command_t::COMMAND_ELEMENT, head)
        (content::field_search::command_t::COMMAND_NEW_CHILD_ELEMENT, "link")
        (content::field_search::command_t::COMMAND_ELEMENT_ATTR, "rel=shortcut icon")
        (content::field_search::command_t::COMMAND_ELEMENT_ATTR, "type=image/x-icon") // should be vnd.microsoft.icon but that's not supported everywhere yet
        (content::field_search::command_t::COMMAND_ELEMENT_ATTR, "href=" + icon_path)
        // TODO retrieve the image sizes from the database so we can
        //      use the real sizes here
        (content::field_search::command_t::COMMAND_ELEMENT_ATTR, "width=16")
        (content::field_search::command_t::COMMAND_ELEMENT_ATTR, "height=16")

        // generate
        ;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
