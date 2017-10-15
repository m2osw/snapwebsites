// Snap Websites Server -- short URL handling
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

/** \file
 *
 * The shorturl plugin is used to generate URIs that are as short as
 * possible for any one page you create on your website. These short
 * URIs are first created on the site itself using a counter. The counter
 * number is used in base 36 and appended after the /s path. So in
 * effect you get shorten paths such as /s/123.
 *
 * The shorturl is then presented to clients in the HTML header and the
 * HTTP header. Because we can only present one such short URL per page,
 * the website administrator has to choose one single shortener and stick
 * to it. (After installation shorturl is disabled, the user can enable
 * it only after he selected which type of shortener he wants to use.)
 *
 * The plugin is expected to also create a set of short URIs using
 * external systems such as TinyURL and goo.gl.
 *
 * \li http://TinyURL.com/
 *
 * API for TinyURL.com is as follow (shortening http://linux.m2osw.com/zmeu-attack)
 * wget -S 'http://tinyurl.com/api-create.php?url=http%3A%2F%2Flinux.m2osw.com%2Fzmeu-attack'
 *
 * (this may not be available anymore? it may have been abused...)
 *
 * \li https://developers.google.com/url-shortener/?hl=en
 *
 * API for goo.gl uses a secure log in (OAuth2) and so will require us
 * to offer an interface to enter that information.
 *
 * \li facebook?
 *
 * I'm not sure whether facebook shortener can be used for 3rd party websites.
 *
 * \li twitter
 *
 * The twitter shortener should also be implemented at some point.
 *
 * \li others?
 *
 * Others? (many are appearing and dying so it is somewhat dangerous
 * to just choose one or another.)
 */

#include "shorturl.h"

#include "../output/output.h"
#include "../messages/messages.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/snap_lock.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(shorturl, 1, 0)


/** \brief Get a fixed shorturl name.
 *
 * The shorturl plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_SHORTURL_DATE:
        return "shorturl::date";

    case name_t::SNAP_NAME_SHORTURL_IDENTIFIER:
        return "shorturl::identifier";

    case name_t::SNAP_NAME_SHORTURL_ID_ROW:
        return "*id_row*";

    case name_t::SNAP_NAME_SHORTURL_INDEX_ROW:
        return "*index_row*";

    case name_t::SNAP_NAME_SHORTURL_NO_SHORTURL:
        return "shorturl::no_shorturl";

    case name_t::SNAP_NAME_SHORTURL_TABLE:
        return "shorturl";

    case name_t::SNAP_NAME_SHORTURL_URL:
        return "shorturl::url";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_SHORTURL_...");

    }
    NOTREACHED();
}

/** \brief Initialize the shorturl plugin.
 *
 * This function is used to initialize the shorturl plugin object.
 */
shorturl::shorturl()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the shorturl plugin.
 *
 * Ensure the shorturl object is clean before it is gone.
 */
shorturl::~shorturl()
{
}


/** \brief Get a pointer to the shorturl plugin.
 *
 * This function returns an instance pointer to the shorturl plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the shorturl plugin.
 */
