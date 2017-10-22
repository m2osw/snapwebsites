// Snap Websites Server -- Snap Software Description handling
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
 * \brief Snap Software Description plugin.
 *
 * This plugin manages Snap Software Descriptions. This means it lets you
 * enter software descriptions, including links, logos, licenses, fees,
 * etc. and then transforms that data to XML and makes those files available
 * to the world to see.
 *
 * This is a complete redesign from the PAD File XML format which is really
 * weak and exclusively designed for Microsoft windows executables (even if
 * you can say Linux in there, the format is a one to one match with the
 * Microsoft environment and as such has many limitations.)
 *
 * The format is described on snapwebsites.org:
 * http://snapwebsites.org/implementation/feature-requirements/pad-and-snsd-files-feature/snap-software-description
 */

#include "snap_software_description.h"

#include "../attachment/attachment.h"
#include "../filter/filter.h"
#include "../list/list.h"
#include "../shorturl/shorturl.h"

#include <snapwebsites/http_strings.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qdomxpath.h>
#include <snapwebsites/xslt.h>

#include <QFile>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(snap_software_description, 1, 0)



/** \brief Get a fixed snap_software_description plugin name.
 *
 * The snap_software_description plugin makes use of different names
 * in the database. This function ensures that you get the right
 * spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_CATEGORY:
        return "snap_software_description::category";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_ENABLE:
        return "snap_software_description::enable";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_HTTP_HEADER:
        return "X-Snap-Software-Description";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_LAST_UPDATE:
        return "snap_software_description::last_update";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_PUBLISHER_FIELD:
        return "snap_software_description::publisher";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_PUBLISHER_TYPE_PATH:
        return "types/snap-software-description/publisher";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_MAX_FILES:
        return "snap_software_description::max_files";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_PATH:
        return "admin/settings/snap-software-description";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_TEASER_END_MARKER:
        return "snap_software_description::teaser_end_marker";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_TEASER_TAGS:
        return "snap_software_description::teaser_tags";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_TEASER_WORDS:
        return "snap_software_description::teaser_words";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SUPPORT_FIELD:
        return "snap_software_description::support";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SUPPORT_TYPE_PATH:
        return "types/snap-software-description/support";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_TABLE_OF_CONTENT:
        return "snap_software_description::table_of_content";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_...");

    }
    NOTREACHED();
}


/** \class snap_software_description
 * \brief The snap_software_description plugin handles application authentication.
 *
 * Any Snap! website can be setup to accept application authentication.
 *
 * The website generates a token that can be used to log you in.
 */


/** \brief Initialize the snap_software_description plugin.
 *
 * This function initializes the snap_software_description plugin.
 */
snap_software_description::snap_software_description()
    //: f_snap(nullptr) -- auto-init
{
}

/** \brief Destroy the snap_software_description plugin.
 *
 * This function cleans up the snap_software_description plugin.
 */
snap_software_description::~snap_software_description()
{
}


/** \brief Get a pointer to the snap_software_description plugin.
 *
 * This function returns an instance pointer to the snap_software_descriptiosnap_software_descriptionin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the snap_software_description plugin.
 */
