// Snap Websites Server -- Sitemap XML
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

#include "sitemapxml.h"

#include "../permissions/permissions.h"
#include "../shorturl/shorturl.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qdomnodemodel.h>
#include <snapwebsites/qxmlmessagehandler.h>

#include <iostream>

#include <QDateTime>
#include <QDomDocument>
#include <QDomProcessingInstruction>
#include <QFile>
#include <QXmlQuery>

#include <snapwebsites/poison.h>


/** \file
 * \brief This plugin generates a sitemap.xml for your website.
 *
 * The plugin knows how to gerenate XML sitemap files. Either one
 * if small enough (10Mb / 50,000 files) or any number of site
 * maps and one site map index file.
 *
 * \todo
 * The XML files get saved in the sites table at this point. I
 * think we should save them as attachments. We first need to make
 * sure we can really overwrite an attachment, otherwise we could
 * end up with millions of files per website for nothing.
 *
 * To validate the resulting XML files, use the following commands:
 *
 * \code
 *      # For an XML sitemap file
 *      xmllint --schema plugins/sitemapxml/sitemap.xsd sitemap.xml
 *
 *      # For an XML site index file
 *      xmllint --schema plugins/sitemapxml/siteindex.xsd siteindex.xml
 * \endcode
 *
 * The result should be "\<filename> validates" along a copy of the input
 * file. You may avoid a copy of the input with --noout or redirect the
 * output with --output or the standard shell redirection (>).
 */

SNAP_PLUGIN_START(sitemapxml, 1, 0)


/** \brief Get a fixed sitemapxml name.
 *
 * The sitemapxml plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_SITEMAPXML_COUNT: // in site table, int32
        return "sitemapxml::count";

    case name_t::SNAP_NAME_SITEMAPXML_FILENAME_NUMBER_XML:
        return "sitemap%1.xml";

    case name_t::SNAP_NAME_SITEMAPXML_INCLUDE:
        return "sitemapxml::include";

    case name_t::SNAP_NAME_SITEMAPXML_NAMESPACE:
        return "sitemapxml";

    case name_t::SNAP_NAME_SITEMAPXML_SITEMAP_NUMBER_XML: // in site table, string
        return "sitemapxml::sitemap%1.xml";

    case name_t::SNAP_NAME_SITEMAPXML_SITEMAP_XML: // in site table, string
        return "sitemapxml::sitemap.xml";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_SITEMAPXML_...");

    }
    NOTREACHED();
}


/** \brief Define the image URI.
 *
 * This function saves the specified URI as the image URI.
 *
 * This parameter is mandatory so you will have to call this function
 * with a valid URI.
 *
 * \param[in] uri  The URI to the image.
 */
void sitemapxml::url_image::set_uri(QString const & uri)
{
    f_uri = uri;
}


/** \brief Set the image title.
 *
 * This function saves the title of the image. The title is optional
 * but it certainly is a good idea to have it.
 *
 * \param[in] title  The title of the image.
 */
void sitemapxml::url_image::set_title(QString const & title)
{
    f_title = title;
}


/** \brief Set the image caption.
 *
 * This function saves the caption of the image. You may have a title
 * and a caption. The caption should be more descriptive if you
 * define both.
 *
 * \param[in] caption  The image caption.
 */
void sitemapxml::url_image::set_caption(QString const & caption)
{
    f_caption = caption;
}


/** \brief Set the geographic location of the photo.
 *
 * If the image represents a geographic location, then include that
 * location information. This is a human readable string and not
 * the longitude and latitude or other similar number.
 *
 * \param[in] geo_location  The geographic location.
 */
void sitemapxml::url_image::set_geo_location(QString const & geo_location)
{
    f_geo_location = geo_location;
}


/** \brief Set a URI to the image license.
 *
 * An image can be linked to a license describing how it can be reused.
 * This license should spell out how the image can be used or whether
 * it cannot be used anywhere else.
 *
 * \param[in] license_uri  The URI of the license for this image.
 */
void sitemapxml::url_image::set_license_uri(QString const & license_uri)
{
    f_license_uri = license_uri;
}


/** \brief Retrieve the image URI.
 *
 * This function returns the URI where the image is defined. This
 * is the only mandatory parameter. All the other parameters may
 * return an empty string.
 *
 * \return The image URI.
 */
QString const & sitemapxml::url_image::get_uri() const
{
    return f_uri;
}


/** \brief Retrieve the image title.
 *
 * This function returns the title of the image. It should be plain
 * text only.
 *
 * \return The image title.
 */
QString const & sitemapxml::url_image::get_title() const
{
    return f_title;
}


/** \brief Retrieve the image caption.
 *
 * This function returns the caption of the image. It should be
 * plain text only.
 *
 * \return The image caption.
 */
QString const & sitemapxml::url_image::get_caption() const
{
    return f_caption;
}


/** \brief Retrieve the image geographic location.
 *
 * This function returns the geographic location information. This
 * is a string describing the location in human terms (i.e. not
 * longitude / latitude numbers and such.)
 *
 * \return The geographic location.
 */
QString const & sitemapxml::url_image::get_geo_location() const
{
    return f_geo_location;
}


