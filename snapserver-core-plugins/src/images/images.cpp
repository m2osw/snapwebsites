// Snap Websites Server -- handling of images
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

#include "images.h"

#include "../listener/listener.h"
#include "../messages/messages.h"
#include "../users/users.h"

#include <snapwebsites/dbutils.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/snap_image.h>

#include <iostream>
#include <algorithm>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(images, 1, 0)

//
// Magick Documentation
// http://www.imagemagick.org/Magick++/Image.html
// http://www.imagemagick.org/script/formats.php
//


/** \brief Get a fixed images name.
 *
 * The images plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_IMAGES_ACTION:
        return "images";

    case name_t::SNAP_NAME_IMAGES_MODIFIED:
        return "images::modified";

    case name_t::SNAP_NAME_IMAGES_PROCESS_IMAGE:
        return "processimage";

    case name_t::SNAP_NAME_IMAGES_ROW:
        return "images";

    case name_t::SNAP_NAME_IMAGES_SCRIPT:
        return "images::script";

    default:
        // invalid index
        throw snap_logic_exception(QString("invalid name_t::SNAP_NAME_OUTPUT_... (%1)").arg(static_cast<int>(name)));

    }
    NOTREACHED();
}




// list of functions
images::func_t const images::g_commands[] =
{
    {
        "alpha",
        1, 1, 1,
        &images::func_alpha
    },
    {
        "background_color",
        1, 1, 1,
        &images::func_background_color
    },
    {
        "blur",
        0, 2, 1,
        &images::func_blur
    },
    {
        "border",
        1, 1, 1,
        &images::func_border
    },
    {
        "border_color",
        1, 1, 1,
        &images::func_border_color
    },
    {
        "charcoal",
        0, 2, 1,
        &images::func_charcoal
    },
    {
        "composite",
        1, 1, 1,
        &images::func_composite
    },
    {
        "contrast",
        1, 1, 1,
        &images::func_contrast
    },
    {
        "create",
        0, 0, 0,
        &images::func_create
    },
    {
        "crop",
        1, 1, 1,
        &images::func_crop
    },
    {
        "density",
        1, 2, 1,
        &images::func_density
    },
    {
        "emboss",
        0, 2, 1,
        &images::func_emboss
    },
    {
        "erase",
        0, 0, 1,
        &images::func_erase
    },
    {
        "flip",
        0, 0, 1,
        &images::func_flip
    },
    {
        "flop",
        0, 0, 1,
        &images::func_flop
    },
    {
        "hash",
        5, 5, 1,
        &images::func_hash
    },
    {
        "matte_color",
        1, 1, 1,
        &images::func_matte_color
    },
    {
        "modulate",
        3, 3, 1,
        &images::func_modulate
    },
    {
        "negate",
        0, 1, 1,
        &images::func_negate
    },
    {
        "normalize",
        0, 0, 1,
        &images::func_normalize
    },
    {
        "oil_paint",
        1, 1, 1,
        &images::func_oil_paint
    },
    {
        "on_error",
        1, 1, 0,
        &images::func_on_error
    },
    {
        "pop",
        0, 0, 1,
        &images::func_pop
    },
    {
        "read",
        2, 3, 1,
        &images::func_read
    },
    {
        "reduce_noise",
        0, 1, 1,
        &images::func_reduce_noise
    },
    {
        "resize",
        1, 1, 1,
        &images::func_resize
    },
    {
        "rotate",
        1, 1, 1,
        &images::func_rotate
    },
    {
        "shade",
        3, 3, 1,
        &images::func_shade
    },
    {
        "shadow",
        4, 4, 1,
        &images::func_shadow
    },
    {
        "sharpen",
        2, 2, 1,
        &images::func_sharpen
    },
    {
        "shear",
        2, 2, 1,
        &images::func_shear
    },
    {
        "solarize",
        1, 1, 1,
        &images::func_solarize
    },
    {
        "swap",
        0, 0, 2,
        &images::func_swap
    },
    {
        "trim",
        0, 0, 1,
        &images::func_trim
    },
    {
        "write",
        2, 2, 1,
        &images::func_write
    }
};


int const images::g_commands_size(sizeof(images::g_commands) / sizeof(images::g_commands[0]));





/** \class images
 * \brief The images plugin to handle image attachment or preview of
 *        other documents.
 *
 * The images plugin is used to transform existing images in different
 * ways (i.e. different sizes, depths, compression) and to convert any
 * other attachment in an image for preview purposes (i.e. a PDF first
 * page.)
 *
 * The functions supported are close to unlimited since we offer a way
 * to write a set of actions to apply to the image just like command
 * line options to the convert tool from ImageMagick.
 *
 * \note
 * Note that the images are generally not handled in realtime because
 * that would slowdown the front end computer. Instead we make use
 * of the listener to know once a specific image transformation is
 * available, then load it. This way a backend computer can be used
 * to work on said transformations.
 */






/** \brief Initialize the images plugin.
 *
 * This function is used to initialize the images plugin object.
 */
images::images()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the images plugin.
 *
 * Ensure the images object is clean before it is gone.
 */
images::~images()
{
}


/** \brief Initialize the images.
 *
 * This function terminates the initialization of the images plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void images::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN0(images, "server",   server,             attach_to_session);
    SNAP_LISTEN(images,  "server",   server,             register_backend_cron,   _1);
    SNAP_LISTEN(images,  "server",   server,             register_backend_action, _1);
    SNAP_LISTEN(images,  "links",    links::links,       modified_link,           _1);
    SNAP_LISTEN(images,  "path",     path::path,         can_handle_dynamic_path, _1,  _2);
    SNAP_LISTEN(images,  "content",  content::content,   create_content,          _1,  _2,  _3);
    SNAP_LISTEN(images,  "content",  content::content,   modified_content,        _1);
    SNAP_LISTEN(images,  "listener", listener::listener, listener_check,          _1,  _2,  _3,  _4);
    SNAP_LISTEN(images,  "versions", versions::versions, versions_libraries,      _1);
    SNAP_LISTEN(images,  "filter",   filter::filter,     replace_token,           _1,  _2,  _3);
    SNAP_LISTEN(images,  "filter",   filter::filter,     token_help,              _1);
}


/** \brief Get a pointer to the images plugin.
 *
 * This function returns an instance pointer to the images plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the images plugin.
 */
