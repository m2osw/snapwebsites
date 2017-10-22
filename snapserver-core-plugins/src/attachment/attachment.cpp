// Snap Websites Server -- handle the access to attachments
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

#include "attachment.h"

#include "../content/content.h"
#include "../messages/messages.h"
#include "../permissions/permissions.h"
#include "../users/users.h"

#include <snapwebsites/dbutils.h>
#include <snapwebsites/http_strings.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

#include <iostream>

#include <QFile>
#include <QLocale>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(attachment, 1, 0)

// using the SNAP_NAME_CONTENT_... for this one.
/* \brief Get a fixed attachment name.
 *
 * The attachment plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_ATTACHMENT_ACTION_EXTRACTFILE:
        return "extractfile";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_ATTACHMENT_...");

    }
    NOTREACHED();
}









/** \brief Initialize the attachment plugin.
 *
 * This function is used to initialize the attachment plugin object.
 */
attachment::attachment()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the attachment plugin.
 *
 * Ensure the attachment object is clean before it is gone.
 */
attachment::~attachment()
{
}


/** \brief Get a pointer to the attachment plugin.
 *
 * This function returns an instance pointer to the attachment plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the attachment plugin.
 */
attachment * attachment::instance()
{
    return g_plugin_attachment_factory.instance();
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString attachment::icon() const
{
    return "/images/attachment/attachment-logo-64x64.png";
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
QString attachment::description() const
{
    return "Handle the output of attachments, which includes sending the"
        " proper compressed file and in some cases transforming the file"
        " on the fly before sending it to the user (i.e. resizing an image"
        " to \"better\" sizes for the page being presented.)";
}


/** \brief Return our dependencies
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString attachment::dependencies() const
{
    return "|content|messages|path|permissions|";
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
int64_t attachment::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 12, 20, 22, 50, 12, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void attachment::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the attachment.
 *
 * This function terminates the initialization of the attachment plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void attachment::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(attachment, "server", server, register_backend_action, _1);
    SNAP_LISTEN(attachment, "path", path::path, can_handle_dynamic_path, _1, _2);
    SNAP_LISTEN(attachment, "content", content::content, page_cloned, _1);
    SNAP_LISTEN(attachment, "content", content::content, copy_branch_cells, _1, _2, _3);
    SNAP_LISTEN(attachment, "permissions", permissions::permissions, permit_redirect_to_login_on_not_allowed, _1, _2);
}


/** \brief Allow a second opinion on who can handle this path.
 *
 * This function is used here to allow the attachment plugin to handle
 * attachments that have a different filename (i.e. have some extensions
 * that could be removed for us to find the wanted file.)
 *
 * Although we could use an "easier" mechanism such as query string
 * entries to tweak the files, it is much less natural than supporting
 * "random" filenames for extensions.
 *
 * The attachment plugin support is limited to ".gz". However, other
 * core plugins support other magical extensions (i.e. image and
 * javascript.)
 *
 * \param[in] ipath  The path being checked.
 * \param[in] plugin_info  The current information about this path plugin.
 */
void attachment::on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info)
{
    // is that path already going to be handled by someone else?
    // (avoid wasting time if that is the case)
    if(plugin_info.get_plugin()
    || plugin_info.get_plugin_if_renamed())
    {
        return;
    }

    // TODO: will other plugins check for their own extension schemes?
    //       (I would imagine that this plugin will support more than
    //       just the .min.css/js and .gz extensions...)
    //
    QString const cpath(ipath.get_cpath());

    if(cpath.endsWith(".min.css") || cpath.endsWith(".min.css.gz"))
    {
        if(check_for_minified_js_or_css(ipath, plugin_info, ".css"))
        {
            return;
        }
    }

    if(cpath.endsWith(".min.js") || cpath.endsWith(".min.js.gz"))
    {
        if(check_for_minified_js_or_css(ipath, plugin_info, ".js"))
        {
            return;
        }
    }

    if(cpath.endsWith(".gz") && !cpath.endsWith("/.gz"))
    {
        if(check_for_uncompressed_file(ipath, plugin_info))
        {
            return;
        }
    }
}


/** \brief Check whether we have a normal or uncompressed version of the file.
 *
 * This function checks for two things:
 *
 * 1. If we have a version of the file that's compressed then we want to
 *    rename the path without the .gz because the path needs to check the
 *    name without the .gz
 * 2. Whether it is compressed or not, if the client sent us a If-None-Math
 *    header with the correct ETag, then we want to return a 304 instead
 *
 * \param[in,out] ipath  The path being checked.
 * \param[in,out] plugin_info  The dynamic plugin information.
 *
 * \return true if this was a match.
 */
bool attachment::check_for_uncompressed_file(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info)
{
    QString cpath(ipath.get_cpath());
    cpath = cpath.left(cpath.length() - 3);
    content::path_info_t attachment_ipath;
    attachment_ipath.set_path(cpath);

    // file exists?
    libdbproxy::table::pointer_t revision_table(content::content::instance()->get_revision_table());
    if(!revision_table->exists(attachment_ipath.get_revision_key())
    || !revision_table->getRow(attachment_ipath.get_revision_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT)))
    {
        return false;
    }

    // load the MD5 key for that attachment
    libdbproxy::value const attachment_key(revision_table->getRow(attachment_ipath.get_revision_key())->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT))->getValue());
    if(attachment_key.size() != 16)
    {
        return false;
    }

    libdbproxy::table::pointer_t files_table(content::content::instance()->get_files_table());
    if(!files_table->exists(attachment_key.binaryValue()))
    {
        // TODO: also offer a dynamic version which compress the
        //       file on the fly (but we wouldd have to save it and
        //       that could cause problems with the backend if we
        //       were to not use the maximum compression?)
        return false;
    }

    bool const field_name(files_table->getRow(attachment_key.binaryValue())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_DATA_GZIP_COMPRESSED)));
    if(field_name)
    {
        // use the MD5 sum
        QString const md5sum(dbutils::key_to_string(attachment_key.binaryValue()));
        f_snap->set_header("ETag", md5sum);

        // user may mark a page with the "no-cache" type in which case
        // we want to skip on setting up the cache
        //
        // this makes me think that we need a cache control defined in
        // the content plugin
        //
        {
            // this is considered a valid entry so we can setup the
            // cache to last "forever"; a script with its version
            // NEVER changes; you always have to bump the version
            // to get the latest changes
            //
            cache_control_settings & server_cache_control(f_snap->server_cache_control());
            server_cache_control.set_max_age(3600); // cache for 1h
            server_cache_control.set_must_revalidate(false); // default is true

            // check whether this file is public (can be saved in proxy
            // caches, i.e. is viewable by a visitor) or private (only
            // cache on client's machine)
            //
            content::permission_flag result;
            path::path::instance()->access_allowed("", attachment_ipath, "view", permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR), result);
            server_cache_control.set_public(result.allowed());

            // let the system check the various cache definitions
            // found in the page being worked on
            //
            content::content * content_plugin(content::content::instance());
            content_plugin->set_cache_control_page(attachment_ipath);

            // cache control for the page
            //
            cache_control_settings & page_cache_control(f_snap->page_cache_control());
            page_cache_control.set_max_age(3600);  // cache for 1h
            page_cache_control.set_must_revalidate(false); // default is true

            // we set the ETag header and cache information so we can
            // call the not_modified() function now;
            //
            // if the ETag did not change, the function does not return;
            // instead it sends a 304 as the response to the client
            //
            f_snap->not_modified();
            // ... function may never return ...
        }

        // tell the path plugin that we know how to handle this one
        plugin_info.set_plugin_if_renamed(this, attachment_ipath.get_cpath());
        ipath.set_parameter("attachment_field", content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_DATA_GZIP_COMPRESSED));
        return true;
    }

    return false;
}