/** \brief Retreive the URI to the image license.
 *
 * This function returns a string representing the URI one can
 * go to in order to read the license assigned to this image.
 *
 * \return The image license.
 */
QString const & sitemapxml::url_image::get_license_uri() const
{
    return f_license_uri;
}


/** \brief Initialize the URL information to default values.
 *
 * This function initializes the URL info class to default
 * values.
 */
sitemapxml::url_info::url_info()
    //: f_uri("") -- auto-init
    //, f_last_modification() -- auto-init
{
}


/** \brief Set the URI of this resource.
 *
 * This is the URI (often called URL) of the resource being
 * added to the XML sitemap.
 *
 * \param[in] uri  The URI of the resource being added.
 */
void sitemapxml::url_info::set_uri(QString const & uri)
{
    f_uri = uri;
}


/** \brief Set the last modification date.
 *
 * This function let you set the last modification date of the resource.
 * By default this is set to zero which means no modification date will
 * be saved in the XML sitemap.
 *
 * The date is in seconds.
 *
 * \param[in] last_modification  The last modification Unix date.
 */
void sitemapxml::url_info::set_last_modification(time_t last_modification)
{
    if(last_modification < 0)
    {
        last_modification = 0;
    }
    f_last_modification = last_modification;
}


/** \brief Add one image to that page.
 *
 * This function adds one image to the page.
 *
 * The image needs to have a URI. All other parameters are optional.
 *
 * \note
 * The number of images is limited to 1,000 which should be plenty
 * in pretty much all cases. The sitemapxml plugin ignores images
 * added after the array reaches 1,000 entries.
 *
 * \param[in] image  The image to add.
 */
void sitemapxml::url_info::add_image(url_image const & image)
{
    if(image.get_uri().isEmpty())
    {
        throw sitemapxml_exception_missing_uri("This image object must have a URI defined.");
    }

    // ignore once 1,000 images is reached
    if(f_images.size() < 1000)
    {
        f_images.push_back(image);
    }
}


/** \brief Remove all images previously added with add_image().
 *
 * This function resets the image vector so one can add new images
 * in a different url_info.
 */
void sitemapxml::url_info::reset_images()
{
    f_images.clear();
}


/** \brief Get the URI.
 *
 * This URL has a URI which represents the location of the page including
 * the protocol and the domain name.
 *
 * \return The URI pointing to this resource.
 */
QString sitemapxml::url_info::get_uri() const
{
    return f_uri;
}


/** \brief Get the date when it was last modified.
 *
 * This function returns the date when that page was last modified.
 * This is a Unix date and time in seconds.
 *
 * \return The last modification date of this resource.
 */
time_t sitemapxml::url_info::get_last_modification() const
{
    return f_last_modification;
}


/** \brief Retrieve the list of images in this URL.
 *
 * A URI representing a page can be assigned images that appear
 * on that page. This function returns the array of images that
 * were added in this way.
 */
sitemapxml::url_image::vector_t const & sitemapxml::url_info::get_images() const
{
    return f_images;
}


/** \brief Compare two sitemap entries to sort them.
 *
 * The < operator is used to sort sitemap entries so we can put the
 * most important once first, which means showing the pages that
 * were last modified first.
 *
 * The function returns true when 'this' last modification date is larger
 * than 'rhs' last modificat date. So it is inverted compared to what one
 * migh expect (i.e. the largest modification date will appears first in
 * a sort).
 *
 * \param[in] rhs  The other URL to compare with this one
 *
 * \return true if this is considered smaller than rhs.
 */
bool sitemapxml::url_info::operator < (url_info const & rhs) const
{
    // notice that the < and > are inverted ebcause we want the latest
    // first, so the largest 'last modification' first
    //
    if(f_last_modification > rhs.f_last_modification)
    {
        return true;
    }
    if(f_last_modification < rhs.f_last_modification)
    {
        return false;
    }

    // here the order is "random", it just needs to be a valid order
    // also pages are not likely to have the same modification date
    //
    return f_uri >= rhs.f_uri;
}


/** \brief Initialize the sitemapxml plugin.
 *
 * This function is used to initialize the sitemapxml plugin object.
 */
sitemapxml::sitemapxml()
    //: f_snap(NULL) -- auto-init
{
}


/** \brief Clean up the sitemapxml plugin.
 *
 * Ensure the sitemapxml object is clean before it is gone.
 */
sitemapxml::~sitemapxml()
{
}


/** \brief Get a pointer to the sitemapxml plugin.
 *
 * This function returns an instance pointer to the sitemapxml plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the sitemapxml plugin.
 */