images * images::instance()
{
    return g_plugin_images_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString images::settings_path() const
{
    return "/admin/images";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString images::icon() const
{
    return "/images/images/images-logo-64x64.png";
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
QString images::description() const
{
    return "Transform images in one way or another. Also used to generate"
          " previews of attachments such as the first page of a PDF file.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString images::dependencies() const
{
    return "|listener|messages|path|versions|";
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
int64_t images::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 10, 15, 16, 58, 30, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void images::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Allow a second opinion on who can handle this path.
 *
 * This function is used here to allow attachments that can be represented
 * using an image (i.e. preview or MIME type icon.)
 *
 * The function recognized one image filename under the attachment. So if
 * you had an image uploaded as:
 *
 * \code
 * .../my-page/image.png
 * \endcode
 *
 * This plugin understands entries such as:
 *
 * \code
 * .../my-page/image.png/icon.png
 * \endcode
 *
 * Note that for this to work you need two things:
 *
 * \li The image.png must somehow be given a permission depth of 1 or more.
 * \li A plugin or the administrator must link the image.png document to
 *     an images script that will generate the icon.png data field in that
 *     document.
 *
 * At this point we can handle any file format that ImageMagick can transform
 * in an image. For example, we can generate a PDF preview in the exact same
 * manner. The following is an example of a script to handle a PDF document
 * and generate a high quality image of 648x838 pixels called preview.jpg:
 *
 * \code
 * on_error "create\nread /images/default-preview.jpg\nwrite ${INPUT} previous.jpg"
 * create
 * density 300
 * read ${INPUT} data
 * alpha off
 * resize 648x838
 * write ${INPUT} preview.jpg
 * \endcode
 *
 * The script is run by this images plugin. It makes use of the ImageMagick
 * library to do all the heavy image work.
 *
 * \param[in] ipath  The path being checked.
 * \param[in] plugin_info  The current information about this path plugin.
 */
void images::on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info)
{
    // in this case we ignore the result, all we are interested in is
    // whatever is put in the plugin info object
    check_virtual_path(ipath, plugin_info);
}


images::virtual_path_t images::check_virtual_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info)
{
    // is that path already going to be handled by someone else?
    // (avoid wasting time if that is the case)
    if(plugin_info.get_plugin()
    || plugin_info.get_plugin_if_renamed())
    {
        return virtual_path_t::VIRTUAL_PATH_INVALID;
    }

    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    if(content_table->exists(ipath.get_key()))
    {
        // if it exists, it is not dynamic so ignore it (this should
        // never happen because it is tested in the path plugin!)
        return virtual_path_t::VIRTUAL_PATH_INVALID;
    }

    content::path_info_t parent_ipath;
    ipath.get_parent(parent_ipath);

    // make sure we do not lose the branch, revision, and locale
    //
    parent_ipath.force_branch(ipath.get_branch());
    parent_ipath.force_revision(ipath.get_revision());
    parent_ipath.force_locale(ipath.get_locale());
    parent_ipath.set_main_page(ipath.is_main_page());

    if(!content_table->exists(parent_ipath.get_key()))
    {
        // this should always be true, although we may later want to support
        // more levels, at this point I do not really see the point of doing
        // so outside of organization which can be done with a name as in:
        //
        // icon_blah.png
        // icon_foo.png
        // preview_blah.png
        // preview_foo.png
        // ...
        //
        // so for now, ignore such (and that gives a way for other plugins
        // to support similar capabilities as the images plugin, just at
        // a different level!)
        return virtual_path_t::VIRTUAL_PATH_INVALID;
    }

    // is the parent an attachment?
    QString const owner(content_table->row(parent_ipath.get_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_PRIMARY_OWNER))->value().stringValue());
    if(owner != content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT_PLUGIN))
    {
        // something is dearly wrong if empty... and if not the attachment
        // plugin, we assume we do not support this path
        return virtual_path_t::VIRTUAL_PATH_INVALID;
    }

    // verify that the attachment key exists
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    if(!revision_table->exists(parent_ipath.get_revision_key())
    || !revision_table->row(parent_ipath.get_revision_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT)))
    {
        // again, check whether we have an attachment...
        return virtual_path_t::VIRTUAL_PATH_INVALID;
    }

    // make sure that the page is NORMAL
    content::path_info_t::status_t status(parent_ipath.get_status());
    if(status.get_state() != content::path_info_t::status_t::state_t::NORMAL)
    {
        // this could be deleted or hidden...
        return virtual_path_t::VIRTUAL_PATH_INVALID;
    }

    // get the key of that attachment, it should be a file md5
    QtCassandra::QCassandraValue attachment_key(revision_table->row(parent_ipath.get_revision_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT))->value());
    if(attachment_key.size() != 16)
    {
        // no or invalid key?!
        return virtual_path_t::VIRTUAL_PATH_INVALID;
    }

    // the field name is the basename of the ipath preceeded by the
    // "content::attachment::data" default name
    QString const cpath(ipath.get_cpath());
    int const pos(cpath.lastIndexOf("/"));
    if(pos <= 0)
    {
        // what the heck happened?!
        return virtual_path_t::VIRTUAL_PATH_INVALID;
    }
    QString const filename(cpath.mid(pos + 1));
    QString field_name(QString("%1::%2").arg(content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_DATA)).arg(filename));

    // Does the file exist at this point?
    QtCassandra::QCassandraTable::pointer_t files_table(content_plugin->get_files_table());
    if(!files_table->exists(attachment_key.binaryValue())
    || !files_table->row(attachment_key.binaryValue())->exists(field_name))
    {
        // often, the original image can be used as is because the
        // sub-image is just an "optimization"; this has to be asked
        // by the end user by adding the fallback=ok query string
        snap_uri const & uri(f_snap->get_uri());
        if(!uri.has_query_option("fallback")
        || uri.query_option("fallback") != "ok")
        {
            // no fallback
            return virtual_path_t::VIRTUAL_PATH_NOT_AVAILABLE;
        }

        // the fallback option is set to "ok", check for the default
        // field; check the default attachment key
        field_name = content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_DATA);
        if(!files_table->exists(attachment_key.binaryValue())
        || !files_table->row(attachment_key.binaryValue())->exists(field_name))
        {
            return virtual_path_t::VIRTUAL_PATH_NOT_AVAILABLE;
        }

        // Note: the permissions will prevent this from happening
        //       if the parent page does not include a field:
        //         permissions::dynamic
        //       of 1 or more
    }

    // tell the path plugin that we know how to handle this one
    plugin_info.set_plugin_if_renamed(this, parent_ipath.get_cpath());
    ipath.set_parameter("attachment_field", field_name);

    return virtual_path_t::VIRTUAL_PATH_READY;
}


void images::on_listener_check(snap_uri const & uri, content::path_info_t & page_ipath, QDomDocument doc, QDomElement result)
{
    NOTUSED(uri);

    path::dynamic_plugin_t info;
    switch(check_virtual_path(page_ipath, info))
    {
    case virtual_path_t::VIRTUAL_PATH_READY:
        result.setAttribute("status", "success");
        break;

    case virtual_path_t::VIRTUAL_PATH_INVALID:
        {
            // this is not acceptable
            QDomElement message(doc.createElement("message"));
            result.appendChild(message);
            QDomText unknown_path(doc.createTextNode("unknown path"));
            message.appendChild(unknown_path);
            result.setAttribute("status", "failed");
        }
        break;

    case virtual_path_t::VIRTUAL_PATH_NOT_AVAILABLE:
        // TODO: enhance this code so we can know whether it is worth
        //       waiting (i.e. if a script runs, we would know what
        //       path will be created and thus immediately know whether
        //       it is worth the wait.)
        result.setAttribute("status", "wait");
        break;

    }
}


bool images::on_path_execute(content::path_info_t & ipath)
{
    // TODO: we probably do not want to check for attachments to send if the
    //       action is not "view"...

    // make sure that the session time limit does not get updated on
    // an attachment
    //
    users::users::instance()->transparent_hit();

    // attachments should never be saved with a compression extension
    //
    // HOWEVER, we'd like to offer a way for the system to allow extensions
    // but if we are here the system already found the page and thus found
    // it with[out] the extension as defined in the database...
    //
    QString field_name;
    content::path_info_t attachment_ipath;
    QString const renamed(ipath.get_parameter("renamed_path"));
    if(renamed.isEmpty())
    {
        attachment_ipath = ipath;
        field_name = content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_DATA);
    }
    else
    {
        // TODO: that data may NOT be available yet in which case a plugin
        //       needs to offer it... how do we do that?!
        attachment_ipath.set_path(renamed);

        // make sure we do not lose the branch, revision, and locale
        //
        attachment_ipath.force_branch(ipath.get_branch());
        attachment_ipath.force_revision(ipath.get_revision());
        attachment_ipath.force_locale(ipath.get_locale());
        attachment_ipath.set_main_page(ipath.is_main_page());

        field_name = ipath.get_parameter("attachment_field");
    }

    QtCassandra::QCassandraTable::pointer_t revision_table(content::content::instance()->get_revision_table());
    QtCassandra::QCassandraValue attachment_key(revision_table->row(attachment_ipath.get_revision_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT))->value());
    if(attachment_key.nullValue())
    {
        // somehow the file key is not available
        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND, "Attachment Not Found",
                QString("The attachment \"%1\" was not found.").arg(ipath.get_key()),
                QString("Could not find field \"%1\" of file \"%2\" (maybe renamed \"%3\").")
                        .arg(field_name)
                        .arg(QString::fromLatin1(attachment_key.binaryValue().toHex()))
                        .arg(renamed));
        NOTREACHED();
    }

    QtCassandra::QCassandraTable::pointer_t files_table(content::content::instance()->get_files_table());
    if(!files_table->exists(attachment_key.binaryValue())
    || !files_table->row(attachment_key.binaryValue())->exists(field_name))
    {
        // somehow the file data is not available
        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND, "Attachment Not Found",
                QString("The attachment \"%1\" was not found.").arg(ipath.get_key()),
                QString("Could not find field \"%1\" of file \"%2\".")
                        .arg(content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_DATA))
                        .arg(QString::fromLatin1(attachment_key.binaryValue().toHex())));
        NOTREACHED();
    }

    QtCassandra::QCassandraRow::pointer_t file_row(files_table->row(attachment_key.binaryValue()));

    // TODO: If the user is loading the file as an attachment,
    //       we need those headers

    //int pos(cpath.lastIndexOf('/'));
    //QString basename(cpath.mid(pos + 1));
    //f_snap->set_header("Content-Disposition", "attachment; filename=" + basename);

    //f_snap->set_header("Content-Transfer-Encoding", "binary");

    // get the file data
    QByteArray data(file_row->cell(field_name)->value().binaryValue());

    // get the attachment MIME type and tweak it if it is a known text format
    //QtCassandra::QCassandraValue attachment_mime_type(file_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_MIME_TYPE))->value());
    //QString content_type(attachment_mime_type.stringValue());
    //if(content_type == "text/javascript"
    //|| content_type == "text/css"
    //|| content_type == "text/xml")
    //{
    //    // TBD -- we probably should check what's defined inside those
    //    //        files before assuming it's using UTF-8.
    //    content_type += "; charset=utf-8";
    //}
    // Our MIME type is always expected to be an image file format that we
    // know about
    snap_image img;
    if(img.get_info(data))
    {
        smart_snap_image_buffer_t img_info(img.get_buffer(0));
        f_snap->set_header("Content-Type", img_info->get_mime_type());
    }

    // the actual file data now
    f_snap->output(data);

    return true;
}


