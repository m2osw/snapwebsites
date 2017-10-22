// Snap Websites Server -- verify passwords of all the parts used by snap
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

#include "./password.h"

#include "../output/output.h"
#include "../messages/messages.h"
#include "../permissions/permissions.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/fuzzy_string_compare.h>
#include <snapwebsites/snap_lock.h>

#include <algorithm>
#include <iostream>

#include <openssl/rand.h>

#include <QChar>

#include <snapwebsites/poison.h>


/** \file
 * \brief The password plugin is used to check password policies.
 *
 * Our implementation follows all sorts of schemes that are offered on
 * many websites. However, it is important to note that things are
 * changing quickly and security does not really mean having a super
 * strong password policy in place. Instead, it is to have passwords
 * that are generally hard to crack. We are trying to move toward that
 * specific target, yet we still offer the old fashion policy rules
 * because most users expect to have them.
 *
 * We have a few documents under snapwebsites/doc (in the source)
 * that describe various points on passwords:
 *
 * \li WhereDoSecurityPoliciesComeFrom.pdf
 * \li AboutPasswordEntropy-NIST.SP.800-63-2.pdf
 * \li CCS_Password_Metric_Measurement.pdf
 *
 * And a few links that you may find useful:
 *
 * \li http://reusablesec.blogspot.com/2010/10/new-paper-on-password-security-metrics.html
 */

SNAP_PLUGIN_START(password, 1, 0)



namespace
{

// random bytes generator
//
template<int RANDOM_BUFFER_SIZE>
class random_generator
{
public:
    unsigned char get_byte()
    {
        if(f_pos >= RANDOM_BUFFER_SIZE)
        {
            f_pos = 0;
        }

        if(f_pos == 0)
        {
            // get the random bytes
            RAND_bytes(f_buf, RANDOM_BUFFER_SIZE);
        }

        ++f_pos;
        return f_buf[f_pos - 1];
    }

private:
    unsigned char       f_buf[RANDOM_BUFFER_SIZE];
    size_t              f_pos = 0;
};


} // no name namespace



/* \brief Get a fixed password name.
 *
 * The password plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_PASSWORD_BLOCKED_USER_COUNTER:
        return "password::blocked_user_counter";

    case name_t::SNAP_NAME_PASSWORD_BLOCKED_USER_COUNTER_LIFETIME:
        return "password::blocked_user_counter_lifetime";

    case name_t::SNAP_NAME_PASSWORD_BLOCKED_USER_FIREWALL_DURATION:
        return "password::blocked_user_firewall_duration";

    case name_t::SNAP_NAME_PASSWORD_CHECK_BLACKLIST:
        return "password::check_blacklist";

    case name_t::SNAP_NAME_PASSWORD_CHECK_USERNAME:
        return "password::check_username";

    case name_t::SNAP_NAME_PASSWORD_CHECK_USERNAME_REVERSED:
        return "password::check_username_reversed";

    case name_t::SNAP_NAME_PASSWORD_COUNT_BAD_PASSWORD_503S:
        return "password::count_bad_password_503s";

    case name_t::SNAP_NAME_PASSWORD_COUNT_FAILURES:
        return "password::count_failures";

    case name_t::SNAP_NAME_PASSWORD_DELAY_BETWEEN_PASSWORD_CHANGES:
        return "password::delay_between_password_changes";

    case name_t::SNAP_NAME_PASSWORD_EXISTS_IN_BLACKLIST:
        return "password::exists_in_blacklist";

    case name_t::SNAP_NAME_PASSWORD_INVALID_PASSWORDS_BLOCK_DURATION:
        return "password::invalid_passwords_block_duration";

    case name_t::SNAP_NAME_PASSWORD_INVALID_PASSWORDS_COUNTER:
        return "password::invalid_passwords_counter";

    case name_t::SNAP_NAME_PASSWORD_INVALID_PASSWORDS_COUNTER_LIFETIME:
        return "password::invalid_passwords_counter_lifetime";

    case name_t::SNAP_NAME_PASSWORD_INVALID_PASSWORDS_SLOWDOWN:
        return "password::invalid_passwords_slowdown";

    case name_t::SNAP_NAME_PASSWORD_LIMIT_DURATION:
        return "password::limit_duration";

    case name_t::SNAP_NAME_PASSWORD_MAXIMUM_DURATION:
        return "password::maximum_duration";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_DIGITS:
        return "password::minimum_digits";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_LENGTH:
        return "password::minimum_length";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_LENGTH_OF_VARIATIONS:
        return "password::minimum_length_of_variations";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_LETTERS:
        return "password::minimum_letters";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_LOWERCASE_LETTERS:
        return "password::minimum_lowercase_letters";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_OLD_PASSWORDS:
        return "password::minimum_old_passwords";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_SPACES:
        return "password::minimum_spaces";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_SPECIALS:
        return "password::minimum_specials";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_UNICODE:
        return "password::minimum_unicode";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_UPPERCASE_LETTERS:
        return "password::minimum_uppercase_letters";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_VARIATION:
        return "password::minimum_variation";

    case name_t::SNAP_NAME_PASSWORD_OLD_PASSWORDS_MAXIMUM_AGE:
        return "password::old_passwords_maximum_age";

    case name_t::SNAP_NAME_PASSWORD_PREVENT_OLD_PASSWORDS:
        return "password::prevent_old_passwords";

    case name_t::SNAP_NAME_PASSWORD_TABLE:
        return "password";

    default:
        // invalid index
        throw snap_logic_exception(QString("invalid name_t::SNAP_NAME_PASSWORD_... (%1)").arg(static_cast<int>(name)));

    }
    NOTREACHED();
}









/** \brief Initialize the password plugin.
 *
 * This function is used to initialize the password plugin object.
 */