sitemapxml * sitemapxml::instance()
{
    return g_plugin_sitemapxml_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString sitemapxml::settings_path() const
{
    return "/admin/settings/sitemapxml";
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
QString sitemapxml::description() const
{
    return "Generates the sitemap.xml file which is used by search engines to"
        " discover your website pages. You can change the settings to hide"
        " different pages or all your pages.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString sitemapxml::dependencies() const
{
    return "|permissions|robotstxt|shorturl|";
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
int64_t sitemapxml::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 12, 20, 1, 15, 42, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the content with our references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void sitemapxml::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    // additional sitemap<###>.xml are added dynamically as the CRON
    // processes find out that additional pages are required.
    //
    content::content::instance()->add_xml("sitemapxml");
}


/** \brief Initialize the sitemapxml.
 *
 * This function terminates the initialization of the sitemapxml plugin
 * by registering for different events.
 */
void sitemapxml::bootstrap(::snap::snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN0(sitemapxml, "server", server, backend_process);
    SNAP_LISTEN(sitemapxml, "content", content::content, copy_branch_cells, _1, _2, _3);
    SNAP_LISTEN(sitemapxml, "robotstxt", robotstxt::robotstxt, generate_robotstxt, _1);
    SNAP_LISTEN(sitemapxml, "shorturl", shorturl::shorturl, allow_shorturl, _1, _2, _3, _4);
}


/** \brief Implementation of the robotstxt signal.
 *
 * This function adds the Sitemap field to the robotstxt file as a global field.
 * (i.e. you're expected to have only one main sitemap.)
 *
 * \note
 * Note that at this time the sitemap.xml file is expected to reside on the
 * exact same domain. This would need to be a parameter we can change.
 * For example, for all our websites we could make use of a specialized
 * computer to handle all the sitemaps and place the results on:
 *
 * http://sitemap.snapwebsites.net/
 *
 * That would reduce the load on the important servers that need to respond
 * to normal users as fast as possible.
 *
 * \param[in] r  The robotstxt object.
 */
void sitemapxml::on_generate_robotstxt(robotstxt::robotstxt * r)
{
    r->add_robots_txt_field(f_snap->get_site_key() + "sitemap.xml", "Sitemap", "", true);
}


/** \brief Called whenever the user tries to access a sitemap.xml file.
 *
 * This function generates and returns the sitemap.xml file contents.
 *
 * The sitemap.xml is generated by reading all the pages defined in the
 * database and removing any page that is clearly marked as "not for
 * the sitemap" (most often non-public pages, and any page the user
 * marks as hidden.)
 *
 * The sitemap is really generated by the backend. The front end only
 * spits out the map that is ready to be sent to the requested.
 *
 * \bug
 * When the backend regenerates a new set of XML sitemap files, it
 * will quickly replace all the old XML sitemaps. If a robot was
 * reading the old sitemaps (assuming there are multiple of them)
 * then it may end up reading a mix of old and new sitemaps. To
 * avoid this problem, we need to keep track of who reads what and
 * keep a copy of the old sitemaps for a little while.
 *
 * \param[in] ipath  The URL path used to access this page.
 *
 * \return true if the sitemap.xml file is properly generated, false otherwise.
 */
bool sitemapxml::on_path_execute(content::path_info_t & ipath)
{
    QString const cpath(ipath.get_cpath());
    if(cpath == "sitemap.xsl")
    {
        // this is the XSL file used to transform the XML sitemap to HTML
        // and thus make it human readable (outside of the text version)
        //
        // TODO: store a pre-compressed version
        //
        QFile xsl(":/plugins/sitemapxml/sitemapxml-to-html.xsl");
        if(!xsl.open(QIODevice::ReadOnly))
        {
            SNAP_LOG_FATAL("sitemapxml::on_path_execute() could not open sitemapxml-to-html.xsl resource file.");
            return false;
        }
        QByteArray data(xsl.readAll());
        f_snap->set_header("Content-Type", "text/xml; charset=utf-8");
        f_snap->output(data);
        return true;
    }

    // We don't generate the sitemap from here, that is reserved
    // for the backend... instead we get information from the
    // database such as the count & actual XML data.
    //
    // Until the backend runs, the sitemap does not exist and the
    // site returns a 404.
    //
    // Try something like this to get the XML sitemaps:
    //
    //       snapbackend -c snapserver.conf
    //       wget http://your-domain.com/sitemap.xml
    //
    // If the first file is a siteindex, then the other sitemaps are
    // numbered starting at 1:
    //
    //       wget http://your-domain.com/sitemap1.xml
    //       wget http://your-domain.com/sitemap2.xml
    //       wget http://your-domain.com/sitemap3.xml
    //       ...
    //

    libdbproxy::value count_value(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_SITEMAPXML_COUNT)));
    int const count(count_value.safeInt32Value());
    if(count <= 0)
    {
        // no sitemap available at this point
        SNAP_LOG_TRACE("No XML sitemap is available.");
        return false;
    }

    libdbproxy::value sitemap_data;
    if(count == 1)
    {
        // special case when there is just one file
        if(cpath != "sitemap.xml" && cpath != "sitemap.txt")
        {
            // wrong filename!
            // (this should not happen unless someone creates a page
            // and mark sitemapxml as the owner...)
            //
            SNAP_LOG_TRACE("Not a valid XML sitemap filename.");
            return false;
        }
        sitemap_data = f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_SITEMAPXML_SITEMAP_XML));
    }
    else
    {
        // there are "many" files, that's handled differently than 1 file
        //
        // TODO: handle .txt files when we have multiple sitemaps?
        //       (I do not think that will work with a siteindex because
        //       the .txt is limited to one file and as such it cannot be
        //       more than the 50,000 URLs limit)
        //
        QRegExp re("sitemap([0-9]*).xml");
        if(!re.exactMatch(cpath))
        {
            // invalid filename for a sitemap
            SNAP_LOG_WARNING("unexpected sitemap filename: \"")(ipath.get_key())("\".");
            return false;
        }

        // check the sitemap number
        snap_string_list sitemap_number(re.capturedTexts());
        if(sitemap_number.size() != 2)
        {
            // invalid filename?! (this case should never happen)
            SNAP_LOG_ERROR("Unexpected number of captured text in filename: \"")(ipath.get_key())("\", we got ")(sitemap_number.size())(".");
            return false;
        }

        if(sitemap_number[1].isEmpty())
        {
            // send sitemap listing all the available sitemaps (siteindex)
            sitemap_data = f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_SITEMAPXML_SITEMAP_XML));
        }
        else
        {
            // we know that the number is only composed of valid digits
            int const index(sitemap_number[1].toInt());
            if(index == 0 || index > count)
            {
                // this index is out of whack!?
                SNAP_LOG_ERROR("Index ")(index)(" is out of bounds. Maximum is ")(count)(".");
                return false;
            }

            // send the requested sitemap
            sitemap_data = f_snap->get_site_parameter("sitemapxml::" + cpath);
        }
    }

    QString const xml(sitemap_data.stringValue());
    QString const extension(f_snap->get_uri().option("extension"));
    if(extension == ".txt")
    {
        // WARNING: This QXmlQuery was not yet replaced because
        //            (1) it uses a QDomNodeModel, which is cool
        //            (2) it outputs the results directly in a QString
        //            (3) the query is directly a QFile
        //          both of which are not supported by our xslt class
        //
        f_snap->set_header("Content-type", "text/plain; charset=utf-8");
        QDomDocument d("urlset");
        if(!d.setContent(xml, true))
        {
            SNAP_LOG_FATAL("sitemapxml::on_path_execute() could not set the DOM content.");
            return false;
        }
        QXmlQuery q(QXmlQuery::XSLT20);
        QMessageHandler msg;
        q.setMessageHandler(&msg);
        QDomNodeModel m(q.namePool(), d);
        QXmlNodeModelIndex x(m.fromDomNode(d.documentElement()));
        QXmlItem i(x);
        q.setFocus(i);
        QFile xsl(":/plugins/sitemapxml/sitemapxml-to-text.xsl");
        if(!xsl.open(QIODevice::ReadOnly))
        {
            SNAP_LOG_FATAL("sitemapxml::on_path_execute() could not open sitemapxml-to-text.xsl resource file.");
            return false;
        }
        q.setQuery(&xsl);
        if(!q.isValid())
        {
            throw sitemapxml_exception_invalid_xslt_data(QString("invalid XSLT query for SITEMAP.XML \"%1\" detected by Qt (text format)").arg(ipath.get_key()));
        }
        QString out;
        q.evaluateTo(&out);
        f_snap->output(out);
    }
    else
    {
        f_snap->set_header("Content-type", "text/xml; charset=utf-8");
        f_snap->output(xml);
    }
    return true;
}


