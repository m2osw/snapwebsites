// Snap Websites Server -- different feed handlers (RSS, Atom, RSS_Cloud, PubSubHubbub, etc.)
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

#include "feed.h"

#include "../attachment/attachment.h"
#include "../filter/filter.h"
#include "../list/list.h"
#include "../locale/snap_locale.h"
#include "../path/path.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qdomxpath.h>
#include <snapwebsites/qhtmlserializer.h>
#include <snapwebsites/qxmlmessagehandler.h>
#include <snapwebsites/xslt.h>

#include <QFile>
#include <QTextStream>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(feed, 1, 0)


/** \brief Get a fixed feed name.
 *
 * The feed plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_FEED_ADMIN_SETTINGS:
        return "admin/settings/feed";

    case name_t::SNAP_NAME_FEED_AGE:
        return "feed::age";

    case name_t::SNAP_NAME_FEED_ATTACHMENT_TYPE:
        return "types/taxonomy/system/content-types/feed/attachment";

    case name_t::SNAP_NAME_FEED_DESCRIPTION:
        return "feed::description";

    case name_t::SNAP_NAME_FEED_EXTENSION:
        return "feed::extension";

    case name_t::SNAP_NAME_FEED_MIMETYPE:
        return "feed::mimetype";

    case name_t::SNAP_NAME_FEED_PAGE_LAYOUT:
        return "feed::page_layout";

    case name_t::SNAP_NAME_FEED_SETTINGS_ALLOW_MAIN_ATOM_XML:
        return "feed::allow_main_atom_xml";

    case name_t::SNAP_NAME_FEED_SETTINGS_ALLOW_MAIN_RSS_XML:
        return "feed::allow_main_rss_xml";

    case name_t::SNAP_NAME_FEED_SETTINGS_DEFAULT_LOGO:
        return "feed::default_logo";

    case name_t::SNAP_NAME_FEED_SETTINGS_PATH:
        return "admin/settings/feed";

    case name_t::SNAP_NAME_FEED_SETTINGS_TEASER_END_MARKER:
        return "feed::teaser_end_marker";

    case name_t::SNAP_NAME_FEED_SETTINGS_TEASER_TAGS:
        return "feed::teaser_tags";

    case name_t::SNAP_NAME_FEED_SETTINGS_TEASER_WORDS:
        return "feed::teaser_words";

    case name_t::SNAP_NAME_FEED_SETTINGS_TOP_MAXIMUM_NUMBER_OF_ITEMS_IN_ANY_FEED:
        return "feed::top_maximum_number_of_items_in_any_feed";

    case name_t::SNAP_NAME_FEED_TITLE:
        return "feed::title";

    case name_t::SNAP_NAME_FEED_TTL:
        return "feed::ttl";

    case name_t::SNAP_NAME_FEED_TYPE:
        return "feed::type";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_FEED_...");

    }
    NOTREACHED();
}


/** \brief Initialize the feed plugin.
 *
 * This function is used to initialize the feed plugin object.
 */
feed::feed()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the feed plugin.
 *
 * Ensure the feed object is clean before it is gone.
 */
feed::~feed()
{
}


/** \brief Get a pointer to the feed plugin.
 *
 * This function returns an instance pointer to the feed plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the feed plugin.
 */