/** \brief Check whether we have a minified version of the file.
 *
 * This entry allows us to return a minified version of a file
 * if it exists, or even a minified compressed version.
 *
 * \param[in] ipath  The path being checked.
 * \param[in] plugin_info  The dynamic plugin information.
 * \param[in] extension  The expected extension without the .gz, it MUST start
 *                       with a period.
 *
 * \return true if this was a match.
 */
bool attachment::check_for_minified_js_or_css(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info, QString const & extension)
{
    // break up the full filename as a path and a versioned_filename
    //
    snap_string_list segments(ipath.get_segments());
    if(segments.isEmpty())
    {
        // that should never occur
        return false;
    }
    QString versioned_filename(segments.back());
    segments.pop_back();
    QString const path(segments.join("/") + "/");

    // depending on whether we have the .gz, define which fields we want to
    // check for the data of this file
    bool must_be_compressed(false);
    content::name_t name(content::name_t::SNAP_NAME_CONTENT_FILES_DATA_MINIFIED);
    content::name_t fallback_name(content::name_t::SNAP_NAME_CONTENT_FILES_DATA);
    if(versioned_filename.endsWith(".min" + extension))
    {
        versioned_filename = versioned_filename.left(versioned_filename.length() - extension.length() - 4);

        // we can use the encoded version only if the client supports gzip
        // (note that we are not going to be using the best possible
        // compression in this case...)
        snap_child::compression_vector_t compressions(f_snap->get_compression());
        if(compressions.contains(snap_child::compression_t::COMPRESSION_GZIP))
        {
            name = content::name_t::SNAP_NAME_CONTENT_FILES_DATA_MINIFIED_GZIP_COMPRESSED;
            fallback_name = content::name_t::SNAP_NAME_CONTENT_FILES_DATA_GZIP_COMPRESSED;
        }
        //http_strings::WeightedHttpString encodings(f_snap->snapenv("HTTP_ACCEPT_ENCODING"));
        //float const gzip_level(std::max(std::max(encodings.get_level("gzip"), encodings.get_level("x-gzip")), encodings.get_level("*")));
        //if(gzip_level > 0.0)
        //{
        //    name = content::name_t::SNAP_NAME_CONTENT_FILES_DATA_MINIFIED_GZIP_COMPRESSED;
        //    fallback_name = content::name_t::SNAP_NAME_CONTENT_FILES_DATA_GZIP_COMPRESSED;
        //}
    }
    else //if(last_segment.endsWith(".min" + extension + ".gz"))
    {
        versioned_filename = versioned_filename.left(versioned_filename.length() - extension.length() - 7);

        name = content::name_t::SNAP_NAME_CONTENT_FILES_DATA_MINIFIED_GZIP_COMPRESSED;
        fallback_name = content::name_t::SNAP_NAME_CONTENT_FILES_DATA_GZIP_COMPRESSED;

        // the user asked for the .gz version and if not available we have
        // to fail...
        must_be_compressed = true;
    }

    // We may have 2 or 3 segments in the basename:
    //      <filename>_<version>
    //      <filename>_<version>_<browser>
    //
    // We want to at least find the version for now
    //
    // TODO: handle the browser...
    //
    QString filename;
    QString version;
    QString browser;
    snap_string_list version_segments(versioned_filename.split('_'));
    if(version_segments.size() == 1)
    {
        // the version is missing... (keep for now because we still have
        // old entries that do not include the version of the file...)
        //
        // TODO: once we reset the database another time, we can come back
        //       to this one and transform it into an error (i.e. missing
        //       version in JS/CSS filename)
        //
        filename = version_segments[0];
    }
    else if(version_segments.size() == 2)
    {
        // the version is specified, break it up accordingly
        filename = version_segments[0];
        version = version_segments[1];
    }
    else if(version_segments.size() == 3)
    {
        // the version is specified, break it up accordingly
        filename = version_segments[0];
        version = version_segments[1];
        browser = version_segments[2];
    }
    else
    {
        // any other combo is considered invalid
        throw attachment_exception_invalid_filename(QString("A JavaScript or CSS filename must include 2 to 3 segments: <name>_<version>[_<browser>], filename \"%1\"is invalid").arg(ipath.get_cpath()));
    }

    // check that the file exists
    //
    // filename now includes:
    //
    //      . the path
    //      . the filename with:
    //          . NO special extensions, and
    //          . NO version, and
    //          . NO browser
    //      . the extension
    //
    content::path_info_t attachment_ipath;
    attachment_ipath.set_path(path + filename + extension);

    // verify the revision, if different, then we want to
    // use the one that the user specified and not the most
    // recent one
    //
    if(!version.isEmpty()
    && attachment_ipath.get_extended_revision() != version)
    {
        // 'filename' is used only in case of errors
        //
        attachment_ipath.force_extended_revision(version, filename);
    }

    // make sure the path is valid (i.e. it could be a 404)
    //
    if(!attachment_ipath.has_revision())
    {
        return false;
    }

    content::content * content_plugin(content::content::instance());

    libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());
    QString const revision_key(attachment_ipath.get_revision_key());
    if(!revision_table->exists(revision_key)
    || !revision_table->getRow(revision_key)->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT)))
    {
        return false;
    }

    // retrieve the md5 which has to be exactly 16 bytes
    libdbproxy::value attachment_key(revision_table->getRow(revision_key)->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT))->getValue());
    if(attachment_key.size() != 16)
    {
        return false;
    }

    // check that this file exists in the "files" table
    libdbproxy::table::pointer_t files_table(content_plugin->get_files_table());
    if(!files_table->exists(attachment_key.binaryValue()))
    {
        return false;
    }

    for(;;)
    {
        // check for the minified version
        bool const field_name(files_table->getRow(attachment_key.binaryValue())->exists(content::get_name(name)));
        bool field_fallback_name(false);
        if(!field_name)
        {
            field_fallback_name = files_table->getRow(attachment_key.binaryValue())->exists(content::get_name(fallback_name));
        }
        if(field_name || field_fallback_name)
        {
            // this compression only applies if no errors occur later
            if(name == content::name_t::SNAP_NAME_CONTENT_FILES_DATA_MINIFIED_GZIP_COMPRESSED
            && !must_be_compressed)
            {
                f_snap->set_header("Content-Encoding", "gzip", snap_child::HEADER_MODE_NO_ERROR);
            }

            // use the version since it is a unique number
            // NO NO NO, we need to use the Last-Modified only (or max-age)
            // but ETag would mean we'd get to send a 304 each time which
            // is not necessary since the version is in the URI!
            //f_snap->set_header("ETag", version);

            // get the last modification time of this very version
            libdbproxy::value revision_modification(revision_table->getRow(revision_key)->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->getValue());
            QDateTime expires(QDateTime().toUTC());
            expires.setTime_t(revision_modification.safeInt64Value() / 1000000);
            QLocale us_locale(QLocale::English, QLocale::UnitedStates);
            f_snap->set_header("Last-Modified", us_locale.toString(expires, "ddd, dd MMM yyyy hh:mm:ss' GMT'"), snap_child::HEADER_MODE_EVERYWHERE);

            // user may mark a page with the "no-cache" type in which case
            // we want to skip on setting up the cache
            //
            // this makes me think that we need a cache control defined in
            // the content plugin
            //
            {
                // this is considered a valid entry so we can setup the
                // cache to last "forever"; a script with its version
                // NEVER changes; you always have to bump the version
                // to get the latest changes
                //
                cache_control_settings & server_cache_control(f_snap->server_cache_control());
                server_cache_control.set_max_age(cache_control_settings::AGE_MAXIMUM);
                server_cache_control.set_must_revalidate(false); // default is true

                // check whether this file is public (can be saved in proxy
                // caches, i.e. is viewable by a visitor) or private (only
                // cache on client's machine)
                //
                content::permission_flag result;
                path::path::instance()->access_allowed("", attachment_ipath, "view", permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR), result);
                server_cache_control.set_public(result.allowed());

                // let the system check the various cache definitions
                // found in the page being worked on
                //
                content_plugin->set_cache_control_page(attachment_ipath);

                // cache control for the page
                //
                cache_control_settings & page_cache_control(f_snap->page_cache_control());
                page_cache_control.set_max_age(cache_control_settings::AGE_MAXIMUM);
                page_cache_control.set_must_revalidate(false); // default is true

                // we set the ETag header and cache information so we can
                // call the not_modified() function now;
                //
                // if the ETag did not change, the function does not return;
                // instead it sends a 304 as the response to the client
                //
                f_snap->not_modified();
                // ... function may never return ...
            }

            // tell the path plugin that we know how to handle this one
            plugin_info.set_plugin_if_renamed(this, attachment_ipath.get_cpath());
            ipath.set_parameter("attachment_field", content::get_name(field_name ? name : fallback_name));
            ipath.set_parameter("attachment_version", version);
            return true;
        }

        // TODO? offer an on the fly version minimized and compressed?

        if(name == content::name_t::SNAP_NAME_CONTENT_FILES_DATA_MINIFIED
        || must_be_compressed)
        {
            // compressed and uncompressed checked, nothing more we can do
            break;
        }

        name = content::name_t::SNAP_NAME_CONTENT_FILES_DATA_MINIFIED;
        fallback_name = content::name_t::SNAP_NAME_CONTENT_FILES_DATA;
    }

    return false;
}