/** \brief Implementation of the generate_sitemapxml signal.
 *
 * This function readies the generate_sitemapxml signal. This signal
 * is expected to be sent only by the sitemapxml plugin backend process
 * as it is considered to be extremely slow.
 *
 * This very function generates the XML sitemap from all the static pages
 * linked to the types/taxonomy/system/sitemapxml/include tag.
 *
 * Other plugins that have dynamic pages should implement this signal in
 * order to add their own public pages to the XML sitemap. (See the
 * char_chart plugin as such an example.)
 *
 * \param[in] r  At this point this parameter is ignored
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool sitemapxml::generate_sitemapxml_impl(sitemapxml * r)
{
    NOTUSED(r);

    libdbproxy::table::pointer_t branch_table(content::content::instance()->get_branch_table());

    path::path * path_plugin(path::path::instance());

    QString const site_key(f_snap->get_site_key_with_slash());

    content::path_info_t include_ipath;
    include_ipath.set_path("types/taxonomy/system/sitemapxml/include");
    links::link_info xml_sitemap_info(get_name(name_t::SNAP_NAME_SITEMAPXML_INCLUDE), false, include_ipath.get_key(), include_ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(xml_sitemap_info));
    links::link_info xml_sitemap;
    while(link_ctxt->next_link(xml_sitemap))
    {
        QString const page_key(xml_sitemap.key());

        // valid for this site?
        if(!page_key.startsWith(site_key))  // this should never be false!
        {
            // invalid page?!?
            continue;
        }

        content::path_info_t page_ipath;
        page_ipath.set_path(page_key);

        // anonymous user has access to that page??
        // check the path, not the site_key + path
        // XXX should we use VISITOR or RETURNING VISITOR as the status?
        content::permission_flag result;
        path_plugin->access_allowed
            ( ""            // anonymous user
            , page_ipath    // this page
            , "view"        // can the anonymous user view this page
            , permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR)    // anonymous users are Visitors
            , result        // give me the result here
            );

//std::cerr << "Found key [" << page_key << "] allowed? " << (result.allowed() ? "YES" : "Nope") << "\n";
        if(!result.allowed())
        {
            // no allowed, forget it
            continue;
        }

        url_info url;

        // set the URI of the page
        url.set_uri(page_key);

        // use the last modification date from that page
        libdbproxy::value modified(branch_table->getRow(page_ipath.get_branch_key())->getCell(QString(content::get_name(content::name_t::SNAP_NAME_CONTENT_MODIFIED)))->getValue());
        if(!modified.nullValue())
        {
            url.set_last_modification(modified.int64Value() / 1000000L); // micro-seconds -> seconds
        }

        // TODO: add support for images, this can work by looking at
        //       the attachments of a page and images there get
        //       added here; maybe only images with a valid caption
        //       or something of the sort if we want to limit the
        //       list
        //
        //<image:image>
        //    <image:loc>http://example.com/image.jpg</image:loc>
        //</image:image>
        // http://googlewebmastercentral.blogspot.com/2010/04/adding-images-to-your-sitemaps.html
        // https://support.google.com/webmasters/answer/178636

        // This is a test to see that the XML sitemap is properly generated
        // although it worked, it is not a real image so we do not add it
        //if(site_key == page_key)
        //{
        //    url_image im;
        //    im.set_uri("http://csnap.m2osw.com/favicon.ico");
        //    im.set_title("ExDox");
        //    im.set_caption("Favorite Icon");
        //    im.set_geo_location("California");
        //    im.set_license_uri("http://csnap.m2osw.com/terms-and-conditions");
        //    url.add_image(im);
        //}

        // TODO: add support for news feed information in sitemaps
        // <news:news>
        //   <news:title>Best XML sitemap ever</news:title>
        // </news:news>
        // https://support.google.com/news/publisher/answer/74288

        add_url(url);
    }
    return true;
}


/** \brief Implementation of the backend process signal.
 *
 * This function captures the backend processing signal which is sent
 * by the server whenever the backend tool is run against a site.
 *
 * The XML sitemap plugin generates XML files file the list of
 * pages that registered themselves as "sitemapxml::include".
 *
 * \todo
 * Right now we call a signal: generate_sitemapxml(), which expects
 * users to hit us with an add_url() call. All the URLs are kept
 * in memory. This means an enormous foot print as sites grow
 * and grow. Instead we probably want the add_url() to save the
 * data in a table and then use the table to generate the resulting
 * XML sitemap.
 *
 * \todo
 * Each time we regenerate the sitemap and an index is used, the
 * update times saved in that index is start_date(). In other words,
 * we are going to tell Google and others that everything changes
 * every five minutes (which may be very true, but that is probably
 * not a good idea.)
 *
 * \todo
 * In link with the 5 minute constant change, for a large website
 * we should have sitemap1.xml which encompasses the last X
 * modified pages (probably using a "now - date < diff" check,)
 * and then all the other "rather static" pages would be in
 * sitemap2.xml, sitemap3.xml, etc.
 *
 * \todo
 * When a siteindex is generated for X number of sitemaps and later
 * the number decreases, the additional sitemap###.xml files do not
 * get deleted. This means someone can attempt to access a page
 * such as sitemap3.xml and not get a clean 404 as it should.
 *
 * \todo
 * Various search engines offer a way to ping them whenever your sitemap
 * changed so they can crawl the new data. We could implement such with
 * an "external" (user defined) list of URLs that include a way to include
 * the URI to the XML sitemap file. For example, the Google sitemap robot
 * can be pinged using:
 *
 * http://www.google.com/webmasters/sitemaps/ping?sitemap=http%3A%2F%2Fexample.com%2Fsitemap.xml
 *
 * See also:
 * https://support.google.com/webmasters/answer/183669?hl=en
 */