/** \brief Signal that a page was created.
 *
 * This function is called whenever the content plugin creates a new page.
 *
 * The function saves the full key to the page that was just
 * created so images that include this page can be updated by the backend
 * as required.
 *
 * \param[in,out] ipath  The path to the page being modified.
 * \param[in] owner  The plugin owner of the page.
 * \param[in] type  The type of the page.
 */
void images::on_create_content(content::path_info_t & ipath, QString const & owner, QString const & type)
{
    NOTUSED(owner);
    NOTUSED(type);

    //
    // TODO: automate connections between new pages and image transformations
    //
    // go through the list of scripts (children of /admin/images/scripts)
    // and see whether this new ipath key matches an entry;
    //
    // we can check with several parameters such as:
    //  . byte size
    //  . dimensions (width x height)
    //  . depth
    //  . extension
    //  . MIME type
    //  . path
    //
    //  TBD -- we may want to make use of the list plugin expression
    //         support to determine these; or even make each script
    //         a list! that way we can have any one page added to those
    //         scripts and let the images plugin know when a new page
    //         is added to the list so it can process it.
    //

    on_modified_content(ipath);
}


/** \brief Signal that a page was modified.
 *
 * This function is called whenever a plugin modified a page and then called
 * the modified_content() signal of the content plugin.
 *
 * This function checks whether the page is an attachment linked to
 * an images plugin script. If so, then the script need to be run against
 * the attachment so the page is re-added to the list of pages to check
 * for image transformation.
 *
 * \todo
 * If a script changes, then we need to know that and make sure to
 * re-generate all the images in link with that script.
 *
 * \param[in,out] ipath  The path to the page being modified.
 */