/** \brief Execute a page: generate the complete attachment of that page.
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
bool attachment::on_path_execute(content::path_info_t & ipath)
{
#ifdef DEBUG
    SNAP_LOG_TRACE("attachment::on_path_execute(")(ipath.get_key())(")");
#endif
    // TODO: we probably do not want to check for attachments to send if the
    //       action is not "view"...

    // make sure that the session time limit does not get updated on
    // an attachment
    //
    users::users::instance()->transparent_hit();

    // attachments should never be saved with a compression extension
    //
    // HOWEVER, we would like to offer a way for the system to allow extensions
    // but if we are here the system already found the page and thus found
    // it with[out] the extension as defined in the database...
    //
    QString field_name;
    content::path_info_t attachment_ipath;
    QString const renamed(ipath.get_parameter("renamed_path"));
    char const * files_data(content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_DATA));
    if(renamed.isEmpty())
    {
        attachment_ipath = ipath;
        field_name = files_data;
SNAP_LOG_TRACE("renamed is empty, setting attachment_ipath=")(attachment_ipath.get_key())(", field_name=")(field_name);
    }
    else
    {
SNAP_LOG_TRACE("renamed=")(renamed)(", field_name=")(field_name);
        // TODO: that data may NOT be available yet in which case a plugin
        //       needs to offer it... how do we do that?!
        attachment_ipath.set_path(renamed);
        field_name = ipath.get_parameter("attachment_field");

        // the version may have been tweaked too?
        QString const version(ipath.get_parameter("attachment_version"));
        if(!version.isEmpty())
        {
            attachment_ipath.force_extended_revision(version, renamed);
        }

        // verify that this field is acceptable as a field name to access
        // the data (ipath parameters can be somewhat tainted)
        QString const starts_with(QString("%1::").arg(files_data));
        if(field_name != files_data
        && !field_name.startsWith(starts_with))
        {
            // field name not acceptable
            f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND, "Unacceptable Attachment Field Name",
                    QString("Field name \"%1\" is not acceptable to access the file data.").arg(field_name),
                    QString("Field name is not \"%1\" and does not start with \"%2\".")
                            .arg(field_name)
                            .arg(starts_with));
            NOTREACHED();
        }
    }

    // get the file MD5 which must be exactly 16 bytes
SNAP_LOG_TRACE("**** getting revision key for ipath=")(ipath.get_key())(", cpath=")(ipath.get_cpath());
    libdbproxy::table::pointer_t revision_table(content::content::instance()->get_revision_table());
    libdbproxy::value const attachment_key(
            revision_table
                ->getRow(attachment_ipath.get_revision_key())
                    ->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT))
                        ->getValue()
            );
    if(attachment_key.size() != 16)
    {
        // somehow the file key is not available
        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND, "Attachment Not Found",
                QString("Attachment \"%1\" was not found.").arg(ipath.get_key()),
                QString("Could not find field \"%2\" of file \"%3\" (maybe renamed \"%4\").")
                        .arg(ipath.get_key())
                        .arg(field_name)
                        .arg(QString::fromLatin1(attachment_key.binaryValue().toHex()))
                        .arg(renamed));
        NOTREACHED();
    }

    // make sure that the data field exists
    libdbproxy::table::pointer_t files_table(content::content::instance()->get_files_table());
    if(!files_table->exists(attachment_key.binaryValue())
    || !files_table->getRow(attachment_key.binaryValue())->exists(field_name))
    {
        // somehow the file data is not available
        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND, "Attachment Not Found",
                QString("Attachment \"%1\" was not found.").arg(ipath.get_key()),
                QString("Could not find field \"%1\" of file \"%2\" (maybe renamed \"%3\").")
                        .arg(field_name)
                        .arg(QString::fromLatin1(attachment_key.binaryValue().toHex()))
                        .arg(renamed));
        NOTREACHED();
    }

    libdbproxy::row::pointer_t file_row(files_table->getRow(attachment_key.binaryValue()));

    // get the attachment MIME type and tweak it if it is a known text format
    libdbproxy::value attachment_mime_type(file_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_MIME_TYPE))->getValue());
    QString content_type(attachment_mime_type.stringValue());
    if(content_type == "text/javascript"
    || content_type == "text/css")
    {
        // TBD -- we probably should check what is defined inside those
        //        files before assuming it is using UTF-8.
        content_type += "; charset=utf-8";

        // Chrome and IE check this header for CSS and JS data
        f_snap->set_header("X-Content-Type-Options", "nosniff");
    }
    else
    {
        // All other files are marked with an ETag so we can avoid resending
        // them but clients are expected to query for them on each load
        // (i.e. a must-revalidate type of cache)
        //
        QString const md5sum(dbutils::key_to_string(attachment_key.binaryValue()));
        f_snap->set_header("ETag", md5sum);

        // user may mark a page with the "no-cache" type in which case
        // we want to skip on setting up the cache
        //
        // this makes me think that we need a cache control defined in
        // the content plugin
        //
        {
            // this is considered a valid entry so we can setup the
            // cache to last "forever"; a script with its version
            // NEVER changes; you always have to bump the version
            // to get the latest changes
            //
            cache_control_settings & server_cache_control(f_snap->server_cache_control());
            server_cache_control.set_max_age(3600); // cache for 1h before rechecking
            server_cache_control.set_must_revalidate(false); // default is true

            // check whether this file is public (can be saved in proxy
            // caches, i.e. is viewable by a visitor) or private (only
            // cache on client's machine)
            //
            content::permission_flag result;
            path::path::instance()->access_allowed("", attachment_ipath, "view", permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR), result);
            server_cache_control.set_public(result.allowed());

            // let the system check the various cache definitions
            // found in the page being worked on
            //
            content::content * content_plugin(content::content::instance());
            content_plugin->set_cache_control_page(attachment_ipath);

            // cache control for the page
            //
            cache_control_settings & page_cache_control(f_snap->page_cache_control());
            page_cache_control.set_max_age(3600);  // cache for 1h before rechecking
            page_cache_control.set_must_revalidate(false); // default is true

            // we set the ETag header and cache information so we can
            // call the not_modified() function now;
            //
            // if the ETag did not change, the function does not return;
            // instead it sends a 304 as the response to the client
            //
            f_snap->not_modified();
            // ... function may never return ...
        }

        if(content_type == "text/xml")
        {
            content_type += "; charset=utf-8";
        }
    }
    f_snap->set_header("Content-Type", content_type);

    // If the user is loading the file as an attachment, make sure to
    // include the disposition and transfer encoding info
    //
    snap_uri const & main_uri(f_snap->get_uri());
    if(main_uri.has_query_option("download")
    && main_uri.query_option("download") == "attachment")
    {
        QString const cpath(ipath.get_cpath());
        int const pos(cpath.lastIndexOf('/'));
        QString const basename(cpath.mid(pos + 1));
        f_snap->set_header("Content-Disposition", "attachment; filename=" + basename);
        f_snap->set_header("Content-Transfer-Encoding", "binary");
    }

    // the actual file data now
    libdbproxy::value data(file_row->getCell(field_name)->getValue());
    f_snap->output(data.binaryValue());

    return true;
}


/** \brief Someone just cloned a page.
 *
 * Check whether the clone represents a file. If so, we want to add a
 * reference from that file to this new page.
 *
 * This must happen in pretty much all cases.
 *
 * \param[in] tree  The tree of pages that got cloned.
 */
