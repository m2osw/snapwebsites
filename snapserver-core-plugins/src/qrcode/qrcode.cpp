// Copyright (c) 2014-2019  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// self
//
#include    "qrcode.h"


// other plugins
//
#include    "../attachment/attachment.h"
#include    "../permissions/permissions.h"
#include    "../shorturl/shorturl.h"


// snapdev
//
#include    <snapdev/not_reached.h>
#include    <snapdev/not_used.h>


// C++
//
#include    <iostream>


// QtEncode
//
#include    <qrencode.h>


// Magick++
//
#include    <Magick++.h>


// last include
//
#include    <snapdev/poison.h>



namespace snap
{
namespace qrcode
{


SERVERPLUGINS_START(qrcode, 1, 0)
    , ::serverplugins::description(
            "Generate the QR Code of the website public pages.")
    , ::serverplugins::icon("/images/qrcode/qrcode-logo-64x64.png")
    , ::serverplugins::settings_path("/admin/settings/qrcode")
    , ::serverplugins::dependency("attachment")
    , ::serverplugins::dependency("editor")
    , ::serverplugins::dependency("path")
    , ::serverplugins::dependency("permissions")
    , ::serverplugins::dependency("shorturl")
    , ::serverplugins::help_uri("https://snapwebsites.org/help")
    , ::serverplugins::categorization_tag("utilities")
SERVERPLUGINS_END(qrcode)



/** \brief Get a fixed qrcode name.
 *
 * The qrcode plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const *get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_QRCODE_DEFAULT_EDGE:
        return "qrcode::default_edge";

    case name_t::SNAP_NAME_QRCODE_DEFAULT_SCALE:
        return "qrcode::default_scale";

    case name_t::SNAP_NAME_QRCODE_PRIVATE_ENABLE:
        return "qrcode::private_enable";

    case name_t::SNAP_NAME_QRCODE_SHORTURL_ENABLE:
        return "qrcode::shorturl_enable";

    case name_t::SNAP_NAME_QRCODE_TRACK_USAGE_ENABLE:
        return "qrcode::track_usage_enable";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_QRCODE_...");

    }
    snapdev::NOT_REACHED();
}




/** \class qrcode
 * \brief The QR Code plugin generates images representing URLs.
 *
 * Note that the QR Code mechanism can be used to encode anything, really.
 * However, here it is used to encode URLs for pages. We may extend the
 * use of the plugin for other various features such as auto-login and
 * similar features.
 *
 * Once the plugin is installed, it is capable of generating codes for
 * any page, although by default it will only generate codes for public
 * pages.
 *
 * It is possible to select various settings such as the size of the QR
 * Code and whether it encodes the full URI or the Short URL.
 *
 * \todo
 * As an extension, we may want to offer a way to generate QR Codes in
 * the settings area. That should work for any URI, even foreign ones.
 * Also that generator could create very large versions for high DPI
 * quality for print purposes.
 *
 * \todo
 * We may also want to offer a way to also pass QR Code images in the
 * image filters to get codes that are watermarked, inverted, flipped,
 * etc.
 */

typedef std::unique_ptr<QRcode, snapdev::raii_pointer_deleter<QRcode, decltype(&::QRcode_free), &::QRcode_free>> raii_qrcode_t;


namespace
{


void data_deleter(unsigned char * data)
{
    delete [] data;
}


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
time_t qrcode::do_update(time_t last_updated, unsigned int phase)
{
    SERVERPLUGINS_PLUGIN_UPDATE_INIT();

    if(phase == 0)
    {
        SERVERPLUGINS_PLUGIN_UPDATE(2015, 12, 20, 20, 1, 30, content_update);
    }

    SERVERPLUGINS_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void qrcode::content_update(int64_t variables_timestamp)
{
    snapdev::NOT_USED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the qrcode.
 *
 * This function terminates the initialization of the qrcode plugin
 * by registering for different events.
 */
void qrcode::bootstrap()
{
    SERVERPLUGINS_LISTEN(qrcode, "path", path::path, can_handle_dynamic_path, boost::placeholders::_1, boost::placeholders::_2);
}


/** \brief Check whether the path can be handled by us.
 *
 * This function returns the plugin name if the path starts with
 * "images/qrcode/" as we dynamic handle paths under that path.
 *
 * \param[in,out] ipath  The path being checked.
 * \param[in,out] plugin_info  The info of the plugin to handle this path.
 */
void qrcode::on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info)
{
    if(ipath.get_cpath().startsWith("images/qrcode/"))
    {
        plugin_info.set_plugin(this);
    }
}


/** \brief Capture all the path under /images/qrcode.
 *
 * This function is the crux of this plugin. It generates a QR Code for
 * any path defined under the /images/qrcode/... path, including URIs
 * to images (although it prevents creating QR Code or QR Code image paths
 * because that would create an infinite number of paths.)
 *
 * At this time the system limits the number of elements in the path to
 * 20 under /images/qrcode.
 *
 * \param[in,out] ipath  The to the page to transfer.
 *
 * \return true if the QR code was created successfully.
 */
bool qrcode::on_path_execute(content::path_info_t & ipath)
{
    // we should be called only if the path starts with "images/qrcode/"
    // but double checking is always a good idea
    QString const cpath(ipath.get_cpath());
    if(!cpath.startsWith("images/qrcode/"))
    {
        // no idea on how to handle that one
        return false;
    }

    // retrieve the path we are after
    QString qrcode_path(cpath.mid(14));

    // TBD: I think this is useless since the path has to be have
    //      been canonicalized, right?
    //
    while(!qrcode_path.isEmpty() && qrcode_path[0] == '/')
    {
        qrcode_path.remove(0, 1);
    }

    // is that a path to a QR Code image? if so, totally ignore
    // and show the "not available" image instead
    if(!qrcode_path.startsWith("images/qrcode")
    && qrcode_path != "qrcode-not-available.png")
    {
        // make it a standard path to something
        content::path_info_t page_ipath;
        page_ipath.set_path(qrcode_path == "index" || qrcode_path == "index.html" ? "" : qrcode_path);

        content::content * content_plugin(content::content::instance());
        content::path_info_t settings_ipath;
        settings_ipath.set_path("admin/settings/qrcode");
        libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());
        libdbproxy::row::pointer_t settings_row(revision_table->getRow(settings_ipath.get_revision_key()));

        // verify that at least this user has permission to that page
        //
        permissions::permissions * permissions_plugin(permissions::permissions::instance());
        QString const & login_status(permissions_plugin->get_login_status());
        bool const accept_private_pages(settings_row->getCell(get_name(name_t::SNAP_NAME_QRCODE_PRIVATE_ENABLE))->getValue().safeSignedCharValue() != 0);
        QString const & user_path(accept_private_pages ? permissions_plugin->get_user_path() : "");
        content::permission_flag allowed;
        path::path::instance()->access_allowed(user_path, page_ipath, "view", login_status, allowed);
        if(allowed.allowed())
        {
            libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
            if(content_table->exists(page_ipath.get_key()))
            {
                // by default we expect the normal page URL to be used,
                // only it may be too long or the user may setup the
                // system to use the short URLs instead
                //
                std::string url_utf8(page_ipath.get_key().toUtf8().data());

                // we switch to the Short URL if:
                //   1) the user decided to use Short URLs (which happens to
                //      also be the default)
                //   2) the URL of the page to create a QR code for is
                //      longer than 2900 characters (because the maximum
                //      in binary is 2953 and we may add a "...?qrcode=true")
                //   3) there is a short URL to use
                //
                // Note that the QR code will fail if the page has a URL
                // that is too long (~2950) and no short URL is available.
                //
                bool const use_short_url(settings_row->getCell(get_name(name_t::SNAP_NAME_QRCODE_SHORTURL_ENABLE))->getValue().safeSignedCharValue(0, 1) != 0);
                if((use_short_url || url_utf8.length() > 2900)
                && content_table->getRow(page_ipath.get_key())->exists(shorturl::get_name(shorturl::name_t::SNAP_NAME_SHORTURL_URL)))
                {
                    // use the Short URL instead
                    //
                    // TODO: use a Short URL interface instead of directly
                    //       poking the data ourselves
                    //
                    QString const shorturl(content_table->getRow(page_ipath.get_key())->getCell(shorturl::get_name(shorturl::name_t::SNAP_NAME_SHORTURL_URL))->getValue().stringValue());
                    url_utf8 = shorturl.toUtf8().data();
                }

                // track QR code usage (i.e. add a query string "qrcode=true"
                // to the URL)
                //
                // TBD: we may want to give the user a way to choose
                //      the name of the variable.
                //
                // TBD: this very simply trick can easily be used to "falsify"
                //      the data; we could instead use a random session number
                //      only our sessions are viewed as "temporary" data
                //      whereas our QR code are permanent; also once someone
                //      has access to that code, we have the same problem
                //      again anyway...
                //
                bool const track_qrcode(settings_row->getCell(get_name(name_t::SNAP_NAME_QRCODE_TRACK_USAGE_ENABLE))->getValue().safeSignedCharValue() != 0);
                if(track_qrcode)
                {
                    url_utf8 += "?qrcode=true";
                }

                // let administrator choose version, level?
                //
                raii_qrcode_t code(QRcode_encodeString(url_utf8.c_str(), 0, QR_ECLEVEL_H, QR_MODE_8, 1));
                if(code)
                {
                    // convert resulting QR Code to a black and white blob
                    int const width(code->width);
                    bool scale_ok(false);
                    int scale(3);
                    if(f_snap->get_uri().has_query_option("scale"))
                    {
                        // query string overwrites the internal default and
                        // administrator setup
                        int const s(f_snap->get_uri().query_option("scale").toLongLong(&scale_ok, 10));
                        if(scale_ok && s > 0)
                        {
                            scale = s;
                        }
                    }
                    if(!scale_ok)
                    {
                        int8_t const s(settings_row->getCell(get_name(name_t::SNAP_NAME_QRCODE_DEFAULT_SCALE))->getValue().safeSignedCharValue());
                        if(s > 0)
                        {
                            scale = s;
                        }
                    }
                    if(scale <= 0)
                    {
                        scale = 1;
                    }
                    else if(scale > 5)
                    {
                        scale = 5;
                    }
                    int edge(5);
                    bool edge_ok(false);
                    if(f_snap->get_uri().has_query_option("edge"))
                    {
                        // query string overwrites the internal default and
                        // administrator setup
                        int const e(f_snap->get_uri().query_option("edge").toLongLong(&edge_ok, 10));
                        if(edge_ok && e > 0)
                        {
                            edge = e;
                        }
                    }
                    if(!edge_ok && settings_row->exists(get_name(name_t::SNAP_NAME_QRCODE_DEFAULT_EDGE)))
                    {
                        int8_t const e(settings_row->getCell(get_name(name_t::SNAP_NAME_QRCODE_DEFAULT_EDGE))->getValue().safeSignedCharValue());
                        if(e >= 0 && e <= 50)
                        {
                            edge = e;
                        }
                    }
                    int const scaled_width(width * scale + edge * 2);
                    size_t const size(scaled_width * scaled_width);
                    std::shared_ptr<unsigned char> data; // use reset(), see SNAP-507
                    data.reset(new unsigned char[size], data_deleter);
                    unsigned char *ptr(data.get());
                    memset(ptr, 255, size);
                    for(int y(0); y < width; ++y)
                    {
                        for(int x(0); x < width; ++x)
                        {
                            if((code->data[y * width + x] & 1) != 0)
                            {
                                for(int h(0); h < scale; ++h)
                                {
                                    for(int w(0); w < scale; ++w)
                                    {
                                        ptr[(y * scale + edge + h) * scaled_width + x * scale + edge + w] = 0;
                                    }
                                }
                            }
                            //else -- all the other bytes are already set to 255
                        }
                    }
                    Magick::Blob blob(ptr, size);

                    // convert the blob to an image
                    Magick::Image image;
                    image.size(Magick::Geometry(scaled_width, scaled_width));
                    image.depth(8);
                    image.magick("GRAY");
                    image.read(blob);

                    // put a comment so you do not need a full decoder to
                    // know what the QR code corresponds to
                    //
                    std::string const comment(
                        QString("QR code for %1\nGenerated by https://snapwebsites.org/")
                                .arg(page_ipath.get_key())
                                .toUtf8().data());
                    image.comment(comment);

                    // convert the image to an in-memory PNG file
                    //
                    Magick::Blob output;
                    image.write(&output, "PNG");
                    QByteArray array(static_cast<char const *>(output.data()), static_cast<int>(output.length()));
                    f_snap->output(array);

                    // tell the browser it is a PNG
                    //
                    f_snap->set_header("Content-Type", "image/png");
                    return true;
                }
            }
        }
    }

    // in all other cases, show the user the "not available" image
    content::path_info_t attachment_ipath;
    attachment_ipath.set_path("images/qrcode/qrcode-not-available.png");
    return attachment::attachment::instance()->on_path_execute(attachment_ipath);
}



} // namespace qrcode
} // namespace snap
// vim: ts=4 sw=4 et