snap_software_description * snap_software_description::instance()
{
    return g_plugin_snap_software_description_factory.instance();
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
QString snap_software_description::description() const
{
    return "The Snap Software Description plugin offers you a way to"
          " define a set of descriptions for software that you are offering"
          " for download on your website. The software may be free or for"
          " a fee. It may also be a shareware.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString snap_software_description::dependencies() const
{
    return "|attachment|content|editor|layout|list|output|path|";
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
int64_t snap_software_description::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 11, 29, 4, 39, 7, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the snap_software_description plugin content.
 *
 * This function updates the contents in the database using the
 * system update settings found in the resources.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update
 *                                 (in micro-seconds).
 */
void snap_software_description::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Bootstrap the snap_software_description.
 *
 * This function adds the events the snap_software_description plugin is listening for.
 *
 * \param[in] snap  The child handling this request.
 */
void snap_software_description::bootstrap(::snap::snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN0(snap_software_description, "server", server, backend_process);
    SNAP_LISTEN(snap_software_description, "layout", layout::layout, generate_header_content, _1, _2, _3);
    SNAP_LISTEN(snap_software_description, "layout", layout::layout, generate_page_content, _1, _2, _3);
    SNAP_LISTEN(snap_software_description, "robotstxt", robotstxt::robotstxt, generate_robotstxt, _1);
    SNAP_LISTEN(snap_software_description, "shorturl", shorturl::shorturl, allow_shorturl, _1, _2, _3, _4);
}


/** \brief Get the path to the root description.
 *
 * The Snap Software Description system works from a root and leaves.
 * The leaves are other catalogs or files. Files are terminal (they
 * cannot have children.)
 *
 * The location of the root is currently hard coded as:
 *
 * \code
 * http://example.com/types/snap-websites-description.xml
 * \endcode
 *
 * This function returns that path with the correct URL as the domain
 * name.
 *
 * The path is directly to where the Snap Software Description backend
 * process will save that root XML file.
 *
 * Later we may allow end users to change this default to another path.
 */
QString snap_software_description::get_root_path()
{
    return QString("%1types/snap-software-description/category/snap-software-description.xml").arg(f_snap->get_site_key_with_slash());
}


/** \brief Generate the header common content.
 *
 * This function adds an HTTP header with a URL to the Snap Software
 * Description root file.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] header  The page being generated.
 * \param[in,out] metadata  The body being generated.
 */
void snap_software_description::on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    NOTUSED(header);
    NOTUSED(metadata);

    // only put that info on the home page ("/")
    // and all the types specific to snap-software-description
    // that way we save some time on other pages
    //
    QString const cpath(ipath.get_cpath());
    if(cpath.startsWith("types/snap-software-description"))
    {
        f_snap->set_header(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_HTTP_HEADER), get_root_path());
    }
}


/** \brief Generate links in the header.
 *
 * This function generates one alternate link per feed made available.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void snap_software_description::on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    NOTUSED(page);

    // only on the home page; no need to replicate that info on all pages
    //
    if(ipath.get_cpath() == "")
    {
        FIELD_SEARCH
            (content::field_search::command_t::COMMAND_MODE, content::field_search::mode_t::SEARCH_MODE_EACH)
            (content::field_search::command_t::COMMAND_ELEMENT, body)

            (content::field_search::command_t::COMMAND_DEFAULT_VALUE, "Snap Software Description")
            (content::field_search::command_t::COMMAND_SAVE, QString("formats[href=\"%1\"][type=\"text/xml\"]").arg(get_root_path()))

            // generate
            ;
    }
}


/** \brief Implementation of the robotstxt signal.
 *
 * This function adds the Snap Software Description field to the
 * robotstxt file as a global field. (i.e. you are expected to
 * have only one Snap Software Description root file per website.)
 *
 * \param[in] r  The robotstxt object.
 */
void snap_software_description::on_generate_robotstxt(robotstxt::robotstxt * r)
{
    r->add_robots_txt_field(get_root_path(), "Snap-Websites-Description", "", true);
}


/** \brief Prevent short URL on snap-software-description.xml files.
 *
 * snap-software-description.xml and any other file generated by this
 * plugin really do not need a short URL so we prevent those on such paths.
 *
 * \param[in,out] ipath  The path being checked.
 * \param[in] owner  The plugin that owns that page.
 * \param[in] type  The type of this page.
 * \param[in,out] allow  Whether the short URL is allowed.
 */
void snap_software_description::on_allow_shorturl(content::path_info_t & ipath, QString const & owner, QString const & type, bool & allow)
{
    NOTUSED(owner);
    NOTUSED(type);

    if(!allow)
    {
        // already forbidden, cut short
        return;
    }

    //
    // all our files do not need a short URL definition
    //
    QString const cpath(ipath.get_cpath());
    if(cpath.startsWith("types/snap-software-description") && cpath.endsWith(".xml"))
    {
        allow = false;
    }
}


/** \brief Implementation of the backend process signal.
 *
 * This function captures the backend processing signal which is sent
 * by the server whenever the backend tool is run against a site.
 *
 * The backend processing of the Snap Software Description plugin
 * generates all the XML files somehow linked to the Snap Software
 * Description plugin.
 *
 * The files include tags as described in the documentation:
 * http://snapwebsites.org/implementation/feature-requirements/pad-and-snsd-files-feature/snap-software-description
 *
 * The backend processing is done with multiple levels as in:
 *
 * \li start with the root, which is defined as files directly
 *     linked to ".../types/snap-software-description", and
 *     categories: types defined under
 *     ".../types/snap-software-description/...".
 * \li as we find files, create their respective XML files.
 * \li repeat the process with each category; defining sub-categories.
 * \li repeat the process with sub-categories; defining sub-sub-categories.
 *
 * We start at sub-sub-categories (3 levels) because there is generally
 * no reason to go further. The category tree is probably not that well
 * defined for everyone where sub-sub-sub-categories would become useful.
 */
void snap_software_description::on_backend_process()
{
    SNAP_LOG_TRACE("snap_software_description::on_backend_process(): process snap-software-description.xml content.");

    // RAII to restore the main path
    class restore_path_t
    {
    public:
        restore_path_t(snap_child * snap)
            : f_snap(snap)
        {
        }

        ~restore_path_t()
        {
            // reset the main URI
            try
            {
                f_snap->set_uri_path("/");
            }
            catch(snap_uri_exception_invalid_path const &)
            {
            }
        }

    private:
        snap_child *                f_snap = nullptr;
    };

    restore_path_t rp(f_snap);

    content::content * content_plugin(content::content::instance());
    //libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());

    content::path_info_t snap_software_description_settings_ipath;
    snap_software_description_settings_ipath.set_path(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_PATH));
    f_snap_software_description_settings_row = revision_table->getRow(snap_software_description_settings_ipath.get_revision_key());

    if(!create_publisher())
    {
        return;
    }
    if(!create_support())
    {
        return;
    }

    // load catalog parser once
    if(!load_xsl_file(":/xsl/layout/snap-software-description-catalog-parser.xsl", f_snap_software_description_parser_catalog_xsl))
    {
        return;
    }

    // load file parser once
    if(!load_xsl_file(":/xsl/layout/snap-software-description-file-parser.xsl", f_snap_software_description_parser_file_xsl))
    {
        return;
    }

    // load padfile parser once
    if(!load_xsl_file(":/xsl/layout/padfile-parser.xsl", f_padfile_xsl))
    {
        return;
    }

    content::path_info_t ipath;
    ipath.set_path("/types/snap-software-description/category");

    {
        content::path_info_t table_of_content_link_ipath;
        table_of_content_link_ipath.set_path("/types/snap-software-description/table-of-contents");
        links::link_info info(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_TABLE_OF_CONTENT), true,
                                        table_of_content_link_ipath.get_key(), table_of_content_link_ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info child_info;
        if(link_ctxt->next_link(child_info))
        {
            f_table_of_content_ipath.reset(new content::path_info_t);
            f_table_of_content_ipath->set_path(child_info.key());
        }
    }

    // reset the document on each access
    //
    f_padlist_xml = QDomDocument();
    QDomElement root(f_padlist_xml.createElement("snap"));
    f_padlist_xml.appendChild(root);

    create_catalog(ipath, 0);

    save_pad_file_data();
}


