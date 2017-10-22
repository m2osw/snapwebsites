// Snap Websites Server -- robots.txt
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

#include "robotstxt.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>

#include <iostream>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(robotstxt, 1, 0)


/** \brief Get a fixed permissions plugin name.
 *
 * The permissions plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_ROBOTSTXT_FORBIDDEN_PATH:
        return "types/taxonomy/system/robotstxt/forbidden";

    case name_t::SNAP_NAME_ROBOTSTXT_FORBIDDEN:
        return "robotstxt::forbidden";

    case name_t::SNAP_NAME_ROBOTSTXT_NOARCHIVE:
        return "robotstxt::noarchive";

    case name_t::SNAP_NAME_ROBOTSTXT_NOFOLLOW:
        return "robotstxt::nofollow";

    case name_t::SNAP_NAME_ROBOTSTXT_NOIMAGEINDEX:
        return "robotstxt::noimageindex";

    case name_t::SNAP_NAME_ROBOTSTXT_NOINDEX:
        return "robotstxt::noindex";

    case name_t::SNAP_NAME_ROBOTSTXT_NOSNIPPET:
        return "robotstxt::nosnippet";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_ROBOTSTXT_...");

    }
    NOTREACHED();
}


const char *        robotstxt::ROBOT_NAME_ALL = "*";
const char *        robotstxt::ROBOT_NAME_GLOBAL = "";
const char *        robotstxt::FIELD_NAME_DISALLOW = "Disallow";

/** \brief Initialize the robotstxt plugin.
 *
 * This function is used to initialize the robotstxt plugin object.
 */
robotstxt::robotstxt()
    //: f_snap(NULL) -- auto-init
    : f_robots_path("#")
{
}


/** \brief Clean up the robotstxt plugin.
 *
 * Ensure the robotstxt object is clean before it is gone.
 */
robotstxt::~robotstxt()
{
}


/** \brief Get a pointer to the robotstxt plugin.
 *
 * This function returns an instance pointer to the robotstxt plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the robotstxt plugin.
 */
robotstxt * robotstxt::instance()
{
    return g_plugin_robotstxt_factory.instance();
}


///** \brief Send users to the plugin settings.
// *
// * This path represents this plugin settings.
// */
//QString robotstxt::settings_path() const
//{
//    return "/admin/settings/robotstxt";
//}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString robotstxt::icon() const
{
    return "/images/robotstxt/robotstxt-logo-64x64.png";
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
QString robotstxt::description() const
{
    return "Generates the robots.txt file which is used by search engines to"
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
QString robotstxt::dependencies() const
{
    return "|layout|path|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run yet.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when this plugin was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t robotstxt::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 12, 20, 19, 58, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the content with our references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void robotstxt::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml("robotstxt");
}


/** \brief Initialize the robotstxt.
 *
 * This function terminates the initialization of the robotstxt plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void robotstxt::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(robotstxt, "layout", layout::layout, generate_header_content, _1, _2, _3);
    SNAP_LISTEN(robotstxt, "layout", layout::layout, generate_page_content, _1, _2, _3);
}


/** \brief Check for the "robots.txt" path.
 *
 * This function ensures that the URL is robots.txt and if so write
 * the robots.txt file content in the QBuffer of the path.
 *
 * \param[in] ipath  The URL being managed.
 *
 * \return true if the robots.txt file is properly generated, false otherwise.
 */
bool robotstxt::on_path_execute(content::path_info_t & ipath)
{
    if(ipath.get_cpath() == "robots.txt")
    {
        // XXX we may only need the globals (i.e. test website) but at
        // this point we do not make the distinction when calling the
        // generate_robotstxt() signal (it should not make much difference
        // although speed wise it would probably be a good optimization)
        //
        generate_robotstxt(this);
        output();
        return true;
    }

    return false;
}


/** \brief Output the results.
 *
 * This function outputs the contents of the robots.txt file.
 */
void robotstxt::output() const
{
    f_snap->set_header("Content-Type", "text/plain; charset=utf-8");
    // TODO: change the "Expires" header to 1 day because we don't need
    //       users to check for the robots.txt that often!?

//std::cout << "----------------------------- GENERATING robots.txt\n";
    f_snap->output("# More info http://www.robotstxt.org/\n");
    f_snap->output("# Generated by https://snapwebsites.org/\n");

    robots_txt_t::const_iterator global = f_robots_txt.find("");
    if(global != f_robots_txt.end())
    {
        // in this case we don't insert any User-agent
        for(robots_field_array_t::const_iterator i = global->second.begin(); i != global->second.end(); ++i)
        {
            f_snap->output(i->f_field);
            f_snap->output(": ");
            f_snap->output(i->f_value);
            f_snap->output("\n");
        }
    }

    libdbproxy::value test_site(f_snap->get_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_TEST_SITE)));
    if(test_site.safeSignedCharValue(0, 0))
    {
        // test websites just disallow everything and thus here we do
        // not need to do more than output a default robots.txt
        //
        f_snap->output("User-agent: *\n");
        f_snap->output("Disallow: /\n");
        return;
    }

    robots_txt_t::const_iterator all = f_robots_txt.find("*");
    if(all != f_robots_txt.end())
    {
        f_snap->output("User-agent: *\n");
        for(robots_field_array_t::const_iterator i = all->second.begin(); i != all->second.end(); ++i)
        {
            f_snap->output(i->f_field);
            f_snap->output(": ");
            f_snap->output(i->f_value);
            f_snap->output("\n");
        }
    }

    for(robots_txt_t::const_iterator r = f_robots_txt.begin(); r != f_robots_txt.end(); ++r)
    {
        if(r->first == "*" || r->first == "")
        {
            // skip the "all robots" ("*") and "global" ("") entries
            continue;
        }
        f_snap->output("User-agent: ");
        f_snap->output(r->first);
        f_snap->output("\n");
        for(robots_field_array_t::const_iterator i = r->second.begin(); i != r->second.end(); ++i)
        {
            f_snap->output(i->f_field);
            f_snap->output(": ");
            f_snap->output(i->f_value);
            f_snap->output("\n");
        }
    }
}