void attachment::on_page_cloned(content::content::cloned_tree_t const& tree)
{
    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());
    libdbproxy::table::pointer_t files_table(content_plugin->get_files_table());

    char const * attachment_reference(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT_REFERENCE));
    QString const content_attachment_reference(QString("%1::").arg(attachment_reference));
    size_t const max_pages(tree.f_pages.size());
    for(size_t idx(0); idx < max_pages; ++idx)
    {
        content::content::cloned_page_t const & page(tree.f_pages[idx]);
        size_t const max_branches(page.f_branches.size());
        for(size_t branch(0); branch < max_branches; ++branch)
        {
            snap_version::version_number_t const b(page.f_branches[branch].f_branch);
            content::path_info_t page_ipath(page.f_destination);
            page_ipath.force_branch(b);

            libdbproxy::row::pointer_t branch_row(branch_table->getRow(page_ipath.get_branch_key()));
            branch_row->clearCache();

            auto column_predicate(std::make_shared<libdbproxy::cell_range_predicate>());
            column_predicate->setStartCellKey(content_attachment_reference);
            column_predicate->setEndCellKey(QString("%1;").arg(attachment_reference));
            column_predicate->setCount(100);
            column_predicate->setIndex(); // behave like an index
            for(;;)
            {
                branch_row->readCells(column_predicate);
                libdbproxy::cells const branch_cells(branch_row->getCells());
                if(branch_cells.isEmpty())
                {
                    // done
                    break;
                }
                // handle one batch
                for(libdbproxy::cells::const_iterator nc(branch_cells.begin());
                                                                 nc != branch_cells.end();
                                                                 ++nc)
                {
                    libdbproxy::cell::pointer_t branch_cell(*nc);
                    QByteArray cell_key(branch_cell->columnKey());

                    // this key starts with SNAP_NAME_CONTENT_ATTACHMENT_REFERENCE + "::"
                    // and then represents an md5
                    QByteArray md5(cell_key.mid( content_attachment_reference.length() ));

                    // with that md5 we can access the files table
                    signed char const one(1);
                    files_table->getRow(md5)->getCell(QString("%1::%2").arg(content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_REFERENCE)).arg(page_ipath.get_key()))->setValue(one);
                }
            }
        }
    }
}