void sitemapxml::on_backend_process()
{
    SNAP_LOG_TRACE("sitemapxml::on_backend_process(): process sitemap.xml content.");

    // now give other plugins a chance to add dynamic links to the sitemap.xml
    // file; we don't give the users access to the XML file, they call our
    // add_url() function instead
    generate_sitemapxml(this);

    // sort the result by f_last_modification date, see operator < () for details
    sort(f_url_info.begin(), f_url_info.end());

    // loop through all the URLs
    int32_t position(1);
    size_t const max_urls(f_url_info.size());
    for(size_t index(0);; ++position)
    {
        generate_one_sitemap(position, index);
        if(index >= max_urls)
        {
            // breaking here so we do not get the '++position' and get the wrong count
            break;
        }
    }

    if(position > 1)
    {
        // we need a siteindex since we have multiple XML sitemaps
        generate_sitemap_index(position);
    }

    // save the number of sitemap.xml files we just generated
    // (this does not count the sitemap index if we createdone)
    //
    libdbproxy::value count;
    count.setInt32Value(position);
    f_snap->set_site_parameter(get_name(name_t::SNAP_NAME_SITEMAPXML_COUNT), count);

    // we also save the date in the content::updated field because the
    // user does not directly interact with this data and thus
    // content::updated would otherwise never reflect the last changes
    //
    libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
    uint64_t const start_date(f_snap->get_start_date());
    QString const site_key(f_snap->get_site_key_with_slash());
    QString const content_updated(content::get_name(content::name_t::SNAP_NAME_CONTENT_UPDATED));
    QString const content_modified(content::get_name(content::name_t::SNAP_NAME_CONTENT_MODIFIED));
    QString const sitemap_xml(site_key + "sitemap.xml");
    content_table->getRow(sitemap_xml)->getCell(content_updated)->setValue(start_date);
    content_table->getRow(sitemap_xml)->getCell(content_modified)->setValue(start_date);
    QString const sitemap_txt(site_key + "sitemap.txt");
    content_table->getRow(sitemap_txt)->getCell(content_updated)->setValue(start_date);
    content_table->getRow(sitemap_txt)->getCell(content_modified)->setValue(start_date);

#ifdef DEBUG
SNAP_LOG_TRACE() << "Updated [" << sitemap_xml << "]";
#endif
}