void images::on_modified_content(content::path_info_t & ipath)
{
    // check whether an image script is linked to this object
    links::link_info info(get_name(name_t::SNAP_NAME_IMAGES_SCRIPT), false, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info script_info;
    if(link_ctxt->next_link(script_info))
    {
        // here we do not need to loop, if we find at least one link then
        // request the backend to regenerate these different views
        content::content * content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t files_table(content_plugin->get_files_table());
        QtCassandra::QCassandraTable::pointer_t branch_table(content::content::instance()->get_branch_table());

        // TODO: Delay this add to the end of the process so we can avoid
        //       adding delays to our data processing
        //
        // add an arbitrary 2 seconds so the software has the time to
        // save all the info before it gets picked up by the backend
        int64_t const start_date(f_snap->get_start_date() + 2 * 1000000);

        // check whether we already had an entry for this image in the files
        // table, images row.
        QtCassandra::QCassandraValue old_date_value(branch_table->row(ipath.get_branch_key())->cell(get_name(name_t::SNAP_NAME_IMAGES_MODIFIED))->value());
        if(!old_date_value.nullValue())
        {
            // not null, there is an old date
            int64_t const old_date(old_date_value.int64Value());
            if(old_date == start_date)
            {
                // we already marked that as a change on this run, ignore
                // further requests
                return;
            }

            // delete a previous entry so we avoid transforming the
            // same image with the same transformation twice
            QByteArray old_key;
            QtCassandra::appendInt64Value(old_key, old_date);
            QtCassandra::appendStringValue(old_key, ipath.get_key());
            files_table->row(get_name(name_t::SNAP_NAME_IMAGES_ROW))->dropCell(old_key);
        }

        // we include the date in the key so that way older things get
        // processed first (this is good on a system with lots of websites)
        // although we'll need to make sure that we can handle all the work
        // and if necessary make use of multiple threads to work on the
        // actual transformations (not here)
        QByteArray key;
        QtCassandra::appendInt64Value(key, start_date);
        QtCassandra::appendStringValue(key, ipath.get_key());
        bool const modified(true);
        files_table->row(get_name(name_t::SNAP_NAME_IMAGES_ROW))->cell(key)->setValue(modified);

        // save a reference back to the new entry in the files_table
        // (this we keep so we can see when the image modifications were
        // requested and then once done how long it took the system to
        // do the work.)
        branch_table->row(ipath.get_branch_key())->cell(get_name(name_t::SNAP_NAME_IMAGES_MODIFIED))->setValue(start_date);

        f_ping_backend = true;
    }
}


/** \brief When a link is created, we get this message!
 *
 * We now listen to the modified_link signal so we know any time the
 * link is created.
 *
 * \todo
 * Look into whether the modified_link signal is enough and the
 * modified_content one could be removed (most certainly.)
 *
 * \param[in] info  The link that was just modified.
 */
void images::on_modified_link(links::link_info const & info)
{
    content::path_info_t ipath;
    ipath.set_path(info.key());
    on_modified_content(ipath);
}


/** \brief Capture this event which happens last.
 *
 * \note
 * We may want to create another "real" end of session message?
 */
void images::on_attach_to_session()
{
    if(f_ping_backend)
    {
        // send a PING to the backend
        f_snap->udp_ping(get_name(name_t::SNAP_NAME_IMAGES_ACTION));
    }
}


/** \brief Register the "images" action.
 *
 * This function registers this plugin as supporting the
 * "images" CRON action.
 *
 * This action is used to apply a "script" against images and other
 * attachment to generate a transformed image. The "script" support
 * most of the features available in the convert tool of ImageMagick.
 * So one can add transparency, borders, rotate, change colors, etc.
 *
 * The transformation includes the conversion of other attachments
 * such as PDF files to a preview image (or even a full scale,
 * printable version of the source image.)
 *
 * In most cases, the transformations are initiated when a client
 * sends a PING signal. Others may be defined as scripts to run
 * against specific types of data (i.e. always create a preview for
 * a Word processor document, make it 350 wide and automatically
 * compute the height, add a border, and a shadow.)
 *
 * \param[in,out] actions  The list of supported CRON actions where we add
 *                         ourselves.
 */
void images::on_register_backend_cron(server::backend_action_set & actions)
{
    actions.add_action(get_name(name_t::SNAP_NAME_IMAGES_ACTION), this);
}


/** \brief Register the transform action.
 *
 * This function registers this plugin as supporting the
 * "images" action.
 *
 * This action is used to apply a "script" against images and other
 * attachment to generate a transformed image. The "script" support
 * most of the features available in the convert tool of ImageMagick.
 * So one can add transparency, borders, rotate, change colors, etc.
 *
 * The transformation includes the conversion of other attachments
 * such as PDF files to a preview image (or even a full scale,
 * printable version of the source image.)
 *
 * The transformations are in most cases initiated when a client
 * sends a valid request via the listener plugin. Others may be
 * defined "internally" (i.e. always create a preview for a
 * Word processor document, make it 350 wide and automatically
 * compute the height, add a border and a shadow.)
 *
 * \param[in,out] actions  The list of supported actions where we add ourselves.
 */
void images::on_register_backend_action(server::backend_action_set & actions)
{
    actions.add_action(get_name(name_t::SNAP_NAME_IMAGES_PROCESS_IMAGE), this);
}


/** \brief Add the version of the ImageMagick library.
 *
 * This function adds the ImageMagick library version to the filter token.
 * This library is not expected to be linked against with core, only this
 * plugin and derivatives.
 *
 * \param[in] token  The token being worked on.
 */
void images::on_versions_libraries(filter::filter::token_info_t & token)
{
    token.f_replacement += "<li>";
    size_t ignore;
    token.f_replacement += MagickCore::GetMagickVersion(&ignore);
    token.f_replacement += " (compiled with " MagickLibVersionText ")</li>";
}


/** \brief Start the images transform server.
 *
 * When running the backend the user can ask to run the "images"
 * server (--action images). This function captures those events.
 * It loops until stopped with a STOP message via the UDP address/port.
 * Note that Ctrl-C won't work because it does not support killing
 * both: the parent and child processes (we do a fork() to create
 * this child.)
 *
 * This process goes through the complete list of transformations that
 * have been required and work on them as much as possible.
 *
 * \param[in] action  The action this function is being called with.
 */
void images::on_backend_action(QString const & action)
{
    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t files_table(content_plugin->get_files_table());

    if(action == get_name(name_t::SNAP_NAME_IMAGES_ACTION))
    {
        f_backend = dynamic_cast<snap_backend *>(f_snap);
        if(!f_backend)
        {
            throw images_exception_no_backend("could not determine the snap_backend pointer");
        }

        transform_images();
    }
    else if(action == get_name(name_t::SNAP_NAME_IMAGES_PROCESS_IMAGE))
    {
        QString const url(f_snap->get_server_parameter("URL"));
        content::path_info_t ipath;
        ipath.set_path(url);
        on_modified_content(ipath);
    }
    else
    {
        // unknown action (we should not have been called with that name!)
        throw snap_logic_exception(QString("images.cpp: images::on_backend_action(\"%1\") called with an unknown action...").arg(action));
    }
}


/** \brief This function transforms all the images and documents.
 *
 * This function is given the key to one image that requested some form
 * of transformation. The information about the transformation is found
 * in the database.
 *
 * \return the number of micro seconds to the next transformation
 *         or zero if no more transformations are necessary
 */
int64_t images::transform_images()
{
    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t files_table(content_plugin->get_files_table());
    files_table->clearCache();
    QtCassandra::QCassandraRow::pointer_t images_row(files_table->row(get_name(name_t::SNAP_NAME_IMAGES_ROW)));
    images_row->clearCache();
    QString const site_key(f_snap->get_site_key_with_slash());

    // we use a smaller number (100) instead of a larger number (1000)
    // in case the user makes changes we are more likely to catch the
    // latest version instead of using an older cached version
    auto column_predicate = std::make_shared<QtCassandra::QCassandraCellRangePredicate>();
    column_predicate->setCount(100);
    column_predicate->setIndex(); // behave like an index

    // loop until all cells were deleted or the STOP signal was received
    for(;;)
    {
        // Note: because it is sorted, the oldest entries are worked on first
        //
        images_row->readCells(column_predicate);
        QtCassandra::QCassandraCells const cells(images_row->cells());
        if(cells.isEmpty())
        {
            // no more transformation, we can sleep for 5 min.
            // (but here we return zero)
            return 0;
        }

        // handle one batch
        for(QtCassandra::QCassandraCells::const_iterator c(cells.begin());
                c != cells.end();
                ++c)
        {
            // reset start date so it looks like we just got
            // a new client request
            f_snap->init_start_date();

            int64_t const start_date(f_snap->get_start_date());

            // the cell
            QtCassandra::QCassandraCell::pointer_t cell(*c);
            // the key starts with the "start date" and it is followed by a
            // string representing the row key in the content table
            QByteArray const & key(cell->columnKey());

            int64_t const page_start_date(QtCassandra::int64Value(key, 0));
            if(page_start_date > start_date)
            {
                // since the columns are sorted, anything after that will be
                // inaccessible, date wise, so we are 100% done for this
                // round; return the number of microseconds to wait before
                // we can handle the next transformation
                return page_start_date - start_date;
            }

            QString const image_key(QtCassandra::stringValue(key, sizeof(int64_t)));
            if(!image_key.startsWith(site_key))
            {
                // "wrong" site, ignore this entry on this run
                continue;
            }

            // print out the row being worked on
            // (if it crashes it is really good to know where)
            {
                QString name;
                uint64_t time(QtCassandra::uint64Value(key, 0));
                char buf[64];
                struct tm t;
                time_t const seconds(time / 1000000);
                gmtime_r(&seconds, &t);
                strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
                name = QString("%1.%2 (%3) %4").arg(buf).arg(time % 1000000, 6, 10, QChar('0')).arg(time).arg(image_key);
                SNAP_LOG_TRACE("images plugin working on column \"")(name)("\"");
            }

            if(do_image_transformations(image_key))
            {
                // we handled that image so drop it now
                images_row->dropCell(key);
            }

            // quickly end this process if the user requested a stop
            if(f_backend->stop_received())
            {
                // clean STOP
                //
                // We can return zero here because pop_message() will
                // anyway return immediately with false when STOP was
                // received.
                return 0;
            }
        }
    }
}


/** \brief Apply all the transformation to one page.
 *
 * This function applies all the necessary transformations to the specified
 * page. The function may return prematurely if it detects that the STOP
 * signal we sent to the process. In that case the function returns false
 * to make sure that the caller does not mark the page as done.
 *
 * \param[in] image_key  The key of the page to work on.
 *
 * \return true if all the transformations were applied.
 */
bool images::do_image_transformations(QString const & image_key)
{
    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    content_table->clearCache();
    QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
    branch_table->clearCache();
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    revision_table->clearCache();
    content::path_info_t image_ipath;
    image_ipath.set_path(image_key);

    //
    // TODO: at this point we only work on the current branch but we
    //       really need to work on all branches (but I think that
    //       the previous loop should be in charge of that scheme...
    //       and call us here with the information of which branches
    //       to work on.)
    //
    //       Note that the current branch should have priority over
    //       all the other branches, so we should process the current
    //       branches of all the pages from all the websites; then
    //       come back and work on all the working branches of all
    //       the pages from all the websites; finally, do another
    //       round with all the old branches if time allows. This
    //       means if a user switch to an old branch, all the image
    //       transformations may not be up to date for a little
    //       while until it gets picked up as a current branch!
    //

    // get the images
    links::link_info info(get_name(name_t::SNAP_NAME_IMAGES_SCRIPT), false, image_ipath.get_key(), image_ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info script_info;
    while(link_ctxt->next_link(script_info))
    {
        // quickly end this process if the user requested a stop
        if(f_backend->stop_received())
        {
            // clean STOP
            //
            // In this case the STOP prevents the transformations
            // from being complete so we return false to make sure
            // we get called again
            return false;
        }

        // read the image script from the destination of this link
        QString const script_key(script_info.key());
        content::path_info_t script_ipath;
        script_ipath.set_path(script_key);
        QString script(revision_table->row(script_ipath.get_revision_key())->cell(get_name(name_t::SNAP_NAME_IMAGES_SCRIPT))->value().stringValue());
        if(script.isEmpty())
        {
            // We have a problem here! This is a waste of time.
            // We could unlink from this entry, but by doing so we may
            // break something else in the long run.
            //
            // TBD: do we need to do anything here?
            continue;
        }

        // ignore the returned result here (we expect the script to
        // include a write); however other plugins may want to use
        // an image locally and not save it to the database in which
        // case the result would be useful!
        content::path_info_t::map_path_info_t image_ipaths;
        image_ipaths["INPUT"] = &image_ipath;
        apply_image_script(script, image_ipaths);
    }

    // if we reach here then we are 100% done with all those transformations
    // so we can return true
    return true;
}


/** \brief Apply a script against one or more images.
 *
 * Source: http://www.imagemagick.org/Magick++/Documentation.html
 *
 * \warning
 * The returned image may be an empty image in case the script fails.
 *
 * \param[in] script  The script to process the images.
 * \param[in] image_ipaths  An array of ipaths to images.
 *
 * \return The resulting image (whatever is current on the stack at the time
 *         the script ends.)
 */
Magick::Image images::apply_image_script(QString const & script, content::path_info_t::map_path_info_t image_ipaths)
{
    QString s(script);

    parameters_t params;
    bool repeat;
    do
    {
        repeat = false;
        f_on_error.clear();

        s.replace("\r", "\n");
        snap_string_list commands(s.split("\n"));

        params.f_image_ipaths = image_ipaths;
        params.f_image_stack.clear();

        int max_commands(commands.size());
        for(int idx(0); idx < max_commands; ++idx)
        {
            params.f_command = commands[idx].simplified();
            if(params.f_command.isEmpty())
            {
                // skip empty lines (could be many if script lines ended with \r\n)
                continue;
            }
            if(params.f_command[0] == '#')
            {
                // line commented out are also skipped
                continue;
            }

            // find the first parameter
            // (remember that we already simplified the string)
            int pos(params.f_command.indexOf(" "));
            if(pos < 0)
            {
                pos = params.f_command.length();
            }
            QString const cmd(params.f_command.mid(0, pos));

            // search for this command using a fast binary search
            QByteArray const name(cmd.toUtf8());
            char const *n(name.data());

            size_t p(static_cast<size_t>(-1));
            size_t i(0);
            size_t j(g_commands_size);
            while(i < j)
            {
                // get the center position of the current range
                p = i + (j - i) / 2;
                int const r(strcmp(n, g_commands[p].f_command_name));
                if(r == 0)
                {
                    break;
                }
                if(r > 0)
                {
                    // move the range up (we already checked p so use p + 1)
                    i = p + 1;
                }
                else
                {
                    // move the range down (we never check an item at position j)
                    j = p;
                }
            }

            // found?
            if(i >= j)
            {
                messages::messages msg;
                msg.set_error("Unknown Command",
                        QString("Command \"%1\" is not known.").arg(cmd),
                        QString("Command in \"%1\" was not found in our list of commands.").arg(params.f_command),
                        false);
                continue;
            }

            // need to clear (previous command parameters are still
            // defined in that array!)
            params.f_params.clear();

            // found it! verify the number of arguments
            if(params.f_command.length() > static_cast<int>(pos + 1))
            {
                QString const cmd_params(params.f_command.mid(pos + 1));
                QString ps;
                int const cmd_params_max(cmd_params.length());
                for(int pidx(0); pidx < cmd_params_max; ++pidx)
                {
                    if(cmd_params[pidx].unicode() == ' ')
                    {
                        // separator
                        params.f_params << ps;
                        ps.clear();
                    }
                    else if(cmd_params[pidx].unicode() == '"'
                         || cmd_params[pidx].unicode() == '\'')
                    {
                        // this parameter is a string, parse up to the next
                        // quote; quotes are not included in the result
                        short const quote(cmd_params[pidx].unicode());
                        bool found(false);
                        for(++pidx; pidx < cmd_params_max; ++pidx)
                        {
                            if(cmd_params[pidx].unicode() == quote)
                            {
                                // skip the closing quote
                                ++pidx;
                                found = true;
                                break;
                            }
                            ps += cmd_params[pidx];
                        }
                        if(!found)
                        {
                            messages::messages msg;
                            msg.set_warning("Invalid String Parameter",
                                    QString("String parameters must have matching opening and closing quotes."),
                                    QString("Invalid string in \"%1\" (position %2).").arg(params.f_command).arg(params.f_params.size()));
                        }

                        // strings get auto added, only if followed by a space
                        // we want to remove that space to avoid getting an
                        // empty parameter added!
                        for(; pidx < cmd_params_max && cmd_params[pidx].unicode() == ' '; ++pidx);

                        // did we reach the end of the input string?
                        // if so, then we're done and have to exit this
                        // loop now without adding the last parameter here
                        // (it is done after the for() loop we are in)
                        if(pidx >= cmd_params_max)
                        {
                            break;
                        }

                        params.f_params << ps;
                        ps.clear();
                    }
                    else
                    {
                        ps += cmd_params[pidx];
                    }
                }
                // last part added here since we won't hit a ' ' before the end
                params.f_params << ps;
            }
            size_t const max_params(params.f_params.size());
            if(max_params < g_commands[p].f_min_params || max_params > g_commands[p].f_max_params)
            {
                // we create a message but this is run by a backend so
                // the end users won't see those; we'll need to find
                // a way, probably use the author of the script page
                // to send that information to someone
                messages::messages msg;
                msg.set_error("Invalid Number of Parameters",
                        QString("Invalid number of parameters for images.density (%1, expected 1 or 2)").arg(max_params),
                        QString("Invalid number of parameters in \"%1\"").arg(params.f_command),
                        false);
                continue;
            }

            // verify the minimum stack size
            if(params.f_image_stack.size() < g_commands[p].f_min_stack)
            {
                // we create a message but this is run by a backend so
                // the end users won't see those; we'll need to find
                // a way, probably use the author of the script page
                // to send that information to someone
                messages::messages msg;
                msg.set_error("Invalid Number of Images",
                        QString("Invalid number of images for %1 (expected %2, need %3)").arg(cmd).arg(static_cast<int>(g_commands[p].f_min_stack)).arg(params.f_image_stack.size()),
                        QString("Invalid number of images in the stack at this point for \"%1\"").arg(params.f_command),
                        false);
                continue;
            }

            // transform variables (if any) to actual paths
// for now keep a log to see what is happening
SNAP_LOG_INFO(" ++ [")(params.f_command)("]");
            for(int k(0); k < params.f_params.size(); ++k)
            {
                int start_pos(0);
                for(;;)
                {
                    QString const param(params.f_params[k]);
                    start_pos = param.indexOf("${", start_pos);
                    if(start_pos < 0)
                    {
                        break;
                    }
                    // there is a variable start point ("${")
                    start_pos += 2;
                    int const end_pos(param.indexOf("}", start_pos));
                    if(start_pos < end_pos )
                    {
                        // variable name is not empty
                        QString var_name(param.mid(start_pos, end_pos - start_pos));
                        content::path_info_t::map_path_info_t::const_iterator var(params.f_image_ipaths.find(var_name.toUtf8().data()));
                        if(var != params.f_image_ipaths.end())
                        {
                            start_pos -= 2;
                            QString const var_value(var->second->get_key());
                            params.f_params[k].replace(start_pos, end_pos + 1 - start_pos, var_value);
                        }
                    }
                }
SNAP_LOG_INFO() << " -- param[" << k << "] = [" << params.f_params[k] << "]";
            }

            // call the command
            if(!(this->*g_commands[p].f_command)(params))
            {
                // the command failed, return a default image instead
                if(f_on_error.isEmpty())
                {
                    return Magick::Image();
                }

                // the user defined a fallback on error, execute it
                //
                // the on error string cannot appear on multiple lines
                // so we replace and escaped 'n' or 'r' (i.e. \n
                // and \r in the input string) to actual '\n' and '\r'.
                s = f_on_error.replace("\\n", "\n").replace("\\r", "\r");
                repeat = true;
                break;
            }
        }
    }
    while(repeat);

    if(params.f_image_stack.empty())
    {
        // no image on the stack...
        return Magick::Image();
    }

    return params.f_image_stack.back();
}


bool images::get_color(QString str, Magick::Color & color)
{
    // TODO: add support for rgb(), rgba(), hsl(), hsla(), yuv(), yuva()
    if(str[0] == '#')
    {
        // '#' was specified, remove it before parsing the color
        str = str.mid(1);
    }

    bool valid(false);
    int const int_color(str.toInt(&valid, 16));
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid RGB Color",
                QString("Color \"%1\" is not valid.").arg(str),
                "Specified color is not valid.",
                false);
        return false;
    }

    // internally we use an RGB color because we cannot be sure
    // what the Quantum is (we could also pass the color directly
    // to the constructor, but that would certainly prevent us
    // from supporting a growing number of color definitions.)
    //
    Magick::ColorRGB rgb;

    rgb.red  (((int_color >> 16) & 255) / 255.0);
    rgb.green(((int_color >>  8) & 255) / 255.0);
    rgb.blue (((int_color >>  0) & 255) / 255.0);

    // also make sure it is 100% opaque
    rgb.alphaQuantum(0.0);

    color = rgb;

    return true;
}