feed *feed::instance()
{
    return g_plugin_feed_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString feed::settings_path() const
{
    return "/admin/settings/feed";
}


/** \brief A path or URI to a logo for the feed system.
 *
 * This function returns a 64x64 icons representing the feed plugin.
 *
 * \return A path to the feed logo.
 */
QString feed::icon() const
{
    return "/images/feed/feed-logo-64x64.png";
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
QString feed::description() const
{
    return "System used to generate RSS, Atom and other feeds. It also"
          " handles subscriptions for subscription based feed systems"
          " such as RSS Cloud and PubSubHubbub.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString feed::dependencies() const
{
    return "|editor|layout|messages|output|users|";
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
int64_t feed::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 2, 5, 16, 38, 42, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update
 *                                 (in micro-seconds).
 */
void feed::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the feed.
 *
 * This function terminates the initialization of the feed plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void feed::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN0(feed, "server", server, backend_process);
    SNAP_LISTEN(feed, "layout", layout::layout, generate_page_content, _1, _2, _3);
}


/** \brief Generate links in the header.
 *
 * This function generates one alternate link per feed made available.
 *
 * \todo
 * In the generate_header_content(), we should add a link of type "self"
 * which references the atom feed.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void feed::on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    NOTUSED(page);

    // avoid those links on administrative pages, totally useless!
    if(ipath.get_cpath().startsWith("admin/"))
    {
        return;
    }

    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());

    content::path_info_t attachment_type_ipath;
    attachment_type_ipath.set_path(get_name(name_t::SNAP_NAME_FEED_ATTACHMENT_TYPE));
    links::link_info feed_info(get_name(name_t::SNAP_NAME_FEED_TYPE), false, attachment_type_ipath.get_key(), attachment_type_ipath.get_branch());
    QSharedPointer<links::link_context> feed_ctxt(links::links::instance()->new_link_context(feed_info));
    links::link_info feed_child_info;
    while(feed_ctxt->next_link(feed_child_info))
    {
        content::path_info_t attachment_ipath;
        attachment_ipath.set_path(feed_child_info.key());

        QtCassandra::QCassandraRow::pointer_t row(revision_table->row(attachment_ipath.get_revision_key()));
        QString const mimetype(row->cell(get_name(name_t::SNAP_NAME_FEED_MIMETYPE))->value().stringValue());

        FIELD_SEARCH
            (content::field_search::command_t::COMMAND_MODE, content::field_search::mode_t::SEARCH_MODE_EACH)
            (content::field_search::command_t::COMMAND_ELEMENT, body)
            (content::field_search::command_t::COMMAND_PATH_INFO_REVISION, attachment_ipath)

            (content::field_search::command_t::COMMAND_FIELD_NAME, get_name(name_t::SNAP_NAME_FEED_TITLE))
            (content::field_search::command_t::COMMAND_SELF)
            (content::field_search::command_t::COMMAND_SAVE, QString("formats[href=\"%1\"][type=\"%2\"]").arg(attachment_ipath.get_key()).arg(mimetype))

            // generate
            ;
    }
}


/** \brief Make copies of attachments as required.
 *
 * The Feed plugin allows users to define a different path for their
 * various feeds. This function saves those files in different
 * locations.
 *
 * \param[in] ipath  The path to the feed settings.
 * \param[in] succeeded  Whether the save succeeded.
 */
void feed::on_finish_editor_form_processing(content::path_info_t & ipath, bool & succeeded)
{
    if(!succeeded
    || ipath.get_cpath() != "admin/settings/feed")
    {
        return;
    }

    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    QtCassandra::QCassandraRow::pointer_t settings_row(revision_table->row(ipath.get_revision_key()));

    QtCassandra::QCassandraValue value;

    if(!settings_row->cell(get_name(name_t::SNAP_NAME_FEED_SETTINGS_ALLOW_MAIN_RSS_XML))->value().safeSignedCharValue(0, 0))
    {
        // if this one is off, then make sure the file is deleted if it exists
        content::path_info_t rss_xml_ipath;
        rss_xml_ipath.set_path("rss.xml");
        content_plugin->trash_page(rss_xml_ipath);
    }

    if(!settings_row->cell(get_name(name_t::SNAP_NAME_FEED_SETTINGS_ALLOW_MAIN_ATOM_XML))->value().safeSignedCharValue(0, 0))
    {
        // if this one is off, then make sure the file is deleted if it exists
        content::path_info_t atom_xml_ipath;
        atom_xml_ipath.set_path("atom.xml");
        content_plugin->trash_page(atom_xml_ipath);
    }
}


/** \brief Implementation of the backend process signal.
 *
 * This function captures the backend processing signal which is sent
 * by the server whenever the backend tool is run against a site.
 *
 * The feed plugin generates XML files with the list of
 * pages that are saved in various lists defined under /feed.
 * By default we offer the /feed/main list which presents all the
 * public pages marked as a feed using the feed::feed tag named
 * /types/taxonomy/system/content-types/feed/main
 */
void feed::on_backend_process()
{
    snap_uri const & main_uri(f_snap->get_uri());
    SNAP_LOG_TRACE("backend_process: process feed.rss content for \"")(main_uri.get_uri())("\".");

    generate_feeds();
}


/** \brief Generate all the feeds.
 *
 * This function goes through the list of feeds defined under /feed and
 * generates an XML document with the complete list of pages found in
 * each feed. The XML document is then parsed through the various feed
 * XSLT transformation stylesheets to generate the final output (RSS,
 * Atom, etc.)
 */
void feed::generate_feeds()
{
    content::content * content_plugin(content::content::instance());
    layout::layout * layout_plugin(layout::layout::instance());
    path::path * path_plugin(path::path::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());

    // the children of this location are the XSLT 2.0 files to convert the
    // data to an actual feed file
    content::path_info_t admin_feed_ipath;
    admin_feed_ipath.set_path(get_name(name_t::SNAP_NAME_FEED_ADMIN_SETTINGS));
    QVector<QString> feed_formats;

    int64_t const start_date(f_snap->get_start_date());

    content::path_info_t feed_settings_ipath;
    feed_settings_ipath.set_path(get_name(name_t::SNAP_NAME_FEED_SETTINGS_PATH));
    QtCassandra::QCassandraRow::pointer_t feed_settings_row(revision_table->row(feed_settings_ipath.get_revision_key()));

    // TODO: if a feed has its own definitions for the Teaser Words, Tags,
    //       End Marker, then use the per feed definitions...
    //       (And below the end marker URI and title--and whether to use
    //       that anchor.)
    //
    filter::filter::filter_teaser_info_t teaser_info;
    teaser_info.set_max_words (feed_settings_row->cell(get_name(name_t::SNAP_NAME_FEED_SETTINGS_TEASER_WORDS     ))->value().safeInt64Value(0, DEFAULT_TEASER_WORDS));
    teaser_info.set_max_tags  (feed_settings_row->cell(get_name(name_t::SNAP_NAME_FEED_SETTINGS_TEASER_TAGS      ))->value().safeInt64Value(0, DEFAULT_TEASER_TAGS));
    teaser_info.set_end_marker(feed_settings_row->cell(get_name(name_t::SNAP_NAME_FEED_SETTINGS_TEASER_END_MARKER))->value().stringValue());

    QString default_logo(feed_settings_row->cell(get_name(name_t::SNAP_NAME_FEED_SETTINGS_DEFAULT_LOGO))->value().stringValue());
    int64_t const top_max_items(feed_settings_row->cell(get_name(name_t::SNAP_NAME_FEED_SETTINGS_TOP_MAXIMUM_NUMBER_OF_ITEMS_IN_ANY_FEED))->value().safeInt64Value(0, 100));

    // first loop through the list of feeds defined under /feed
    content::path_info_t ipath;
    ipath.set_path("feed");
    if(!content_table->exists(ipath.get_key())
    || !content_table->row(ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
    {
        return;
    }
    links::link_info info(content::get_name(content::name_t::SNAP_NAME_CONTENT_CHILDREN), false, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info child_info;
    while(link_ctxt->next_link(child_info))
    {
        // this path is to a list of pages for a specific feed
        content::path_info_t child_ipath;
        child_ipath.set_path(child_info.key());

        QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(child_ipath.get_revision_key()));

        // TODO: is the page layout directly a feed XSL file or is it
        //       the name to an attachment? (or maybe we should just check
        //       for a specifically named attachment?)
        QString feed_parser_layout(revision_row->cell(get_name(name_t::SNAP_NAME_FEED_PAGE_LAYOUT))->value().stringValue());
        if(feed_parser_layout.isEmpty())
        {
            // already loaded?
            if(f_feed_parser_xsl.isEmpty())
            {
                QFile file(":/xsl/layout/feed-parser.xsl");
                if(!file.open(QIODevice::ReadOnly))
                {
                    SNAP_LOG_FATAL("feed::generate_feeds() could not open the feed-parser.xsl resource file.");
                    return;
                }
                QByteArray data(file.readAll());
                f_feed_parser_xsl = QString::fromUtf8(data.data(), data.size());
                if(f_feed_parser_xsl.isEmpty())
                {
                    SNAP_LOG_FATAL("feed::generate_feeds() could not read the feed-parser.xsl resource file.");
                    return;
                }
            }
            feed_parser_layout = f_feed_parser_xsl;
        }
        // else -- so? load from an attachment? (TBD)

        // replace <xsl:include ...> with other XSLT files (should be done
        // by the parser, but Qt's parser does not support it yet)
        layout_plugin->replace_includes(feed_parser_layout);

        // get the list, we expect that all the feed lists are ordered by
        // creation or publication date of the page as expected by the
        // various feed APIs
        //
        // TODO: fix the max. # of entries to make use of a user defined
        //       setting for that specific feed (instead of 100).
        //
        int64_t const feed_max_items(100);

        list::list * list_plugin(list::list::instance());
        list::list_item_vector_t list(list_plugin->read_list(child_ipath, 0, std::min(top_max_items, feed_max_items)));
        bool first(true);
        QDomDocument result;
        int const max_items(list.size());
        for(int i(0); i < max_items; ++i)
        {
            content::path_info_t page_ipath;
            page_ipath.set_path(list[i].get_uri());

            // only pages that can be handled by layouts are added
            // others are silently ignored
            quiet_error_callback feed_error_callback(f_snap, true);
            plugins::plugin * layout_ready(path_plugin->get_plugin(page_ipath, feed_error_callback));
            layout::layout_content * layout_ptr(dynamic_cast<layout::layout_content *>(layout_ready));
            if(layout_ptr)
            {
                // since we are a backend, the main ipath remains equal
                // to the home page and that is what gets used to generate
                // the path to each page in the feed data so we have to
                // change it before we apply the layout
                f_snap->set_uri_path(QString("/%1").arg(page_ipath.get_cpath()));

                QDomDocument doc(layout_plugin->create_document(page_ipath, layout_ready));
                layout_plugin->create_body(doc, page_ipath, feed_parser_layout, layout_ptr, false, "feed-parser");

                QDomNodeList long_dates(doc.elementsByTagName("created-long-date"));
                int const max_long_dates(long_dates.size());
                for(int idx(0); idx < max_long_dates; ++idx)
                {
                    QDomNode short_date(long_dates.at(idx));
                    QDomElement long_date_element(short_date.toElement());
                    time_t const date(f_snap->string_to_date(long_date_element.text()));
                    struct tm t;
                    localtime_r(&date, &t);
                    char date2822[256];
                    strftime(date2822, sizeof(date2822), "%a, %d %b %Y %T %z", &t);
                    for(;;)
                    {
                        QDomNode child(long_date_element.firstChild());
                        if(child.isNull())
                        {
                            break;
                        }
                        long_date_element.removeChild(child);
                    }
                    snap_dom::append_plain_text_to_node(long_date_element, date2822);
                }

                // generate the teaser
                if(teaser_info.get_max_words() > 0)
                {
                    QDomElement output_description(snap_dom::get_child_element(doc, "snap/page/body/output/description"));
                    // do not create a link, often those are removed in some
                    // weird way; readers will make the title a link anyway
                    //teaser_info.set_end_marker_uri(page_ipath.get_key(), "Click to read the full article.");
                    filter::filter::body_to_teaser(output_description, teaser_info);
                }

                if(first)
                {
                    first = false;
                    result = doc;
                }
                else
                {
                    // only keep the output of further pages
                    // (the header should be the same, except for a few things
                    // such as the path and data extracted from the main page,
                    // which should not be used in the feed...)
                    QDomElement output(snap_dom::get_child_element(doc, "snap/page/body/output"));
                    QDomElement body(snap_dom::get_child_element(result, "snap/page/body"));
                    body.appendChild(output);
                }
            }
            //else -- log the error?
        }

        // only create the feed output if data was added to the result
        if(!first)
        {
            locale::locale * locale_plugin(locale::locale::instance());
            locale_plugin->set_timezone();
            locale_plugin->set_locale();

            QDomElement metadata_tag(snap_dom::get_child_element(result, "snap/head/metadata"));

            // /snap/head/metadata/desc[@type="description"]/data
            // (only if still undefined)
            //
            // avoid adding the description from the feed description
            // if the website description was already added...
            QDomXPath dom_xpath;
            dom_xpath.setXPath("/snap/head/metadata/desc[@type='description']/data");
            QDomXPath::node_vector_t current_description(dom_xpath.apply(result));
            if(current_description.isEmpty())
            {
                QString const feed_description(revision_row->cell(get_name(name_t::SNAP_NAME_FEED_DESCRIPTION))->value().stringValue());
                QDomElement desc(result.createElement("desc"));
                metadata_tag.appendChild(desc);
                desc.setAttribute("type", "description");
                QDomElement data(result.createElement("data"));
                desc.appendChild(data);
                snap_dom::insert_html_string_to_xml_doc(data, feed_description);
            }

            // /snap/head/metadata/desc[@type="feed::uri"]/data
            {
                QDomElement desc(result.createElement("desc"));
                metadata_tag.appendChild(desc);
                desc.setAttribute("type", "feed::uri");
                QDomElement data(result.createElement("data"));
                desc.appendChild(data);
                QDomText date_text(result.createTextNode(child_ipath.get_key()));
                data.appendChild(date_text);
            }

            // /snap/head/metadata/desc[@type="feed::name"]/data
            QString name(child_ipath.get_key());
            {
                int pos(name.lastIndexOf('/'));
                if(pos > 0)
                {
                    name = name.mid(pos + 1);
                }
                pos = name.lastIndexOf('.');
                if(pos > 0)
                {
                    name = name.mid(0, pos);
                }
                QDomElement desc(result.createElement("desc"));
                metadata_tag.appendChild(desc);
                desc.setAttribute("type", "feed::name");
                QDomElement data(result.createElement("data"));
                desc.appendChild(data);
                QDomText date_text(result.createTextNode(name));
                data.appendChild(date_text);
            }

            // /snap/head/metadata/desc[@type="feed::now"]/data
            // /snap/head/metadata/desc[@type="feed::now-long-date"]/data
            //
            // for lastBuildDate
            {
                time_t now(time(nullptr));
                struct tm t;
                localtime_r(&now, &t);

                // for Atom
                // /snap/head/metadata/desc[@type="feed::now"]/data/...
                {
                    char date3339[256];
                    strftime(date3339, sizeof(date3339), "%Y-%m-%dT%H:%M:%S%z", &t);
                    size_t const len(strlen(date3339));
                    if(len > 3 && len < sizeof(date3339) - 2)
                    {
                        // add the missing colon between HH and MM in the timezone
                        date3339[len + 1] = '\0';
                        date3339[len] = date3339[len - 1];
                        date3339[len - 1] = date3339[len - 2];
                        date3339[len - 2] = ':';
                    }
                    QDomElement desc(result.createElement("desc"));
                    metadata_tag.appendChild(desc);
                    desc.setAttribute("type", "feed::now");
                    QDomElement data(result.createElement("data"));
                    desc.appendChild(data);
                    QDomText date_text(result.createTextNode(QString::fromUtf8(date3339)));
                    data.appendChild(date_text);
                }

                // for RSS
                // /snap/head/metadata/desc[@type="feed::now-long-date"]/data/...
                {
                    char date2822[256];
                    strftime(date2822, sizeof(date2822), "%a, %d %b %Y %T %z", &t);
                    QDomElement desc(result.createElement("desc"));
                    metadata_tag.appendChild(desc);
                    desc.setAttribute("type", "feed::now-long-date");
                    QDomElement data(result.createElement("data"));
                    desc.appendChild(data);
                    QDomText date_text(result.createTextNode(QString::fromUtf8(date2822)));
                    data.appendChild(date_text);
                }

                // the feed image/logo/icon
                // /snap/head/metadata/desc[@type="feed::default_logo"]/data/img[@src=...][@width=...][@height=...]
                if(!default_logo.isEmpty())
                {
                    // the default_logo often comes with a src="..." which
                    // is not a full URL, make sure it is
                    //
                    int end(-1);
                    int pos(default_logo.indexOf("src=\""));
                    if(pos == -1)
                    {
                        pos = default_logo.indexOf("src='");
                        if(pos != -1)
                        {
                            pos += 5;
                            end = default_logo.indexOf("'", pos);
                        }
                    }
                    else
                    {
                        pos += 5;
                        end = default_logo.indexOf("\"", pos);
                    }
                    if(pos != -1)
                    {
                        // make sure this is a full URL
                        content::path_info_t logo_ipath;
                        logo_ipath.set_path(default_logo.mid(pos, end - pos));
                        default_logo.replace(pos, end - pos, logo_ipath.get_key());
                    }

                    QDomElement desc(result.createElement("desc"));
                    metadata_tag.appendChild(desc);
                    desc.setAttribute("type", "feed::default_logo");
                    QDomElement data(result.createElement("data"));
                    desc.appendChild(data);
                    snap_dom::insert_html_string_to_xml_doc(data, default_logo);
                }
            }

            {
                QtCassandra::QCassandraValue const ttl(revision_row->cell(get_name(name_t::SNAP_NAME_FEED_TTL))->value());
                if(ttl.size() == sizeof(int64_t))
                {
                    int64_t const ttl_us(ttl.int64Value());
                    if(ttl_us >= 3600000000) // we force at least 1h
                    {
                        QDomElement desc(result.createElement("desc"));
                        metadata_tag.appendChild(desc);
                        desc.setAttribute("type", "ttl");
                        QDomElement data(result.createElement("data"));
                        desc.appendChild(data);
                        // convert ttl from microseconds to minutes
                        // (1,000,000 microseconds/second x 60 seconds/minute)
                        QDomText ttl_text(result.createTextNode(QString("%1").arg(ttl_us / (1000000 * 60))));
                        data.appendChild(ttl_text);
                    }
                }
            }

            // do this one instead of giving 'result' to XSLT which
            // would convert the document to string once per format!
            QString const doc_str(result.toString(-1));

            // formats loaded yet?
            if(feed_formats.isEmpty())
            {
                links::link_info feed_info(content::get_name(content::name_t::SNAP_NAME_CONTENT_CHILDREN), false, admin_feed_ipath.get_key(), admin_feed_ipath.get_branch());
                QSharedPointer<links::link_context> feed_link_ctxt(links::links::instance()->new_link_context(feed_info));
                links::link_info feed_child_info;
                while(feed_link_ctxt->next_link(feed_child_info))
                {
                    // this path is to a list of pages for a specific feed
                    //content::path_info_t child_ipath;
                    //child_ipath.set_path(feed_child_info.key());
                    //QString const cpath(child_ipath.get_cpath());
                    QString const key(feed_child_info.key());
                    int const pos(key.lastIndexOf("."));
                    QString const extension(key.mid(pos + 1));
                    if(extension == "xsl")
                    {
                        snap_child::post_file_t feed_xsl;
                        feed_xsl.set_filename("attachment:" + key);
                        if(f_snap->load_file(feed_xsl))
                        {
                            // got valid attachment!
                            QByteArray data(feed_xsl.get_data());
                            feed_formats.push_back(QString::fromUtf8(data.data()));
                        }
                        else
                        {
                            SNAP_LOG_WARNING("failed loading \"")(key)("\" as one of the feed formats.");
                        }
                    }
                }
            }

            // now generate the actual output (RSS, Atom, etc.)
            // from the data we just gathered
            bool success(true);
            int const max_formats(feed_formats.size());
            for(int i(0); i < max_formats; ++i)
            {
                xslt x;
                x.set_xsl(feed_formats[i]);
                x.set_document(doc_str); // keep doc_str so we convert the document only once
                QDomDocument feed_result("feed");
                x.evaluate_to_document(feed_result);

                QDomXPath feed_dom_xpath;
                {
                    feed_dom_xpath.setXPath("//*[@ns]");
                    QDomXPath::node_vector_t ns_tags(feed_dom_xpath.apply(feed_result));
                    int const max_ns(ns_tags.size());
                    for(int j(0); j < max_ns; ++j)
                    {
                        // we found the widget, display its label instead
                        QDomElement e(ns_tags[j].toElement());
                        QString const ns(e.attribute("ns"));
                        snap_string_list const ns_name_value(ns.split("="));
                        e.removeAttribute("ns");
                        if(ns_name_value.size() == 2)
                        {
                            e.setAttribute(ns_name_value[0], ns_name_value[1]);
                        }
                        else
                        {
                            SNAP_LOG_ERROR("invalid namespace (")(ns)(") specification in feed");
                        }
                    }
                }

                {
                    feed_dom_xpath.setXPath("/feed/entry/content");
                    QDomXPath::node_vector_t content_tags(feed_dom_xpath.apply(feed_result));
                    int const max_content(content_tags.size());
                    for(int j(0); j < max_content; ++j)
                    {
                        QDomElement e(content_tags[j].toElement());

                        // make sure the lang attribute is correct
                        QString const lang(e.attribute("xml_lang"));
                        e.removeAttribute("xml_lang");
                        if(!lang.isEmpty())
                        {
                            // Somehow the NS does not want to work...
                            //e.setAttributeNS("http://www.w3.org/2000/xml", "lang", base);
                            e.setAttribute("xml:lang", lang);
                        }

                        // make sure the base attribute is correct
                        QString const base(e.attribute("base"));
                        e.removeAttribute("base");
                        if(!base.isEmpty())
                        {
                            // Somehow the NS does not want to work...
                            //e.setAttributeNS("http://www.w3.org/2000/xml", "base", base);
                            e.setAttribute("xml:base", base);
                        }
                    }
                }

                {
                    feed_dom_xpath.setXPath("//*[@feed-cdata = 'yes']");
                    QDomXPath::node_vector_t feed_cdata_tags(feed_dom_xpath.apply(feed_result));
                    int const max_feed_cdata(feed_cdata_tags.size());
                    for(int j(0); j < max_feed_cdata; ++j)
                    {
                        // we found the widget, display its label instead
                        QDomElement e(feed_cdata_tags[j].toElement());
                        e.removeAttribute("feed-cdata");
                        // print the children as text to a buffer
                        QString buffer;
                        QTextStream stream(&buffer);
                        stream.setCodec("UTF-8");
                        // write the children to the buffer and then remove them
                        while(e.hasChildNodes())
                        {
                            e.firstChild().save(stream, 0);
                            e.removeChild(e.firstChild());
                        }
                        // reinject the children as a CDATA section if not empty
                        if(!buffer.isEmpty())
                        {
                            QDomCDATASection cdata_section(e.ownerDocument().createCDATASection(buffer));
                            e.appendChild(cdata_section);
                        }
                    }
                }

                // also get the snap complementary information
                QString title("No Title"); // TODO: translation
                QString extension;
                QString mimetype;
                {
                    feed_dom_xpath.setXPath("//snap-info");
                    QDomXPath::node_vector_t snap_info_tags(feed_dom_xpath.apply(feed_result));
                    if(snap_info_tags.size() != 1)
                    {
                        SNAP_LOG_ERROR("any feed XSLT 2.0 file must include a snap-info tag with various details about the output file.");
                        success = false;
                    }
                    else
                    {
                        // get the tag and remove it from the tree
                        // (we do not want it in the output)
                        QDomElement e(snap_info_tags[0].toElement());
                        e.parentNode().removeChild(e);

                        extension = e.attribute("extension");
                        mimetype = e.attribute("mimetype");

                        QDomElement title_tag(e.firstChildElement("title"));
                        if(!title_tag.isNull())
                        {
                            title = title_tag.text();
                        }
                    }
                }

//std::cout << "***\n*** SRC = [" << doc_str << "]\n";
//std::cout << "*** DOC = [" << feed_result.toString(-1) << "]\n***\n";

                if(success)
                {
                    snap::content::attachment_file attachment(f_snap);

                    attachment.set_multiple(false);
                    attachment.set_parent_cpath(child_ipath.get_cpath());
                    attachment.set_field_name(QString("feed::%1").arg(extension));
                    attachment.set_attachment_owner(attachment::attachment::instance()->get_plugin_name());
                    attachment.set_attachment_type("attachment/public");
                    attachment.set_creation_time(start_date);
                    attachment.set_update_time(start_date);
                    //attachment.set_dependencies(...);
                    attachment.set_file_name(QString("%1.%2").arg(name).arg(extension));
                    attachment.set_file_filename(QString("%1.%2").arg(name).arg(extension));
                    attachment.set_file_creation_time(start_date);
                    attachment.set_file_modification_time(start_date);
                    attachment.set_file_index(1);
                    attachment.set_file_data(feed_result.toString(-1).toUtf8());
                    attachment.set_file_mime_type(mimetype);
                    attachment.set_revision_limit(3);

                    // TODO: we probably want to test the "return value"
                    content_plugin->create_attachment(attachment, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, "");

                    {
                        content::path_info_t attachment_ipath;
                        attachment_ipath.set_path(attachment.get_attachment_cpath());

                        QtCassandra::QCassandraRow::pointer_t attachment_row(revision_table->row(attachment_ipath.get_revision_key()));

                        attachment_row->cell(get_name(name_t::SNAP_NAME_FEED_TITLE))->setValue(title);
                        attachment_row->cell(get_name(name_t::SNAP_NAME_FEED_EXTENSION))->setValue(extension);
                        attachment_row->cell(get_name(name_t::SNAP_NAME_FEED_MIMETYPE))->setValue(mimetype);
                    }

                    mark_attachment_as_feed(attachment);

                    // TODO: this is to support the system main.rss -> rss.xml
                    //       but this should be more much more friendly instead
                    //       of a hack like this...
                    //
                    if(attachment.get_attachment_cpath() == "feed/main/main.rss")
                    {
                        int8_t const rss_xml(feed_settings_row->cell(get_name(name_t::SNAP_NAME_FEED_SETTINGS_ALLOW_MAIN_RSS_XML))->value().safeSignedCharValue(0, 0));
                        if(rss_xml)
                        {
                            // change filename to "/rss.xml"
                            attachment.set_parent_cpath("");
                            attachment.set_file_name("rss.xml");
                            attachment.set_file_filename("rss.xml");
                            content_plugin->create_attachment(attachment, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, "");
                            mark_attachment_as_feed(attachment);
                        }
                    }
                    else if(attachment.get_attachment_cpath() == "feed/main/main.atom")
                    {
                        int8_t const atom_xml(feed_settings_row->cell(get_name(name_t::SNAP_NAME_FEED_SETTINGS_ALLOW_MAIN_ATOM_XML))->value().safeSignedCharValue(0, 0));
                        if(atom_xml)
                        {
                            // change filename to "/atom.xml"
                            attachment.set_parent_cpath("");
                            attachment.set_file_name("atom.xml");
                            attachment.set_file_filename("atom.xml");
                            content_plugin->create_attachment(attachment, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, "");
                            mark_attachment_as_feed(attachment);
                        }
                    }
                }
            }
        }
    }

    // just in case, reset the main URI
    f_snap->set_uri_path("/");
}


/** \brief Mark the attachment (Feed data) as such.
 *
 * Since we allow users to save copies of various feeds in other
 * places, we have a separate function to create the necessary
 * links against the attachment files once saved.
 *
 * \param[in] attachment  The attachment that was just created.
 */
void feed::mark_attachment_as_feed(snap::content::attachment_file & attachment)
{
    content::path_info_t attachment_ipath;
    attachment_ipath.set_path(attachment.get_attachment_cpath());

    content::path_info_t type_ipath;
    type_ipath.set_path(get_name(name_t::SNAP_NAME_FEED_ATTACHMENT_TYPE));

    QString const link_name(get_name(name_t::SNAP_NAME_FEED_TYPE));
    bool const source_unique(true);
    bool const destination_unique(false);
    links::link_info source(link_name, source_unique, attachment_ipath.get_key(), attachment_ipath.get_branch());
    links::link_info destination(link_name, destination_unique, type_ipath.get_key(), type_ipath.get_branch());
    links::links::instance()->create_link(source, destination);
}






//
// Google PubSubHubHub documentation:
// https://pubsubhubbub.googlecode.com/git/pubsubhubbub-core-0.4.html
//
// RSS documentation:
// http://www.rssboard.org/rss-specification (2.x)
// http://web.resource.org/rss/1.0/
// http://www.rssboard.org/rss-0-9-1-netscape
// http://www.rssboard.org/rss-0-9-0
//
// Atom Documentation:
// https://tools.ietf.org/html/rfc4287#section-4.2.13
//
// RSS/Atom Verification by W3C
// http://validator.w3.org/feed/
// 

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