/** \brief Save the list of files as PAD file maps.
 *
 * While in create_catalog() we collect the path to all the files and
 * here we save a set of files that include these lists.
 *
 * The funtion creates two files: padmap.txt which is a simple text file
 * with one URL to each PAD file in plain text format; it also creates
 * a list.xml file which is similar, only in XML with a small header as
 * defined in the padlist-parser.xsl file.
 */
void snap_software_description::save_pad_file_data()
{
    content::content * content_plugin(content::content::instance());
    attachment::attachment * attachment_plugin(attachment::attachment::instance());

    // save the padmap.txt
    //
    {
        QString const filename("padmap.txt");
        int64_t const start_date(f_snap->get_start_date());

        snap::content::attachment_file attachment(f_snap);

        attachment.set_multiple(false);
        attachment.set_parent_cpath("");    // available as an attachment to the home page
        attachment.set_field_name("snap_software_description::padmap_txt");
        attachment.set_attachment_owner(attachment_plugin->get_plugin_name());
        attachment.set_attachment_type("attachment/public");
        attachment.set_creation_time(start_date);
        attachment.set_update_time(start_date);
        //attachment.set_dependencies(...);
        attachment.set_file_name(filename);
        attachment.set_file_filename(filename);
        attachment.set_file_creation_time(start_date);
        attachment.set_file_modification_time(start_date);
        attachment.set_file_index(1);
        attachment.set_file_data(f_padmap_txt.toUtf8());
        attachment.set_file_mime_type("text/plain");
        attachment.set_revision_limit(3);

        content_plugin->create_attachment(attachment, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, "");
    }

    // the PAD list.xml file is created now that we have a complete
    // list of all the files offered
    //
    QString padlist_xsl;
    if(load_xsl_file(":/xsl/layout/padlist-parser.xsl", padlist_xsl))
    {
        f_padlist_xml.documentElement().setAttribute("version", SNAPWEBSITES_VERSION_STRING);

        xslt x;
        x.set_xsl(padlist_xsl);
        x.set_document(f_padlist_xml);
        QString const output(QString("<?xml version=\"1.0\"?>%1").arg(x.evaluate_to_string()));

        {
            QString const filename("list.xml");
            int64_t const start_date(f_snap->get_start_date());

            snap::content::attachment_file attachment(f_snap);

            attachment.set_multiple(false);
            attachment.set_parent_cpath("");    // available as an attachment to the home page
            attachment.set_field_name("snap_software_description::padlist_xml");
            attachment.set_attachment_owner(attachment_plugin->get_plugin_name());
            attachment.set_attachment_type("attachment/public");
            attachment.set_creation_time(start_date);
            attachment.set_update_time(start_date);
            //attachment.set_dependencies(...);
            attachment.set_file_name(filename);
            attachment.set_file_filename(filename);
            attachment.set_file_creation_time(start_date);
            attachment.set_file_modification_time(start_date);
            attachment.set_file_index(1);
            attachment.set_file_data(output.toUtf8());
            attachment.set_file_mime_type("text/xml");
            attachment.set_revision_limit(3);

            content_plugin->create_attachment(attachment, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, "");
        }
    }
}