bool images::func_alpha(parameters_t & params)
{
    QString const mode(params.f_params[0].toLower());
    if(mode == "off" || mode == "deactivate")
    {
        params.f_image_stack.back().matte(false);
    }
    else if(mode == "on" || mode == "activate")
    {
        params.f_image_stack.back().matte(true);
    }
    // TODO: add support for: set, opaque, transparent, extract, copy
    //                        shape, remove, background
    else
    {
        messages::messages msg;
        msg.set_error("Invalid Parameters",
                QString("Invalid parameter to alpha command \"%1\", expected one of: activate, background, deactivate, copy, extract, opaque, remove, set, shape, transparent)").arg(mode),
                QString("Invalid parameters in \"%1\"").arg(params.f_command),
                false);
        return false;
    }

    return true;
}


bool images::func_background_color(parameters_t & params)
{
    // matte color is HTML like RGB (i.e. #123456)
    //
    Magick::Color color;
    if(!get_color(params.f_params[0], color))
    {
        return false;
    }
    params.f_image_stack.back().backgroundColor(color);

    return true;
}


bool images::func_blur(parameters_t & params)
{
    double radius(1.0);
    double sigma(0.5);
    bool valid(false);

    size_t const size(params.f_params.size());
    if(size >= 1)
    {
        radius = params.f_params[0].toDouble(&valid);
        if(!valid
        || radius < 0.0)
        {
            messages::messages msg;
            msg.set_error("Invalid Radius",
                    QString("blur() expects a positive double or null number, \"%1\" is not valid.").arg(params.f_params[0]),
                    "The parameter is not a valid double or it is negative or zero.",
                    false);
            return false;
        }
    }
    if(size >= 2)
    {
        sigma = params.f_params[1].toDouble(&valid);
        if(!valid
        || sigma <= 0.0)
        {
            messages::messages msg;
            msg.set_error("Invalid Sigma",
                    QString("blur() expects a positive double number, \"%1\" is not valid.").arg(params.f_params[1]),
                    "The parameter is not a valid double or it is negative or zero.",
                    false);
            return false;
        }
    }

    params.f_image_stack.back().blur(radius, sigma);
    return true;
}


