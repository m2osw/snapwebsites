// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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
#include    "header.h"


// other plugins
//
#include    "../output/output.h"


// snapdev
//
#include    <snapdev/not_reached.h>
#include    <snapdev/not_used.h>


// last include
//
#include    <snapdev/poison.h>



namespace snap
{
namespace header
{


CPPTHREAD_PLUGIN_START(header, 1, 0)
    , ::cppthread::plugin_description(
            "Allows you to add/remove HTML and HTTP headers to your content."
            " Note that this module can, but should not be used to manage meta"
            " data for your page.")
    , ::cppthread::plugin_icon("/images/header/header-logo-64x64.png")
    , ::cppthread::plugin_settings("/admin/settings/header")
    , ::cppthread::plugin_dependency("layout")
    , ::cppthread::plugin_dependency("output")
    , ::cppthread::plugin_dependency("path")
    , ::cppthread::plugin_help_uri("https://snapwebsites.org/help")
    , ::cppthread::plugin_categorization_tag("security")
    , ::cppthread::plugin_categorization_tag("spam")
CPPTHREAD_PLUGIN_END()

/** \brief Get a fixed header name.
 *
 * The header plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_HEADER_INTERNAL:
        return "header::internal";

    case name_t::SNAP_NAME_HEADER_GENERATOR:
        return "header::generator";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_HEADER_...");

    }
    snapdev::NOT_REACHED();
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
int64_t header::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 1, 15, 17, 58, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void header::content_update(int64_t variables_timestamp)
{
    snapdev::NOT_USED(variables_timestamp);
    content::content::instance()->add_xml("header");
}


/** \brief Initialize the header.
 *
 * This function terminates the initialization of the header plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void header::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(header, "layout", layout::layout, generate_header_content, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3);
}


/** \brief Execute header page: generate the complete output of that page.
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
bool header::on_path_execute(content::path_info_t & ipath)
{
    f_snap->output(layout::layout::instance()->apply_layout(ipath, this));

    return true;
}


void header::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    // a type is just like a regular page
    output::output::instance()->on_generate_main_content(ipath, page, body);
}


/** \brief Generate the page common content.
 *
 * This function generates some meta data headers that is expected in a page
 * by default.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] header_dom  The header being generated.
 * \param[in,out] metadata  The metada being generated.
 */
void header::on_generate_header_content(content::path_info_t & ipath, QDomElement & header_dom, QDomElement & metadata)
{
    QDomDocument doc(header_dom.ownerDocument());

    content::content * content_plugin(content::content::instance());

    // TODO we actually most probably want a location where the user put that
    //      information in a unique place (i.e. the header settings, see the
    //      shorturl settings)

    {   // snap/head/metadata/generator
        QDomElement created(doc.createElement("generator"));
        metadata.appendChild(created);
        libdbproxy::value generator(content_plugin->get_content_parameter(ipath, get_name(name_t::SNAP_NAME_HEADER_GENERATOR), content::content::param_revision_t::PARAM_REVISION_BRANCH));
        if(!generator.nullValue())
        {
            // also save that one as a header
            f_snap->set_header("Generator", generator.stringValue());

            QDomText text(doc.createTextNode(generator.stringValue()));
            created.appendChild(text);
        }
    }


// WARNING WARNING WARNING
// WARNING WARNING WARNING
// WARNING WARNING WARNING
//   at this time this is hard coded; instead, we want to allow the
//   administrator to define the general refererrer policy and
//   possibly set a different policy depending on the page the user
//   accesses; and if at or under "/admin", then make sure that at
//   most we have "origin" (i.e. "none", "origin-when-cross-origin",
//   etc. have a higher priority and would be used instead)
//
//   TODO: write a class that searches for the referrer policy
//         and keeps the most restrictive one then we always
//         specify the policy as follow (unless the user choose
//         "browser-default" which would mean sending "" and let
//         the browser do whatever it wants, so we might as well
//         not send it in that case.)
//

    {   // snap/head/metadata/desc[@type='user']/data/origin
        snap_string_list const & segments(ipath.get_segments());
        if(segments.size() > 0
        && segments[0] == "admin")
        {
            QDomElement desc(doc.createElement("desc"));
            desc.setAttribute("type", "user");
            desc.setAttribute("name", "referrer");
            metadata.appendChild(desc);

            QDomElement data(doc.createElement("data"));
            desc.appendChild(data);

            QDomText referrer(doc.createTextNode("origin"));
            data.appendChild(referrer);

            // Set the HTTP header too
            f_snap->set_header("Referrer-Policy", "origin");
        }
    }
}



} // namespace header
} // namespace snap
// vim: ts=4 sw=4 et