/** \brief Implementation of the generate_robotstxt signal.
 *
 * This function readies the generate_robotstxt signal.
 *
 * This function generates the main folders that we consider as forbidden.
 * It adds the "/cgi-bin/" path automatically. It also goes through the
 * list of paths marked as forbidden and adds them. If you have some really
 * special entries you want to add to the robots.txt file, you may implement
 * this signal too.
 *
 * The system list include the following directories at this time:
 *
 * \li "/admin/" -- all administration only pages
 *
 * \param[in] r  The robotstxt plugin, use to call the add_robots_txt_field()
 *               and other functions.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool robotstxt::generate_robotstxt_impl(robotstxt * r)
{
    r->add_robots_txt_field("/cgi-bin/");

    // admin is marked as forbidden so the following loop will capture it
    // so no need to add it manually
    //r->add_robots_txt_field("/admin/");

    content::path_info_t forbidden_ipath;
    forbidden_ipath.set_path(get_name(name_t::SNAP_NAME_ROBOTSTXT_FORBIDDEN_PATH));
    links::link_info robots_info(get_name(name_t::SNAP_NAME_ROBOTSTXT_FORBIDDEN), false, forbidden_ipath.get_key(), forbidden_ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(robots_info));
    links::link_info robots_txt;
    while(link_ctxt->next_link(robots_txt))
    {
        content::path_info_t page_ipath;
        page_ipath.set_path(robots_txt.key());
        r->add_robots_txt_field(QString("/%1/").arg(page_ipath.get_cpath()));
    }

    return true;
}


/** \brief Add Disallows to the robots.txt file.
 *
 * This function can be used to disallow a set of folders your plugin is
 * responsible for. All the paths that are protected in some way (i.e. the
 * user needs to be logged in to access that path) should be disallowed in
 * the robots.txt file.
 *
 * Note that all the system administrative functions are found under /admin/
 * which is already disallowed by the robots.txt plugin itself. So is the
 * /cgi-bin/ folder.
 *
 * \todo
 * The order can be important so we will need to work on that part at some point.
 * At this time we print the entries in this order:
 *
 * \li global entries (i.e. robot = "")
 * \li the "all" robots list of fields
 * \li the other robots
 *
 * One way to setup the robots file goes like this:
 *
 * User-agent: *
 * Disallow: /
 *
 * User-agent: Good-guy
 * Disallow: /admin/
 *
 * This way only Good-guy is expected to spider your website.
 *
 * \param[in] value  The content of this field
 * \param[in] field  The name of the field being added (default "Disallow")
 * \param[in] robot  The name of the robot (default "*")
 * \param[in] unique  The field is unique, if already defined throw an error
 */
void robotstxt::add_robots_txt_field(QString const & value,
                                     QString const & field,
                                     QString const & robot,
                                     bool unique)
{
    if(field.isEmpty())
    {
        throw robotstxt_exception_invalid_field_name("robots.txt field name cannot be empty");
    }

    robots_field_array_t & d = f_robots_txt[robot];
    if(unique)
    {
        // verify unicity
        for(robots_field_array_t::const_iterator i = d.begin(); i != d.end(); ++i)
        {
            if(i->f_field == field)
            {
                throw robotstxt_exception_already_defined("field \"" + field + "\" is already defined");
            }
        }
    }
    robots_field_t f;
    f.f_field = field;
    f.f_value = value;
    d.push_back(f);
}