/** \brief Generate one site.
 *
 * This function generates one sitemap.xml file. When doing so, we
 * follow the constrains setup by the sitemap specifications:
 *
 * \li One sitemap.xml cannot have more than 50,000 items in it.
 * \li One sitemap.xml cannot be more than 10MB (10,485,760 bytes).
 *
 * The function returns once one of these limits is reached or when
 * all the URLs were saved in the output.
 *
 * Note that the administrator can change the parameters and decide
 * whether to allow such large files or use (much) smaller footprints
 * per XML sitemap file.
 *
 * \note
 * Frequency and priority are not used by Google anymore since these
 * parameters most often give wrong signals. The last modification date
 * is much more important and useful since they can use that information
 * to see when a page was last modified and not re-crawl the page until
 * it is marked as modified since their last crawled it.
 *
 * https://www.seroundtable.com/google-priority-change-frequency-xml-sitemap-20273.html
 *
 * \par
 * This also means we need to make sure to look into whether a page includes
 * another and use the largest 'last modification date' of both of these.
 *
 * \todo
 * The data saved should be compressed so we do not have to
 * compress on the fly when the user accesses the data. This
 * is not too bad now, but once we offer files that are (many)
 * megabytes...
 */
void sitemapxml::generate_one_sitemap(int32_t const position, size_t & index)
{
    uint64_t const start_date(f_snap->get_start_date());
    QString const site_key(f_snap->get_site_key_with_slash());
    // TODO: offer the administrator a way to change the maximum limit
    //       in bytes (instead of the top maximum of 10MB) because 10MB
    //       downloads may put their servers on their knees for a while!
    size_t const max_size(10 * 1024 * 1024);

    QString last_result;

    QDomDocument doc;

    // add the XML "processing instruction"
    QDomProcessingInstruction xml_marker(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"utf-8\""));
    doc.appendChild(xml_marker);

    // The stylesheet makes use of a processing instruction entry
    // The XSLT file transforms the XML in an HTML table with styles
    // <?xml-stylesheet type="text/xsl" href="/sitemap.xsl"?>
    QDomProcessingInstruction stylesheet(doc.createProcessingInstruction("xml-stylesheet", "type=\"text/xsl\" href=\"/sitemap.xsl\""));
    doc.appendChild(stylesheet);

    // create the root tag
    QDomElement root(doc.createElement("urlset"));
    root.setAttribute("xmlns", "http://www.sitemaps.org/schemas/sitemap/0.9");
    root.setAttribute("xmlns:image", "http://www.google.com/schemas/sitemap-image/1.1");

    doc.appendChild(root);

    // limit the maximum to 50,000 per file
    // TODO: offer administrators to change the 50,000 limit
    size_t const max_per_file(50000);
    size_t max_urls(index + max_per_file);
    if(max_urls > f_url_info.size())
    {
        max_urls = f_url_info.size();
    }
    for(size_t count(1); index < max_urls; ++index, ++count)
    {
        url_info const & u(f_url_info[index]);

        // append one more entry

        // create /url
        QDomElement url(doc.createElement("url"));
        root.appendChild(url);

        // create /url/loc
        QDomElement loc(doc.createElement("loc"));
        url.appendChild(loc);
        snap_dom::append_plain_text_to_node(loc, u.get_uri());

        // create /url/lastmod (optional)
        time_t const t(u.get_last_modification());
        if(t != 0)
        {
            QDomElement lastmod(doc.createElement("lastmod"));
            url.appendChild(lastmod);
            snap_dom::append_plain_text_to_node(lastmod, f_snap->date_to_string(t * 1000000, snap_child::date_format_t::DATE_FORMAT_LONG));
        }

        // create the /url/xhtml:link (rel="alternate")
        // see http://googlewebmastercentral.blogspot.com/2012/05/multilingual-and-multinational-site.html
        // (requires a pattern to generate the right URIs)
        // see layouts/white-theme-parser.xsl for the pattern information,
        // we have the mode that defines the "pattern" for the URI, but we
        // need to know where it is defined which is not done yet

        // if this entry has one or more images, add them now
        url_image::vector_t const & images(u.get_images());
        size_t const max_images(images.size());
        if(max_images > 0)
        {
            for(size_t j(0); j < max_images; ++j)
            {
                url_image const & im(images[j]);

                // create url/image:image
                QDomElement image_tag(doc.createElement("image:image"));
                url.appendChild(image_tag);

                // create url/image:image/image:loc
                QDomElement image_loc(doc.createElement("image:loc"));
                image_tag.appendChild(image_loc);
                snap_dom::append_plain_text_to_node(image_loc, im.get_uri());

                // create url/image:image/image:caption (optional)
                QString const caption(im.get_caption());
                if(!caption.isEmpty())
                {
                    QDomElement image_caption(doc.createElement("image:caption"));
                    image_tag.appendChild(image_caption);
                    snap_dom::append_plain_text_to_node(image_caption, caption);
                }

                // create url/image:image/image:geo_location (optional)
                QString const geo_location(im.get_geo_location());
                if(!geo_location.isEmpty())
                {
                    QDomElement image_geo_location(doc.createElement("image:geo_location"));
                    image_tag.appendChild(image_geo_location);
                    snap_dom::append_plain_text_to_node(image_geo_location, geo_location);
                }

                // create url/image:image/image:title (optional)
                QString const title(im.get_title());
                if(!title.isEmpty())
                {
                    QDomElement image_title(doc.createElement("image:title"));
                    image_tag.appendChild(image_title);
                    snap_dom::append_plain_text_to_node(image_title, title);
                }

                // create url/image:image/image:license (optional)
                QString const license_uri(im.get_license_uri());
                if(!license_uri.isEmpty())
                {
                    QDomElement image_license_uri(doc.createElement("image:license"));
                    image_tag.appendChild(image_license_uri);
                    snap_dom::append_plain_text_to_node(image_license_uri, license_uri);
                }
            }
        }

        // TODO: append the news:news once available

        // add a little comment at the top as some humans look at that stuff...
        QDomComment comment(doc.createComment(QString(
                    "\n  Generator: sitemapxml plugin"
                    "\n  Creation date: %1"
                    "\n  Sitemap/URL counts: %2/%3"
                    "\n  System: http://snapwebsites.org/"
                    "\n")
                .arg(f_snap->date_to_string(start_date, snap_child::date_format_t::DATE_FORMAT_HTTP))
                .arg(position)
                .arg(count)));
        doc.insertAfter(comment, xml_marker);

        // we have to check the result each time since the 10Mb may
        // be reached at any time (frankly I know that we could
        // ameliorate this 10 folds... but for now, this is simple
        // enough. With super large sites, we will suffer, though.)
        //
        QString const result(doc.toString(-1));
        if(!last_result.isEmpty())
        {
            QByteArray result_utf8(result.toUtf8());
            if(result_utf8.size() > static_cast<int>(max_size)) // stupid Qt returning int for sizes...
            {
                // this last result is too large, use the previous one...
                //
                // in this case we are not done (we just created an entry and
                // we cannot save it, so we are going to have multiple sitemaps)
                //
                break;
            }
        }

        // keep a copy of the last valid result (i.e. not too many URLs
        // and not too large)
        //
        last_result = result;

        // remove the comment because we will add another URL
        // so the info will change...
        //
        doc.removeChild(comment);
    }

    // if we reach here, we either saved all the URLs still present
    // or we reached the maximum number of URLs we could save in one
    // file
    //
    if(position == 1 && index >= f_url_info.size())
    {
        // only one sitemap.xml file, save using the "sitemap.xml" filename
        libdbproxy::value result_value;
        result_value.setStringValue(last_result);
        f_snap->set_site_parameter(get_name(name_t::SNAP_NAME_SITEMAPXML_SITEMAP_XML), result_value);
    }
    else
    {
        // we already saved other sitemap###.xml files, save this final
        // set of URLs in a new sitemap
        //
        QString const filename(QString(get_name(name_t::SNAP_NAME_SITEMAPXML_SITEMAP_NUMBER_XML)).arg(position));
        libdbproxy::value result_value;
        result_value.setStringValue(last_result);
        f_snap->set_site_parameter(filename, result_value);

        content::content * content_plugin(content::content::instance());
        libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());

        content::path_info_t ipath;
        ipath.set_path(QString(get_name(name_t::SNAP_NAME_SITEMAPXML_FILENAME_NUMBER_XML)).arg(position));
        ipath.force_branch(snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
        ipath.force_revision(snap_version::SPECIAL_VERSION_FIRST_REVISION);
        ipath.force_locale("");
        content_plugin->create_content(ipath, get_plugin_name(), "page/public");

        signed char const final_page(1);
        content_table->getRow(ipath.get_key())->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_FINAL))->setValue(final_page);
    }
}