bool images::func_border(parameters_t & params)
{
    Magick::Geometry geometry(params.f_params[0].toUtf8().data());
    params.f_image_stack.back().border(geometry);
    return true;
}


bool images::func_border_color(parameters_t & params)
{
    // matte color is HTML like RGB (i.e. #123456)
    //
    Magick::Color color;
    if(!get_color(params.f_params[0], color))
    {
        return false;
    }
    params.f_image_stack.back().borderColor(color);

    return true;
}


bool images::func_charcoal(parameters_t & params)
{
    double radius(1.0);
    double sigma(0.5);
    bool valid(false);

    size_t const size(params.f_params.size());
    if(size >= 1)
    {
        radius = params.f_params[0].toDouble(&valid);
        if(!valid
        || radius < 0.0)
        {
            messages::messages msg;
            msg.set_error("Invalid Radius",
                    QString("charcoal() expects a positive double or null number, \"%1\" is not valid.").arg(params.f_params[0]),
                    "The parameter is not a valid double or it is negative or zero.",
                    false);
            return false;
        }
    }
    if(size >= 2)
    {
        sigma = params.f_params[1].toDouble(&valid);
        if(!valid
        || sigma <= 0.0)
        {
            messages::messages msg;
            msg.set_error("Invalid Sigma",
                    QString("charcoal() expects a positive double number, \"%1\" is not valid.").arg(params.f_params[1]),
                    "The parameter is not a valid double or it is negative or zero.",
                    false);
            return false;
        }
    }

    params.f_image_stack.back().charcoal(radius, sigma);
    return true;
}


bool images::func_composite(parameters_t & params)
{
    QString const composite_str(params.f_params[0].toLower());
    Magick::CompositeOperator composite_operator(Magick::UndefinedCompositeOp);

    // TODO: add all composite operators
    // file:///usr/share/doc/imagemagick/www/Magick++/Enumerations.html#CompositeOperator
    //

    if(composite_str == "over")
    {
        composite_operator = Magick::OverCompositeOp;
    }
    else if(composite_str == "copy")
    {
        composite_operator = Magick::CopyCompositeOp;
    }
    else
    {
        messages::messages msg;
        msg.set_error("Invalid Parameters",
                QString("Unknown composite parameter \"%1\".").arg(composite_str),
                QString("Invalid parameters in \"%1\"").arg(params.f_command),
                false);
        return false;
    }
    params.f_image_stack.back().compose(composite_operator);
    return true;
}


bool images::func_contrast(parameters_t & params)
{
    bool valid(false);
    int const contrast(params.f_params[0].toInt(&valid));
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid Parameters",
                QString("contrast() expects an integer as parameter \"%1\".").arg(params.f_params[0]),
                QString("Invalid parameters in \"%1\"").arg(params.f_command),
                false);
        return false;
    }
    params.f_image_stack.back().contrast(contrast);
    return true;
}


bool images::func_create(parameters_t & params)
{
    Magick::Image im;
    params.f_image_stack.push_back(im);
    return true;
}


bool images::func_crop(parameters_t & params)
{
    Magick::Geometry geometry(params.f_params[0].toUtf8().data());
    params.f_image_stack.back().crop(geometry);
    return true;
}


bool images::func_density(parameters_t & params)
{
    bool valid(false);
    int const x(params.f_params[0].toInt(&valid));
    int y(0);
    if(valid)
    {
        if(params.f_params.size() == 2)
        {
            y = params.f_params[1].toInt(&valid);
        }
        else
        {
            y = x;
        }
    }
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid Parameters",
                "Invalid parameters for images.density (expected valid integers)",
                QString("Invalid parameters in \"%1\"").arg(params.f_command),
                false);
        return false;
    }
    params.f_image_stack.back().density(Magick::Geometry(x, y));
    return true;
}


bool images::func_emboss(parameters_t & params)
{
    double radius(1.0);
    double sigma(0.5);
    bool valid(false);

    size_t const size(params.f_params.size());
    if(size >= 1)
    {
        radius = params.f_params[0].toDouble(&valid);
        if(!valid
        || radius < 0.0)
        {
            messages::messages msg;
            msg.set_error("Invalid Radius",
                    QString("emboss() expects a positive or null double number, \"%1\" is not valid.").arg(params.f_params[0]),
                    "The parameter is not a valid double or it is negative or zero.",
                    false);
            return false;
        }
    }
    if(size >= 2)
    {
        sigma = params.f_params[1].toDouble(&valid);
        if(!valid
        || sigma <= 0.0)
        {
            messages::messages msg;
            msg.set_error("Invalid Sigma",
                    QString("emboss() expects a positive double number, \"%1\" is not valid.").arg(params.f_params[1]),
                    "The parameter is not a valid double or it is negative or zero.",
                    false);
            return false;
        }
    }

    params.f_image_stack.back().emboss(radius, sigma);
    return true;
}


bool images::func_erase(parameters_t & params)
{
    params.f_image_stack.back().erase();
    return true;
}


bool images::func_flip(parameters_t & params)
{
    params.f_image_stack.back().flip();
    return true;
}


bool images::func_flop(parameters_t & params)
{
    params.f_image_stack.back().flop();
    return true;
}


