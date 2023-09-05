// Copyright (c) 2015-2019  Made to Order Software Corp.  All Rights Reserved
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
#include "avatar.h"


// snapdev lib
//
#include <snapdev/not_used.h>


// OpenSSL lib
//
#include <openssl/md5.h>


// last include
//
#include <snapdev/poison.h>



namespace snap
{
namespace avatar
{



SERVERPLUGINS_START(avatar)
    , ::serverplugins::description(
            "Transform user emails in comments, pages, profiles"
            " to Avatar images.")
    , ::serverplugins::icon("/images/avatar/avatar-logo-64x64.png")
    , ::serverplugins::settings_path("/admin/settings/avatar")
    , ::serverplugins::dependency("filter")
    , ::serverplugins::help_uri("https://snapwebsites.org/help")
    , ::serverplugins::categorization_tag("user")
SERVERPLUGINS_END(avatar)




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
time_t avatar::do_update(time_t last_updated, unsigned int phase)
{
    SERVERPLUGINS_PLUGIN_UPDATE_INIT();

    if(phase == 0)
    {
        SERVERPLUGINS_PLUGIN_UPDATE(2015, 12, 20, 22, 42, 42, content_update);
    }

    SERVERPLUGINS_PLUGIN_UPDATE_EXIT();
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
    snapdev::NOT_USED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the avatar.
 *
 * This function terminates the initialization of the avatar plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void avatar::bootstrap()
{
    SERVERPLUGINS_LISTEN(avatar, "filter", filter::filter, replace_token, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3);
    SERVERPLUGINS_LISTEN(avatar, "filter", filter::filter, token_help, boost::placeholders::_1);
}


/** \brief Replace the [avatar::...] tokens.
 *
 * This function transforms avatar tokens to HTML.
 *
 * \li [avatar::avatar(email)] -- transforms the email address in an image.
 */
void avatar::on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token)
{
    snapdev::NOT_USED(ipath, xml);

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


} // namespace avatar
} // namespace snap
// vim: ts=4 sw=4 et