/** \brief Generate a sitemap.xml index to other sitemaps.
 *
 * This function generates the sitemap.xml file which is an index of
 * sitemaps because the website already reached a critical size where
 * each sitemap was either too large or the website has over 50,000
 * publicly available pages.
 *
 * \todo
 * The sitemap index needs to also be limited to 50,000 URLs and
 * a maximum size of 10MB. However, 50,000 * 50,000 is 2.5 billion
 * pages. I do not think too many websites will reach that limit
 * any time soon (for what anyway?) Even wikipedia is at 37 million
 * in Oct 2015 and covers way more than one man can ever hope to
 * learn in a lifetime.
 *
 * \param[in] position  The number of XML sitemap that were generated.
 */
void sitemapxml::generate_sitemap_index(int32_t position)
{
    uint64_t const start_date(f_snap->get_start_date());
    QString const site_key(f_snap->get_site_key_with_slash());

    QDomDocument doc;

    // add the XML "processing instruction"
    QDomProcessingInstruction xml_marker(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"utf-8\""));
    doc.appendChild(xml_marker);

    // add a little comment at the top as some humans look at that stuff...
    QDomComment comment(doc.createComment(QString(
                "\n  Generator: sitemapxml plugin"
                "\n  Creation date: %1"
                "\n  Number of sitemaps: %2"
                "\n  System: http://snapwebsites.org/"
                "\n")
            .arg(f_snap->date_to_string(start_date, snap_child::date_format_t::DATE_FORMAT_HTTP))
            .arg(position)));
    doc.appendChild(comment);

    // The stylesheet makes use of a processing instruction entry
    // The XSLT file transforms the XML in an HTML table with styles
    // <?xml-stylesheet type="text/xsl" href="/sitemap.xsl"?>
    QDomProcessingInstruction stylesheet(doc.createProcessingInstruction("xml-stylesheet", "type=\"text/xsl\" href=\"/sitemap.xsl\""));
    doc.appendChild(stylesheet);

    // create the root tag
    QDomElement root(doc.createElement("sitemapindex"));
    root.setAttribute("xmlns", "http://www.sitemaps.org/schemas/sitemap/0.9");

    doc.appendChild(root);

    for(int32_t index(1); index <= position; ++index)
    {
        // create /sitemap
        QDomElement sitemap(doc.createElement("sitemap"));
        root.appendChild(sitemap);

        // create /sitemap/loc
        QDomElement loc(doc.createElement("loc"));
        sitemap.appendChild(loc);
        snap_dom::append_plain_text_to_node(loc, QString("%1sitemap%2.xml").arg(site_key).arg(index));

        // create /sitemap/lastmod
        QDomElement lastmod(doc.createElement("lastmod"));
        sitemap.appendChild(lastmod);
        snap_dom::append_plain_text_to_node(lastmod, f_snap->date_to_string(start_date, snap_child::date_format_t::DATE_FORMAT_LONG));
    }

    // only one sitemap.xml file, save using the "sitemap.xml" filename
    libdbproxy::value value;
    value.setStringValue(doc.toString(-1));
    f_snap->set_site_parameter(get_name(name_t::SNAP_NAME_SITEMAPXML_SITEMAP_XML), value);
}