/** \brief Create the list of publishers.
 *
 * This function is used to create the list of publishers.
 *
 * Publishers are attached (linked) to files. You may have any number
 * of them. In many cases you have just one publisher on a website.
 *
 * \return true if the publisher was generated as expected, false otherwise.
 */
bool snap_software_description::create_publisher()
{
    // load file parser
    if(!load_xsl_file(":/xsl/layout/snap-software-description-publisher-parser.xsl", f_snap_software_description_parser_publisher_xsl))
    {
        return false;
    }

    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    list::list * list_plugin(list::list::instance());
    path::path * path_plugin(path::path::instance());
    attachment::attachment * attachment_plugin(attachment::attachment::instance());
    layout::layout * layout_plugin(layout::layout::instance());
    int64_t const start_date(f_snap->get_start_date());

    // publishers are linked to a specific system type which is a list.
    // we use the list because that way we automatically avoid publishers
    // that got deleted, hidden, moved, etc. which the list of types may
    // not always catch directly
    //

    content::path_info_t ipath;
    ipath.set_path(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_PUBLISHER_TYPE_PATH));
    list::list_item_vector_t list(list_plugin->read_list(ipath, 0, -1));
    int const max_items(list.size());
    for(int idx(0); idx < max_items; ++idx)
    {
        content::path_info_t publisher_ipath;
        publisher_ipath.set_path(list[idx].get_uri());

        // only pages that can be handled by layouts are added
        // others are silently ignored (note that only broken
        // pages should fail the following test)
        //
        quiet_error_callback snap_software_description_error_callback(f_snap, true);
        plugins::plugin * layout_ready(path_plugin->get_plugin(publisher_ipath, snap_software_description_error_callback));
        layout::layout_content * layout_ptr(dynamic_cast<layout::layout_content *>(layout_ready));
        if(!layout_ptr)
        {
            // log the error?
            // this is probably not the role of the snap-software-description
            // implementation...
            //
            continue;
        }

        // modified since we last generated that file?
        //
        libdbproxy::row::pointer_t row(content_table->getRow(publisher_ipath.get_key()));
        int64_t const modified(row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_MODIFIED))->getValue().safeInt64Value(0, 0));
        int64_t const last_snsd_update(row->getCell(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_LAST_UPDATE))->getValue().safeInt64Value(0, 0));
        if(last_snsd_update > 0
        && (modified == 0 || modified < last_snsd_update))
        {
            continue;
        }

        // since we are a backend, the main ipath remains equal
        // to the home page and that is what gets used to generate
        // the path to each page in the feed data so we have to
        // change it before we apply the layout
        f_snap->set_uri_path(QString("/%1").arg(publisher_ipath.get_cpath()));

        QDomDocument doc(layout_plugin->create_document(publisher_ipath, layout_ready));
        layout_plugin->create_body(doc, publisher_ipath, f_snap_software_description_parser_publisher_xsl, layout_ptr, false, "snap-software-description-publisher");

        QDomXPath dom_xpath;
        dom_xpath.setXPath("/snap/page/body/output/snsd-publisher");
        QDomXPath::node_vector_t publisher_tag(dom_xpath.apply(doc));
        if(publisher_tag.isEmpty())
        {
            SNAP_LOG_FATAL("skipping publisher as the output of create_body() did not give us the expected tags.");
            continue;
        }

        QString const output(QString("<?xml version=\"1.0\"?>%1").arg(snap_dom::xml_to_string(publisher_tag[0])));

        {
            snap::content::attachment_file attachment(f_snap);

            attachment.set_multiple(false);
            attachment.set_parent_cpath(publisher_ipath.get_cpath());
            attachment.set_field_name("snap_software_description::publisher_xml");
            attachment.set_attachment_owner(attachment_plugin->get_plugin_name());
            attachment.set_attachment_type("attachment/public");
            attachment.set_creation_time(start_date);
            attachment.set_update_time(start_date);
            //attachment.set_dependencies(...);
            attachment.set_file_name("publisher.xml");
            attachment.set_file_filename("publisher.xml");
            attachment.set_file_creation_time(start_date);
            attachment.set_file_modification_time(start_date);
            attachment.set_file_index(1);
            attachment.set_file_data(output.toUtf8());
            attachment.set_file_mime_type("text/xml");
            attachment.set_revision_limit(3);

            content_plugin->create_attachment(attachment, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, "");
        }

        row->getCell(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_LAST_UPDATE))->setValue(start_date);
    }

    return true;
}


/** \brief Create the list of support pages.
 *
 * This function is used to create the list of support pages.
 *
 * Support pages are attached (linked) to files. You may have any number
 * of them. In many cases you have just one support page on a website.
 *
 * \return true if the support page was generated as expected, false otherwise.
 */