/** \brief Retrieve the robots setup for a page.
 *
 * This function loads the robots setup for the specified page.
 *
 * Note that the function returns an empty string if the current setup is
 * index,follow pr index,follow,archive since those represent the default
 * value of the robots meta tag.
 *
 * See some documentation here:
 *
 * https://developer.mozilla.org/en-US/docs/Web/HTML/Element/meta
 *
 * \todo
 * Pages that are forbidden (because it or one of its parents is forbidden)
 * should always return an empty list of robots in f_robots_cache. This is
 * not the end of the world, but we would save on transfer amounts over time.
 *
 * \param[in,out] ipath  The path of the page for which links are checked to determine the robots setup.
 */
void robotstxt::define_robots(content::path_info_t & ipath)
{
    if(ipath.get_key() != f_robots_path)
    {
        // Define the X-Robots-Tag HTTP header
        // and the robots meta data
        //
        snap_string_list robots;

        // test websites are a special case
        //
        libdbproxy::value test_site(f_snap->get_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_TEST_SITE)));
        if(test_site.safeSignedCharValue(0, 0))
        {
            // test websites are all completely forbidden from indexing by
            // robots since these sites are really not to be considered by
            // search engines at all
            //
            robots += QString("noindex");
            robots += QString("nofollow");
            robots += QString("noarchive");
            robots += QString("nocache");
        }
        else
        {
            {
// linking [http://csnap.m2osw.com/] / [http://csnap.m2osw.com/types/taxonomy/system/robotstxt/noindex]
// <link name="noindex" to="noindex" mode="1:*">/types/taxonomy/system/robotstxt/noindex</link>
                links::link_info robots_info(get_name(name_t::SNAP_NAME_ROBOTSTXT_NOINDEX), true, ipath.get_key(), ipath.get_branch());
                robots_info.set_branch(ipath.get_branch());
                QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(robots_info));
                links::link_info robots_txt;
                if(link_ctxt->next_link(robots_txt))
                {
                    robots += QString("noindex");   // All
                }
            }
            {
                links::link_info robots_info(get_name(name_t::SNAP_NAME_ROBOTSTXT_NOFOLLOW), true, ipath.get_key(), ipath.get_branch());
                robots_info.set_branch(ipath.get_branch());
                QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(robots_info));
                links::link_info robots_txt;
                if(link_ctxt->next_link(robots_txt))
                {
                    robots += QString("nofollow");  // All
                }
            }
            {
                links::link_info robots_info(get_name(name_t::SNAP_NAME_ROBOTSTXT_NOARCHIVE), true, ipath.get_key(), ipath.get_branch());
                robots_info.set_branch(ipath.get_branch());
                QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(robots_info));
                links::link_info robots_txt;
                if(link_ctxt->next_link(robots_txt))
                {
                    robots += QString("noarchive");  // Google, Yahoo!
                    robots += QString("nocache");    // Bing
                }
            }
            {
                links::link_info robots_info(get_name(name_t::SNAP_NAME_ROBOTSTXT_NOSNIPPET), true, ipath.get_key(), ipath.get_branch());
                robots_info.set_branch(ipath.get_branch());
                QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(robots_info));
                links::link_info robots_txt;
                if(link_ctxt->next_link(robots_txt))
                {
                    robots += QString("nosnippet");  // Google
                }
            }
            {
                links::link_info robots_info(get_name(name_t::SNAP_NAME_ROBOTSTXT_NOIMAGEINDEX), true, ipath.get_key(), ipath.get_branch());
                robots_info.set_branch(ipath.get_branch());
                QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(robots_info));
                links::link_info robots_txt;
                if(link_ctxt->next_link(robots_txt))
                {
                    robots += QString("noimageindex");  // Google
                }
            }
            // TODO: add the search engine specific tags
        }

        f_robots_cache = robots.join(",");
        f_robots_path = ipath.get_key();
    }
}


/** \brief Add the X-Robots to the header.
 *
 * If the robots metadata is set to something else than index,follow[,archive]
 * then we want to add an X-Robots to the HTTP header. This is useful
 * to increase changes that the robots understand what we're trying to
 * tell it.
 *
 * \param[in,out] ipath  The path concerned by this request.
 * \param[in,out] header  The HTML header element.
 * \param[in,out] metadata  The XML metadata used with the XSLT parser.
 */
void robotstxt::on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    NOTUSED(header);
    NOTUSED(metadata);

    define_robots(ipath);
    if(!f_robots_cache.isEmpty())
    {
        // Set the HTTP header
        f_snap->set_header("X-Robots-Tag", f_robots_cache);
    }
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
void robotstxt::on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    define_robots(ipath);
    if(!f_robots_cache.isEmpty())
    {
        QDomDocument doc(page.ownerDocument());

        // /snap/body/robots/tracking/...(noindex noarchive etc.)...
        QDomElement created_root(doc.createElement("robots"));
        body.appendChild(created_root);
        QDomElement created(doc.createElement("tracking"));
        created_root.appendChild(created);
        QDomText text(doc.createTextNode(f_robots_cache));
        created.appendChild(text);
    }
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