bool images::func_hash(parameters_t & params)
{
    bool valid(false);

    double const start_offset(params.f_params[0].toDouble(&valid));
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid Start Offset",
                QString("hash() expects a double number for start offset, \"%1\" is not valid.").arg(params.f_params[0]),
                "Invalid parameter.",
                false);
        return false;
    }

    double const thickness(params.f_params[1].toDouble(&valid));
    if(!valid
    || thickness <= 0.1)
    {
        messages::messages msg;
        msg.set_error("Invalid Start Offset",
                QString("hash() expects a double number for thickness, \"%1\" is not valid.").arg(params.f_params[1]),
                "Invalid parameter.",
                false);
        return false;
    }

    double const space(params.f_params[2].toDouble(&valid));
    if(!valid
    || space <= 0.1)
    {
        messages::messages msg;
        msg.set_error("Invalid Start Offset",
                QString("hash() expects a double number for space, \"%1\" is not valid.").arg(params.f_params[2]),
                "Invalid parameter.",
                false);
        return false;
    }

    double angle(params.f_params[3].toDouble(&valid));
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid Start Offset",
                QString("hash() expects a double number for space, \"%1\" is not valid.").arg(params.f_params[3]),
                "Invalid parameter.",
                false);
        return false;
    }

    Magick::Color color;
    if(!get_color(params.f_params[4], color))
    {
        messages::messages msg;
        msg.set_error("Invalid Start Offset",
                QString("hash() expects a double number for space, \"%1\" is not valid.").arg(params.f_params[4]),
                "Invalid parameter.",
                false);
        return false;
    }

    params.f_image_stack.back().strokeColor(color);
    params.f_image_stack.back().strokeWidth(thickness);

    // if the hash is perfectly horizontal or vertical, then we only need
    // one loop again the "opposite edge"; here we have two conditional
    // loops to test those two special cases:
    //
    double x1(0.0);
    double y1(0.0);
    double x2(0.0);
    double y2(0.0);

    size_t width(params.f_image_stack.back().columns());
    size_t height(params.f_image_stack.back().rows());

    angle = fmod(angle, 360.0) * M_PI / 180.0;

    double const increment(thickness + space);

    // TODO: we want to have an integer as an index and multiply
    //       the thickness + start position instead of incremental
    //       position as doing now...
    //
    //           pos = start [+ offset] + increment * index
    //
    //       (the offset would be the moving backward by gap as we do
    //       in the last case)
    //
    if(sin(fabs(angle)) < 0.000001)
    {
        // "perfectly" horizontal lines
        // we could use splice unless some of the numbers were not integers...
        x1 = 0.0;
        x2 = width;
        for(y1 = start_offset + thickness / 2.0; y1 < width + thickness; y1 += increment)
        {
            y2 = y1;
            params.f_image_stack.back().draw(Magick::DrawableLine(x1, y1, x2, y2));
        }
    }
    else if(cos(fabs(angle)) < 0.000001)
    {
        // "perfectly" vertical lines
        y1 = 0.0;
        y2 = height;
        for(x1 = start_offset + thickness / 2.0; x1 < width; x1 += increment)
        {
            x2 = x1;
            params.f_image_stack.back().draw(Magick::DrawableLine(x1, y1, x2, y2));
        }
    }
    else
    {
        // we can go from left to right in the image and draw lines
        x1 = start_offset + thickness / 2.0;
        y1 = 0.0;
        y2 = height;
        double const gap(height / tan(angle));
        if(gap > 0)
        {
            x1 -= ceil(gap / increment) * increment;
        }
        for(x2 = x1 + height / tan(angle); x1 < width + thickness || x2 < width + thickness; x1 += increment, x2 = x1 + gap)
        {
            params.f_image_stack.back().draw(Magick::DrawableLine(x1, y1, x2, y2));
        }
    }

    return true;
}


bool images::func_matte_color(parameters_t & params)
{
    // matte color is HTML like RGB (i.e. #123456)
    //
    Magick::Color color;
    if(!get_color(params.f_params[0], color))
    {
        return false;
    }
    params.f_image_stack.back().matteColor(color);

    return true;
}


bool images::func_modulate(parameters_t & params)
{
    bool valid(false);

    double const brightness(params.f_params[0].toDouble(&valid));
    if(!valid
    || brightness < 0.0
    || brightness > 2.0)
    {
        messages::messages msg;
        msg.set_error("Invalid Brightness",
                QString("modulate() expects a double number between 0.0 and 2.0, \"%1\" is not valid.").arg(params.f_params[0]),
                "Somehow the specified page has no image",
                false);
        return false;
    }

    double const saturation(params.f_params[1].toDouble(&valid));
    if(!valid
    || saturation < 0.0
    || saturation > 2.0)
    {
        messages::messages msg;
        msg.set_error("Invalid Saturation",
                QString("modulate() expects a double number between 0.0 and 2.0, \"%1\" is not valid.").arg(params.f_params[1]),
                "Somehow the specified page has no image",
                false);
        return false;
    }

    double const hue(params.f_params[2].toDouble(&valid));
    if(!valid
    || hue < 0.0
    || hue > 2.0)
    {
        messages::messages msg;
        msg.set_error("Invalid Hue",
                QString("modulate() expects a double number between 0.0 and 2.0, \"%1\" is not valid.").arg(params.f_params[2]),
                "Somehow the specified page has no image",
                false);
        return false;
    }

    // do not ask... ImageMagick likes percent values as values
    // between 0 to 100...
    //
    params.f_image_stack.back().modulate(brightness * 100.0, saturation * 100.0, hue * 100.0);
    return true;
}


bool images::func_negate(parameters_t & params)
{
    bool grayscale(false);
    if(params.f_params.size() > 0)
    {
        grayscale = params.f_params[0] == "true";
    }

    params.f_image_stack.back().negate(grayscale);
    return true;
}


bool images::func_normalize(parameters_t & params)
{
    params.f_image_stack.back().normalize();
    return true;
}


bool images::func_oil_paint(parameters_t & params)
{
    bool valid(false);
    double radius(params.f_params[0].toDouble(&valid));
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid Radius",
                QString("oil_paint() expects a double number representing a radius, \"%1\" is not valid.").arg(params.f_params[0]),
                "Somehow the specified page has no image",
                false);
        return false;
    }

    params.f_image_stack.back().oilPaint(radius);
    return true;
}


bool images::func_on_error(parameters_t & params)
{
    // this is quite peculiar, it saves a string that becomes the script
    // in the event an error occurs in another function
    f_on_error = params.f_params[0];
    return true;
}


bool images::func_pop(parameters_t & params)
{
    params.f_image_stack.pop_back();
    return true;
}


bool images::func_read(parameters_t & params)
{
    // param 1 is the ipath (key)
    // param 2 is the name used to load the file from the files table
    // param 3 is the image number, zero by default (optional -- currently unused)

    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    QtCassandra::QCassandraTable::pointer_t files_table(content_plugin->get_files_table());

    content::path_info_t ipath;
    ipath.set_path(params.f_params[0]);
    QByteArray md5(revision_table->row(ipath.get_revision_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT))->value().binaryValue());
    if(md5.size() != 16)
    {
        // there is no file in this page so we have to skip it
        messages::messages msg;
        msg.set_error("Missing Image File",
                QString("Loading of image in \"%1\" failed (no valid md5 found).").arg(ipath.get_revision_key()),
                "Somehow the specified page has no image",
                false);
        return false;
    }
    QString const output_name(params.f_params[1]);
    QString field_name;
    if(output_name == "data")
    {
        field_name = content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_DATA);
    }
    else
    {
        field_name = QString("%1::%2").arg(content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_DATA)).arg(output_name);
    }
    QByteArray image_data(files_table->row(md5)->cell(field_name)->value().binaryValue());
    if(image_data.isEmpty())
    {
        // there is no file in this page so we have to skip it
        messages::messages msg;
        msg.set_error("Empty Image File",
                QString("Image in \"%1\" is currently empty.").arg(ipath.get_revision_key()),
                "Somehow the specified file is empty so not an image",
                false);
        return false;
    }

    Magick::Blob blob(image_data.data(), image_data.length());
    try
    {
        params.f_image_stack.back().read(blob);
    }
    catch(std::exception const & e)
    {
        messages::messages msg;
        msg.set_error("Invalid Image File",
                QString("Image in \"%1\" could not be read.").arg(ipath.get_revision_key()),
                QString("Somehow loading this image file failed with an exception: %1").arg(e.what()),
                false);
        return false;
    }

    return true;
}


bool images::func_reduce_noise(parameters_t & params)
{
    if(params.f_params.size() > 0)
    {
        bool valid(false);
        double order(params.f_params[0].toDouble(&valid));
        if(!valid)
        {
            messages::messages msg;
            msg.set_error("Invalid Order",
                    QString("reduce_noise() expects a double number representing an order, \"%1\" is not valid.").arg(params.f_params[0]),
                    "The parameter is not valid",
                    false);
            return false;
        }
        params.f_image_stack.back().reduceNoise(order);
    }
    else
    {
        params.f_image_stack.back().reduceNoise();
    }

    return true;
}