bool snap_software_description::create_support()
{
    // load file parser
    if(!load_xsl_file(":/xsl/layout/snap-software-description-support-parser.xsl", f_snap_software_description_parser_support_xsl))
    {
        return false;
    }

    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    list::list * list_plugin(list::list::instance());
    path::path * path_plugin(path::path::instance());
    attachment::attachment * attachment_plugin(attachment::attachment::instance());
    layout::layout * layout_plugin(layout::layout::instance());
    int64_t const start_date(f_snap->get_start_date());

    // support pages are linked to a specific system type which is a list.
    // we use the list because that way we automatically avoid support pages
    // that got deleted, hidden, moved, etc. which the list of types may
    // not always catch directly
    //

    content::path_info_t ipath;
    ipath.set_path(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SUPPORT_TYPE_PATH));
    list::list_item_vector_t list(list_plugin->read_list(ipath, 0, -1));
    int const max_items(list.size());
    for(int idx(0); idx < max_items; ++idx)
    {
        content::path_info_t support_ipath;
        support_ipath.set_path(list[idx].get_uri());

        // only pages that can be handled by layouts are added
        // others are silently ignored (note that only broken
        // pages should fail the following test)
        //
        quiet_error_callback snap_software_description_error_callback(f_snap, true);
        plugins::plugin * layout_ready(path_plugin->get_plugin(support_ipath, snap_software_description_error_callback));
        layout::layout_content * layout_ptr(dynamic_cast<layout::layout_content *>(layout_ready));
        if(!layout_ptr)
        {
            // log the error?
            // this is probably not the role of the snap-software-description
            // implementation...
            //
            continue;
        }

        // modified since we last generated that file?
        //
        libdbproxy::row::pointer_t row(content_table->getRow(support_ipath.get_key()));
        int64_t const modified(row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_MODIFIED))->getValue().safeInt64Value(0, 0));
        int64_t const last_snsd_update(row->getCell(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_LAST_UPDATE))->getValue().safeInt64Value(0, 0));
        if(last_snsd_update > 0
        && (modified == 0 || modified < last_snsd_update))
        {
            continue;
        }

        // since we are a backend, the main ipath remains equal
        // to the home page and that is what gets used to generate
        // the path to each page in the feed data so we have to
        // change it before we apply the layout
        f_snap->set_uri_path(QString("/%1").arg(support_ipath.get_cpath()));

        QDomDocument doc(layout_plugin->create_document(support_ipath, layout_ready));
        layout_plugin->create_body(doc, support_ipath, f_snap_software_description_parser_support_xsl, layout_ptr, false, "snap-software-description-support");

        QDomXPath dom_xpath;
        dom_xpath.setXPath("/snap/page/body/output/snsd-support");
        QDomXPath::node_vector_t publisher_tag(dom_xpath.apply(doc));
        if(publisher_tag.isEmpty())
        {
            SNAP_LOG_FATAL("skipping support as the output of create_body() did not give us the expected tags.");
            continue;
        }

        QString const output(QString("<?xml version=\"1.0\"?>%1").arg(snap_dom::xml_to_string(publisher_tag[0])));

        {
            snap::content::attachment_file attachment(f_snap);

            attachment.set_multiple(false);
            attachment.set_parent_cpath(support_ipath.get_cpath());
            attachment.set_field_name("snap_software_description::support_xml");
            attachment.set_attachment_owner(attachment_plugin->get_plugin_name());
            attachment.set_attachment_type("attachment/public");
            attachment.set_creation_time(start_date);
            attachment.set_update_time(start_date);
            //attachment.set_dependencies(...);
            attachment.set_file_name("support.xml");
            attachment.set_file_filename("support.xml");
            attachment.set_file_creation_time(start_date);
            attachment.set_file_modification_time(start_date);
            attachment.set_file_index(1);
            attachment.set_file_data(output.toUtf8());
            attachment.set_file_mime_type("text/xml");
            attachment.set_revision_limit(3);

            content_plugin->create_attachment(attachment, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, "");
        }

        row->getCell(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_LAST_UPDATE))->setValue(start_date);
    }

    return true;
}


/** \brief Create a catalog.
 *
 * This function is called recursively to create all catalog files
 * for all categories. Note that if a category is considered empty,
 * then it does not get created.
 *
 * The root catalog is saved in /types/snap-software-description
 * with the .xml extension. The other catalogs are saved under
 * each category found under /types/snap-software-description.
 *
 * The software specific XML files are created on various pages
 * throughout the website, but never under /types/snap-software
 * description.
 *
 * The function calls itself as it finds children representing
 * categories, which have to have a catalog. The function takes
 * a depth parameter, which allows it to avoid going too deep
 * in that matter. We actually only allow three levels of
 * categorization. After the third level, we ignore further
 * children.
 *
 * The interface is aware of the maximum number of categorization
 * levels and thus prevents end users from creating more than
 * the allowed number of levels.
 *
 * Note that the maximum number of level is purely for our own
 * sake since there are no real limits to the categorization
 * of a software.
 *
 * The software makes use of the list plugin to create its own
 * lists since the list plugin can do all the work to determine
 * what page is linked with what type, whether the page is
 * publicly available, verify that the page was not deleted,
 * etc. However, a page can only support one list, so it
 * supports the list of files and nothing about the categories.
 * In other words, we are still responsible for the categories.
 *
 * The list saves an item count. We use that parameter to know
 * whether to include a category in our XML files or not. However,
 * the top snap-software-description.xml file is always created.
 *
 * \param[in] ipath  The path of the category to work on.
 * \param[in] depth  The depth at which we currently are working.
 */