void attachment::on_copy_branch_cells(libdbproxy::cells& source_cells, libdbproxy::row::pointer_t destination_row, snap_version::version_number_t const destination_branch)
{
    NOTUSED(destination_branch);

    libdbproxy::table::pointer_t files_table(content::content::instance()->get_files_table());

    std::string content_attachment_reference(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT_REFERENCE));
    content_attachment_reference += "::";

    libdbproxy::cells left_cells;

    // handle one batch
    for(libdbproxy::cells::const_iterator nc(source_cells.begin());
            nc != source_cells.end();
            ++nc)
    {
        libdbproxy::cell::pointer_t source_cell(*nc);
        QByteArray cell_key(source_cell->columnKey());

        if(cell_key.startsWith(content_attachment_reference.c_str()))
        {
            // copy our fields as is
            destination_row->getCell(cell_key)->setValue(source_cell->getValue());

            // make sure the (new) list is checked so we actually get a list
            content::path_info_t ipath;
            ipath.set_path(destination_row->rowName());

            // this key starts with SNAP_NAME_CONTENT_ATTACHMENT_REFERENCE + "::"
            // and then represents an md5
            QByteArray md5(cell_key.mid( content_attachment_reference.length() ));

            // with that md5 we can access the files table
            signed char const one(1);
            files_table->getRow(md5)->getCell(QString("%1::%2").arg(content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_REFERENCE)).arg(ipath.get_key()))->setValue(one);
        }
        else
        {
            // keep the other branch fields as is, other plugins can handle
            // them as required by implementing this signal
            //
            // note that the map is a map a shared pointers so it is fast
            // to make a copy like this
            left_cells[cell_key] = source_cell;
        }
    }

    // overwrite the source with the cells we allow to copy "further"
    source_cells = left_cells;
}