password::password()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the password plugin.
 *
 * Ensure the password object is clean before it is gone.
 */
password::~password()
{
}


/** \brief Get a pointer to the password plugin.
 *
 * This function returns an instance pointer to the password plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the password plugin.
 */
password * password::instance()
{
    return g_plugin_password_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString password::settings_path() const
{
    return "/admin/settings/password";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString password::icon() const
{
    return "/images/password/password-logo-64x64.png";
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
QString password::description() const
{
    return "Check passwords of newly created users for strength."
          " The plugin verifies various settings to ensure the strength of passwords."
          " It can also check a database of black listed passwords.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString password::dependencies() const
{
    return "|editor|messages|output|permissions|users|";
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
int64_t password::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 2, 13, 13, 11, 51, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                        to the database by this update (in micro-seconds).
 */
void password::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the password.
 *
 * This function terminates the initialization of the password plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void password::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(password, "editor", editor::editor, prepare_editor_form, _1);
    SNAP_LISTEN(password, "editor", editor::editor, init_editor_widget, _1, _2, _3, _4, _5);
    SNAP_LISTEN(password, "layout", layout::layout, generate_page_content, _1, _2, _3);
    SNAP_LISTEN(password, "users", users::users, check_user_security, _1);
    SNAP_LISTEN(password, "users", users::users, user_logged_in, _1);
    SNAP_LISTEN(password, "users", users::users, save_password, _1, _2, _3);
    SNAP_LISTEN(password, "users", users::users, invalid_password, _1, _2);
    SNAP_LISTEN(password, "users", users::users, blocked_user, _1, _2);
}


/** \brief Add the password widgets to the editor XSLT.
 *
 * The editor is extended by the password plugin by adding a password
 * and a password + confirm widgets.
 *
 * \param[in] e  A pointer to the editor plugin.
 */
void password::on_prepare_editor_form(editor::editor * e)
{
    e->add_editor_widget_templates_from_file(":/xsl/password_widgets/password-form.xsl");
}


/** \brief Check for a password/confirm widget.
 *
 * This function gets called any time a field is initialized for use
 * in the editor. Here we check for the widget type, if it is
 * a password/confirm widget, then we add the policy information in
 * JavaScript.
 *
 * \param[in,out] ipath  The path where the form appears.
 * \param[in] field_id  The name of the field.
 * \param[in] field_type  The type of the field.
 * \param[in,out] widget  The concerned widget.
 * \param[in,out] data_row  The row pointer to by ipath.
 */
void password::on_init_editor_widget(content::path_info_t & ipath, QString const & field_id, QString const & field_type, QDomElement & widget, libdbproxy::row::pointer_t data_row)
{
    NOTUSED(ipath);
    NOTUSED(field_id);
    NOTUSED(widget);
    NOTUSED(data_row);

    f_added_policy = f_added_policy || field_type == "password_confirm";
}


/** \brief Capture various hits to the website to process some AJAX calls.
 *
 * The blacklist page and some other such pages receive AJAX requests
 * that are not specific to the editor and this function will handle them.
 *
 * \param[in,out] ipath  The path being processed.
 *
 * \return true if the path was handled.
 */
bool password::on_path_execute(content::path_info_t & ipath)
{
    QString const action(ipath.get_parameter("action"));
    if(action == "administer")
    {
        QString const password_function(f_snap->postenv("password_function"));
        if(password_function == "is_password_blacklisted")
        {
            on_path_execute__is_password_blacklisted(ipath);
            return true;
        }
        if(password_function == "blacklist_new_passwords")
        {
            on_path_execute__blacklist_new_passwords(ipath);
            return true;
        }
        if(password_function == "blacklist_remove_passwords")
        {
            on_path_execute__blacklist_remove_passwords(ipath);
            return true;
        }
    }

    // the default is to call the output() function and let it
    // do whatever it does by default
    //
    f_snap->output(layout::layout::instance()->apply_layout(ipath, this));

    return true;
}


/** \brief Generate the main content of a page handled by this plugin.
 *
 * Some pages are owned by the password plugin and this function is
 * used to generate the output. This is used because we want to
 * capture some of the hits when a page sends us an AJAX request.
 *
 * \param[in] ipath  The path being rendered.
 * \param[in,out] page  The page elements.
 * \param[in,out] body  The body element.
 */
void password::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    output::output::instance()->on_generate_main_content(ipath, page, body);
}


/** \brief Add the policy if we have a password/confirm widget.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The XML element named "page".
 * \param[in,out] body  The XML element named "body".
 */
void password::on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    NOTUSED(ipath);
    NOTUSED(body);

    if(f_added_policy)
    {
        policy_t const pp("users");

        QString const code(QString(
            "/* password plugin: policy */"
            "password__policy__minimum_length=%1;"
            "password__policy__minimum_lowercase_letters=%2;"
            "password__policy__minimum_uppercase_letters=%3;"
            "password__policy__minimum_letters=%4;"
            "password__policy__minimum_digits=%5;"
            "password__policy__minimum_spaces=%6;"
            "password__policy__minimum_specials=%7;"
            "password__policy__minimum_unicode=%8;"
            "password__policy__minimum_variation=%9;"
            "password__policy__minimum_length_of_variations=%10;")
                    .arg(pp.get_minimum_length())
                    .arg(pp.get_minimum_lowercase_letters())
                    .arg(pp.get_minimum_uppercase_letters())
                    .arg(pp.get_minimum_letters())
                    .arg(pp.get_minimum_digits())
                    .arg(pp.get_minimum_spaces())
                    .arg(pp.get_minimum_specials())
                    .arg(pp.get_minimum_unicode())
                    .arg(pp.get_minimum_variation())
                    .arg(pp.get_minimum_length_of_variations()));
        content::content * content_plugin(content::content::instance());
        content_plugin->add_inline_javascript(page.ownerDocument(), code);
    }
}


/** \brief Check whether a password is blacklisted or not.
 *
 * This function is called whenever we receive an AJAX request from
 * the blacklist manager page.
 *
 * It generates an AJAX response informing the client on whether
 * the specified password is indeed blacklisted or not.
 *
 * \param[in,out] ipath  The path information.
 */
void password::on_path_execute__is_password_blacklisted(content::path_info_t & ipath)
{
    QString const user_password(f_snap->postenv("password").toLower());
    libdbproxy::table::pointer_t table(password::password::instance()->get_password_table());
    if(table->exists(user_password))
    {
        messages::messages::instance()->set_info(
            "Blacklisted!",
            QString("The password \"%1\" is in your password blacklist. No one will be able to use it.").arg(user_password)
        );
    }
    else
    {
        messages::messages::instance()->set_warning(
            "Not Blacklisted",
            QString("Password \"%1\" is not blacklisted. One of your users can still make use of it, assuming the corresponding policy allows it.").arg(user_password),
            "Sending answer querying about whether a password is blacklisted."
        );
    }

    // create the AJAX response
    server_access::server_access * server_access_plugin(server_access::server_access::instance());
    server_access_plugin->create_ajax_result(ipath, true);
    server_access_plugin->ajax_output();
}


/** \brief Check whether a password is blacklisted or not.
 *
 * This function is called whenever we receive an AJAX request from
 * the blacklist manager page.
 *
 * It will add the specified passwords (in the "password" POST variable)
 * to the password blacklist.
 *
 * \param[in,out] ipath  The path information.
 */
void password::on_path_execute__blacklist_new_passwords(content::path_info_t & ipath)
{
    blacklist_t bl;
    bl.add_passwords(f_snap->postenv("password"));

    size_t const count(bl.passwords_applied());
    size_t const skipped(bl.passwords_skipped());

    if(count > 0)
    {
        messages::messages::instance()->set_info(
            "Blacklisted",
            QString("%1 password%2%3 %4 added to your password blacklist.")
                    .arg(count)
                    .arg(count != 1 ? "s" : "")
                    .arg(skipped > 0 ? QString(" (out of %1 passwords)").arg(count + skipped) : "")
                    .arg(count != 1 ? "were" : "was")
        );
    }
    else
    {
        messages::messages::instance()->set_warning(
            "Already Blacklisted",
            "All of these passwords were already blacklisted.",
            "Letting user know that all the passwords he specified were already in his password blacklist."
        );
    }

    // create the AJAX response
    server_access::server_access * server_access_plugin(server_access::server_access::instance());
    server_access_plugin->create_ajax_result(ipath, true);
    server_access_plugin->ajax_output();
}


/** \brief Remove the specified list of passwords from the password blacklist.
 *
 * This function is called whenever we receive an AJAX request from
 * the blacklist manager page.
 *
 * It will remove the specified passwords (in the "password" POST variable)
 * from the password blacklist.
 *
 * \param[in,out] ipath  The path information.
 */
void password::on_path_execute__blacklist_remove_passwords(content::path_info_t & ipath)
{
    blacklist_t bl;
    bl.remove_passwords(f_snap->postenv("password"));

    size_t const count(bl.passwords_applied());
    size_t const skipped(bl.passwords_skipped());

    if(count > 0)
    {
        messages::messages::instance()->set_info(
            "Whitelisted",
            QString("%1 password%2%3 %4 removed from your password blacklist.")
                    .arg(count)
                    .arg(count != 1 ? "s" : "")
                    .arg(skipped > 0 ? QString(" (out of %1 passwords)").arg(count + skipped) : "")
                    .arg(count != 1 ? "were" : "was")
        );
    }
    else
    {
        messages::messages::instance()->set_warning(
            "Not Blacklisted",
            "All of these passwords were not in your password blacklist.",
            "Letting user know that none of the passwords he specified were in his password blacklist."
        );
    }

    // create the AJAX response
    server_access::server_access * server_access_plugin(server_access::server_access::instance());
    server_access_plugin->create_ajax_result(ipath, true);
    server_access_plugin->ajax_output();
}


/** \brief Initialize the password table.
 *
 * This function creates the password table if it does not exist yet.
 * Otherwise it simple initializes the f_password_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The password table is used to record passwords that get blacklisted.
 * All of those are exclusively coming from the backend. There is
 * no interface on the website to add invalid password to avoid any
 * problems.
 *
 * \return The pointer to the list table.
 */
libdbproxy::table::pointer_t password::get_password_table()
{
    if(!f_password_table)
    {
        f_password_table = f_snap->get_table(get_name(name_t::SNAP_NAME_PASSWORD_TABLE));
    }
    return f_password_table;
}


/** \brief Check a password of a user.
 *
 * This function checks the user password for strength and against a
 * blacklist.
 *
 * \note
 * The password may be set to "!" in which case it gets ignored. This
 * is because "!" cannot be valid as the editor will enforce a length
 * of at least 8 characters (10 by default) and thus "!" cannot in
 * any way represent a password entered by the end user.
 *
 * \param[in,out] secure  Whether the password / user is considered secure.
 */
void password::on_check_user_security(users::users::user_security_t & security)
{
    if(!security.get_secure().allowed()
    || !security.has_password())
    {
        return;
    }

    QString const reason(check_password_against_policy(security.get_user_info(), security.get_password(), security.get_policy()));
    if(!reason.isEmpty())
    {
        SNAP_LOG_TRACE("password::on_check_user_security(): password was not accepted: ")(reason);
        security.get_secure().not_permitted(reason);
        security.set_status(users::users::status_t::STATUS_PASSWORD);
    }
}


/** \brief Check password against a specific policy.
 *
 * This function is used to calculate the strength of a password depending
 * on a policy.
 *
 * When the \p user_info parameter is specified (not the empty string) then
 * the new \p user_password is eventually checked against the old passwords
 * the user used.
 *
 * \warning
 * No user email address or user identifier are available in user_info
 * when a new user is being created.
 *
 * \param[in] user_info      The user info (email/id reference in users table).
 * \param[in] user_password  The password being checked.
 * \param[in] policy         The policy used to verify the password strength.
 *
 * \return A string with some form of error message about the password
 *         weakness(es) or an empty string if the password is okay.
 */
QString password::check_password_against_policy(users::users::user_info_t user_info, QString const & user_password, QString const & policy)
{
    policy_t const pp(policy);

    policy_t up;
    up.count_password_characters(user_password);

    // check whether any counter is too low to be a match with this policy
    //
    QString const too_small(up.compare(pp));
    if(!too_small.isEmpty())
    {
        return too_small;
    }

    // check whether this password is in the password blacklist
    //
    QString const blacklisted(pp.is_blacklisted(user_password));
    if(!blacklisted.isEmpty())
    {
        return blacklisted;
    }

    // TODO: add test against the username once we have that feature
    //       available; this is checked against the password with
    //       the Levenshtein fuzzy string compare function.
#if 0
    QString const username("random");
    int64_t check_username(pp.get_check_username());
    if(check_username > 0)
    {
        // TODO: this needs to breakup the password in length equal
        //       to username.length() + 0,1,2..,check_username and
        //       each version of the password checked against username;
        //       as a bonus we can repeat the test with the username
        //       string reversed
        int const r(strstr_with_levenshtein_distance(user_password.toLower().toStdWString(), username.toLower().toStdWString(), check_username));
        if(r >= check_username)
        {
            return "your username cannot, even as an approximation, appear in your password";
        }
        if(pp.get_check_username_reversed())
        {
            QString username_reversed;
            username_reversed.reserve(username.length());
            std::reverse_copy(username.begin(), username.end(), username_reversed.begin());
            // TODO: same as above, we want to use a function which search
            //       the reversed username within the password
            int const rr(strstr_with_levenshtein_distance(user_password.toLower().toStdWString(), username_reversed.toLower().toStdWString(), check_username));
            if(rr >= check_username)
            {
                return "your username cannot, even as a reversed approximation, appear in your password";
            }
        }
    }
#endif

    // now verify that the password is new, that the user is not
    // reusing an old password
    //
    if(pp.get_prevent_old_passwords()
    && user_info.is_valid())  // WARNING: this may not be the current user, so do not check whether it is logged in.
    {
        int64_t const minimum_count(pp.get_minimum_old_passwords());
        int64_t const maximum_age(pp.get_old_passwords_maximum_age());
        int64_t const age_limit(f_snap->get_start_date() - maximum_age * 86400LL * 1000000LL);

        QString result;

        for(int64_t idx(1);; ++idx)
        {
            // if no such password entry exists, we are done
            //
            QString const password_name(QString("%1_%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD)).arg(idx));
            if( !user_info.value_exists(password_name) )
            {
                break;
            }

            // see whether that old password timed out, if so, we want
            // to delete it (and any following passwords)
            //
            QString const password_modified_name(QString("%1_%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_MODIFIED)).arg(idx));
            libdbproxy::value const old_password_modified(user_info.get_value(password_modified_name));
            int64_t const password_start_date(old_password_modified.safeInt64Value(0, 0));
            if(idx >= minimum_count && password_start_date < age_limit)
            {
                // delete all the passwords starting at drop and on
                //
                // we have to loop because the user may not have come here in
                // a long time or he/she may have had many password changes
                // at some point and all are now timed out
                //
                for(;; ++idx)
                {
                    QString const old_password_name(QString("%1_%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD)).arg(idx));
                    //if(!row->exists(old_password_name))
                    if(!user_info.value_exists(old_password_name))
                    {
                        // no more passwords, we stop now
                        break;
                    }

                    QString const old_password_modified_name(QString("%1_%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_MODIFIED)).arg(idx));
                    QString const old_password_salt_name    (QString("%1_%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_SALT    )).arg(idx));
                    QString const old_password_digest_name  (QString("%1_%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_DIGEST  )).arg(idx));

                    //row->dropCell(old_password_name);
                    user_info.delete_value(old_password_name);
                    user_info.delete_value(old_password_modified_name);
                    user_info.delete_value(old_password_salt_name);
                    user_info.delete_value(old_password_digest_name);
                }
                break;
            }

            // no error yet?
            //
            if(result.isEmpty())
            {
                QString const password_salt_name  (QString("%1_%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_SALT  )).arg(idx));
                QString const password_digest_name(QString("%1_%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_DIGEST)).arg(idx));

                //libdbproxy::value const old_password       (row->getCell(password_name         )->getValue());
                libdbproxy::value const old_password       (user_info.get_value(password_name         ));
                libdbproxy::value const old_password_salt  (user_info.get_value(password_salt_name    ));
                libdbproxy::value const old_password_digest(user_info.get_value(password_digest_name  ));

                // we have to encrypt the new password with the old digest to
                // get a hash similar to the saved hash
                //
                users::users * users_plugin(users::users::instance());

                QByteArray hash;
                users_plugin->encrypt_password(old_password_digest.stringValue(), user_password, old_password_salt.binaryValue(), hash);
                if(old_password.size() == hash.size()
                && memcmp(old_password.binaryValue().data(), hash.data(), hash.size()) == 0)
                {
                    // this is an old password, prevent its use
                    //
                    result = "you used this password before and cannot reuse it at this time, please try again with a new password";

                    // WARNING: here we continue looping so that way we
                    //          can remove old password which is important
                    //          because we do not want to hold on really
                    //          old passwords forever
                }
            }
        }

        if(!result.isEmpty())
        {
            return result;
        }
    }

    return QString();
}


/** \brief Create a default password.
 *
 * In some cases an administrator may want to create an account for a user
 * which should then have a valid, albeit unknown, password.
 *
 * This function can be used to create that password.
 *
 * It is strongly advised to NOT send such passwords to the user via email
 * because they will contain all sorts of "strange" characters and emails
 * are notoriously not safe.
 *
 * The password will be at least 64 characters, more if the policy
 * requires more. The type of characters is also defined by the
 * policy and quite shuffled before the function returns.
 *
 * \param[in] policy  Create password that is valid for this policy.
 *
 * \return The string with the new password.
 */
QString password::create_password(QString const & policy)
{
    // to create a password that validates against a certain policy
    // we have to make sure that we have all the criterias covered
    // so we need to have the policy information and generate the
    // password as expected
    //
    policy_t pp(policy);

    random_generator<256> gen;

    QString result;

    // to generate characters of each given type, we loop through
    // each set and then we randomize the final string
    //
    int64_t const minimum_lowercase_letters(pp.get_minimum_lowercase_letters());
    if(minimum_lowercase_letters > 0)
    {
        for(int64_t idx(0); idx < minimum_lowercase_letters; ++idx)
        {
            // lower case letters are between 'a' and 'z'
            ushort const c(gen.get_byte() % 26 + 'a');
            result += QChar(c);
        }
    }

    int64_t const minimum_uppercase_letters(pp.get_minimum_uppercase_letters());
    if(minimum_uppercase_letters > 0)
    {
        for(int64_t idx(0); idx < minimum_uppercase_letters; ++idx)
        {
            // lower case letters are between 'A' and 'Z'
            ushort const c(gen.get_byte() % 26 + 'A');
            result += QChar(c);
        }
    }

    int64_t const minimum_letters(pp.get_minimum_letters());
    if(minimum_letters > minimum_lowercase_letters + minimum_uppercase_letters)
    {
        for(int64_t idx(minimum_lowercase_letters + minimum_uppercase_letters); idx < minimum_uppercase_letters; ++idx)
        {
            // letters are between 'A' and 'Z' or 'a' and 'z'
            ushort c(gen.get_byte() % (26 * 2) + 'A');
            if(c > 'Z')
            {
                c += 'a' - 'Z' - 1;
            }
            result += QChar(c);
        }
    }

    int64_t const minimum_digits(pp.get_minimum_digits());
    if(minimum_digits > 0)
    {
        for(int64_t idx(0); idx < minimum_digits; ++idx)
        {
            // digits are between '0' and '9'
            int byte(gen.get_byte());
            ushort const c1(byte % 10 + '0');
            result += QChar(c1);
            if(idx + 1 < minimum_digits)
            {
                ++idx;
                ushort const c2(byte / 10 % 10 + '0');
                result += QChar(c2);
            }
        }
    }

    int64_t const minimum_spaces(pp.get_minimum_spaces());
    if(minimum_spaces > 0)
    {
        for(int64_t idx(0); idx < minimum_spaces; ++idx)
        {
            // TBD: should we support all the different types of
            //      spaces instead?
            ushort const c(' ');
            result += QChar(c);
        }
    }

    int64_t const minimum_specials(pp.get_minimum_specials());
    if(minimum_specials > minimum_spaces)
    {
        for(int64_t idx(minimum_spaces); idx < minimum_specials; )
        {
            ushort const byte(gen.get_byte());
            QChar const c(byte);
            switch(c.category())
            {
            case QChar::Letter_Lowercase:
            case QChar::Letter_Other:
            case QChar::Letter_Uppercase:
            case QChar::Letter_Titlecase:
            case QChar::Number_DecimalDigit:
            case QChar::Number_Letter:
            case QChar::Number_Other:
            case QChar::Mark_SpacingCombining:
            case QChar::Separator_Space:
            case QChar::Separator_Line:
            case QChar::Separator_Paragraph:
                break;

            default:
                result += c;
                ++idx;
                break;

            }
        }
    }

    int64_t const minimum_unicode(pp.get_minimum_unicode());
    if(minimum_unicode > 0)
    {
        for(int64_t idx(0); idx < minimum_unicode; )
        {
            // Unicode are characters over 0x0100, although
            // we avoid surrogates because they are more complicated
            // to handle and not as many characters are assigned in
            // those pages
            //
            ushort const s((gen.get_byte() << 8) | gen.get_byte());
            if(s >= 0x0100 && (s < 0xD800 || s > 0xDFFF))
            {
                QChar const c(s);
                if(c.unicodeVersion() != QChar::Unicode_Unassigned)
                {
                    // only keep assigned (known) unicode characters
                    //
                    // TODO: 
                    //
                    result += c;
                    ++idx;
                }
            }
        }
    }

    // we want a minimum of 64 character long passwords at this point
    //
    for(int64_t const minimum_length(std::max(pp.get_minimum_length(), static_cast<int64_t>(64))); result.length() < minimum_length; )
    {
        // include some other characters from the ASCII range to reach
        // the minimum length of the policy
        //
        ushort const byte(gen.get_byte() % (0x7E - 0x20 + 1) + 0x20);
        result += QChar(byte);
    }

    // shuffle all the characters once so that way it does not appear
    // in the order it was created above
    //
    for(int j(0); j < result.length(); ++j)
    {
        int i(0);
        if(result.length() < 256)
        {
            i = gen.get_byte() % result.length();
        }
        else
        {
            i = ((gen.get_byte() << 8) | gen.get_byte()) % result.length();
        }
        QChar c(result[i]);
        result[i] = result[j];
        result[j] = c;
    }

    // make sure that it worked as expected
    //
    QString const reason(check_password_against_policy(users::users::user_info_t(), result, policy));
    if(!reason.isEmpty())
    {
        throw snap_logic_exception("somehow we generated a password that did not match the policy we were working against...");
    }

    return result;
}


/** \brief Check whether the user password timed out.
 *
 * The last time the password was changed is saved in the users
 * table. If that password was last changed a long time ago
 * and the current "users" policy says that we should timeout
 * the password, then this function makes sure the user is
 * forced to change his password.
 *
 * \param[in] logged_info  The information about the user being logged in.
 */
void password::on_user_logged_in(users::users::user_logged_info_t & logged_info)
{
    // load the policy
    policy_t const pp(logged_info.get_password_policy());

    // policy limits password lifespan?
    if(pp.get_limit_duration())
    {
        // duration limited to... (in microseconds)
        int64_t const duration(pp.get_maximum_duration() * 86400LL * 1000000LL);

        // retrieve the last modification time of this user's password
        auto const & user_info(logged_info.get_user_info());
        int64_t const last_modified(user_info.get_value(users::name_t::SNAP_NAME_USERS_PASSWORD_MODIFIED).safeInt64Value(0, 0));

        // compare against current date
        int64_t const start_date(f_snap->get_start_date());
        if(last_modified != 0
        && last_modified + duration < start_date)
        {
            // password was last modified a long time ago and needs to be
            // replaced now
            //
            logged_info.force_user_to_change_password();
        }
    }
}


void password::on_save_password(users::users::user_info_t & user_info, QString const & user_password, QString const & password_policy)
{
    NOTUSED(user_password);

    //if(!row->exists(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD)))
    if(!user_info.value_exists(users::name_t::SNAP_NAME_USERS_PASSWORD))
    {
        return;
    }

    policy_t const pp(password_policy);
    if(!pp.get_prevent_old_passwords())
    {
        return;
    }

    // if a password already exists, make sure to make a copy
    //
    // the copies are kept to force users to not reuse an old
    // password; we copy evenrything because to check the
    // password we need the salt and digest information
    //
    // copies are organized in two main ways:
    //
    // . number of copies
    // . total amount of time we keep a password
    //
    // The number of copies is a minimum, it may grow over if we
    // are to keep passwords for longer and the user changes his
    // password often; however, we will keep at least that many
    // even if the time elapses (i.e. if you have a policy that
    // requires 5 copies and they time out after 1 year, a user
    // with 3 old passwords will be kept as is even after a year)
    //
    // this code does a full roll of all the password history;
    //

    int64_t const start_date(f_snap->get_start_date());
    int64_t const old_password(start_date - pp.get_old_passwords_maximum_age() * 86400LL * 1000000LL);
    int64_t const minimum_count(pp.get_minimum_old_passwords());

    //libdbproxy::value previous_password         (row->getCell(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD         ))->getValue());
    libdbproxy::value previous_password         (user_info.get_value(users::name_t::SNAP_NAME_USERS_PASSWORD         ));
    libdbproxy::value previous_password_modified(user_info.get_value(users::name_t::SNAP_NAME_USERS_PASSWORD_MODIFIED));
    libdbproxy::value previous_password_salt    (user_info.get_value(users::name_t::SNAP_NAME_USERS_PASSWORD_SALT    ));
    libdbproxy::value previous_password_digest  (user_info.get_value(users::name_t::SNAP_NAME_USERS_PASSWORD_DIGEST  ));

    bool more(true);
    int64_t drop(0);
    for(int64_t idx(1); more; ++idx)
    {
        // define the names of the next data entries
        QString const password_name         (QString("%1_%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD         )).arg(idx));
        QString const password_modified_name(QString("%1_%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_MODIFIED)).arg(idx));
        QString const password_salt_name    (QString("%1_%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_SALT    )).arg(idx));
        QString const password_digest_name  (QString("%1_%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_DIGEST  )).arg(idx));

        libdbproxy::value next_password;
        libdbproxy::value next_password_modified;
        libdbproxy::value next_password_salt;
        libdbproxy::value next_password_digest;

        //if(row->exists(password_name))
        if(user_info.value_exists(password_name))
        {
            //next_password_modified = row->getCell(password_modified_name)->getValue();
            next_password_modified = user_info.get_value(password_modified_name);
            int64_t const password_start_date(next_password_modified.safeInt64Value(0, 0));
            if(idx >= minimum_count && password_start_date < old_password)
            {
                 more = false;
                 drop = idx;
            }
            else
            {
                //next_password          = row->getCell(password_name         )->getValue();
                next_password          = user_info.get_value(password_name         );
                next_password_salt     = user_info.get_value(password_salt_name    );
                next_password_digest   = user_info.get_value(password_digest_name  );
            }
        }
        else
        {
            more = false;
        }

        //row->getCell(password_name         )->setValue(previous_password);
        user_info.set_value(password_name         , previous_password);
        user_info.set_value(password_modified_name, previous_password_modified);
        user_info.set_value(password_salt_name    , previous_password_salt);
        user_info.set_value(password_digest_name  , previous_password_digest);

        previous_password          = next_password;
        previous_password_modified = next_password_modified;
        previous_password_salt     = next_password_salt;
        previous_password_digest   = next_password_digest;
    }

    if(drop > 0)
    {
        // delete all the passwords starting at drop and on
        //
        // we have to loop because the user may not have come here in
        // a long time or he/she may have had many password changes
        // at some point and all are now timed out
        //
        for(;; ++drop)
        {
            QString const password_name(QString("%1_%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD)).arg(drop));
            //if(!row->exists(password_name))
            if(!user_info.value_exists(password_name))
            {
                // no more passwords, we stop now
                break;
            }

            QString const password_modified_name(QString("%1_%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_MODIFIED)).arg(drop));
            QString const password_salt_name    (QString("%1_%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_SALT    )).arg(drop));
            QString const password_digest_name  (QString("%1_%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_DIGEST  )).arg(drop));

            //row->dropCell(password_name);
            user_info.delete_value(password_name);
            user_info.delete_value(password_modified_name);
            user_info.delete_value(password_salt_name);
            user_info.delete_value(password_digest_name);
        }
    }
}


/** \brief User entered an invalid password.
 *
 * This function gets called whenever the user enters an invalid password.
 * The function increments a counter to know how many times the user entered
 * an invalid password.
 *
 * After a certain number of times, the system reacts by blocking the user
 * for a temporary amount of time.
 *
 * \param[in] user_info  The row in the users table of the user who failed testing
 *                       his/her password.
 * \param[in] policy     The policy concerned with this invalid password.
 */
void password::on_invalid_password(users::users::user_info_t & user_info, QString const & policy)
{
    policy_t pp(policy);

    int64_t count(0);

    // increase failure counter
    {
        snap_lock lock(user_info.get_user_key()); // TODO: change to id

        libdbproxy::value count_failures(user_info.get_value(get_name(name_t::SNAP_NAME_PASSWORD_COUNT_FAILURES)));
        count = count_failures.safeInt64Value(0, 0) + 1LL;
        count_failures.setInt64Value(count);
        count_failures.setTtl(pp.get_invalid_passwords_counter_lifetime() * 60LL * 60LL);
        user_info.set_value(get_name(name_t::SNAP_NAME_PASSWORD_COUNT_FAILURES),count_failures);
    }

    if(count > pp.get_invalid_passwords_counter())
    {
        // user tried too many times, add a temporary block
        //
        libdbproxy::value value;
        value.setSignedCharValue(1);
        value.setTtl(pp.get_invalid_passwords_block_duration() * 60LL * 60LL);
        user_info.set_value(users::name_t::SNAP_NAME_USERS_PASSWORD_BLOCKED,value);
    }

    //
    // this could generate an Apache2 timeout error once the counter is
    // 'pretty large'...
    //
    // If so, you may increase your Apache2 TimeOut parameter
    //
    // IMPORTANT NOTE: Although we could send this sleep()
    //                 amount to our snap.cgi, we do not because
    //                 the we envision to get rid of snap.cgi and
    //                 Apache2 at some point...
    //
    sleep((count - 1) * pp.get_invalid_passwords_slowdown());
}


/** \brief Once a user is blocked, call this on each further login attempt.
 *
 * This function further counts the number of login attempts that are
 * invalid. This allows us to block the user IP address instead of just
 * blocking the log in process itself.
 *
 * The duration is defined by the blocked user counter lifetime
 * and the blocked user firewall duration. The number of times
 * the user can attempt once the login is blocked is defined
 * by the blocked user counter.
 *
 * The time is defined in days (instead of hours for the login block.)
 *
 * \param[in] user_info  The row in the users table of the user who failed testing
 *                       his/her password.
 * \param[in] policy     The policy concerned with this invalid password.
 */
void password::on_blocked_user(users::users::user_info_t & user_info, QString const & policy)
{
    policy_t pp(policy);

    int64_t count(0);

    {
        snap_lock lock(user_info.get_user_key());  // TODO: change to id

        libdbproxy::value count_503s(user_info.get_value(get_name(name_t::SNAP_NAME_PASSWORD_COUNT_BAD_PASSWORD_503S)));
        count = count_503s.safeInt64Value(0, 0) + 1LL;
        count_503s.setInt64Value(count);
        count_503s.setTtl(pp.get_blocked_user_counter_lifetime() * 24LL * 60LL * 60LL);
        user_info.set_value(get_name(name_t::SNAP_NAME_PASSWORD_COUNT_BAD_PASSWORD_503S),count_503s);
    }

    // WARNING: This counter does not get incremented if the user enters
    //          his password properly; for this reason, we use a bit of
    //          randomness here to make sure that hackers cannot determine
    //          whether one of the passwords they entered is the correct
    //          one... (i.e. the number of times a hacker can enter an
    //          invalid password after the user was blocked will vary
    //          slightly: <block-user-counter> + (0 to 10)
    //
    //          This means the hacker cannot know that one of the passwords
    //          he entered while receiving 503 errors is the one.
    //
    random_generator<1> gen;
    int const jitter(gen.get_byte() % 11);
    if(count > pp.get_blocked_user_counter() + jitter)
    {
        // user tried too many times, now tell the firewall about it
        //
        // TBD: we may still want to define a way to tell the firewall
        //      how long it should block the user in days rather than
        //      1 day, 1 week, 1 month...
        //
        //libdbproxy::value value;
        //value.setSignedCharValue(1);
        //value.setTtl(pp.get_blocked_user_firewall_duration() * 24LL * 60LL * 60LL);
        //row->getCell(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_BLOCKED))->setValue(value);
        QString const remote_addr(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REMOTE_ADDR)));
        server::block_ip(remote_addr, pp.get_blocked_user_firewall_duration(), "password plugin blocking user on too many login attempts");
    }
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