bool snap_software_description::create_catalog(content::path_info_t & catalog_ipath, int const depth)
{
    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());
    list::list * list_plugin(list::list::instance());
    attachment::attachment * attachment_plugin(attachment::attachment::instance());
    int64_t const start_date(f_snap->get_start_date());

    QDomDocument doc;
    QDomElement root(doc.createElement("snap"));
    doc.appendChild(root);

    bool has_data(false);

    int const max_files(f_snap_software_description_settings_row->getCell(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_MAX_FILES))->getValue().safeInt64Value(0, 1000));
    list::list_item_vector_t list(list_plugin->read_list(catalog_ipath, 0, max_files));
    int const max_items(list.size());
    for(int idx(0); idx < max_items; ++idx)
    {
        content::path_info_t file_ipath;
        file_ipath.set_path(list[idx].get_uri());

        if(!create_file(file_ipath))
        {
            continue;
        }

        has_data = true;

        // add the file to our catalog
        //
        {
            QDomElement file(doc.createElement("file"));
            root.appendChild(file);
            QDomText file_uri(doc.createTextNode(file_ipath.get_key()));
            file.appendChild(file_uri);
        }

        // also get the PADFile files ready
        //
        {
            // plain text list
            f_padmap_txt += QString("%1/padfile.xml\n").arg(file_ipath.get_key());

            // XML list
            QDomElement file(f_padlist_xml.createElement("file"));
            QDomText text(f_padlist_xml.createTextNode(file_ipath.get_key()));
            file.appendChild(text);
            f_padlist_xml.documentElement().appendChild(file);
        }
    }

    // save the resulting XML document

    // if we already are pretty deep, ignore any possible sub-categories
    if(depth < 5)
    {
        links::link_info info(content::get_name(content::name_t::SNAP_NAME_CONTENT_CHILDREN), false, catalog_ipath.get_key(), catalog_ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info child_info;
        while(link_ctxt->next_link(child_info))
        {
            content::path_info_t sub_category_ipath;
            sub_category_ipath.set_path(child_info.key());

            // now manage all sub-categories; if this category
            // and all of its children have no files then we get
            // false as the return value
            //
            if(!create_catalog(sub_category_ipath, depth + 1))
            {
                continue;
            }

            has_data = true;

            // add the sub-category to our list
            //
            QDomElement sub_category(doc.createElement("sub-category"));
            root.appendChild(sub_category);
            QDomText sub_category_uri(doc.createTextNode(sub_category_ipath.get_key()));
            sub_category.appendChild(sub_category_uri);

            libdbproxy::row::pointer_t revision_row(revision_table->getRow(sub_category_ipath.get_revision_key()));
            QString const category_name(revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->getValue().stringValue());
            sub_category.setAttribute("name", snap_dom::remove_tags(category_name));
        }
    }

    // We always create the top-most .xml because otherwise we end up
    // creating links to an inexistant file. Also servers will automatically
    // go to that XML file and expect to find it (instead of getting a 404)
    // and such a file can be empty meaning that no files are available for
    // download.
    //
    if(!has_data
    && depth != 0)
    {
        return false;
    }

    // modified since we last generated that file?
    //
    libdbproxy::row::pointer_t row(content_table->getRow(catalog_ipath.get_key()));
    //int64_t const modified(row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_MODIFIED))->getValue().safeInt64Value(0, 0));
    //int64_t const last_snsd_update(row->getCell(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_LAST_UPDATE))->getValue().safeInt64Value(0, 0));
    //if(last_snsd_update > 0
    //&& (modified == 0 || modified < last_snsd_update))
    //{
    //    continue;
    //}

    {
        if(f_table_of_content_ipath)
        {
            QDomElement tag(doc.createElement("toc"));
            root.appendChild(tag);
            QDomText text(doc.createTextNode(f_table_of_content_ipath->get_key()));
            tag.appendChild(text);
        }

        {
            QDomElement tag(doc.createElement("base_uri"));
            root.appendChild(tag);
            QDomText text(doc.createTextNode(f_snap->get_site_key_with_slash()));
            tag.appendChild(text);
        }

        {
            QDomElement tag(doc.createElement("page_uri"));
            root.appendChild(tag);
            QDomText text(doc.createTextNode(catalog_ipath.get_key()));
            tag.appendChild(text);
        }

        xslt x;
        x.set_xsl(f_snap_software_description_parser_catalog_xsl);
        x.set_document(doc);
        QString const output(QString("<?xml version=\"1.0\"?>%1").arg(x.evaluate_to_string()));

        {
            // the root file is named "snap-software-description.xml"
            // and the files further down are named "catalog.xml"
            //
            QString const filename(depth == 0 ? "snap-software-description.xml" : "catalog.xml");

            snap::content::attachment_file attachment(f_snap);

            attachment.set_multiple(false);
            attachment.set_parent_cpath(catalog_ipath.get_cpath());
            attachment.set_field_name("snap_software_description::catalog_xml");
            attachment.set_attachment_owner(attachment_plugin->get_plugin_name());
            attachment.set_attachment_type("attachment/public");
            attachment.set_creation_time(start_date);
            attachment.set_update_time(start_date);
            //attachment.set_dependencies(...);
            attachment.set_file_name(filename);
            attachment.set_file_filename(filename);
            attachment.set_file_creation_time(start_date);
            attachment.set_file_modification_time(start_date);
            attachment.set_file_index(1);
            attachment.set_file_data(output.toUtf8());
            attachment.set_file_mime_type("text/xml");
            attachment.set_revision_limit(3);

            content_plugin->create_attachment(attachment, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, "");
        }

        row->getCell(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_LAST_UPDATE))->setValue(start_date);
    }

    return true;
}