void attachment::on_handle_error_by_mime_type(snap_child::http_code_t err_code, QString const & err_name, QString const & err_description, QString const & path)
{
    struct default_error_t
    {
        default_error_t(snap_child * snap, snap_child::http_code_t err_code, QString const & err_name, QString const & err_description, QString const & path)
            : f_snap(snap)
            , f_err_code(err_code)
            , f_err_name(err_name)
            , f_err_description(err_description)
            , f_path(path)
        {
        }

        void emit_error(QString const & more_details)
        {
            // log the extract details, we do not need to re-log the error
            // info which the path plugin has already done
            if(!more_details.isEmpty())
            {
                SNAP_LOG_FATAL("attachment::on_handle_error_by_mime_type(): ")(more_details);
            }

            // force header to text/html anyway
            f_snap->set_header(get_name(::snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER), "text/html; charset=utf8", snap_child::HEADER_MODE_EVERYWHERE);

            // generate the body
            QString const html(f_snap->error_body(f_err_code, f_err_name, f_err_description));

            f_snap->output_result(snap_child::HEADER_MODE_ERROR, html.toUtf8());
        }

    private:
        snap_child *            f_snap = nullptr;
        snap_child::http_code_t f_err_code;
        QString const &         f_err_name;
        QString const &         f_err_description;
        QString const &         f_path;
    } default_err(f_snap, err_code, err_name, err_description, path);

    // in this case we want to return a file with the same format as the
    // one pointed to by ipath, only we send a default "not allowed" version
    // of it (i.e. for an image, send a GIF that clearly shows "image not
    // allowed" or something that clearly tells us that a permission prevents
    // us from seening the file
    //
    // this replaces the default HTML usually sent with such errors because
    // those are really not talkative
    //
    // see the die() function in the snap_child class for other information
    // about these things
    QString field_name;
    content::path_info_t attachment_ipath;
    // TODO: the renamed_path / attachment_field are not available here because
    //       the server does not know about the path_content_t type...
    //QString const renamed(ipath.get_parameter("renamed_path"));
    //if(renamed.isEmpty())
    {
        attachment_ipath.set_path(path);
        field_name = content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_DATA);
    }
    //else
    //{
    //    // TODO: that data may NOT be available yet in which case a plugin
    //    //       needs to offer it... how do we do that?!
    //    attachment_ipath.set_path(renamed);
    //    field_name = ipath.get_parameter("attachment_field");
    //}

    libdbproxy::table::pointer_t revision_table(content::content::instance()->get_revision_table());
    libdbproxy::value attachment_key(
            revision_table
                ->getRow(attachment_ipath.get_revision_key())
                    ->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT))
                        ->getValue()
            );
    if(attachment_key.nullValue())
    {
        // somehow the file key is not available
        default_err.emit_error(
                    QString("Could not find field \"%1\" of file \"%2\" in revision table.")
                            .arg(field_name)
                            .arg(QString::fromLatin1(attachment_key.binaryValue().toHex())));
        return;
    }

    libdbproxy::table::pointer_t files_table(content::content::instance()->get_files_table());
    if(!files_table->exists(attachment_key.binaryValue())
    || !files_table->getRow(attachment_key.binaryValue())->exists(field_name))
    {
        // somehow the file data is not available
        default_err.emit_error(
                QString("Could not find field \"%1\" of file \"%2\" in files table.")
                        .arg(content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_DATA))
                        .arg(QString::fromLatin1(attachment_key.binaryValue().toHex())));
        return;
    }

    libdbproxy::row::pointer_t file_row(files_table->getRow(attachment_key.binaryValue()));

    // TODO: If the user is loading the file as an attachment,
    //       we need those headers (TBD--would we reaaly want to do that
    //       here? probably, although that means we offer the user a
    //       download with nothingness inside.)

    //int pos(cpath.lastIndexOf('/'));
    //QString basename(cpath.mid(pos + 1));
    //f_snap->set_header("Content-Disposition", "attachment; filename=" + basename);

    //f_snap->set_header("Content-Transfer-Encoding", "binary");

    // get the attachment MIME type and tweak it if it is a known text format
    libdbproxy::value attachment_mime_type(file_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_FILES_MIME_TYPE))->getValue());
    QString const content_type(attachment_mime_type.stringValue());
    if(content_type == "text/html")
    {
        default_err.emit_error("The attachment being downloaded is text/html, displaying default error.");
        return;
    }

    // if known text format, use UTF-8 as the charset
    QString content_type_header(content_type);
    if(content_type == "text/javascript"
    || content_type == "text/css"
    || content_type == "text/xml")
    {
        // TBD -- we probably should check what is defined inside those
        //        files before assuming it is using UTF-8.
        content_type_header += "; charset=utf-8";
    }
    f_snap->set_header("Content-Type", content_type_header, snap_child::HEADER_MODE_EVERYWHERE);

    // dynamic JavaScript error--we may also want to put a console.log()
    if(content_type == "text/javascript")
    {
        QString const js(QString("/* an error occurred while reading this .js file:\n"
                        " * %1 %2\n"
                        " * %3\n"
                        " */\n")
                .arg(static_cast<int>(err_code))
                .arg(QString(err_name).replace("*/", "**"))
                .arg(QString(err_description).replace("*/", "**")));
        f_snap->output_result(snap_child::HEADER_MODE_ERROR, js.toUtf8());
        return;
    }

    // dynamic CSS error--I'm not too sure we could show something on the
    //                    screen as a result
    if(content_type == "text/css")
    {
        QString const css(QString("/* An error occurred while reading this .css file:\n"
                        " * %1 %2\n"
                        " * %3\n"
                        " */\n")
                .arg(static_cast<int>(err_code))
                .arg(QString(err_name).replace("*/", "**"))
                .arg(QString(err_description).replace("*/", "**")));
        f_snap->output_result(snap_child::HEADER_MODE_ERROR, css.toUtf8());
        return;
    }

    // dynamic XML error--we create a "noxml" XML file
    if(content_type == "text/xml")
    {
        QString const css(QString("<?xml version=\"1.0\"?><!-- an error occurred while reading this .css file:\n"
                        "%1 %2\n"
                        "%3\n"
                        "%4\n"
                        "--><noxml></noxml>\n")
                .arg(static_cast<int>(err_code))
                .arg(QString(err_name).replace("--", "=="))
                .arg(QString(err_description).replace("--", "==")));
        f_snap->output_result(snap_child::HEADER_MODE_ERROR, css.toUtf8());
        return;
    }

    // obviously, since the file is not authorized we cannot send the
    // actual file data which we could access with the following line:
    //libdbproxy::value data(file_row->getCell(field_name)->getValue());

    // the actual file data now; this is defined using the MIME type
    // (and the error code?)
    snap_string_list const mime_type_parts(content_type.split('/'));
    if(mime_type_parts.size() != 2)
    {
        // no recovery on that one for now
        default_err.emit_error(QString("Could not break MIME type \"%1\" in two strings.").arg(content_type));
        return;
    }
    QString const major_mime_type(mime_type_parts[0]);
    QString const minor_mime_type(mime_type_parts[1]);

    // now check the following in that order:
    //
    //    1. Long name in the database
    //    2. Long name in resources
    //    3. Short name in the database
    //    5. Short name in resources
    //
    libdbproxy::value data;
    QString const long_name(QString("%1::%2::%3")
                .arg(major_mime_type)
                .arg(minor_mime_type)
                .arg(static_cast<int>(err_code)));
    if(files_table->getRow(content::get_name(content::name_t::SNAP_NAME_CONTENT_ERROR_FILES))->exists(long_name))
    {
        // long name exists in the database, use it
        data = files_table->getRow(content::get_name(content::name_t::SNAP_NAME_CONTENT_ERROR_FILES))->getCell(long_name)->getValue();
    }
    else
    {
        QString const short_name(QString("%1::%2")
                    .arg(major_mime_type)
                    .arg(minor_mime_type));

        // try with the long name in the resources
        QString const long_filename(QString(":/plugins/%1/mime-types/%2.xml").arg(get_plugin_name()).arg(long_name));
        QFile long_rsc_content(long_filename);
        if(long_rsc_content.open(QFile::ReadOnly))
        {
            data.setBinaryValue(long_rsc_content.readAll());
        }
        else if(files_table->getRow(content::get_name(content::name_t::SNAP_NAME_CONTENT_ERROR_FILES))->exists(short_name))
        {
            // short name exists in the database, use it
            data = files_table->getRow(content::get_name(content::name_t::SNAP_NAME_CONTENT_ERROR_FILES))->getCell(short_name)->getValue();
        }
        else
        {
            // try with the short name in the resources
            QString const short_filename(QString(":/plugins/%1/mime-types/%2.xml").arg(get_plugin_name()).arg(short_name));
            QFile short_rsc_content(short_filename);
            if(short_rsc_content.open(QFile::ReadOnly))
            {
                data.setBinaryValue(short_rsc_content.readAll());
            }
            else
            {
                // no data available, use the default HTML as fallback
                default_err.emit_error(QString("Could not find file for MIME type \"%1\" in database or resources.").arg(content_type));
                return;
            }
        }
    }

    f_snap->output(data.binaryValue());
}