bool images::func_resize(parameters_t & params)
{
    Magick::Geometry size(params.f_params[0].toUtf8().data());
    params.f_image_stack.back().resize(size);
    return true;
}


bool images::func_rotate(parameters_t & params)
{
    bool valid(false);
    double const angle(params.f_params[0].toDouble(&valid));
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid Angle",
                QString("rotate() expects a double number representing an angle, \"%1\" is not valid.").arg(params.f_params[0]),
                "The parameter is not valid",
                false);
        return false;
    }
    params.f_image_stack.back().rotate(angle);

    return true;
}


bool images::func_shade(parameters_t & params)
{
    bool valid(false);

    double const azimuth(params.f_params[0].toDouble(&valid));
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid Azimuth",
                QString("shade() expects a double number representing the azimuth, \"%1\" is not valid.").arg(params.f_params[0]),
                "The parameter is not valid",
                false);
        return false;
    }

    double const elevation(params.f_params[1].toDouble(&valid));
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid Elevation",
                QString("shade() expects a double number representing the elevation, \"%1\" is not valid.").arg(params.f_params[1]),
                "The parameter is not valid",
                false);
        return false;
    }

    bool const color_shading(params.f_params[2] == "true");

    params.f_image_stack.back().shade(azimuth, elevation, color_shading);

    return true;
}


bool images::func_shadow(parameters_t & params)
{
    bool valid(false);

    double const opacity(params.f_params[0].toDouble(&valid));
    if(!valid
    || opacity < 0.0)
    {
        messages::messages msg;
        msg.set_error("Invalid Opacity",
                QString("shadow() expects a positive or null double number representing the opacity, \"%1\" is not valid.").arg(params.f_params[0]),
                "The parameter is not valid",
                false);
        return false;
    }

    double const sigma(params.f_params[1].toDouble(&valid));
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid Sigma",
                QString("shadow() expects a double number representing sigma, \"%1\" is not valid.").arg(params.f_params[1]),
                "The parameter is not valid",
                false);
        return false;
    }

    ssize_t const x(params.f_params[2].toInt(&valid));
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid Horizontal Position",
                QString("shadow() expects an integer representing the horizontal position, \"%1\" is not valid.").arg(params.f_params[2]),
                "The parameter is not valid",
                false);
        return false;
    }

    ssize_t const y(params.f_params[3].toInt(&valid));
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid Vertical Position",
                QString("shadow() expects an integer representing the vertical position, \"%1\" is not valid.").arg(params.f_params[3]),
                "The parameter is not valid",
                false);
        return false;
    }

    // The shadow() function generates a shadow from an existing image
    // but it does not do the compositing work; also you want to set
    // the background color to black (generally) first because by default
    // your shadow will be white...
    //
    Magick::Image im(params.f_image_stack.back());
    params.f_image_stack.back().shadow(opacity * 100.0, sigma, x, y);
    params.f_image_stack.back().composite(im, x >= 0 ? 0 : -x, y >= 0 ? 0 : -y, Magick::OverCompositeOp);

    return true;
}


bool images::func_sharpen(parameters_t & params)
{
    bool valid(false);

    double const radius(params.f_params[0].toDouble(&valid));
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid Radius",
                QString("sharpen() expects a double number representing the radius, \"%1\" is not valid.").arg(params.f_params[0]),
                "The parameter is not valid",
                false);
        return false;
    }

    double const sigma(params.f_params[1].toDouble(&valid));
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid Elevation",
                QString("sharpen() expects a double number representing sigma, \"%1\" is not valid.").arg(params.f_params[1]),
                "The parameter is not valid",
                false);
        return false;
    }

    params.f_image_stack.back().sharpen(radius, sigma);

    return true;
}


bool images::func_shear(parameters_t & params)
{
    bool valid(false);

    double const x(params.f_params[0].toDouble(&valid));
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid Horizontal Shear",
                QString("shear() expects a double number representing the horizontal shear, \"%1\" is not valid.").arg(params.f_params[0]),
                "The parameter is not valid",
                false);
        return false;
    }

    double const y(params.f_params[1].toDouble(&valid));
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid Vertizontal Shear",
                QString("shear() expects a double number representing the vertizontal shear, \"%1\" is not valid.").arg(params.f_params[1]),
                "The parameter is not valid",
                false);
        return false;
    }

    params.f_image_stack.back().shear(x, y);

    return true;
}


bool images::func_solarize(parameters_t & params)
{
    bool valid(false);

    double const factor(params.f_params[0].toDouble(&valid));
    if(!valid)
    {
        messages::messages msg;
        msg.set_error("Invalid Factor",
                QString("solirize() expects a double number representing the factor, \"%1\" is not valid.").arg(params.f_params[0]),
                "The parameter is not valid",
                false);
        return false;
    }

    params.f_image_stack.back().solarize(factor);

    return true;
}


bool images::func_swap(parameters_t & params)
{
    std::iter_swap(params.f_image_stack.end() - 1, params.f_image_stack.end() - 2);
    return true;
}


bool images::func_trim(parameters_t & params)
{
    params.f_image_stack.back().trim();
    return true;
}


bool images::func_write(parameters_t & params)
{
    // param 1 is the ipath (key)
    // param 2 is the name used to save the file in the files table

    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    QtCassandra::QCassandraTable::pointer_t files_table(content_plugin->get_files_table());

    content::path_info_t ipath;
    ipath.set_path(params.f_params[0]);
    QByteArray md5(revision_table->row(ipath.get_revision_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT))->value().binaryValue());

    QString const output_name(params.f_params[1]);
    if(output_name == "data")
    {
        messages::messages msg;
        msg.set_error("Invalid Parameter",
                "Invalid parameters for write(), the output name cannot be \"data\"",
                QString("Preventing output to the main \"data\" buffer itself").arg(params.f_command),
                false);
        return false;
    }
    int const ext_pos(output_name.lastIndexOf("."));
    if(ext_pos > 0 && ext_pos + 1 < output_name.length())
    {
        QString const ext(output_name.mid(ext_pos + 1));
        try
        {
            params.f_image_stack.back().magick(ext.toUtf8().data());
        }
        catch(Magick::Exception const e)
        {
            // TODO: ignore the error...
            //       we may need to force a default format or report the
            //       error and exit though
        }
    }
    //else -- TBD: should we err in this case?
    Magick::Blob blob;
    params.f_image_stack.back().write(&blob);
    QString field_name(QString("%1::%2").arg(content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_DATA)).arg(output_name));
    QByteArray array(static_cast<char const *>(blob.data()), static_cast<int>(blob.length()));

    files_table->row(md5)->cell(field_name)->setValue(array);

    return true;
}


void images::on_replace_token( content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token )
{
    snap::NOTUSED(ipath);
    snap::NOTUSED(xml);

    if(!token.is_namespace("images::"))
    {
        // not an images plugin token
        return;
    }

    if( token.is_token("images::inline_uri") )
    {
        if( !token.verify_args(1, 1) )
        {
            SNAP_LOG_ERROR("images::on_replace_token(): images::inline_uri() expects exactly 1 argument!");
            return;
        }

        // EX-167: append revisioning information to the filename to overcome
        // browser caching if the user uploads a new version of the picture.
        //
        QString const uri( token.get_arg("uri", 0).f_value );

        // TODO: if the URI already includes a query string, we may need to
        //       do some work on it before we can pass it to set_path()...
        //
        content::path_info_t img_ipath;
        img_ipath.set_path(uri);

SNAP_LOG_TRACE("image_path cpath=")(img_ipath.get_cpath());
        //
        if( img_ipath.has_branch() && img_ipath.has_revision() )
        {
            token.f_replacement = QString("%1?branch=%2&amp;revision=%3")
                        .arg(uri)
                        .arg(img_ipath.get_branch())
                        .arg(img_ipath.get_revision())
                        ;
        }
        else
        {
            token.f_replacement = uri;
        }
SNAP_LOG_TRACE("token.f_replacement=[")(token.f_replacement)("]");
    }
}


void images::on_token_help(filter::filter::token_help_t & help)
{
    help.add_token("images::inline_uri",
        "Substitute inline URI. This is useful for appending versioning information to the asset.");
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