/** \brief Generate an SNSD file.
 *
 * This function reads the data from a file and generates the corresponding
 * \<snsd-file> XML file.
 *
 * If the file cannot be created, false is returned.
 *
 * If the file is created or already exists, the function returns true.
 *
 * The snsd-file format offers 6 different lengths of teasers.
 *
 * On a regular page, we offer users to enter all of the following
 * data which can be used as the various descriptions in an SNSD file:
 *
 * \li Short Title
 * \li (Normal) Title
 * \li Long Title
 * \li Abstract
 * \li Description
 * \li Body
 *
 * All of these may be used by the Snap Software Description
 * implementation. We could also make use of a teaser (especially
 * for the one of 2,000 characters,) only our teaser mechanism does
 * not (yet) allow us to limit the input by characters, only words.
 *
 * Limitation: There is one major limitation in our current implementation.
 * If you have many files for download on a single page marked as a
 * Snap Software Description File (i.e. categorized with a link to a
 * snap-software-description/category tag,) then you may not be able
 * to properly encompass all the available downloads. The concept, though,
 * functions properly assuming you create something like a .deb or a
 * tarball of your software, then you would have just one file to
 * download.
 *
 * \param[in] file_ipath  The ipath to the file.
 *
 * \return true if the file was created and saved or the file already existed,
 *         false if creating the file failed.
 */