void attachment::on_permit_redirect_to_login_on_not_allowed(content::path_info_t & ipath, bool & redirect_to_login)
{
    // this is a signal, we get called whatever the ipath (i.e. it is not
    // specific to a plugin derived from a certain class so not specific
    // to the attachment.)
    //
    libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
    if(content_table->exists(ipath.get_key())
    && content_table->getRow(ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_PRIMARY_OWNER)))
    {
        QString const owner(content_table->getRow(ipath.get_key())->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_PRIMARY_OWNER))->getValue().stringValue());
        if(owner == get_plugin_name())
        {
            // we own this page (attachment)
            redirect_to_login = false;
        }
    }
}


/** \brief Delete all the attachments found under the specified path.
 *
 * This function checks all the children of the specified \p ipath and
 * if any one of them is an attachment, it gets deleted. If the page
 * was already marked as deleted, then nothing happens.
 *
 * The function returns the number of files deleted. If the page
 * at \p ipath does not exist, then the function returns -1.
 *
 * \param[in,out] ipath  The path to the page of which attachments are
 *                       to be deleted.
 *
 * \return -1 on errors, 0 or more representing the number of attachments
 *         that got deleted.
 */
int attachment::delete_all_attachments(content::path_info_t & ipath)
{
    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());

    // page exists at all?
    if(!content_table->exists(ipath.get_key())
    || !content_table->getRow(ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
    {
        // error: page does not exist
        return -1;
    }

    int count(0);

    // check each child, but remember that a child may not be an
    // attachment, if may be a normal child (as in a book or
    // a blog with various ways of defining when this and that gets
    // posted.)
    //
    links::link_info info(content::get_name(content::name_t::SNAP_NAME_CONTENT_CHILDREN), false, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info child_info;
    while(link_ctxt->next_link(child_info))
    {
        content::path_info_t child_ipath;
        child_ipath.set_path(child_info.key());

        // verify that the child exists
        //
        if(!content_table->exists(child_ipath.get_key())
        || !content_table->getRow(child_ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
        {
            continue;
        }

        // ignore pages that are not currently normal or hidden
        // (i.e. hidden pages can be deleted)
        //
        content::path_info_t::status_t const status(child_ipath.get_status());
        if(status.get_state() != content::path_info_t::status_t::state_t::NORMAL
        && status.get_state() != content::path_info_t::status_t::state_t::HIDDEN)
        {
            continue;
        }

        // check whether we have an attachment key in the revision
        // (it has to be there if this page represents an attachment)
        //
        // TBD: Should we check for other clues?
        //      1. page owner could be anything, but if attachment, then
        //         we know for sure that it is an attachment
        //      2. the page is marked as being final (content::final == 1)
        //      3. branch includes one or more back references
        //
        libdbproxy::value const attachment_key(revision_table->getRow(child_ipath.get_revision_key())->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_ATTACHMENT))->getValue());
        if(attachment_key.nullValue())
        {
            // not considered an attachment, leave this one alone
            continue;
        }

        // okay, we consider this child to be an attachment, delete!
        content_plugin->trash_page(child_ipath);
        ++count;
    }

    return count;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