/** \brief Add a URL to the XML sitemap.
 *
 * This function adds the specified URL information to the XML sitemap.
 * This is generally called from the different implementation of the
 * generate_sitemapxml signal.
 *
 * \todo
 * At this time all the data is kept in memory, which is fine for
 * our current purpose, however, some day we will have to change
 * that because really large websites are likely to create many
 * mega bytes of URL data which will need to be sorted, etc. and
 * Cassandra will be able to do that for us, but probably not
 * a computer memory (not as efficiently). We could also optimize
 * by gathering the URLs as they get created instead of having
 * a backend and a signal which does that work every five minutes.
 *
 * \param[in] url  The URL information to add to the sitemap.
 */
void sitemapxml::add_url(url_info const & url)
{
    f_url_info.push_back(url);
}


/** \brief Prevent short URL on sitemap.xml files.
 *
 * sitemap.xml really do not need a short URL so we prevent those on
 * such paths.
 *
 * \param[in,out] ipath  The path being checked.
 * \param[in] owner  The plugin that owns that page.
 * \param[in] type  The type of this page.
 * \param[in,out] allow  Whether the short URL is allowed.
 */
void sitemapxml::on_allow_shorturl(content::path_info_t & ipath, QString const & owner, QString const & type, bool & allow)
{
    NOTUSED(owner);
    NOTUSED(type);

    if(!allow)
    {
        // already forbidden, cut short
        return;
    }

    //
    // a sitemap XML file may include a number as in:
    //
    //    sitemap101.xml
    //
    // so our test uses the start and end of the filename; this is not
    // 100% correct since sitemap-video.xml will match too... but it
    // is really fast (TODO--still we want to fix that at some point...)
    //
    QString const cpath(ipath.get_cpath());
    if((cpath.startsWith("sitemap") && cpath.endsWith(".xml"))
    || cpath == "sitemap.txt"
    || cpath == "sitemap.xsl")
    {
        allow = false;
    }
}


void sitemapxml::on_copy_branch_cells(libdbproxy::cells& source_cells, libdbproxy::row::pointer_t destination_row, snap_version::version_number_t const destination_branch)
{
    NOTUSED(destination_branch);

    content::content::copy_branch_cells_as_is(source_cells, destination_row, get_name(name_t::SNAP_NAME_SITEMAPXML_NAMESPACE));
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