shorturl * shorturl::instance()
{
    return g_plugin_shorturl_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString shorturl::settings_path() const
{
    return "/admin/settings/shorturl";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString shorturl::icon() const
{
    return "/images/shorturl/shorturl-logo-64x64.png";
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
QString shorturl::description() const
{
    return "Fully automated management of short URLs for this website.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString shorturl::dependencies() const
{
    return "|messages|path|output|sessions|";
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
int64_t shorturl::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 1, 16, 23, 39, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our shorturl references.
 *
 * Send our shorturl to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void shorturl::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the shorturl.
 *
 * This function terminates the initialization of the shorturl plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void shorturl::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(shorturl, "layout", layout::layout, generate_header_content, _1, _2, _3);
    SNAP_LISTEN(shorturl, "content", content::content, create_content, _1, _2, _3);
    SNAP_LISTEN(shorturl, "content", content::content, page_cloned, _1);
    SNAP_LISTEN(shorturl, "path", path::path, check_for_redirect, _1);
}


/** \brief Initialize the content table.
 *
 * This function creates the shorturl table if it doesn't exist yet. Otherwise
 * it simple initializes the f_shorturl_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * \return The pointer to the shorturl table.
 */
QtCassandra::QCassandraTable::pointer_t shorturl::get_shorturl_table()
{
    if(!f_shorturl_table)
    {
        f_shorturl_table = f_snap->get_table(get_name(name_t::SNAP_NAME_SHORTURL_TABLE));
    }
    return f_shorturl_table;
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
bool shorturl::on_path_execute(content::path_info_t & ipath)
{
    f_snap->output(layout::layout::instance()->apply_layout(ipath, this));

    return true;
}


/** \brief Check for paths under "s/..." and redirect them.
 *
 * As expected, this function redirects the user, with a 301, to the
 * page specified in a ShortCut. The plugin knows that the user hit
 * a shortcut if the path starts with "s/".
 *
 * If the specified shortcut does not exist on that website, then
 * the system does a soft redirect to "s". We should look in a
 * way to actually prevent caching of such a request since (1) it
 * includes an error and (2) that shortcut URI may become a valid
 * redirect later.
 *
 * The function uses the 301 code when redirecting because a 302 or
 * 303 do not work as expected in terms of SEO. That is, the juice
 * you would otherwise receive would be lost.
 *
 * \todo
 * We want to allow administrator to define the shortcut path
 * instead of "s/...". That has to be done in the settings before
 * the shorturl plugin gets installed (either that or we do nothing
 * until that data is defined.) This is very important because
 * changing that information later is very problematic (i.e. all
 * the old URIs will stop working.)
 *
 * \param[in,out] ipath  The path the user is going to now.
 */
void shorturl::on_check_for_redirect(content::path_info_t & ipath)
{
    if(ipath.get_cpath().startsWith("s/"))
    {
        QString const identifier(ipath.get_cpath().mid(2));
        QString const url(get_shorturl(identifier, 36));
        if(!url.isEmpty())
        {
            // TODO: add an easy to use/see tracking system for the
            //       shorturl plugin so an administrator can see who
            //       used which shorturl

            // redirect the user
            //
            http_link link(f_snap, ipath.get_key().toUtf8().data(), "shortlink");
            link.set_redirect();
            f_snap->add_http_link(link);

            // SEO wise, using HTTP_CODE_FOUND (and probably HTTP_CODE_SEE_OTHER)
            // is not as good as HTTP_CODE_MOVED_PERMANENTLY...
            f_snap->page_redirect(url, snap_child::http_code_t::HTTP_CODE_MOVED_PERMANENTLY);
            NOTREACHED();
        }

        // This is nearly an error; we do not expect users to be sent to
        // invalid shortcuts (although old pages that got deleted have their
        // shortcuts invalidated too...)
        //
        messages::messages::instance()->set_error(
            "Shortcut Not Found",
            QString("The shortcut you specified (%1) was not found on this website.").arg(identifier),
            "shorturl::on_check_for_redirect() could not find specified shortcut.",
            false
        );

        // soft redirect to /s
        ipath.set_path("s");
    }
}


/** \brief Generate the page main content.
 *
 * This function generates the main content of the page. Other
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
void shorturl::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    output::output::instance()->on_generate_main_content(ipath, page, body);
}


/** \brief Convert a Short URL identifier to a path.
 *
 * This function attempts to convert a Short URL identifier to a path,
 * assuming such a path exists.
 *
 * The id must represent the identifier number in the specified base.
 *
 * \param[in] id  The identifier of the Short URL to fetch.
 * \param[in] base  The base used to convert the identifier to a number.
 *
 * \return The full path to the Short URL or an empty string if the
 *         conversion fails.
 */
QString shorturl::get_shorturl(QString const & id, int base)
{
    bool ok;
    uint64_t const identifier(id.toULongLong(&ok, base));
    if(ok)
    {
        return get_shorturl(identifier);
    }

    return "";
}


/** \brief Convert a Short URL identifier to a path.
 *
 * This function attempts to convert a Short URL identifier to a path,
 * assuming such a path exists.
 *
 * The id must represent the identifier number. Note that identifier zero
 * is never considered valid for a Short URL.
 *
 * \param[in] id  The identifier of the Short URL to fetch.
 * \param[in] base  The base used to convert the identifier to a number.
 *
 * \return The full path to the Short URL or an empty string if the
 *         conversion fails.
 */
QString shorturl::get_shorturl(uint64_t identifier)
{
    if(identifier != 0)
    {
        QtCassandra::QCassandraTable::pointer_t shorturl_table(get_shorturl_table());
        QString const index(f_snap->get_website_key() + "/" + get_name(name_t::SNAP_NAME_SHORTURL_INDEX_ROW));
        QtCassandra::QCassandraValue identifier_value;
        identifier_value.setUInt64Value(identifier);
        QtCassandra::QCassandraValue url(shorturl_table->row(index)->cell(identifier_value.binaryValue())->value());
        if(!url.nullValue())
        {
            return url.stringValue();
        }
    }

    return "";
}


/** \brief Generate the header common content.
 *
 * This function generates some content that is expected in a page
 * by default.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] header  The page being generated.
 * \param[in,out] metadata  The body being generated.
 */
void shorturl::on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    NOTUSED(header);

    // only setup the shorturl if we are on the main page
    //
    snap_uri const & main_uri(f_snap->get_uri());
    if(main_uri.path() == ipath.get_cpath())
    {
        content::field_search::search_result_t result;

        FIELD_SEARCH
            (content::field_search::command_t::COMMAND_MODE, content::field_search::mode_t::SEARCH_MODE_EACH)
            (content::field_search::command_t::COMMAND_ELEMENT, metadata)
            (content::field_search::command_t::COMMAND_PATH_INFO_GLOBAL, ipath)

            // /snap/head/metadata/desc[@type="shorturl"]/data
            (content::field_search::command_t::COMMAND_FIELD_NAME, get_name(name_t::SNAP_NAME_SHORTURL_URL))
            (content::field_search::command_t::COMMAND_SELF)
            (content::field_search::command_t::COMMAND_RESULT, result)
            (content::field_search::command_t::COMMAND_SAVE, "desc[type=shorturl]/data")

            // generate!
            ;

        if(!result.isEmpty())
        {
            http_link link(f_snap, result[0].stringValue().toUtf8().data(), "shortlink");
            f_snap->add_http_link(link);
        }
    }
}


/** \brief Whether that URL supports short URL.
 *
 * If you create a plugin that creates pages that do not require a short
 * URL (i.e. sitemap.xml) then you may implement this signal and set the
 * \p allow parameter to false to avoid wasting time and resources.
 *
 * \param[in] ipath  The path receiving a short URL.
 * \param[in] owner  The owner of the new page (a plugin name).
 * \param[in] type  The type of page.
 * \param[in,out] allow  Whether the path should accept a short URL.
 *
 * \return This function returns true if this plugin does not considers
 *         the \p ipath as a path that does not require a short URL.
 */
bool shorturl::allow_shorturl_impl(content::path_info_t & ipath, QString const & owner, QString const & type, bool & allow)
{
    NOTUSED(owner);
    NOTUSED(type);

    // do not ever create short URLs for admin pages
    QString const cpath(ipath.get_cpath());
    if(cpath.isEmpty()                  // also marked as "no_shorturl" in content.xml
    || cpath == "s"                     // also marked as "no_shorturl" in content.xml
    || cpath == "admin"                 // also marked as "no_shorturl" in content.xml
    || cpath.startsWith("admin/")
    || cpath.endsWith(".css")
    || cpath.endsWith(".js"))
    {
        // do not need on home page, do not allow any short URL on
        // administration pages (no need really since those are not
        // public pages)
        allow = false;
        return false;
    }

    // force the default to 'true' in case another plugin calls this
    // signal improperly
    allow = true;

    return true;
}


/** \brief Implementation of the create_content() signal.
 *
 * For each page being created, we receive this callback. This allows us to
 * quickly add the short URL information in that page.
 *
 * The plugin first checks whether the path accepts short URLs. If not,
 * then the process ends immediately.
 *
 * \param[in] ipath  The page being created and receiving a short URL.
 * \param[in] owner  The owner of the new page (a plugin name).
 * \param[in] type  The type of page.
 */
void shorturl::on_create_content(content::path_info_t & ipath, QString const& owner, QString const & type)
{
    // allow this path to have a short URI?
    bool allow(true);
    allow_shorturl(ipath, owner, type, allow);
    if(!allow)
    {
        return;
    }

    // XXX do not generate a shorturl if the existing URL is less than
    //     a certain size?

    // TODO change to support a per content type short URL scheme

    QtCassandra::QCassandraTable::pointer_t shorturl_table(get_shorturl_table());

    // first generate a site wide unique identifier for that page
    uint64_t identifier(0);
    QString const id_key(QString("%1/%2").arg(f_snap->get_website_key()).arg(get_name(name_t::SNAP_NAME_SHORTURL_ID_ROW)));
    QString const identifier_key(get_name(name_t::SNAP_NAME_SHORTURL_IDENTIFIER));
    QtCassandra::QCassandraValue new_identifier;
    new_identifier.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);

    {
        // the lock only needs to be unique on a per website basis
        snap_lock lock(QString("%1/shorturl").arg(f_snap->get_website_key()));

        // In order to register a unique URI contents we want a
        // unique identifier for each URI, for that purpose we use
        // a special row in the short URI table and since we have a
        // lock we can safely do a read-increment-write cycle.
        if(shorturl_table->exists(id_key))
        {
            QtCassandra::QCassandraRow::pointer_t id_row(shorturl_table->row(id_key));
            QtCassandra::QCassandraCell::pointer_t id_cell(id_row->cell(identifier_key));
            id_cell->setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
            QtCassandra::QCassandraValue current_identifier(id_cell->value());
            if(current_identifier.nullValue())
            {
                // this means no user can register until this value gets
                // fixed somehow!
                messages::messages::instance()->set_error(
                    "Failed Creating Short URL Unique Identifier",
                    "Somehow the Short URL plugin could not create a unique identifier for your new page.",
                    "shorturl::on_create_content() could not load the *id_row* identifier, the row exists but the cell did not make it ("
                                 + id_key + "/" + identifier_key + ").",
                    false
                );
                return;
            }
            identifier = current_identifier.uint64Value();
        }

        // XXX -- we could support a randomize too?
        // Note: generally, public URL shorteners will randomize this number
        //       so no two pages have the same number and they do not appear
        //       in sequence; here we do not need to do that because the
        //       website anyway denies access to all the pages that are to
        //       be hidden from preying eyes
        ++identifier;

        new_identifier.setUInt64Value(identifier);
        shorturl_table->row(id_key)->cell(identifier_key)->setValue(new_identifier);

        // the lock automatically goes away here
    }

    QString const key(ipath.get_key());

    QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
    QtCassandra::QCassandraRow::pointer_t row(content_table->row(key));

    row->cell(identifier_key)->setValue(new_identifier);

    // save the date when the Short URL is generated so if the user changes
    // the parameters we can regenerate only those that were generated before
    // the date of the change
    uint64_t const start_date(f_snap->get_start_date());
    row->cell(get_name(name_t::SNAP_NAME_SHORTURL_DATE))->setValue(start_date);

    // TODO allow the user to change the "%1" number parameters
    QString const site_key(f_snap->get_site_key_with_slash());
    QString const shorturl_url(site_key + QString("s/%1").arg(identifier, 0, 36, QChar('0')));
    QtCassandra::QCassandraValue shorturl_value(shorturl_url);
    row->cell(get_name(name_t::SNAP_NAME_SHORTURL_URL))->setValue(shorturl_value);

    // create an index entry so we can find the entry and redirect the user
    // as required
    QString const index(f_snap->get_website_key() + "/" + get_name(name_t::SNAP_NAME_SHORTURL_INDEX_ROW));
    shorturl_table->row(index)->cell(new_identifier.binaryValue())->setValue(key);
}




/** \brief Someone just cloned a page.
 *
 * Check whether the short URL of the clone needs tweaking.
 *
 * If the source page had no short URL, then nothing happens and we
 * return immediately.
 *
 * Otherwise, we create a new short URL when the source page remains
 * as NORMAL or HIDDEN after the cloning process (i.e. actual copy).
 *
 * We do not create a new short URL in any other situation. Yet, we
 * update the shortcut table to point to the new location of the
 * page (destination URL.)
 *
 * \param[in] tree  The tree of pages that got cloned
 */
void shorturl::on_page_cloned(content::content::cloned_tree_t const& tree)
{
    // the short URL is global (saved in the content table) so we do not
    // need to do anything about the branches and revisions in this function

    // got a short URL in the source?
    QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());

    content::path_info_t::status_t::state_t const source_done_state(tree.f_source.f_done_state.get_state());
    size_t const max_pages(tree.f_pages.size());
    for(size_t idx(0); idx < max_pages; ++idx)
    {
        content::path_info_t source(tree.f_pages[idx].f_source);
        QtCassandra::QCassandraRow::pointer_t content_row(content_table->row(source.get_key()));
        if(content_row->exists(get_name(name_t::SNAP_NAME_SHORTURL_URL)))
        {
            // need a change?
            switch(source_done_state)
            {
            case content::path_info_t::status_t::state_t::UNKNOWN_STATE:
            case content::path_info_t::status_t::state_t::CREATE:
                SNAP_LOG_WARNING("cloning results with a invalid state (")(static_cast<int>(source_done_state))(")");
                // since this is wrong here, it will be wrong on each
                // iteration so we can as well return immediately
                return;

            case content::path_info_t::status_t::state_t::NORMAL:
            case content::path_info_t::status_t::state_t::HIDDEN:
                // in this case we want a new short URL!
                {
                    content::path_info_t destination(tree.f_pages[idx].f_destination);

                    // get destination owner
                    QString const owner(content_table->row(destination.get_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_PRIMARY_OWNER))->value().stringValue());

                    // get destination type
                    // TODO: this requires the link to have been updated already...
                    QString type("page/public");
                    links::link_info type_info(content::get_name(content::name_t::SNAP_NAME_CONTENT_PAGE_TYPE), true, destination.get_key(), destination.get_branch());
                    QSharedPointer<links::link_context> type_link_ctxt(links::links::instance()->new_link_context(type_info));
                    links::link_info type_child_info;
                    if(type_link_ctxt->next_link(type_child_info))
                    {
                        // should always be true because all pages have a type
                        QString const type_key(type_child_info.key());
                        int const pos(type_key.indexOf("/types/taxonomy/system/content-types/"));
                        type = type_key.mid(pos + 37);
                    }

                    // now create a new short URL for this page
                    on_create_content(destination, owner, type);
                }
                break;

            case content::path_info_t::status_t::state_t::MOVED:
            case content::path_info_t::status_t::state_t::DELETED:  // TBD -- do we really want that for deleted pages? we could also delete the short URL...
                // in this case the destination can make use of the existing short URL
                // however, we want to update the shorturl table so it points to
                // 'destination' now
                {
                    content::path_info_t destination(tree.f_pages[idx].f_destination);

                    QtCassandra::QCassandraTable::pointer_t shorturl_table(get_shorturl_table());
                    QtCassandra::QCassandraValue identifier_value(content_table->row(destination.get_key())->cell(get_name(name_t::SNAP_NAME_SHORTURL_IDENTIFIER))->value());
                    
                    // make sure we have a valid identifier
                    if(!identifier_value.nullValue())
                    {
                        QString const index(QString("%1/%2").arg(f_snap->get_website_key()).arg(get_name(name_t::SNAP_NAME_SHORTURL_INDEX_ROW)));
                        shorturl_table->row(index)->cell(identifier_value.binaryValue())->setValue(destination.get_key());
                    }
                }
                break;

            }
        }
    }
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
