// Snap Websites Server -- Internet avatar functionality
// Copyright (C) 2015-2017  Made to Order Software Corp.
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

#include "avatar.h"

#include <snapwebsites/not_used.h>

#include <openssl/md5.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(avatar, 1, 0)


/** \brief Get a fixed avatar name.
 *
 * The avatar plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_AVATAR_TITLE:
        return "avatar::title";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_AVATAR_...");

    }
    NOTREACHED();
}


/** \brief Initialize the avatar plugin.
 *
 * This function is used to initialize the avatar plugin object.
 */
avatar::avatar()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the avatar plugin.
 *
 * Ensure the avatar object is clean before it is gone.
 */
avatar::~avatar()
{
}


/** \brief Get a pointer to the avatar plugin.
 *
 * This function returns an instance pointer to the avatar plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the avatar plugin.
 */
avatar * avatar::instance()
{
    return g_plugin_avatar_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString avatar::settings_path() const
{
    return "/admin/settings/avatar";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString avatar::icon() const
{
    return "/images/avatar/avatar-logo-64x64.png";
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
QString avatar::description() const
{
    return "Transform user emails in comments, pages, profiles"
          " to Avatar images.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString avatar::dependencies() const
{
    return "|filter|";
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
int64_t avatar::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 12, 20, 22, 42, 42, content_update);

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
void avatar::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the avatar.
 *
 * This function terminates the initialization of the avatar plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void avatar::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(avatar, "filter", filter::filter, replace_token, _1, _2, _3);
    SNAP_LISTEN(avatar, "filter", filter::filter, token_help, _1);
}


/** \brief Replace the [avatar::...] tokens.
 *
 * This function transforms avatar tokens to HTML.
 *
 * \li [avatar::avatar(email)] -- transforms the email address in an image.
 */
void avatar::on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token)
{
    NOTUSED(ipath);
    NOTUSED(xml);

    if(!token.is_namespace("avatar::"))
    {
        return;
    }

    if(token.is_token("avatar::avatar"))
    {
        // the parameter is the email address to convert
        token.verify_args(1, 1);
        filter::filter::parameter_t email(token.get_arg("email", 0, filter::filter::token_t::TOK_STRING));
        if(!email.f_value.isEmpty())
        {
            // TODO: verify everything (i.e. that the email is from one of
            //       our users, whether the user is from this website, whether
            //       to use the "local" (snap) image or external image, etc.)
            //
            unsigned char md[MD5_DIGEST_LENGTH];
            std::string const utf8_email(email.f_value.toUtf8().data());
            MD5(reinterpret_cast<unsigned char const *>(utf8_email.c_str()), utf8_email.length(), md);
            QByteArray const md5(reinterpret_cast<char const *>(md), sizeof(md));
            QString const hash(md5.toHex());
            token.f_replacement = QString("<img src=\"http://www.gravatar.com/avatar/%1\"/>").arg(hash);
        }
    }
}


void avatar::on_token_help(filter::filter::token_help_t & help)
{
    help.add_token("avatar::avatar",
        "If available, display an avatar (a photo, a drawing, a flag...) for the specified user [email]. The token takes one parameter which is an email address. This is just one &lt;img&gt; tag. If no avatar is available, some default image tag may still be generated.");
}





//
// Gravatar
//   http://en.gravatar.com/site/implement/
//

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