bool snap_software_description::create_file(content::path_info_t & file_ipath)
{
    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    path::path * path_plugin(path::path::instance());
    attachment::attachment * attachment_plugin(attachment::attachment::instance());
    layout::layout * layout_plugin(layout::layout::instance());
    int64_t const start_date(f_snap->get_start_date());

    // modified since we last generated that file?
    //
    libdbproxy::row::pointer_t row(content_table->getRow(file_ipath.get_key()));
    int64_t const modified(row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_MODIFIED))->getValue().safeInt64Value(0, 0));
    int64_t const last_snsd_update(row->getCell(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_LAST_UPDATE))->getValue().safeInt64Value(0, 0));
    if(last_snsd_update > 0
    && modified < last_snsd_update)  // this case includes 'modified == 0'
    {
        // this assumes that the file.xml is just fine...
        //
        return true;
    }

    // only pages that can be handled by layouts are added
    // others are silently ignored (note that only broken
    // pages should fail the following test)
    //
    quiet_error_callback snap_software_description_error_callback(f_snap, true);
    plugins::plugin * layout_ready(path_plugin->get_plugin(file_ipath, snap_software_description_error_callback));
    layout::layout_content * layout_ptr(dynamic_cast<layout::layout_content *>(layout_ready));
    if(!layout_ptr)
    {
        // log the error?
        // this is probably not the role of the snap-software-description
        // implementation...
        //
        return false;
    }

    // since we are a backend, the main ipath remains equal
    // to the home page and that is what gets used to generate
    // the path to each page in the feed data so we have to
    // change it before we apply the layout
    f_snap->set_uri_path(QString("/%1").arg(file_ipath.get_cpath()));

    // Create the Snap Software Description
    //
    {
        QDomDocument doc(layout_plugin->create_document(file_ipath, layout_ready));
        layout_plugin->create_body(doc, file_ipath, f_snap_software_description_parser_file_xsl, layout_ptr, false, "snap-software-description-file-parser");

        QDomXPath dom_xpath;
        dom_xpath.setXPath("/snap/page/body/output/snsd-file");
        QDomXPath::node_vector_t file_tag(dom_xpath.apply(doc));
        if(file_tag.isEmpty())
        {
            SNAP_LOG_FATAL("skipping file as the output of create_body() did not give us the expected tags.");
            return false;
        }

        QString const output(QString("<?xml version=\"1.0\"?>%1").arg(snap_dom::xml_to_string(file_tag[0])));

        {
            snap::content::attachment_file attachment(f_snap);

            attachment.set_multiple(false);
            attachment.set_parent_cpath(file_ipath.get_cpath());
            attachment.set_field_name("snap_software_description::file_xml");
            attachment.set_attachment_owner(attachment_plugin->get_plugin_name());
            attachment.set_attachment_type("attachment/public");
            attachment.set_creation_time(start_date);
            attachment.set_update_time(start_date);
            //attachment.set_dependencies(...);
            attachment.set_file_name("file.xml");
            attachment.set_file_filename("file.xml");
            attachment.set_file_creation_time(start_date);
            attachment.set_file_modification_time(start_date);
            attachment.set_file_index(1);
            attachment.set_file_data(output.toUtf8());
            attachment.set_file_mime_type("text/xml");
            attachment.set_revision_limit(3);

            content_plugin->create_attachment(attachment, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, "");
        }
    }

    // save the last update before saving the PADFile so if saving
    // the PADFile fails, it just gets ignored
    //
    row->getCell(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_LAST_UPDATE))->setValue(start_date);

    // Create the PADFile
    //
    {
        QDomDocument doc(layout_plugin->create_document(file_ipath, layout_ready));
        layout_plugin->create_body(doc, file_ipath, f_padfile_xsl, layout_ptr, false, "padfile-parser");

        QDomXPath dom_xpath;
        dom_xpath.setXPath("/snap/page/body/output/XML_DIZ_INFO");
        QDomXPath::node_vector_t padfile_tag(dom_xpath.apply(doc));
        if(padfile_tag.isEmpty())
        {
            SNAP_LOG_FATAL("skipping PAD file as the output of create_body() did not give us the expected tags.");

            // The PADFile is not considered important so we don't return
            // false in this case
            //return false;
        }
        else
        {
            QString const output(QString("<?xml version=\"1.0\"?>%1").arg(snap_dom::xml_to_string(padfile_tag[0])));

            snap::content::attachment_file attachment(f_snap);

            attachment.set_multiple(false);
            attachment.set_parent_cpath(file_ipath.get_cpath());
            attachment.set_field_name("snap_software_description::padfile_xml");
            attachment.set_attachment_owner(attachment_plugin->get_plugin_name());
            attachment.set_attachment_type("attachment/public");
            attachment.set_creation_time(start_date);
            attachment.set_update_time(start_date);
            //attachment.set_dependencies(...);
            attachment.set_file_name("padfile.xml");
            attachment.set_file_filename("padfile.xml");
            attachment.set_file_creation_time(start_date);
            attachment.set_file_modification_time(start_date);
            attachment.set_file_index(1);
            attachment.set_file_data(output.toUtf8());
            attachment.set_file_mime_type("text/xml");
            attachment.set_revision_limit(3);

            content_plugin->create_attachment(attachment, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, "");
        }
    }

    return true;
}


/** \brief Load an XSL file.
 *
 * The Snap Software Description backend makes use of many XSL files to
 * transform the data available in a page to an actual Snap Software
 * Description file.
 *
 * This function loads one of the XSL files. It will also apply any
 * \<xsl:include ...> it finds in that file.
 *
 * If the function cannot load the file, false is returned and a fatal
 * error message is sent to the logger.
 *
 * Note that the \p xsl parameter is expected to be a field in this object.
 * That way it gets loaded just once even if you call the function many times.
 *
 * \param[in] filename  The resource filename to load the XSL file.
 * \param[out] xsl  The variable holding the XSL data.
 *
 * \return true if the load succeeded, false otherwise.
 */
bool snap_software_description::load_xsl_file(QString const & filename, QString & xsl)
{
    if(xsl.isEmpty())
    {
        QFile file(filename);
        if(!file.open(QIODevice::ReadOnly))
        {
            SNAP_LOG_FATAL("snap_software_description::load_xsl_file() could not open the \"")
                          (filename)
                          ("\" resource file.");
            return false;
        }
        QByteArray data(file.readAll());
        xsl = QString::fromUtf8(data.data(), data.size());
        if(xsl.isEmpty())
        {
            SNAP_LOG_FATAL("snap_software_description::load_xsl_file() could not read the \"")
                          (filename)
                          ("\" resource file.");
            return false;
        }

        // replace <xsl:include ...> with other XSLT files (should be done
        // by the parser, but Qt's parser does not support it yet)
        layout::layout * layout_plugin(layout::layout::instance());
        layout_plugin->replace_includes(xsl);
    }

    return true;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
