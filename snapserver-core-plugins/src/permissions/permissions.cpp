// Snap Websites Server -- manage permissions for users, forms, etc.
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

//#define SHOW_RIGHTS

#include "permissions.h"

#include "../output/output.h"
#include "../users/users.h"
#include "../messages/messages.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/plugins.h>
#include <snapwebsites/qstring_stream.h>

#include <libdbproxy/value.h>

#include <openssl/rand.h>

#include <snapwebsites/poison.h>

SNAP_PLUGIN_START(permissions, 1, 0)



namespace details
{

// cache table from the content plugin
libdbproxy::table::pointer_t     g_cache_table;

// when client does a reload, we want to regenerate each permissions only
// once so we save the URIs in this vector (we suspect that we receive
// such requests only once and then quit this snap_child instance)
//
QMap<QString, bool>                         g_user_cache_reviewed;
QMap<QString, bool>                         g_plugin_cache_reviewed;


name_t login_status_from_string(QString const & status)
{
    if(status == "spammer")
    {
        return name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_SPAMMER;
    }
    else if(status == "visitor")
    {
        return name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR;
    }
    else if(status == "returning_visitor")
    {
        return name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_VISITOR;
    }
    else if(status == "returning_registered")
    {
        return name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_REGISTERED;
    }
    else if(status == "registered")
    {
        return name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED;
    }
    else
    {
        throw snap_expr::snap_expr_exception_invalid_parameter_value("invalid parameter value to for status expected one of: spammer, visitor, returning_visitor, returning_registered, or registered");
    }
}

}


/** \enum name_t
 * \brief Names used by the permissions plugin.
 *
 * This enumeration is used to avoid entering the same names over and
 * over and the likelihood of misspelling that name once in a while.
 */


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
    case name_t::SNAP_NAME_PERMISSIONS_ACTION_ADMINISTER:
        return "permissions::action::administer";

    case name_t::SNAP_NAME_PERMISSIONS_ACTION_DELETE:
        return "permissions::action::delete";

    case name_t::SNAP_NAME_PERMISSIONS_ACTION_EDIT:
        return "permissions::action::edit";

    case name_t::SNAP_NAME_PERMISSIONS_ACTION_NAMESPACE:
        return "action";

    case name_t::SNAP_NAME_PERMISSIONS_ACTION_PATH:
        return "types/permissions/actions";

    case name_t::SNAP_NAME_PERMISSIONS_ACTION_VIEW:
        return "permissions::action::view";

    case name_t::SNAP_NAME_PERMISSIONS_ADMINISTER_NAMESPACE:
        return "administer";

    case name_t::SNAP_NAME_PERMISSIONS_CHECK_PERMISSIONS:
        return "checkpermissions";

    case name_t::SNAP_NAME_PERMISSIONS_DIRECT_ACTION_ADMINISTER:
        return "permissions::direct::action::administer";

    case name_t::SNAP_NAME_PERMISSIONS_DIRECT_ACTION_DELETE:
        return "permissions::direct::action::delete";

    case name_t::SNAP_NAME_PERMISSIONS_DIRECT_ACTION_EDIT:
        return "permissions::direct::action::edit";

    case name_t::SNAP_NAME_PERMISSIONS_DIRECT_ACTION_VIEW:
        return "permissions::direct::action::view";

    case name_t::SNAP_NAME_PERMISSIONS_DIRECT_GROUP:
        return "permissions::direct::group";

    case name_t::SNAP_NAME_PERMISSIONS_DIRECT_GROUP_RETURNING_REGISTERED_USER:
        return "permissions::direct::group::returning_registered_user";

    case name_t::SNAP_NAME_PERMISSIONS_DIRECT_NAMESPACE:
        return "direct";

    case name_t::SNAP_NAME_PERMISSIONS_DYNAMIC:
        return "permissions::dynamic";

    case name_t::SNAP_NAME_PERMISSIONS_EDIT_NAMESPACE:
        return "edit";

    case name_t::SNAP_NAME_PERMISSIONS_GROUP_NAMESPACE:
        return "group";

    case name_t::SNAP_NAME_PERMISSIONS_GROUPS_PATH:
        return "types/permissions/groups";

    case name_t::SNAP_NAME_PERMISSIONS_LAST_UPDATED:
        return "permissions::last_updated";

    case name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_ADMINISTER:
        return "permissions::link_back::administer";

    case name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_DELETE:
        return "permissions::link_back::delete";

    case name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_EDIT:
        return "permissions::link_back::edit";

    case name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_GROUP:
        return "permissions::link_back::group";

    case name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_NAMESPACE:
        return "link_back";

    case name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_VIEW:
        return "permissions::link_back::view";

    case name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_SPAMMER:
        return "permissions::login_status::spammer";

    case name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR:
        return "permissions::login_status::visitor";

    case name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_VISITOR:
        return "permissions::login_status::returning_visitor";

    case name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_REGISTERED:
        return "permissions::login_status::returning_registered";

    case name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED:
        return "permissions::login_status::registered";

    case name_t::SNAP_NAME_PERMISSIONS_MAKE_ADMINISTRATOR:
        return "makeadministrator";

    case name_t::SNAP_NAME_PERMISSIONS_MAKE_ROOT:
        return "makeroot";

    case name_t::SNAP_NAME_PERMISSIONS_NAMESPACE:
        return "permissions";

    case name_t::SNAP_NAME_PERMISSIONS_PATH:
        return "types/permissions";

    case name_t::SNAP_NAME_PERMISSIONS_PLUGIN:
        return "plugin";

    case name_t::SNAP_NAME_PERMISSIONS_RIGHTS_PATH:
        return "types/permissions/rights";

    case name_t::SNAP_NAME_PERMISSIONS_SECURE_PAGE:
        return "permissions::secure_page";

    case name_t::SNAP_NAME_PERMISSIONS_SECURE_SITE:
        return "permissions::secure_site";

    case name_t::SNAP_NAME_PERMISSIONS_STATUS_PATH:
        return "types/permissions/status";

    case name_t::SNAP_NAME_PERMISSIONS_USERS_PATH:
        return "types/permissions/users";

    case name_t::SNAP_NAME_PERMISSIONS_VIEW_NAMESPACE:
        return "view";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_PERMISSIONS_...");

    }
    NOTREACHED();
}


/** \class snap::permissions::permissions::sets_t
 * \brief Handle sets of permissions.
 *
 * The permissions are represented by sets. A permission set includes rights,
 * which are paths to different permission types. Each plugin can offer its
 * own specific rights or make use of rights offered by other plugins.
 *
 * The user is given rights depending on his status on the website. A simple
 * visitor will only get a very few rights. A full administrator will have
 * many rights.
 *
 * Rights are represented by paths to types. For example, you could be given
 * the right to tweak basic information on your website with this type:
 *
 * \code
 * /types/permissions/rights/administer/website/info
 * \endcode
 *
 * The interesting aspect of having a path is that by itself it already
 * represents a set. So the filter module offers a filter right as follow:
 *
 * \code
 * /types/permissions/rights/administer/website/filter
 * \endcode
 *
 * A user who has the .../website/info right does not have the
 * .../website/fitler right. However, a user who has the .../website
 * right is allowed to access both: .../website/info and .../website/filter.
 * This is because the parent of a right gives the user all the rights
 * below that parent.
 *
 * In the following, the user has access:
 *
 * \code
 * [User]
 * /type/permissions/rights/administer/website
 *
 * [Plugins]
 * /type/permissions/rights/administer/website/info
 * /type/permissions/rights/administer/website/filter
 * \endcode
 *
 * However, that works the other way around in the permissions set. This
 * means the parent is required by the user if the parent is required
 * by the plugin.
 *
 * In the following, the user does not have access:
 *
 * \code
 * [User]
 * /type/permissions/rights/administer/website/info
 * /type/permissions/rights/administer/website/filter
 *
 * [Plugins]
 * /type/permissions/rights/administer/website
 * \endcode
 *
 * A top administrator has the full administer right:
 *
 * \code
 * /type/permissions/rights/administer
 * \endcode
 *
 * Because of those special features, we do not use the QSet class
 * because QSet doesn't know anything about paths, parent/children, etc.
 * Thus we use maps and vectors and compute the intersections ourselves.
 * Actually, we very much avoid inserting duplicates within one set.
 * For example, if a user has the following three rights:
 *
 * \code
 * /type/permissions/rights/administer/website/info
 * /type/permissions/rights/administer/website/filter
 * /type/permissions/rights/administer/website
 * \endcode
 *
 * Only the last one is required in the user's set since it covers the
 * other two in the event the plugin require them too.
 */


/** \brief Initialize a permission sets_t object.
 *
 * A sets_t object includes all the sets linked to path and action.
 * The constructor saves the path and action in the object. These two
 * parameters are read-only parameters.
 *
 * \param[in] snap  The snap child pointer.
 * \param[in] user_path  The path to the user.
 * \param[in,out] ipath  The path being queried.
 * \param[in] action  The action being used in this query.
 * \param[in] login_status  The state of the log in of this user.
 */
permissions::sets_t::sets_t(snap_child * snap, QString const & user_path, content::path_info_t & ipath, QString const & action, QString const & login_status)
    : f_snap(snap)
    , f_user_path(user_path)
    , f_ipath(ipath)
    , f_action(action)
    , f_login_status(login_status)
    //, f_user_rights() -- auto-init
    //, f_user_cache_key("") -- auto-init
    //, f_plugin_permissions() -- auto-init
    //, f_plugin_cache_key() -- auto-init
    //, f_using_user_cache() -- auto-init
    //, f_using_plugin_cache() -- auto-init
{
}


/** \brief Clean up sets_t objects.
 *
 * This function cleans up the sets_t object. Mainly, it determines whether
 * the user and page (plugin) permissions should be saved to the cache table.
 */
permissions::sets_t::~sets_t()
{
    try
    {
        save_to_user_cache();
    }
    catch(snap_exception_base const &)
    {
    }
    catch(libexcept::exception_t const &)
    {
    }

    try
    {
        save_to_plugin_cache();
    }
    catch(libdbproxy::exception const &)
    {
    }
}


/** \brief Set the log in status of the user.
 *
 * This function is used to define the status login of the user. This
 * is used by the get_user_rights() signal to know which set of rights
 * should be added for the user.
 *
 * So defining the user is not enough, you also need to define his
 * currently considered status. You could check whether a user can
 * access a page as a visitor, as a returning registered user, or
 * as a logged in user.
 *
 * \li SETS_LOGIN_STATUS_SPAMMER -- the user is considered a spammer
 * \li SETS_LOGIN_STATUS_VISITOR -- the user is anonymous
 * \li SETS_LOGIN_STATUS_RETURNING_VISITOR -- the user is anonymous, but
 *                                            is a returning visitor
 * \li SETS_LOGIN_STATUS_RETURNING_REGISTERED -- user logged in more than
 *                                               3h ago (time can be changed)
 * \li SETS_LOGIN_STATUS_REGISTERED -- the user logged in recently
 *
 * \note
 * You are not supposed to modify the status while generating user or
 * plugin rights. However, a plugin testing permissions for a user
 * may want to test with multiple login status.
 *
 * \param[in] login_status  The log in status to consider while adding user rights.
 */
void permissions::sets_t::set_login_status(QString const & login_status)
{
    f_login_status = login_status;
    f_user_cache_key.clear();
}


/** \brief Retrieve the log in status of the user.
 *
 * By default the login status of the user is set to "visitor". Other
 * statuses can be used to check whether a user has rights if logged in
 * or as a spammer, etc.
 *
 * This function is used by the get_user_rights() signal to know which
 * set of rights to add to the user.
 *
 * \return The login status of the user being checked.
 */
QString const & permissions::sets_t::get_login_status() const
{
    return f_login_status;
}


/** \brief The user being checked.
 *
 * By default the permissions are checked for the current user. As you can
 * see in the on_validate_action() signal handler, the user parameter is
 * set to the currently logged in user.
 *
 * This function is used by the users plugin to assign the correct rights
 * to this sets_t object.
 *
 * \return The user concerned by this right validation.
 *
 * \sa add_user_right()
 */
QString const & permissions::sets_t::get_user_path() const
{
    return f_user_path;
}


/** \brief The path this permissions are checked against.
 *
 * The user rights are being checked against this path. This path represents
 * the page being viewed and any plugin that "recognizes" that path shall
 * define rights as offered by that plugin.
 *
 * \return The path being checked against user's rights.
 */
content::path_info_t & permissions::sets_t::get_ipath() const
{
    return f_ipath;
}


/** \brief Get the sets action.
 *
 * Whenever add rights to the sets, we pre-intersect those with the action.
 * This is a powerful optimization since that way we avoid adding or testing
 * many things which would be totally useless (i.e. imagine adding 100 rights
 * when that action only offers 3 rights!)
 *
 * \return The action of these sets.
 */
const QString & permissions::sets_t::get_action() const
{
    return f_action;
}


/** \brief Make sure the cache table pointer is defined.
 *
 * This function makes sure we have the pointer to the cache table defined
 * so that our functions can access the g_cache_table variable.
 */
void permissions::sets_t::get_cache_table()
{
    if(!details::g_cache_table)
    {
        details::g_cache_table = content::content::instance()->get_cache_table();
    }
}


/** \brief The key used to read and write the cache data for this user.
 *
 * This function calculates the cache key for a user. This key is used
 * to access the cached data for a given user.
 *
 * \code
 *     Name                 Comments
 *
 *     <status>             User login status, this includes the
 *                          "permissions" and "login_status" namespaces
 *
 *     <action>             The specific action being applied (this may
 *                          feel like a waste for users that are not
 *                          registered, but a plugin could have specific
 *                          need in that regard...)
 * \endcode
 *
 * \note
 * The user does NOT need to be specified in the cache key itself since
 * that data is saved under the user row (i.e. for user with id 3, the
 * cache data is saved in cache."<domain>/user/3")
 *
 * \return The cache key.
 */
QString const & permissions::sets_t::get_user_cache_key()
{
    if(f_user_cache_key.isEmpty())
    {
        f_user_cache_key = QString("%1::%2")
                            .arg(f_login_status)
                            .arg(f_action);
    }

    return f_user_cache_key;
}


/** \brief Mark that the user permissions were modified.
 *
 * This function is called internally to know whether the user
 * permissions were re-generated. If so, we will update the
 * corresponding cache information.
 *
 * By default this is false. It gets set whenever the
 * permissions::get_user_rights() signal gets called and the
 * permissions are not already cached (i.e. there is no point
 * in reading the permissions from the cache and then writing
 * them back to the cache.)
 */
void permissions::sets_t::modified_user_permissions()
{
    f_modified_user_permissions = true;
}


/** \brief Check whether that user has his rights cached.
 *
 * The user rights are cached in the cache table. The cached data is
 * considered invalid if the users permissions get modified at any
 * one time.
 *
 * The function returns false when the data was invalidated or if
 * no cached data is found.
 *
 * At this time the cached data is defined as follow:
 *
 * \li 64 bit of timestamp (see snap_child::get_start_date())
 * \li lines of URIs representing user rights (lines are separated by '\n')
 *
 * The action and login status are used in the user cache key and thus
 * are not required in the cache (also they have to be known at time
 * of call.)
 *
 * \return true if the cached data was read and considered valid.
 */
bool permissions::sets_t::read_from_user_cache()
{
// to avoid the user cache, always return false here
//return false;

    // already read that cacne data?
    //
    if(f_using_user_cache)
    {
        return true;
    }

    QString const & cache_key(get_user_cache_key());

    // TODO: look into why I decided to use "" for the anonymous user
    //       because in the end here we may want to use the original
    //       string everywhere?
    //
    content::path_info_t cache_ipath;
    cache_ipath.set_path(f_user_path.isEmpty() ? users::get_name(users::name_t::SNAP_NAME_USERS_ANONYMOUS_PATH) : f_user_path);

    get_cache_table();

    if(!details::g_cache_table->exists(cache_ipath.get_key())
    || !details::g_cache_table->getRow(cache_ipath.get_key())->exists(cache_key))
    {
        f_user_cache_reset = true;
        return false;
    }

    // cache entry exists, read it
    //
    libdbproxy::value cache_value(details::g_cache_table->getRow(cache_ipath.get_key())->getCell(cache_key)->getValue());

    // check the timestamp
    //
    int64_t const timestamp(cache_value.safeInt64Value());
    libdbproxy::value const last_updated_value(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_PERMISSIONS_LAST_UPDATED)));
    int64_t const last_updated(last_updated_value.safeInt64Value());
    if(timestamp < last_updated)
    {
        // the cache is present but out of date, let the caller compute
        // a new version
        f_user_cache_reset = true;
        return false;
    }

    // check whether the client said we should reset our caches
    //
    // TODO: This is problematic for Anonymous users, any one user who is
    //       not logged in can generate a reset of the permissions caches
    //       under the feet of other Anonymous users... (it may also be
    //       that anonymous users cannot reach this line of code.)
    //
    if(!f_user_cache_reset)
    {
        cache_control_settings const & page_cache_control(f_snap->client_cache_control());
        if((page_cache_control.get_no_cache()
        || page_cache_control.get_max_age() == 0)
        && !details::g_user_cache_reviewed.contains(cache_ipath.get_key()))
        {
            details::g_user_cache_reviewed[cache_ipath.get_key()] = true;

            // okay! user says to not take existing cache in account, so we
            // ignore it here... it should be recalculated and the new version
            // saved so no need to drop the cell and generate a tombstone
            //
            f_user_cache_reset = true;
            // TODO: we are supposed to return false here to force the
            //       server to regenerate the permissions, but it is really
            //       slow at this point so I think that should not be done
            //       on a simple reload; further testing will be needed.
            //return false;
        }
    }

    // convert the cached value in what the caller expects
    //
    QString const all_user_rights(cache_value.stringValue(sizeof(int64_t)));
    int start(0);
    int pos(all_user_rights.indexOf('\n'));
    while(pos != -1)
    {
        f_user_rights.push_back(all_user_rights.mid(start, pos - start));
        start = pos + 1;
        pos = all_user_rights.indexOf('\n', start);
    }
    // there is no left over in that string unless the save_to_user_cache()
    // fails somehow (i.e. all lines end with '\n')

    f_using_user_cache = true;

    // success, data came from cache
    return true;
}


/** \brief Write the current user rights to the cache.
 *
 * This function saves the current user rights to the cache. This is
 * the opposite function to the read_from_user_cache() function.
 *
 * The write is automatically called when the sets_t destructor is
 * called. That way we are sure that the cache is updated appropriately.
 *
 * The function does nothing if the user data came from reading the cache
 * since obviously in that case the data is the exact same.
 */
void permissions::sets_t::save_to_user_cache()
{
    // if we are using the cache, no need to save anything
    // if we never got the chance the calculate the permissions, then
    // also do not save anything (it would be empty and be WRONG.)
    //
    // the permissions of the user are pretty much always calculated,
    // but someone who creates the sets_t early and may exit their
    // function before calling the permissions::get_user_rights()
    // signal would get that problem.
    //
    if(f_using_user_cache
    || !f_modified_user_permissions)
    {
        return;
    }

    // this should have been called in the read, but we cannot assume the
    // read function was called...
    get_cache_table();

    QString const & cache_key(get_user_cache_key());

    content::path_info_t cache_ipath;
    cache_ipath.set_path(f_user_path.isEmpty() ? users::get_name(users::name_t::SNAP_NAME_USERS_ANONYMOUS_PATH) : f_user_path);

    QByteArray value;
    uint64_t const start_date(f_snap->get_start_date());
    libdbproxy::setInt64Value(value, start_date);
    for(auto const & right : f_user_rights)
    {
        libdbproxy::appendStringValue(value, QString("%1\n").arg(right));
    }

    details::g_cache_table->getRow(cache_ipath.get_key())->getCell(cache_key)->setValue(value);
}


/** \brief The key used to read and write the cache data for this page.
 *
 * This function calculates the cache key for a page. This key is used
 * to access the cached data for a given page.
 *
 * \code
 *     Name                 Comments
 *
 *     <namespace>          To clearly distinguish this entry in the cache
 *                          we use a namespace which is:
 *
 *                              permissions::plugin::action
 *
 *     <action>             The specific action being applied (this may
 *                          feel like a waste for users that are not
 *                          registered, but a plugin could have specific
 *                          need in that regard...)
 * \endcode
 *
 * \note
 * The page does NOT need to be specified in the cache key itself since
 * that data is saved under the page row (i.e. for page with path /example,
 * the cache data is saved in cache."<domain>/example")
 *
 * \return The cache key.
 */
QString const & permissions::sets_t::get_plugin_cache_key()
{
    if(f_plugin_cache_key.isEmpty())
    {
        f_plugin_cache_key = QString("%1::%2::%3::%4")
                                .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_NAMESPACE))
                                .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_PLUGIN))
                                .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_ACTION_NAMESPACE))
                                .arg(f_action);
    }

    return f_plugin_cache_key;
}


/** \brief Mark that the permissions were modified.
 *
 * This function is called internally to know whether the plugin
 * permissions were re-generated. If so, we will update the
 * corresponding cache information.
 *
 * By default this is false. It gets set whenever the
 * permissions::get_plugin_permissions() gets called and the
 * permissions are not already cached (i.e. there is no point
 * in reading the permissions from the cache and then writing
 * them back to the cache.)
 */
void permissions::sets_t::modified_plugin_permissions()
{
    f_modified_plugin_permissions = true;
}


/** \brief Check whether that user has his rights cached.
 *
 * The user rights are cached in the cache table. The cached data is
 * considered invalid if the users permissions get modified at any
 * one time.
 *
 * The function returns false when the data was invalidated or if
 * no cached data is found.
 *
 * At this time the cached data is defined as follow:
 *
 * \li 64 bit of timestamp (see snap_child::get_start_date())
 * \li lines of URIs representing user rights (lines are separated by '\n')
 *
 * The action and login status are used in the user cache key and thus
 * are not required in the cache (also they have to be known at time
 * of call.)
 *
 * \return true if the cached data was read and considered valid.
 */
bool permissions::sets_t::read_from_plugin_cache()
{
// to avoid the page cache, always return false here
//return false;

    // already read that cacne data?
    if(f_using_plugin_cache)
    {
        // cache already read
        return true;
    }

    QString const & cache_key(get_plugin_cache_key());

    get_cache_table();

    if(!details::g_cache_table->exists(f_ipath.get_key())
    || !details::g_cache_table->getRow(f_ipath.get_key())->exists(cache_key))
    {
        // no cache available, let the caller compute this one
        f_plugin_cache_reset = true;
        return false;
    }

    // cache entry exists, read it
    libdbproxy::value cache_value(details::g_cache_table->getRow(f_ipath.get_key())->getCell(cache_key)->getValue());

    // check the timestamp
    int64_t const timestamp(cache_value.safeInt64Value());
    libdbproxy::value const last_updated_value(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_PERMISSIONS_LAST_UPDATED)));
    int64_t const last_updated(last_updated_value.safeInt64Value());
    if(timestamp < last_updated)
    {
        // the cache is present but out of date, let the caller compute
        // a new version
        f_plugin_cache_reset = true;
        return false;
    }

    // check whether the client said we should reset our caches
    //
    // TODO: This is problematic for Anonymous users, any one user who is
    //       not logged in can generate a reset of the permissions caches
    //       under the feet of other Anonymous users... (it may also be
    //       that anonymous users cannot reach this line of code.)
    //
    if(!f_plugin_cache_reset)
    {
        cache_control_settings const & page_cache_control(f_snap->client_cache_control());
        if((page_cache_control.get_no_cache()
        || page_cache_control.get_max_age() == 0)
        && !details::g_plugin_cache_reviewed.contains(f_ipath.get_key()))
        {
            details::g_plugin_cache_reviewed[f_ipath.get_key()] = true;

            // okay! user says to not take existing cache in account, so we
            // ignore it here... it should be recalculated and the new version
            // saved so no need to drop the cell and generate a tombstone
            //
            f_plugin_cache_reset = true;
            // TODO: we are supposed to return false here to force the
            //       server to regenerate the permissions, but it is really
            //       slow at this point so I think that should not be done
            //       on a simple reload; further testing will be needed.
            //return false;
        }
    }

    // convert the cached value in what the caller expects
    QString const all_plugin_permissions(cache_value.stringValue(sizeof(int64_t)));
    QString plugin_name;
    int start(0);
    int pos(all_plugin_permissions.indexOf('\n'));
    while(pos != -1)
    {
        // plugin names start with a '*' since a URI cannot it is safe
        if(all_plugin_permissions[start] == '*')
        {
            ++start;
            plugin_name = all_plugin_permissions.mid(start, pos - start);
        }
        else
        {
            if(plugin_name.isEmpty())
            {
                // by returning true the cache will be rebuilt and hopefully
                // correctly so we don't get an empty string here!
                return true;
            }
            f_plugin_permissions[plugin_name].push_back(all_plugin_permissions.mid(start, pos - start));
        }
        start = pos + 1;
        pos = all_plugin_permissions.indexOf('\n', start);
    }
    // there is no left over in that string unless the save_to_user_cache()
    // fails somehow (i.e. all lines end with '\n')

    f_using_plugin_cache = true;

    // success, data came from cache
    return true;
}


/** \brief Write the current user rights to the cache.
 *
 * This function saves the current user rights to the cache. This is
 * the opposite function to the read_from_user_cache() function.
 *
 * The write is automatically called when the sets_t destructor is
 * called. That way we are sure that the cache is updated appropriately.
 *
 * The function does nothing if the user data came from reading the cache
 * since obviously in that case the data is the exact same.
 */
void permissions::sets_t::save_to_plugin_cache()
{
    // if we are using the cache, no need to save anything
    // if we never got the chance the calculate the permissions, then
    // also do not save anything (it would be empty and be WRONG.)
    //
    // the determination of the plugin permissions is often not done
    // because the user has no permissions (rare) or because the user
    // is the root user (any page the root user visits would get in
    // trouble!)
    //
    if(f_using_plugin_cache
    || !f_modified_plugin_permissions)
    {
        return;
    }

    // this should have been called in the read, but we cannot assume the
    // read function was called...
    get_cache_table();

    QString const & cache_key(get_plugin_cache_key());

    QByteArray value;
    uint64_t const start_date(f_snap->get_start_date());
    libdbproxy::setInt64Value(value, start_date);
    for(req_sets_t::const_iterator it(f_plugin_permissions.begin()); it != f_plugin_permissions.end(); ++it)
    {
        libdbproxy::appendStringValue(value, QString("*%1\n").arg(it.key()));
        for(auto right : it.value())
        {
            libdbproxy::appendStringValue(value, QString("%1\n").arg(right));
        }
    }

    details::g_cache_table->getRow(f_ipath.get_key())->getCell(cache_key)->setValue(value);
}


/** \brief Rights the user has are added with this function.
 *
 * This function is to be used to add rights that the user has.
 * A right is a link path (i.e. /types/permissions/rights/\<name>).
 *
 * If the same right is added more than once, then only one instance is
 * kept. Actually, if a better right is added, the old not as good right
 * gets removed.
 *
 * \code
 * # When adding this right:
 * /type/permissions/rights/administer/website
 *
 * # and that right was in the set, then only the new right is kept
 * /type/permissions/rights/administer/website/info
 * \endcode
 *
 * Actually it is possible that adding one new right removes many existing
 * rights because it is a much higher level right:
 *
 * \code
 * # When adding this right:
 * /type/permissions/rights/administer/website
 *
 * # then all those rights get removed:
 * /type/permissions/rights/administer/website/feed/create
 * /type/permissions/rights/administer/website/feed/read
 * /type/permissions/rights/administer/website/feed/settings
 * /type/permissions/rights/administer/website/info
 * /type/permissions/rights/administer/website/layout/images
 * /type/permissions/rights/administer/website/layout/stylesheets
 * ...
 * \endcode
 *
 * \note
 * Internally rights are saved with an ending slash (/) which makes it
 * a lot faster to compare them against each other witht the startsWith()
 * function.
 *
 * \param[in] right  The right being added.
 */
void permissions::sets_t::add_user_right(QString right)
{
    // so the startsWith() works as is:
    if(right.right(1) != "/")
    {
        right = right + "/";
    }
    int const len(right.length());
    int max_rights(f_user_rights.size());
    for(int i(0); i < max_rights; ++i)
    {
        int const l(f_user_rights[i].length());
        if(len > l)
        {
            if(right.startsWith(f_user_rights[i]))
            {
                // we are done, that right is already here
#ifdef DEBUG
#ifdef SHOW_RIGHTS
                SNAP_LOG_DEBUG("USER RIGHT -> [")(right)("] (ignore, \"better\" [shrunk/smaller] already there)");
#endif
#endif
                return;
            }
        }
        else if(l > len)
        {
            if(f_user_rights[i].startsWith(right))
            {
                // this new right is smaller, keep that smaller one instead
                f_user_rights[i] = right;
                ++i;
                while(i < max_rights)
                {
                    if(f_user_rights[i].startsWith(right))
                    {
                        f_user_rights.remove(i);
                        --max_rights; // here max decreases!
                    }
                    else
                    {
                        ++i; // in this case i increases
                    }
                }
#ifdef DEBUG
#ifdef SHOW_RIGHTS
                SNAP_LOG_DEBUG("USER RIGHT -> [")(right)("] (shrunk)");
#endif
#endif
                return;
            }
        }
        else /*if(l == len)*/
        {
            if(f_user_rights[i] == right)
            {
                // that is exactly the same, no need to have it twice
#ifdef DEBUG
#ifdef SHOW_RIGHTS
                SNAP_LOG_DEBUG("USER RIGHT -> [")(right)("] (already present)");
#endif
#endif
                return;
            }
        }
    }

    // this one was not there yet, just append
    f_user_rights.push_back(right);
#ifdef DEBUG
#ifdef SHOW_RIGHTS
    SNAP_LOG_DEBUG("USER RIGHT -> [")(right)("] (add)");
#endif
#endif
}


/** \brief Return the number of user rights.
 *
 * This function returns the number of rights the user has. Note that
 * user rights are added only if those rights match the specified
 * action. So for example we do not add "view" rights for a user if
 * the action is "delete". This means the number of a user rights
 * represents the intersection between all the user rights and the
 * action specified in this sets_t object. If empty, then the user
 * does not even have that very permission at all.
 *
 * \return The number of rights the user has for this action.
 */
int permissions::sets_t::get_user_rights_count() const
{
    return f_user_rights.size();
}


/** \brief Retrieve the vector of user rights.
 *
 * This function lets you check out the list of user rights. This is
 * used by the check_permission() function to display the list of user
 * rights to the administrator.
 *
 * The returned value is a vector to the user rights.
 *
 * \return The right at the specified index.
 */
permissions::sets_t::set_t const & permissions::sets_t::get_user_rights() const
{
    return f_user_rights;
}


/** \brief Return the number of plugin rights.
 *
 * This function returns the number of rights plugins offer. Note that
 * plugin rights are added only if those rights match the specified
 * action. So for example we do not add "view" rights for a plugin if
 * the action is "delete". This means the number of a plugin rights
 * represents the intersection between all the plugin rights and the
 * action specified in this sets_t object. If empty, then the plugins
 * do not even offer that very permission at all.
 *
 * \return The number of rights the plugins have for this action.
 */
int permissions::sets_t::get_plugin_rights_count() const
{
    return f_plugin_permissions.size();
}


/** \brief Retrieve the plugin rights.
 *
 * This function lets you check out the list of plugin rights. This is
 * used by the check_permission() function to display the list of plugin
 * rights to the administrator.
 *
 * \return The permissions added by the plugins.
 */
permissions::sets_t::req_sets_t const & permissions::sets_t::get_plugin_rights() const
{
    return f_plugin_permissions;
}


/** \brief Add a permission from the specified plugin.
 *
 * This function adds a right for the specified plugin. In most cases this
 * works on a per plugin basis. So a plugin adds its own rights only.
 * However, some plugins can add rights for other plugins (right
 * complements or right sharing.)
 *
 * The plugin name is used to create a separate set of rights, one per
 * plugin. The user must have enough rights for each separate group of
 * plugin to be allowed the action sought.
 *
 * \note
 * This function optimizes the set by removing more constraning rights.
 * This means when adding a permissions such as .../administer/website/info
 * and the set already has a permission .../administer/website then that
 * permission gets replaced with the new one because the user may have
 * right .../administer/website and match both those rights anyway.
 *
 * \code
 * # In plugin permissions, the following
 * /type/permissions/rights/administer/website/info
 * /type/permissions/rights/administer/website
 *
 * # Is equivalent to the following:
 * /type/permissions/rights/administer/website/info
 * \endcode
 *
 * \note
 * Similarly, we do not add a plugin permission if an easier to get right
 * is already present in the set of that plugin.
 *
 * \param[in] plugin  The plugin adding this permission.
 * \param[in] right  The right the plugin offers.
 */
void permissions::sets_t::add_plugin_permission(QString const & plugin, QString right)
{
    // so the startsWith() works as is:
    if(right.right(1) != "/")
    {
        right = right + "/";
    }

    if(!f_plugin_permissions.contains(plugin))
    {
#ifdef DEBUG
#ifdef SHOW_RIGHTS
        SNAP_LOG_DEBUG("PLUGIN [")(plugin)("] PERMISSION -> [")(right)("] (add, new plugin)");
#endif
#endif
        f_plugin_permissions[plugin].push_back(right);
        return;
    }

    set_t & set(f_plugin_permissions[plugin]);
    int const len(right.length());
    int max_set(set.size());
    int i(0);
    while(i < max_set)
    {
        int const l(set[i].length());
        if(len > l)
        {
            if(right.startsWith(set[i]))
            {
                // the new right is generally considered easier to get
#ifdef DEBUG
#ifdef SHOW_RIGHTS
                SNAP_LOG_DEBUG("PLUGIN [")(plugin)("] PERMISSION -> [")(set[i])("] (REMOVING)");
#endif
#endif
                set.remove(i);
                --max_set;
                continue;
            }
        }
        else if(l > len)
        {
            if(set[i].startsWith(right))
            {
                // this new right is harder to get, ignore it
#ifdef DEBUG
#ifdef SHOW_RIGHTS
                SNAP_LOG_DEBUG("PLUGIN [")(plugin)("] PERMISSION -> [")(right)("] (skipped)");
#endif
#endif
                return;
            }
        }
        else /*if(l == len)*/
        {
            if(set[i] == right)
            {
                // that's exactly the same, no need to have it twice
#ifdef DEBUG
#ifdef SHOW_RIGHTS
                SNAP_LOG_DEBUG("PLUGIN [")(right)("] PERMISSION -> [")(right)("] (already present)");
#endif
#endif
                return;
            }
        }
        ++i;
    }

    // this one was not there yet, just append
    set.push_back(right);
#ifdef DEBUG
#ifdef SHOW_RIGHTS
    SNAP_LOG_DEBUG("PLUGIN [")(plugin)("] PERMISSION -> [")(right)("] (add)");
#endif
#endif
}


/** \brief Check whether the user has root permissions.
 *
 * This function quickly searches for the one permission that marks a
 * user as the root user. This is done by testing whether the user has
 * the main rights permission (types/permissions/rights).
 *
 * This function is called before checking all the rights on a page
 * because whatever those rights, if the user has root permissions,
 * then he will have the right. Period.
 *
 * This being said, the data that's locked is still checked and the
 * access on those is still prevented if a system lock exists.
 *
 * \return true if the user is a root user.
 */
bool permissions::sets_t::is_root() const
{
    // the top rights type represents the full root user (i.e. all rights)
    //
    content::path_info_t ipath;
    ipath.set_path(get_name(name_t::SNAP_NAME_PERMISSIONS_RIGHTS_PATH));
    QString key(ipath.get_key());
    if(!key.endsWith("/"))
    {
        // for permissions, we add a "/" (to make sure we property
        // distinguish between path such as "/a/b" and "/a/bb")
        //
        key += "/";
    }

    return f_user_rights.contains(key);
}


/** \brief Check whether the user is allowed to perform the action.
 *
 * This function executes the intersection between the user rights
 * and the different plugin rights found while running the
 * get_plugin_permissions() signal. If the intersection of the user
 * rights with any one list is the empty set, then the function returns
 * false. Otherwise it returns true.
 *
 * \return true if the user is allowed to perform this action.
 */
bool permissions::sets_t::allowed() const
{
    if(f_user_rights.isEmpty()
    || f_plugin_permissions.isEmpty())
    {
#ifdef DEBUG
#ifdef SHOW_RIGHTS
        SNAP_LOG_DEBUG("--- intersection of these sets is empty; user is not allowed access to that page!");
#endif
#endif
        // if the plugins added nothing, there are no rights to compare
        // or worst, the user has no rights at all (Should not happen,
        // although someone could add a plugin testing something such as
        // your IP address to strip you off all your rights unless you
        // have an IP address considered "valid")
        return false;
    }

#ifdef DEBUG
#ifdef SHOW_RIGHTS
SNAP_LOG_DEBUG("final USER RIGHTS:");
for(int i(0); i < f_user_rights.size(); ++i)
{
    SNAP_LOG_DEBUG("  [")(f_user_rights[i])("]");
}
SNAP_LOG_DEBUG("final PLUGIN PERMISSIONS:");
for(req_sets_t::const_iterator pp(f_plugin_permissions.begin());
        pp != f_plugin_permissions.end();
        ++pp)
{
    SNAP_LOG_DEBUG("  [")(pp.key())("]:");
    for(int j(0); j < pp->size(); ++j)
    {
        SNAP_LOG_DEBUG("    [")((*pp)[j])("]");
    }
}
#endif
#endif

    int const max_rights(f_user_rights.size());
    for(req_sets_t::const_iterator pp(f_plugin_permissions.begin());
            pp != f_plugin_permissions.end();
            ++pp)
    {
        // enough rights with this one?
        set_t p(*pp);
        set_t::const_iterator i;
        for(i = p.begin(); i != p.end(); ++i)
        {
            // the list of plugin permissions is generally smaller so
            // we loop through that list although this contains can be
            // slow... we may want to change the set_t type to a QMap
            // which uses a faster binary search; although on very small
            // maps it may not be any faster
            for(int j(0); j < max_rights; ++j)
            {
                QString const & plugin_permission ( *i               );
                QString const & user_right        ( f_user_rights[j] );
                if( plugin_permission.startsWith(user_right) )
                {
                    //break 2;
                    goto next_plugin;
                }
            }
        }
        // XXX add a log to determine the name of the plugin that
        //     failed the user?
#ifdef DEBUG
#ifdef SHOW_RIGHTS
        SNAP_LOG_DEBUG("  failed, no match for [")(pp.key())("]");
#endif
#endif
        return false;
next_plugin:;
    }

#ifdef DEBUG
#ifdef SHOW_RIGHTS
    SNAP_LOG_DEBUG("  allowed!!!");
#endif
#endif
    return true;
}


/** \brief Initialize the permissions plugin.
 *
 * This function is used to initialize the permissions plugin object.
 */
permissions::permissions()
    //: f_snap(NULL) -- auto-init
    //, f_login_status("") -- auto-init
    //, f_has_user_path(false) -- auto-init
    //, f_user_path("") -- auto-init
{
}


/** \brief Clean up the permissions plugin.
 *
 * Ensure the permissions object is clean before it is gone.
 */
permissions::~permissions()
{
}


/** \brief Get a pointer to the permissions plugin.
 *
 * This function returns an instance pointer to the permissions plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the permissions plugin.
 */
permissions * permissions::instance()
{
    return g_plugin_permissions_factory.instance();
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
QString permissions::description() const
{
    return "The permissions plugin is one of the most important plugins of the"
          " Snap! system. It allows us to determine whether the current user"
          " has enough rights to act on a specific page.";
}


/** \brief Return our dependencies
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString permissions::dependencies() const
{
    return "|layout|messages|output|users|";
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
int64_t permissions::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2017, 8, 2, 12, 32, 57, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the content with our references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void permissions::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the permissions.
 *
 * This function terminates the initialization of the permissions plugin
 * by registering for different events it supports.
 *
 * \param[in] snap  The child handling this request.
 */
void permissions::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(permissions, "server", server, register_backend_action, _1);
    SNAP_LISTEN(permissions, "server", server, add_snap_expr_functions, _1);
    SNAP_LISTEN(permissions, "path", path::path, validate_action, _1, _2, _3);
    SNAP_LISTEN(permissions, "path", path::path, access_allowed, _1, _2, _3, _4, _5);
    SNAP_LISTEN(permissions, "path", path::path, check_for_redirect, _1);
    SNAP_LISTEN(permissions, "users", users::users, user_verified, _1, _2);
    SNAP_LISTEN(permissions, "layout", layout::layout, generate_header_content, _1, _2, _3);
    SNAP_LISTEN(permissions, "links", links::links, modified_link, _1, _2);
}


/** \brief Implementation of the get_user_rights signal.
 *
 * This function readies the user rights in the specified \p sets.
 *
 * The plugins that capture this function are expected to add user
 * rights to the sets (with the add_user_right() function.) No plugin
 * permissions should be modified at this point. The
 * get_plugin_permissions() is used to that effect.
 *
 * Note that only the rights that correspond to the specified action are
 * to be added here.
 *
 * \code
 *   QString const site_key(f_snap->get_site_key_with_slash());
 *   sets.add_user_right(site_key + "types/permissions/rights/edit/page");
 * \endcode
 *
 * \note
 * Although it is a permission thing, the user plugin makes most of the
 * work of determining the user rights (although the user plugin calls
 * functions of the permission plugin such as the add_user_rights()
 * function.) This is because the user plugin can include the permissions
 * plugin, but not the other way around.
 *
 * \todo
 * Fix the dependencies so one of permissions or user uses the other,
 * at this time they depend on each other!
 *
 * \param[in] perms  A pointer to the permissions plugin.
 * \param[in,out] sets  The sets_t object where rights get added.
 *
 * \return true if the signal has to be sent to other plugins.
 *
 * \sa add_user_rights()
 */
bool permissions::get_user_rights_impl(permissions * perms, sets_t & sets)
{
    // if the user data was cached and is still valid, then we are done here
    //
    // TBD: is this the correct location for that call?
    //
    //      (it could be done before call get_user_rights() although since
    //      this very first impl function is a direct call, we don't really
    //      waste much time at all.)
    //
    if(sets.read_from_user_cache())
    {
        // no need for other plugins to run since we got the user rights
        // from the cache
        //
        return false;
    }

    // the destructor would smash the permissions of a user if we did not
    // know whether it got modified or not (i.e. this function may never
    // get called)
    //
    sets.modified_user_permissions();

    QString const & login_status(sets.get_login_status());

    // if spammers are logged in they don't get access to anything anyway
    // (i.e. they are UNDER visitors!)
    QString const & site_key(f_snap->get_site_key_with_slash());
    if(login_status == get_name(name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_SPAMMER))
    {
        perms->add_user_rights(site_key + "types/permissions/groups/root/administrator/editor/moderator/author/commenter/registered-user/returning-registered-user/returning-visitor/visitor/spammer", sets);
    }
    else
    {
        if(login_status == get_name(name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_VISITOR))
        {
            perms->add_user_rights(site_key + "types/permissions/groups/root/administrator/editor/moderator/author/commenter/registered-user/returning-registered-user/returning-visitor", sets);
        }
        else
        {
            // unfortunately, whatever the login status, if we were not given
            // a valid user path, we just cannot test anything else than
            // some kind of visitor
            QString const & user_path(sets.get_user_path());
//#ifdef DEBUG
//SNAP_LOG_DEBUG("  +-> user key = [")(user_path)("]");
//#endif
            if(user_path.isEmpty()
            || login_status == get_name(name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR))
            {
                // in this case the user is an anonymous user and thus we want to
                // add the anonymous user rights
                perms->add_user_rights(site_key + "types/permissions/groups/root/administrator/editor/moderator/author/commenter/registered-user/returning-registered-user/returning-visitor/visitor", sets);
            }
            else
            {
                content::path_info_t user_ipath;
                user_ipath.set_path(user_path);

                // add all the groups the user is a member of
                libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
                if(!content_table->exists(user_ipath.get_key()))
                {
                    // that user is gone, this will generate a 500 by Apache
                    // (that should not happen, otherwise we may want to
                    // look into a way to change the user in a plain
                    // returning visitor)
                    throw permissions_exception_invalid_path("could not access user \"" + user_ipath.get_key() + "\"");
                }

                // should this one NOT be offered to returning users?
                sets.add_user_right(user_ipath.get_key());

                if(login_status == get_name(name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED))
                {
                    // users who are logged in always have registered-user
                    // rights if nothing else
                    perms->add_user_rights(site_key + "types/permissions/groups/root/administrator/editor/moderator/author/commenter/registered-user", sets);

                    // add assigned groups
                    {
                        QString const link_start_name(
                                    QString("%1::%2::%3")
                                        .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_NAMESPACE))
                                        .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_DIRECT_NAMESPACE))
                                        .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_GROUP_NAMESPACE)));
                        links::link_info info(link_start_name, false, user_ipath.get_key(), user_ipath.get_branch());
                        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
                        links::link_info right_info;
                        while(link_ctxt->next_link(right_info))
                        {
                            QString const right_key(right_info.key());

                            // user -> permissions::direct::group-...
                            perms->add_user_rights(right_key, sets);
                        }
                    }

                    // we can also assign permissions directly to a user so get those too
                    {
                        QString const link_start_name(
                                    QString("%1::%2::%3::%4")
                                        .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_NAMESPACE))
                                        .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_DIRECT_NAMESPACE))
                                        .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_ACTION_NAMESPACE))
                                        .arg(sets.get_action()));
                        links::link_info info(link_start_name, false, user_ipath.get_key(), user_ipath.get_branch());
                        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
                        links::link_info right_info;
                        while(link_ctxt->next_link(right_info))
                        {
                            QString const right_key(right_info.key());

                            // user -> permissions::direct::action::...
                            perms->add_user_rights(right_key, sets);
                        }
                    }
                }
                else
                {
                    // this is a registered user who comes back and is
                    // semi-logged in so we do not give this user full rights
                    // to avoid potential security problems; we have to look
                    // into a way to offer different group/rights for such a
                    // user...
                    perms->add_user_rights(site_key + "types/permissions/groups/root/administrator/editor/moderator/author/commenter/registered-user/returning-registered-user", sets);

                    // add assigned groups
                    // by groups limited to returning registered users, not the logged in registered user
                    {
                        QString const link_start_name(get_name(name_t::SNAP_NAME_PERMISSIONS_DIRECT_GROUP_RETURNING_REGISTERED_USER));
                        links::link_info info(link_start_name, false, user_ipath.get_key(), user_ipath.get_branch());
                        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
                        links::link_info right_info;
                        while(link_ctxt->next_link(right_info))
                        {
                            QString const right_key(right_info.key());
                            perms->add_user_rights(right_key, sets);
                        }
                    }
                }
            }
        }
    }

    // give other plugins a chance to add their own links
    return true;
}


/** \brief Implementation of the get_plugin_permissions signal.
 *
 * This function readies the plugin rights in the specified \p sets.
 *
 * The plugins that capture this function are expected to add plugin
 * permissions to the sets (with the add_plugin_permission() function.)
 * No user rights should be modified in this process. Those are taken
 * cared of by the get_user_rights() signal.
 *
 * Note that for plugins we use the term permissions because the plugin
 * allows that capability, whereas a user has rights. However, in the end,
 * the two terms point to the exact same thing: a string to a right defined
 * in the /types/permissions/actions/\<name>.
 *
 * \code
 *   QString const site_key(f_snap->get_site_key_with_slash());
 *   sets.add_plugin_permission(get_plugin_name(), site_key + "types/permissions/rights/edit");
 * \endcode
 *
 * Note that using the get_plugin_name() is a good idea to avoid typing the
 * wrong name. It is legal for a plugin to add a permission for another
 * plugin in which case the name can be retrieved using the the fully
 * qualified name:
 *
 * \code
 *   users::users::instance()->get_plugin_name();
 * \endcode
 *
 * \note
 * Although the permissions are rather tangled with the content, this
 * plugin checks the basic content setup because the content plugin cannot
 * have references to the permissions (because permissions include content
 * we cannot then include permissions from the content.)
 *
 * \param[in] perms  A pointer to the permissions plugin.
 * \param[in,out] sets  The sets_t object where rights get added.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool permissions::get_plugin_permissions_impl(permissions * perms, sets_t & sets)
{
    NOTUSED(perms);

    // if the user data was cached and is still valid, then we are done here
    //
    // TBD: is this the correct location for that call?
    //
    //      (it could be done before call get_user_rights() although since
    //      this very first impl function is a direct call, we don't really
    //      waste much time at all.)
    //
    if(sets.read_from_plugin_cache())
    {
        return false;
    }

    // the destructor would smash the permissions of a page if we did not
    // know whether it got modified or not (i.e. this function may never
    // get called)
    //
    sets.modified_plugin_permissions();

    // the user plugin cannot include the permissions plugin (since the
    // permissions plugin includes the user plugin) so we implement this
    // user plugin feature in the permissions
    content::path_info_t & ipath(sets.get_ipath());
    if(ipath.get_cpath().left(5) == "user/")
    {
        // user/### cannot be a dynamic path so we do not need to check
        // for a possibly renamed ipath at this level
        QString const user_id(ipath.get_cpath().mid(5));
        QByteArray id_str(user_id.toUtf8());
        char const * s;
        for(s = id_str.data(); *s != '\0'; ++s)
        {
            if(*s < '0' || *s > '9')
            {
                // only digits allowed (i.e. user/123)
                break;
            }
        }
        if(*s == '\0')
        {
#ifdef DEBUG
#ifdef SHOW_RIGHTS
SNAP_LOG_DEBUG("from ")(user_id)(" -> ");
#endif
#endif
            sets.add_plugin_permission(content::content::instance()->get_plugin_name(), ipath.get_key());
            //"user/###/..."
        }
    }

    // the content plugin cannot include the permissions (since the
    // permissions includes the content plugin) so we implement this
    // content plugin feature in the permissions
    //
    // this very page may be assigned direct permissions
    libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
    QString const site_key(f_snap->get_site_key_with_slash());
    QString key(ipath.get_parameter("renamed_path"));
    if(!key.isEmpty())
    {
        content::path_info_t renamed_ipath;
        renamed_ipath.set_path(key);
        key = renamed_ipath.get_key();
        if(!content_table->exists(key)
        || !content_table->getRow(key)->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_PRIMARY_OWNER)))
        {
            // we always immediately expect a valid path when a plugin
            // marks a path calling the (see plugin/path/path.h):
            //
            //     dynamic_plugin_t::set_plugin_if_renamed()
            //
            // although really we let other plugins choose what to do next
            return true;
        }
        ipath.set_real_path(key);
    }
    else
    {
        key = ipath.get_key();
        if(!content_table->exists(key)
        || !content_table->getRow(key)->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_PRIMARY_OWNER)))
        {
            // if that page does not exist, it may be dynamic, try to go up
            // until we have one name in the path then check that the page
            // allows such, if so, we have a chance, otherwise no rights
            // from here... (as an example see /verify in plugins/users/content.xml)
            snap_string_list parts(ipath.get_cpath().split('/'));
            int depth(0);
            for(;;)
            {
                parts.pop_back();
                if(parts.isEmpty())
                {
                    // let other modules take over, we are done here
                    return true;
                }
                ++depth;
                QString const parent_path(parts.join("/"));
                key = site_key + parent_path;
                if(content_table->exists(key))
                {
                    break;
                }
            }
            libdbproxy::row::pointer_t row(content_table->getRow(key));
            char const * dynamic(get_name(name_t::SNAP_NAME_PERMISSIONS_DYNAMIC));
            if(!row->exists(dynamic))
            {
                // well, there is a page, but it does not authorize sub-pages
                return true;
            }
            libdbproxy::value value(row->getCell(dynamic)->getValue());
            if(depth > value.signedCharValue())
            {
                // there is a page, it gives permissions, but this very
                // page is too deep to be allowed
                return true;
            }
            // IMPORTANT NOTE: the ipath here is a reference to the ipath
            //                 we used to call the permission function in
            //                 the path plugin so it will get the real
            //                 path info on return!
            ipath.set_real_path(key);
        }
    }

    content::path_info_t page_ipath;
    page_ipath.set_path(key);

    // if the state is normal, no additional or out of the ordinary
    // permissions are required; otherwise the user needs to have
    // enough permissions (additional group) to access the page
    //
    content::path_info_t::status_t const status(page_ipath.get_status());
    if(status.get_state() != content::path_info_t::status_t::state_t::NORMAL)
    {
        std::string const status_name(content::path_info_t::status_t::status_name_to_string(status.get_state()));
        content::path_info_t status_ipath;
        status_ipath.set_path(QString("%1/%2").arg(get_name(name_t::SNAP_NAME_PERMISSIONS_STATUS_PATH)).arg(status_name.c_str()));

        // here we use the permissions plugin name because the content
        // plugin is already used by the "normal" permissions
        //
        sets.add_plugin_permission(get_plugin_name(), status_ipath.get_key());
    }

    {
        // check local links for this action
        QString const direct_link_start_name(
                        QString("%1::%2::%3::%4")
                            .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_NAMESPACE))
                            .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_DIRECT_NAMESPACE))
                            .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_ACTION_NAMESPACE))
                            .arg(sets.get_action()));
        links::link_info info(direct_link_start_name, false, key, page_ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info right_info;
        while(link_ctxt->next_link(right_info))
        {
            QString const right_key(right_info.key());
#ifdef DEBUG
#ifdef SHOW_RIGHTS
SNAP_LOG_DEBUG("direct: ");
#endif
#endif
            // page -> permissions::direct::action::...
            sets.add_plugin_permission(content::content::instance()->get_plugin_name(), right_key);
        }

        // TODO: should we add support for groups directly from a page?
        //QString const direct_link_start_name(
        //                QString("%1::%2::%3")
        //                    .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_NAMESPACE))
        //                    .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_DIRECT_NAMESPACE))
        //                    .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_GROUP_NAMESPACE)))
    }

    {
        // get the content type (content::page_type) and then retrieve
        // the rights directly from that type
        QString const link_name(get_name(content::name_t::SNAP_NAME_CONTENT_PAGE_TYPE));
        links::link_info info(link_name, true, key, page_ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info content_type_info;
        if(link_ctxt->next_link(content_type_info)) // use if() since it is unique on this end
        {
            content::path_info_t type_ipath;
            type_ipath.set_path(content_type_info.key());

            {
                // read from the content type now
                QString const link_start_name(
                            QString("%1::%2::%3")
                                .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_NAMESPACE))
                                .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_ACTION_NAMESPACE))
                                .arg(sets.get_action()));
                links::link_info perm_info(link_start_name, false, type_ipath.get_key(), type_ipath.get_branch());
                link_ctxt = links::links::instance()->new_link_context(perm_info);
                links::link_info right_info;
                while(link_ctxt->next_link(right_info))
                {
                    QString const right_key(right_info.key());
#ifdef DEBUG
#ifdef SHOW_RIGHTS
SNAP_LOG_DEBUG("page type: ");
#endif
#endif
                    // page -> page type -> permissions::action::...
                    sets.add_plugin_permission(content::content::instance()->get_plugin_name(), right_key);
                }
            }

            {
                // finally, check for groups defined in this content type;
                // groups here function the same way as user groups: they are recursive
                QString const link_start_name(QString("%1::%2")
                                        .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_NAMESPACE))
                                        .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_GROUP_NAMESPACE)));
                links::link_info perm_info(link_start_name, false, type_ipath.get_key(), type_ipath.get_branch());
                link_ctxt = links::links::instance()->new_link_context(perm_info);
                links::link_info right_info;
                while(link_ctxt->next_link(right_info))
                {
                    QString const right_key(right_info.key());
#ifdef DEBUG
#ifdef SHOW_RIGHTS
SNAP_LOG_DEBUG("page group: ");
#endif
#endif
                    // page -> page type -> permissions::group::...
                    add_plugin_permissions(content::content::instance()->get_plugin_name(), right_key, sets);
                }
            }
        }
    }

    return true;
}


/** \brief Generate the actual content of the statistics page.
 *
 * This function generates the contents of the statistics page of the
 * permissions plugin.
 *
 * \todo
 * TBD -- Determine whether this function is really necessary in such a
 * low level plugin (plus this means a dependency to the layout plugin
 * which is probably incorrect here.)
 *
 * \param[in,out] ipath  The path to this page.
 * \param[in,out] page  The page element being generated.
 * \param[in,out] body  The body element being generated.
 */
void permissions::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    // show the permission pages as information (many of these are read-only)
    output::output::instance()->on_generate_main_content(ipath, page, body);
}


/** \brief Validate an action.
 *
 * Whenever a user accesses the website, his action needs to first be
 * verified and then permitted by checking whether the user has enough
 * rights to access the page and apply the action. For example, a user
 * who wants to edit a page needs to have enough rights to edit that
 * page.
 *
 * The name of the action is defined as "view" (the default) or the
 * name of the action defined in the action variable of the URL. By
 * default that variable is "a". So a user who wants to edit a page
 * makes use of "a=edit" as one of the query variables.
 *
 * \param[in,out] ipath  The path the user wants to access.
 * \param[in] action  The action to be taken, the function may redefine it.
 * \param[in,out] err_callback  Call functions on errors.
 */
void permissions::on_validate_action(content::path_info_t & ipath, QString const & action, permission_error_callback & err_callback)
{
    if(action.isEmpty())
    {
        // always emit this error, that is a programmer bug, not a standard
        // user problem that can happen so do not use the err_callback
        f_snap->die(snap_child::http_code_t::HTTP_CODE_ACCESS_DENIED,
                "Access Denied",
                "You are not authorized to access our website in this way.",
                QString("programmer checking permission access with an empty action on page \"%1\".").arg(ipath.get_key()));
        NOTREACHED();
    }

    QString const & login_status(get_login_status());
    QString const & user_path(get_user_path());
    content::permission_flag allowed;
    path::path * path_plugin(path::path::instance());
    path_plugin->access_allowed(user_path, ipath, action, login_status, allowed);
    if(!allowed.allowed())
    {
        // by default we allow redirects to the login page;
        // the signal may set the flag to false to prevent such redirects
        bool redirect_to_login(true);
        permit_redirect_to_login_on_not_allowed(ipath, redirect_to_login);
        QString const method(f_snap->snapenv(get_name(snap::name_t::SNAP_NAME_CORE_REQUEST_METHOD)));
        bool const redirect_method(method == "GET" || method == "POST");

        users::users * users_plugin(users::users::instance());
        users::users::user_info_t user_info(users_plugin->get_user_info());
        if(!user_info.is_valid())
        {
            // special case of spammers
            if(users_plugin->user_is_a_spammer())
            {
                // force a redirect on error, but not from the home page
                if(ipath.get_cpath() != "" && redirect_to_login && redirect_method)
                {
                    // spammers are expected to have enough rights to access
                    // the home page so we try to redirect them there
                    err_callback.on_redirect(
                        // message
                        "Access Denied",
                        QString("The page you were trying to access (%1) requires more privileges.").arg(ipath.get_cpath()),
                        QString("spammer trying to \"%1\" on page \"%2\".").arg(action).arg(ipath.get_cpath()),
                        false,
                        // redirect
                        "/",
                        snap_child::http_code_t::HTTP_CODE_FOUND);
                }
                else
                {
                    // if user does not even have access to the home page...
                    err_callback.on_error(snap_child::http_code_t::HTTP_CODE_ACCESS_DENIED,
                            "Access Denied",
                            "You are not authorized to access our website.",
                            QString("Spammer trying to \"%1\" on page \"%2\" with insufficient rights.").arg(action).arg(ipath.get_cpath()),
                            false);
                }
                return;
            }

            if(ipath.get_cpath() == "login")
            {
                // An IP, Agent, etc. based test could get us here...
                err_callback.on_error(snap_child::http_code_t::HTTP_CODE_ACCESS_DENIED,
                        "Access Denied",
                        action != "view"
                            ? QString("You are not authorized to access the login page with action \"%1\".").arg(action)
                            : "Somehow you are not authorized to access the login page.",
                        QString("User trying to \"%1\" on page \"%2\" with insufficient rights.").arg(action).arg(ipath.get_cpath()),
                        true);
                return;
            }
            if(!redirect_to_login || !redirect_method)
            {
                // The login page is accessible but we do not want to redirect
                // on this file (i.e. probably an attachment)
                err_callback.on_error(snap_child::http_code_t::HTTP_CODE_ACCESS_DENIED,
                        "Access Denied",
                        action != "view"
                            ? QString("You are not authorized to access this document with action \"%1\".").arg(action)
                            : "Somehow you are not authorized to view this page.",
                        QString("User trying to \"%1\" on page \"%2\" with insufficient rights. Not redirecting to /login either since submit is expected to work for visitors.").arg(action).arg(ipath.get_cpath()),
                        true);
                return;
            }

            // if the page is to appear in an IFRAME then we want to
            // remove the frame before showing the login screen, for
            // that purpose we have a special page which does all that
            // work for us; note that if you still want to open a login
            // screen in an IFRAME, you will want to make sure to NOT
            // put the iframe=true parameter in the URL query string
            //
            snap_uri const & main_uri(f_snap->get_uri());
            if(main_uri.has_query_option("iframe"))
            {
                if(main_uri.query_option("iframe") == "true")
                {
                    if(!users_plugin->is_transparent_hit())
                    {
                        err_callback.on_redirect(
                            // message
                            "Unauthorized",
                            QString("The page you were trying to access (%1) requires more privileges. If you think you have such, try to log in first.").arg(ipath.get_cpath()),
                            QString("User trying to \"%1\" on page \"%2\" when not logged in.").arg(action).arg(ipath.get_cpath()),
                            false,
                            // redirect
                            "remove-iframe-for-login",
                            snap_child::http_code_t::HTTP_CODE_FOUND);
                    }
                    // not reached if path checking
                    return;
                }
            }

            // user is anonymous, there is hope, he may have access once
            // logged in
            //
            // TODO all redirects need to also include a valid action!
            //
            // Repairs SNAP-46: do not set referrer to non-main page paths.
            //
            if( ipath.is_main_page() )
            {
                // we want to save this very page as the referrer but
                // it is not specific to a user...
                //
                users_plugin->set_referrer( ipath.get_cpath(), user_info );
            }

            // the title of public pages can be show in the error message;
            // the title should be more helpful to end users, especially
            // for the home page that would otherwise be shown as ""
            //
            // by default show the user the path to the page
            //
            QString page_title(QString("/%1").arg(ipath.get_cpath()));
            content::permission_flag public_page;
            path_plugin->access_allowed("", ipath, "view", get_name(name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR), public_page);
            if(public_page.allowed())
            {
                // the page is public, get the title instead
                //
                content::content * content_plugin(content::content::instance());
                libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());
                page_title = revision_table->getRow(ipath.get_revision_key())->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->getValue().stringValue();
            }

            // check whether the URL included "hit=transparent"
            // because if so the error should be converted to an
            // informational message instead
            //
            {
                QString const qs_hit(f_snap->get_server_parameter("qs_hit"));
                snap_uri const & uri(f_snap->get_uri());
                if(uri.has_query_option(qs_hit)
                && uri.query_option(qs_hit) == users::get_name(users::name_t::SNAP_NAME_USERS_HIT_TRANSPARENT))
                {
                    path::path_error_callback * ptr(dynamic_cast<path::path_error_callback *>(&err_callback));
                    if(ptr != nullptr)
                    {
                        ptr->set_autologout();

                        err_callback.on_redirect(
                            // message
                            "Auto-Logged Out",
                            QString("For safety, we logged you out as you were idle for some time. The page on which you were (%1) requires you to be logged in. You may enter your login name and password below to immediately return to that page.").arg(page_title),
                            QString("User trying to \"%1\" on page \"%2\" when not logged in (session timed out).").arg(action).arg(ipath.get_cpath()),
                            false,
                            // redirect
                            "login",
                            snap_child::http_code_t::HTTP_CODE_FOUND);
                        return;
                    }
                }
            }

            // redirect to the login page
            //
            if(!users_plugin->is_transparent_hit())
            {
                err_callback.on_redirect(
                    // message
                    "Unauthorized",
                    QString("The page you were trying to access (%1) requires more privileges. If you think you have such, try to log in first.").arg(page_title),
                    QString("User trying to \"%1\" on page \"%2\" when not logged in.").arg(action).arg(ipath.get_cpath()),
                    false,
                    // redirect
                    "login",
                    snap_child::http_code_t::HTTP_CODE_FOUND);
            }
            // not reached if path checking
        }
        else
        {
            if(login_status == get_name(name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_REGISTERED)
            && redirect_to_login
            && redirect_method)
            {
                // allowed if logged in?
                //
                content::permission_flag allowed_if_logged_in;
                path_plugin->access_allowed(user_path, ipath, action, get_name(name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED), allowed_if_logged_in);
                if(allowed_if_logged_in.allowed())
                {
                    // TODO: find a way to save the data that is about
                    //       to be lost because we're going to redirect
                    //       the user...

                    // ah! the user is not allowed here but he would be if
                    // only he were recently logged in (with the last 3h or
                    // whatever the administrator set that to.)
                    //
                    if( ipath.is_main_page() )
                    {
                        users_plugin->set_referrer( ipath.get_cpath(), user_info );
                    }
                    if(!users_plugin->is_transparent_hit())
                    {
                        err_callback.on_redirect(
                            // message
                            "Unauthorized",
                            QString("The page you were trying to access (%1) requires you to verify your credentials. Please log in again and the system will send you back there.").arg(ipath.get_cpath()),
                            QString("User trying to \"%1\" on page \"%2\" when not recently logged in.").arg(action).arg(ipath.get_cpath()),
                            false,
                            // redirect
                            "verify-credentials",
                            snap_child::http_code_t::HTTP_CODE_FOUND);
                    }
                    // not reached if path checking
                    return;
                }
            }

            // user is already logged in; no redirect even once we support
            // the double password feature
            //
            err_callback.on_error(snap_child::http_code_t::HTTP_CODE_ACCESS_DENIED,
                    "Access Denied",
                    QString("You are not authorized to apply this action (%1) to this page (%2).").arg(action).arg(ipath.get_key()),
                    QString("User trying to \"%1\" on page \"%2\" with insufficient rights.").arg(action).arg(ipath.get_key()),
                    true);
        }
        return;
    }
}


/** \brief Check whether the page needs to be accessed securely.
 *
 * This callback checks whether we are trying to access the page
 * using HTTP instead of HTTPS. If so, then we check whether the
 * site requests that we always or eventually redirect the user
 * to HTTPS.
 *
 * For example, when the user goes to the login or registration
 * pages, we may want to force HTTPS and only give a secure
 * cookie after that.
 *
 * The function loads the 'permissions::secure_site' parameter
 * from the 'sites' table. There are three possible values
 * at this time:
 *
 * \li secure_mode_t::SECURE_MODE_NO -- the site does not require HTTPS.
 * \li secure_mode_t::SECURE_MODE_PAGE -- the site wants to use HTTPS but
 *     only if the page is linked to the "Permission Secure" tag.
 * \li secure_mode_t::SECURE_MODE_ALWAYS -- we always redirect the user to
 *     the HTTPS version (very much how people do in Apache2, only on a
 *     multi-site installation, it would not be possible to have sites
 *     using HTTP only, sites using HTTPS only, sites using a mix of
 *     HTTP and HTTPS without such a feature implemented in Snap!)
 *
 * If the function determines that a redirect is required, then it
 * calls the snap_child::page_redirect() function and never returns.
 *
 * \note
 * Testing this is blocked by SNAP-381 since a website page includes
 * the protocol and domain name and we do not duplicate them in HTTP
 * and HTTPS (see SNAP-254 for other details).
 *
 * \param[in] ipath  The info path being checked for a redirect.
 */
void permissions::on_check_for_redirect(content::path_info_t & ipath)
{
    // check whether we already are using a secure connection
    //
    snap_uri const & main_uri(f_snap->get_uri());
    if(main_uri.protocol() != "https")
    {
        // is this website marked as a secure site?
        // if so then we want to check whether we have to switch or not
        //
        libdbproxy::value const secure_site(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_PERMISSIONS_SECURE_SITE)));
        signed char const secure_mode(secure_site.safeSignedCharValue());
        if(secure_mode != secure_mode_t::SECURE_MODE_NO)
        {
            // connection is not currently secure,
            // check whether this page requires HTTPS;
            // if so, do a redirect
            //
            bool force_redirect(true);
            if(secure_mode == secure_mode_t::SECURE_MODE_PER_PAGE)
            {
                links::link_info info(get_name(name_t::SNAP_NAME_PERMISSIONS_SECURE_PAGE), true, ipath.get_key(), ipath.get_branch());
                QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
                links::link_info secure_info;
                force_redirect = link_ctxt->next_link(secure_info);
            }
            if(force_redirect)
            {
                // page has to be accessed securely, impose a redirect
                // using HTTPS
                //
                QString redirect(ipath.get_key());
                if(redirect.startsWith("http:"))
                {
                    redirect.insert(4, "s");
                    // we have a valid destination, go there
                    //
                    f_snap->page_redirect(redirect,
                                snap_child::http_code_t::HTTP_CODE_TEMPORARY_REDIRECT,
                                "Redirect to the secure version of this page.",
                                QString("This page (%1) can only be viewed using an encrypted connection. We are redirecting this user to itself using HTTPS instead (%2).")
                                        .arg(ipath.get_key()).arg(redirect));
                    NOTREACHED();
                }
                // else -- no good destination...
                SNAP_LOG_WARNING("somehow ipath key \"")
                                (redirect)
                                ("\" does not start with \"http:\" even though the main URI told us it was not secure.");
            }
        }
    }
}


bool permissions::permit_redirect_to_login_on_not_allowed_impl(content::path_info_t & ipath, bool & redirect_to_login)
{
    // the submit action does not require a log in so we avoid the redirect
    // for that action
    QString const action(ipath.get_parameter("action"));
    if(action == "submit")
    {
        // this was a submit, ignore
        redirect_to_login = false;
        return false;
    }

    return true;
}


/** \brief Get the login status of this user.
 *
 * This function retrieves the so called login status of the user. This
 * status is defined as:
 *
 * \li Spammer -- if the user was discovered to spam any one of our sites
 * \li Visitor -- a non-logged in user who happens to visit one of our sites
 * \li Returning Visitor -- a non-logged in user who came back on our site
 * \li Registered -- the user is currently logged in; this status does not
 *                   indicate the level of permission of the user
 * \li Returning Registered -- the user was logged in before but his
 *                             time is up
 *
 * \note
 * The exact syntax of the status is not disclosed here. Instead, use one
 * of the available get_name():
 *
 * \li name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_SPAMMER
 * \li name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR
 * \li name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_VISITOR
 * \li name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED
 * \li name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_REGISTERED
 *
 * \todo
 * The returning visitor is not yet implemented.
 *
 * \return One of the logged in status.
 */
QString const & permissions::get_login_status()
{
    if(f_login_status.isEmpty())
    {
        users::users * users_plugin(users::users::instance());
        f_login_status = get_name(name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_SPAMMER);
        if(!users_plugin->user_is_a_spammer())
        {
            QString const user_path(get_user_path());
            if(user_path.isEmpty())
            {
                // no user attached, if the session is considered old we
                // considered the user as a returning user
                //
                if(users_plugin->user_session_is_old())
                {
                    f_login_status = get_name(name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_VISITOR);
                }
                else
                {
                    f_login_status = get_name(name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR);
                }
            }
            else if(users_plugin->user_is_logged_in())
               /* || users_plugin->user_has_administrative_rights() -- not required since user_is_logged_in() is always true if user_has_administrative_rights() is true */
            {
                f_login_status = get_name(name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED);
            }
            else
            {
                f_login_status = get_name(name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_REGISTERED);
            }
        }
    }
    return f_login_status;
}


/** \brief Get the path to the current user.
 *
 * This function retrieves the path to the user. If the path is already
 * defined, then the function returns the cached information.
 *
 * \return The user path or "" if anonymous.
 */
QString const & permissions::get_user_path()
{
    if(!f_has_user_path)
    {
        f_has_user_path = true;
        users::users * users_plugin(users::users::instance());
        f_user_path = users_plugin->get_user_info().get_user_path(false);
        if(f_user_path == users::get_name(users::name_t::SNAP_NAME_USERS_ANONYMOUS_PATH)) // anonymous?
        {
            f_user_path.clear();
        }
    }
    return f_user_path;
}


/** \brief Check whether the user has permission to access path.
 *
 * This function checks whether the specified \p user_path has enough
 * rights, type of which is defined by \p action, to access the
 * specified \p ipath.
 *
 * So for example the anonymous user can "view" a page only if that page
 * is publicly visible. The anonymous user has pretty much only the "view"
 * rights (he can fill some forms too, search, registration, log in,
 * comments, etc. but here we'll limit ourselves to "view".) So, this
 * function asks the users plugin: "Can the anonymous user view things?".
 * The answer is yes, so the system proceeds with the question to all
 * the plugins controlling the specified page: "Is there something that
 * the specified user can view?" If so, then those rights are added for
 * the plugins. If all the plugins with control over that page said that
 * they gave the "view" right, then the user is permitted to see the
 * page and the function returns true.
 *
 * Whenever you need to test whether a user can perform a certain action
 * against some content, this is the function you have to use. For example,
 * the xmlsitemap plugin checks whether pages are publicly accessible before
 * adding them to the sitemap.xml file because pages that are not accessible
 * in this way cannot appear in a search engine since the search engine
 * will not even be able to read the page.
 *
 * \param[in] user_path  The user trying to acccess the specified path.
 * \param[in,out] ipath  The path that the user is trying to access.
 * \param[in] action  The action that the user is trying to perform.
 * \param[in] login_status  The supposed status for that user.
 * \param[in] result  The result of the test.
 */
void permissions::on_access_allowed(QString const & user_path, content::path_info_t & ipath, QString const & action, QString const & login_status, content::permission_flag & result)
{
    if(f_valid_actions.find(action) == f_valid_actions.end())
    {
        // check that the action is defined in the database (i.e. valid)
        //
        libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
        QString const site_key(f_snap->get_site_key_with_slash());
        QString const key(QString("%1%2/%3").arg(site_key).arg(get_name(name_t::SNAP_NAME_PERMISSIONS_ACTION_PATH)).arg(action));
        if(!content_table->exists(key))
        {
            // TODO it is rather easy to get here so we need to test whether
            //      the same IP does it over and over again and block them if so
            f_snap->die(snap_child::http_code_t::HTTP_CODE_ACCESS_DENIED,
                    "Unknown Action",
                    "The action you are trying to performed is not known by Snap!",
                    QString("permissions::on_access_allowed() was used with action \"%1\".").arg(action));
            NOTREACHED();
        }
        f_valid_actions[action] = true;
    }

    // setup a 'sets' object
    //
    sets_t sets(f_snap, user_path, ipath, action, login_status);

    // first we get the user rights for that action because in most cases
    // that's a lot smaller and if empty we do not have to get anything else
    // (intersection of an empty set with anything else is the empty set)
#ifdef DEBUG
#ifdef SHOW_RIGHTS
    SNAP_LOG_DEBUG("retrieving USER rights from all plugins... [")
            (sets.get_action())("] [")(login_status)("] [")
            (ipath.get_cpath())("]");
#endif
#endif
    // get all of user's rights
    //
    get_user_rights(this, sets);
    if(sets.get_user_rights_count() != 0)
    {
        if(sets.is_root())
        {
            return;
        }
#ifdef DEBUG
#ifdef SHOW_RIGHTS
        SNAP_LOG_DEBUG("retrieving PLUGIN permissions... [")
                  (sets.get_action())("] / [")
                  (sets.get_ipath().get_key())("]");
#endif
#endif
        get_plugin_permissions(this, sets);
#ifdef DEBUG
#ifdef SHOW_RIGHTS
        SNAP_LOG_DEBUG("now compute the intersection!");
#endif
#endif
        if(sets.allowed())
        {
            return;
        }
    }

    result.not_permitted();
}


/** \brief Add user rights.
 *
 * This function is called to add user rights from the specified group
 * and all of its children.
 *
 * Groups are expected to include links to different permission rights.
 *
 * \param[in] group  The rights of this group are added.
 * \param[in,out] sets  The sets receiving the rights.
 */
void permissions::add_user_rights(QString const & group, sets_t & sets)
{
    // a quick check to make sure that the programmer is not directly
    // adding a right (which he should do to the sets instead of this
    // function although we instead generate an error.)
    //
    // TODO: we probably want to change that "contains()" call with
    //       a "startsWith()" but we need to know whether "group"
    //       may include the protocol, domain name, port...
    //
    if(group.contains(get_name(name_t::SNAP_NAME_PERMISSIONS_RIGHTS_PATH)))
    {
        throw snap_logic_exception("you cannot add rights using add_user_rights(), for those just use sets.add_user_right() directly");
    }

//SNAP_LOG_DEBUG("*** add_user_rights...");
    recursive_add_user_rights(group, sets);
}


/** \brief Recursively retrieve all the user rights.
 *
 * User rights are defined in groups and this function reads all the
 * rights defined in a group and all of its children.
 *
 * The recursivity works over the group children, and children of those
 * children and so on. So the number of iteration will remain relatively
 * limited.
 *
 * \param[in] group  The group being added (a row).
 * \param[in] sets  The sets where the different paths are being added.
 */
void permissions::recursive_add_user_rights(QString const & group, sets_t & sets)
{
//SNAP_LOG_DEBUG("*** user right key: [")(group)("]");
    libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
    if(!content_table->exists(group))
    {
        throw permissions_exception_invalid_group_name("caller is trying to access group \"" + group + "\" (user)");
    }

    libdbproxy::row::pointer_t row(content_table->getRow(group));

    content::path_info_t group_ipath;
    group_ipath.set_path(group);

    // get the rights at this level
    {
        QString const link_start_name(
                        QString("%1::%2::%3")
                            .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_NAMESPACE))
                            .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_ACTION_NAMESPACE))
                            .arg(sets.get_action()));
        links::link_info info(link_start_name, false, group_ipath.get_key(), group_ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info right_info;
        while(link_ctxt->next_link(right_info))
        {
            // a user right is attached to this page
            QString const right_key(right_info.key());
            sets.add_user_right(right_key);
        }
    }

    // get all the children and do a recursive call with them all
    {
        QString const children_name(content::get_name(content::name_t::SNAP_NAME_CONTENT_CHILDREN));
        links::link_info info(children_name, false, group_ipath.get_key(), group_ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info right_info;
        while(link_ctxt->next_link(right_info))
        {
            // a user right is attached to this page
            const QString child_key(right_info.key());
//SNAP_LOG_DEBUG("*** children...");
            recursive_add_user_rights(child_key, sets);
        }
    }
}


/** \brief Add plugin rights.
 *
 * This function is called to add plugin rights from the specified group
 * and all of its children.
 *
 * Groups are expected to include links to different permission rights.
 *
 * \todo
 * It seems to me that the name of the plugin should always be the
 * same when groups are concerned, i.e. "content", but I'm not really
 * 100% convinced of that so at this point it is a parameter. Also it
 * could be that the name should be defined in the group and not by
 * the caller (that may be the real deal).
 *
 * \param[in] plugin_name  The name of the plugin adding these rights.
 * \param[in] group  The rights of this group are added.
 * \param[in,out] sets  The sets receiving the rights.
 */
void permissions::add_plugin_permissions(QString const & plugin_name, QString const & group, sets_t & sets)
{
    // a quick check to make sure that the programmer is not directly
    // adding a right (which he should do to the sets instead of this
    // function although we instead generate an error.)
    if(group.contains("types/permissions/rights"))
    {
        throw snap_logic_exception("you cannot add rights using add_plugin_permissions(), for those just use sets.add_plugin_permission() directly");
    }

    recursive_add_plugin_permissions(plugin_name, group, sets);
}


/** \brief Recursively retrieve all the user rights.
 *
 * User rights are defined in groups and this function reads all the
 * rights defined in a group and all of its children.
 *
 * The recursivity works over the group children, and children of those
 * children and so on. So the number of iteration will remain relatively
 * limited.
 *
 * \param[in] plugin_name  The name of the plugin adding these rights.
 * \param[in] group  The key of the group being added (a row).
 * \param[in] sets  The sets where the different paths are being added.
 */
void permissions::recursive_add_plugin_permissions(QString const & plugin_name, QString const & group, sets_t & sets)
{
    libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
    if(!content_table->exists(group))
    {
        throw permissions_exception_invalid_group_name("caller is trying to access group \"" + group + "\" which does not exist (recursive_add_plugin_permissions)");
    }

    content::path_info_t ipath;
    ipath.set_path(group);

    // get the rights at this level
    {
        // this is always an immediate action (no "direct" namespace)
        QString const link_start_name(
                    QString("%1::%2::%3")
                        .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_NAMESPACE))
                        .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_ACTION_NAMESPACE))
                        .arg(sets.get_action()));
        links::link_info info(link_start_name, false, ipath.get_key(), ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info right_info;
        while(link_ctxt->next_link(right_info))
        {
            // an author is attached to this page
            QString const right_key(right_info.key());
            sets.add_plugin_permission(plugin_name, right_key);
        }
    }

    // get all the children and do a recursive call with them all
    {
        QString const children_name(content::get_name(content::name_t::SNAP_NAME_CONTENT_CHILDREN));
        links::link_info info(children_name, false, ipath.get_key(), ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info right_info;
        while(link_ctxt->next_link(right_info))
        {
            // an author is attached to this page
            QString const child_key(right_info.key());
            recursive_add_plugin_permissions(plugin_name, child_key, sets);
        }
    }
}


/** \brief Register the permissions actions.
 *
 * This function registers this plugin as supporting the following
 * actions:
 *
 * \li "permissions::makeroot" and "permissions::makeadministrator"
 *
 * After an installation and a user was created on the website, the server
 * is ready to create a root user. The "makeroot" action is used for that
 * purpose. The root user can do pretty much anything he wants on the
 * entire cluster of websites defined in a Snap database.
 *
 * Once a website is created and an administrator user is registered on that
 * website, one can use the "makeadministrator" action to mark that user
 * as a Snap administrator. An administrator is limited to working on a
 * specific website, but he can generally change anything that appears on
 * that website.
 *
 * The user is specified by his email address defined with a parameter.
 * The parameter name is ROOT_USER_EMAIL.
 *
 * The backend command line looks something like one of these:
 *
 * \code
 * # For an administrator, make sure to specify the website
 * snapbackend http://www.example.com \
 *             [--config /etc/snapwebsites] \
 *             --action permissions::makeadministrator \
 *             --param ROOT_USER_EMAIL=joe@example.com
 *
 * # For a root user, you do not need to specify the website, but probably should too
 * snapbackend [http://www.example.com] \
 *             [--config /etc/snapwebsites] \
 *             --action permissions::makeroot \
 *             --param ROOT_USER_EMAIL=joe@example.com
 * \endcode
 *
 * If you have problems with it (it does not seem to work,) try with
 * --debug and make sure to look in the snapserver.log and syslog
 * output files.
 *
 * \note
 * These should be user actions, unfortunately that would add a 
 * dependency on the "permissions" plugin in the "users" plugin,
 * which we cannot have (i.e. "permissions" need to know about
 * "users"... so we would end up with a looping dependency.)
 *
 * \li "permissions::checkpermissions"
 *
 * Once a user created an account on a website, it can be difficult to
 * know what permissions that user has. The snapbackend can be used for
 * the purpose to verify whether a user has or does not have access to
 * a page given a certain action.
 *
 * The "checkpermission" action expects four parameters to be
 * defined. If one of these is not defined, it is likely that the
 * function will generate an error.
 *
 * ** USER_EMAIL -- the email address of the user whom the permissions
 * are ot be checked;
 * ** PAGE_URI -- the URI to the page being checked for that user;
 * ** CHECK_ACTION -- the action being checked (view, administer, edit,
 * delete, etc.);
 * ** LOGIN_STATUS -- run the check assuming this user status (one of:
 * "spammer", "visitor", "returning_visitor", "returning_registered",
 * "registered").
 *
 * The USER_EMAIL parameter can be set to an empty string in which case
 * it is viewed as the anonymous visitor. In that case the status should
 * not be set to "returning_registered" or "registered" since those two
 * statuses do not make sense for an anonymous (unregistered) visitor.
 *
 * \code
 * snapbackend http://www.example.com \
 *          --action permissions::checkpermissions \
 *          --param USER_EMAIL=john@example.com \
 *                  PAGE_URI=http://www.example.com/journal/2015/08/13/beautiful-weather \
 *                  CHECK_ACTION=view \
 *                  LOGIN_STATUS=registered
 * \endcode
 *
 * \note
 * The login status names are the same as when you write a script
 * for a list in need of a permissions check.
 *
 * \param[in,out] actions  The list of supported actions where we add ourselves.
 */
void permissions::on_register_backend_action(server::backend_action_set & actions)
{
    actions.add_action(get_name(name_t::SNAP_NAME_PERMISSIONS_MAKE_ADMINISTRATOR), this);
    actions.add_action(get_name(name_t::SNAP_NAME_PERMISSIONS_MAKE_ROOT),          this);
    actions.add_action(get_name(name_t::SNAP_NAME_PERMISSIONS_CHECK_PERMISSIONS),  this);
}


/** \brief Execute a permission action.
 *
 * Mark a user as a root or administrator user;
 *
 * Check permissions for a user.
 *
 * See the on_register_backend_action() function for details about the
 * parameters and available actions.
 *
 * \param[in] action  The action the user wants to execute.
 *
 * \sa on_register_backend_action()
 */
void permissions::on_backend_action(QString const & action)
{
    if(action == get_name(name_t::SNAP_NAME_PERMISSIONS_MAKE_ADMINISTRATOR)
    || action == get_name(name_t::SNAP_NAME_PERMISSIONS_MAKE_ROOT))
    {
        // make specified user an administrator or root user
        users::users * users_plugin(users::users::instance());
        QString const email(f_snap->get_server_parameter("ROOT_USER_EMAIL"));
        users::users::user_info_t const user_info(users_plugin->get_user_info_by_email(email));
        if(!user_info.exists())
        {
            SNAP_LOG_FATAL("User \"")(email)("\" not found. Cannot make user the root or administrator user.");
            exit(1);
        }
        if(!user_info.value_exists(users::name_t::SNAP_NAME_USERS_IDENTIFIER))
        {
            SNAP_LOG_FATAL("error: user \"")(email)("\" was not given an identifier.");
            exit(1);
        }
        libdbproxy::value identifier_value(user_info.get_value(users::name_t::SNAP_NAME_USERS_IDENTIFIER));
        if(identifier_value.nullValue() || identifier_value.size() != sizeof(int64_t))
        {
            SNAP_LOG_FATAL("error: user \"")(email)("\" identifier could not be read.");
            exit(1);
        }
        int64_t const identifier(identifier_value.int64Value());

        //QString const site_key(f_snap->get_site_key_with_slash());
        content::path_info_t user_ipath;
        user_ipath.set_path(QString("%1/%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PATH)).arg(identifier));
        content::path_info_t dpath;
        dpath.set_path(QString("%1/%2")
                .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_GROUPS_PATH))
                .arg(action == get_name(name_t::SNAP_NAME_PERMISSIONS_MAKE_ROOT) ? "root" : "root/administrator"));

        // now link that user to that high level permission
        QString const link_name(QString("%1::%2::%3")
                .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_NAMESPACE))
                .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_DIRECT_NAMESPACE))
                .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_GROUP_NAMESPACE)));
        bool const source_multi(false);
        links::link_info source(link_name, source_multi, user_ipath.get_key(), user_ipath.get_branch());
        //QString const link_to(get_name(name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_GROUP));
        bool const destination_multi(false);
        links::link_info destination(link_name, destination_multi, dpath.get_key(), dpath.get_branch());
        links::links::instance()->create_link(source, destination);
    }
    else if(action == get_name(name_t::SNAP_NAME_PERMISSIONS_CHECK_PERMISSIONS))
    {
        // used to debug permissions from a console (later we may want to
        // allow for such a check in a GUI tool as well)
        QString const email(f_snap->get_server_parameter("USER_EMAIL"));
        QString const page(f_snap->get_server_parameter("PAGE_URI"));
        QString const permission_action(f_snap->get_server_parameter("CHECK_ACTION"));
        QString const status(f_snap->get_server_parameter("LOGIN_STATUS"));
        check_permissions(email, page, permission_action, status);
    }
    else
    {
        // unknown action (we should not have been called with that name!)
        throw snap_logic_exception(QString("permissions.cpp:on_backend_action(): permissions::on_backend_action(\"%1\") called with an unknown action...").arg(action));
    }
}


/** \brief Check what the user permissions are (show the sets).
 *
 * This function gathers the list of URLs that the user has access to
 * and the list of URLs for the specified page. Both lists are shown
 * in full. This is what is used in the on_access_allowed() function.
 *
 * Process:
 *
 * \li Calculate all the URIs for the user and prints those.
 * \li Calculate all the URIs for the page and prints those.
 * \li Calculate the intersection and print the result.
 *
 * \param[in] email  The email to the user being checked.
 * \param[in] page  The URI to the page being checked.
 * \param[in] action  The action being applied.
 * \param[in] status  The user login status.
 */
void permissions::check_permissions(QString const & email, QString const & page, QString const & action, QString const & status)
{
    // Note that this check is about determining the list of pages
    // attached to a user and a page in regard to a given action
    // so it can be slow
    //
    // it is otherwise similar to the 

    if(f_valid_actions.find(action) == f_valid_actions.end())
    {
        // check that the action is defined in the database (i.e. valid)
        libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
        QString const site_key(f_snap->get_site_key_with_slash());
        QString const key(QString("%1%2/%3").arg(site_key).arg(get_name(name_t::SNAP_NAME_PERMISSIONS_ACTION_PATH)).arg(action));
        if(!content_table->exists(key))
        {
            // TODO it is rather easy to get here so we need to test whether
            //      the same IP does it over and over again and block them if so
            //
            //    [ only this is a backend an administrator uses... ]
            //
            std::cerr << "error: " << action << " is not a known action.\n";
            return;
        }
        f_valid_actions[action] = true;
    }

    // define the path to the user data from his email
    users::users::user_info_t user_info(users::users::instance()->get_user_info_by_email(email));
    QString user_path(user_info.get_user_path(false));
    if(user_path == users::get_name(users::name_t::SNAP_NAME_USERS_ANONYMOUS_PATH)) // anonymous?
    {
        user_path.clear();
    }

    // define the path to the page as a content::path_info_t
    content::path_info_t ipath;
    ipath.set_path(page);

    char const * login_status(get_name(details::login_status_from_string(status)));

    // setup a 'sets' object
    sets_t sets(f_snap, user_path, ipath, action, login_status);

    // first we get the user rights for that action because in most cases
    // that's a lot smaller and if empty we do not have to get anything else
    // (intersection of an empty set with anything else is the empty set)
#ifdef DEBUG
#ifdef SHOW_RIGHTS
    std::cout << std::endl << "[" << getpid() << "]: permissions::check_permissions(): retrieving USER rights from all plugins... ["
            << sets.get_action() << "] [" << login_status << "] ["
            << ipath.get_cpath() << "]" << std::endl;
#endif
#endif
    // get all of user's rights
    get_user_rights(this, sets);

    // present user rights to administrator
    bool user_is_root(false);
    int const user_right_count(sets.get_user_rights_count());
    if(user_right_count == 0)
    {
        std::cout << "user \""
                  << email
                  << "\" has no rights for action \""
                  << action
                  << "\"."
                  << std::endl;
    }
    else
    {
        user_is_root = sets.is_root();
        std::cout << "user \""
                  << email
                  << "\""
                  << (user_is_root ? " is considered a root user and" : "")
                  << " has "
                  << user_right_count
                  << " rights:"
                  << std::endl;
        permissions::sets_t::set_t const & rights(sets.get_user_rights());
        for(int idx(0); idx < user_right_count; ++idx)
        {
            std::cout << "  "
                      << idx + 1
                      << ". "
                      << rights[idx]
                      << std::endl;
        }
    }
    std::cout << std::endl;

#ifdef DEBUG
#ifdef SHOW_RIGHTS
    std::cout << "[" << getpid() << "]: permissions::check_permissions(): retrieving PLUGIN permissions... ["
              << sets.get_action() << "] / ["
              << sets.get_ipath().get_key() << "]"
              << std::endl;
#endif
#endif
    get_plugin_permissions(this, sets);

    // present user rights to administrator
    int const plugin_right_count(sets.get_plugin_rights_count());
    if(plugin_right_count == 0)
    {
        std::cout << "page \""
                  << page
                  << "\" has no rights for action \""
                  << action
                  << "\"."
                  << std::endl;
    }
    else
    {
        std::cout << "page \""
                  << page
                  << "\" has "
                  << plugin_right_count
                  << " rights:"
                  << std::endl;
        permissions::sets_t::req_sets_t const & plugins(sets.get_plugin_rights());
        int count(0);
        // auto does not work here because we want to have access to the
        // key which QMap does not give us when using auto
        for(permissions::sets_t::req_sets_t::const_iterator it(plugins.begin());
                it != plugins.end();
                ++it)
        {
            ++count;
            std::cout << "  "
                      << count
                      << ". Permissions offered by plugin: "
                      << it.key()
                      << std::endl;
            permissions::sets_t::set_t const & plugin_permissions(it.value());
            int const max_rights(plugin_permissions.size());
            for(int idx(0); idx < max_rights; ++idx)
            {
                std::cout << "    "
                          << count
                          << "."
                          << idx + 1
                          << ". "
                          << plugin_permissions[idx]
                          << std::endl;
            }
        }
    }
    std::cout << std::endl;

#ifdef DEBUG
#ifdef SHOW_RIGHTS
    std::cout << "[" << getpid() << "]: now compute the intersection!" << std::endl;
#endif
#endif

    std::cout << "The result is that "
              << (user_is_root ? "root " : "")
              << "user \""
              << email
              << "\" "
              << (user_is_root || sets.allowed() ? "can" : "CANNOT")
              << " access page \""
              << page
              << "\" with action \""
              << action
              << "\"."
              << std::endl
              << "  -- Note: If you have a problem, you may want to first delete the caches for that page and try again."
              << std::endl;
}


/** \brief Signal received when a new user was verified.
 *
 * After a user registers, he receives an email with a magic number that
 * needs to be used for the user to be authorized to access the system as
 * a registered user. This signal is sent once the user email got verified.
 *
 * \param[in,out] ipath  The user path.
 * \param[in] identifier  The user identifier.
 */
void permissions::on_user_verified(content::path_info_t & ipath, int64_t identifier)
{
    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());
    uint64_t const created_date(f_snap->get_start_date());

    // All users are also given a permission group that can be used to
    // give each individual user very specific rights.

    // first we create the user specific right
    content::path_info_t permission_ipath;
    {
        // create a specific permission for that new company
        permission_ipath.set_path(QString("%1/%2").arg(get_name(name_t::SNAP_NAME_PERMISSIONS_USERS_PATH)).arg(identifier));
        permission_ipath.force_branch(snap_version::SPECIAL_VERSION_USER_FIRST_BRANCH);
        permission_ipath.force_revision(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));
        permission_ipath.force_locale("xx"); // create the neutral one by default?
        content_plugin->create_content(permission_ipath, get_plugin_name(), "system-page");

        // create a default the title and body
        libdbproxy::row::pointer_t permission_revision_row(revision_table->getRow(permission_ipath.get_revision_key()));
        permission_revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->setValue(created_date);
        // TODO: translate (not too important on this page since it is really not public)
        permission_revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->setValue(QString("User #%1 Private Permission Right").arg(identifier));
        permission_revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_BODY))->setValue(QString("This type represents permissions that are 100% specific to this user."));
    }

    // second we create the user specific group
    content::path_info_t group_ipath;
    {
        // create a specific permission group for that new user
        group_ipath.set_path(QString("%1/users/%2").arg(get_name(name_t::SNAP_NAME_PERMISSIONS_GROUPS_PATH)).arg(identifier));
        group_ipath.force_branch(snap_version::SPECIAL_VERSION_USER_FIRST_BRANCH);
        group_ipath.force_revision(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));
        group_ipath.force_locale("xx");
        content_plugin->create_content(group_ipath, get_plugin_name(), "system-page");

        // save the title, description, and link to the type
        libdbproxy::row::pointer_t group_revision_row(revision_table->getRow(group_ipath.get_revision_key()));
        group_revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->setValue(created_date);
        // TODO: translate (not too important on this page since it is really not public)
        group_revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->setValue(QString("User #%1 Private Permission Group").arg(identifier));
        group_revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_BODY))->setValue(QString("This group represents a user private group."));
    }

    // link the permission to the company and the user
    // this user has view and edit rights
    //
    // WARNING: Note that we link the User Page to this new permission, we
    //          are NOT linking the user to the new permission... it can be
    //          very confusing on this one since user ipath looks very much
    //          like the user, doesn't it?
    {
        QString const link_name(get_name(name_t::SNAP_NAME_PERMISSIONS_ACTION_VIEW));
        QString const link_to(get_name(name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_VIEW));
        bool const source_unique(false);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, ipath.get_key(), ipath.get_branch());
        links::link_info destination(link_to, destination_unique, permission_ipath.get_key(), permission_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }
    {
        QString const link_name(get_name(name_t::SNAP_NAME_PERMISSIONS_ACTION_EDIT));
        QString const link_to(get_name(name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_EDIT));
        bool const source_unique(false);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, ipath.get_key(), ipath.get_branch());
        links::link_info destination(link_to, destination_unique, permission_ipath.get_key(), permission_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }
    {
        QString const link_name(get_name(name_t::SNAP_NAME_PERMISSIONS_ACTION_ADMINISTER));
        QString const link_to(get_name(name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_ADMINISTER));
        bool const source_unique(false);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, ipath.get_key(), ipath.get_branch());
        links::link_info destination(link_to, destination_unique, permission_ipath.get_key(), permission_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    // link the user to his private group right
    {
        QString const link_name(QString("%1::%2::%3")
                        .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_NAMESPACE))
                        .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_DIRECT_NAMESPACE))
                        .arg(get_name(name_t::SNAP_NAME_PERMISSIONS_GROUP_NAMESPACE)));
        QString const link_to(get_name(name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_GROUP));
        bool const source_unique(false);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, ipath.get_key(), ipath.get_branch());
        links::link_info destination(link_to, destination_unique, group_ipath.get_key(), group_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    // then add permissions for the user to be able to edit his own
    // account information
    //
    // TBD: should we have a way to prevent the user from editing his
    //      information?
    {
        QString const link_name(get_name(name_t::SNAP_NAME_PERMISSIONS_ACTION_VIEW));
        QString const link_to(get_name(name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_VIEW));
        bool const source_unique(false);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, permission_ipath.get_key(), permission_ipath.get_branch());
        links::link_info destination(link_to, destination_unique, group_ipath.get_key(), group_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }
    {
        QString const link_name(get_name(name_t::SNAP_NAME_PERMISSIONS_ACTION_EDIT));
        QString const link_to(get_name(name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_EDIT));
        bool const source_unique(false);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, permission_ipath.get_key(), permission_ipath.get_branch());
        links::link_info destination(link_to, destination_unique, group_ipath.get_key(), group_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }
    {
        QString const link_name(get_name(name_t::SNAP_NAME_PERMISSIONS_ACTION_ADMINISTER));
        QString const link_to(get_name(name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_ADMINISTER));
        bool const source_unique(false);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, permission_ipath.get_key(), permission_ipath.get_branch());
        links::link_info destination(link_to, destination_unique, group_ipath.get_key(), group_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }
}



namespace details
{

void call_perms(snap_expr::variable_t & result, snap_expr::variable_t::variable_vector_t const & sub_results)
{
    if(sub_results.size() < 3 || sub_results.size() > 4)
    {
        throw snap_expr::snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call perms() expected 3 or 4 parameters");
    }
    QString const path(sub_results[0].get_string("perms(1)"));
    QString user_path(sub_results[1].get_string("perms(2)"));
    if(user_path == users::get_name(users::name_t::SNAP_NAME_USERS_ANONYMOUS_PATH))
    {
        // permissions for anonymous users is done with an empty user path
        user_path.clear();
    }
    QString action(sub_results[2].get_string("perms(3)"));
    QString status;
    if(sub_results.size() == 4)
    {
        status = sub_results[3].get_string("perms(4)");
    }
    else
    {
        // this is the default status, not too sure that is the best default
        // though...
        status = "returning_registered";
    }
    //SNAP_LOG_WARNING("perms(\"")(path)("\", \"")(user_path)("\", \"")(action)("\", \"")(status)("\")");

    // setup the parameters to the access_allowed() signal
    //
    content::path_info_t ipath;
    ipath.set_path(path);
    if(ipath.get_cpath() == "admin"
    || ipath.get_cpath().startsWith("admin/"))
    {
        action = "administer";
    }
    ipath.set_parameter("action", action);
    quiet_error_callback err_callback(content::content::instance()->get_snap(), false);
    path::path::instance()->validate_action(ipath, action, err_callback);

    char const * login_status(get_name(login_status_from_string(status)));

    // check whether that user is allowed that action with that path and given status
    //
    content::permission_flag allowed;
    path::path::instance()->access_allowed(user_path, ipath, action, login_status, allowed);

    // save the result
    libdbproxy::value value;
    value.setBoolValue(allowed.allowed());
    result.set_value(snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL, value);
    //SNAP_LOG_WARNING("exit call perms \"")(allowed.allowed() ? "allowed!" : "forbidden")("\"");
}


snap_expr::functions_t::function_call_table_t const permissions_functions[] =
{
    { // check whether a user has permissions to access a page
        "perms",
        call_perms
    },
    {
        nullptr,
        nullptr
    }
};


} // namespace details


void permissions::on_add_snap_expr_functions(snap_expr::functions_t & functions)
{
    functions.add_functions(details::permissions_functions);
}


void permissions::on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    NOTUSED(header);

    if(ipath.get_cpath() == "remove-iframe-for-login")
    {
        QDomDocument doc(header.ownerDocument());
        content::content::instance()->add_javascript(doc, "remove-iframe-for-login");
    }

    // check whether the user has edit rights
    content::path_info_t sub_ipath;
    sub_ipath.set_path(ipath.get_key());
    sub_ipath.set_parameter("action", "edit");
    quiet_error_callback err_callback(content::content::instance()->get_snap(), false);
    path::path::instance()->validate_action(sub_ipath, "edit", err_callback);

    content::permission_flag can_edit;
    if(err_callback.has_error())
    {
        can_edit.not_permitted();
    }
    else
    {
        path::path::instance()->access_allowed(
                users::users::instance()->get_user_info().get_user_path(false),
                sub_ipath,
                "edit",
                get_name(name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED),
                can_edit);
    }
    QString const can_edit_page(can_edit.allowed() ? "yes" : "");

    FIELD_SEARCH
        (content::field_search::command_t::COMMAND_ELEMENT, metadata)
        (content::field_search::command_t::COMMAND_MODE, content::field_search::mode_t::SEARCH_MODE_EACH)

        // snap/head/metadata/desc[@type="can_edit"]/data
        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, can_edit_page)
        (content::field_search::command_t::COMMAND_SAVE, "desc[type=can_edit]/data")

        // snap/head/metadata/desc[@type="login_status"]/data
        (content::field_search::command_t::COMMAND_DEFAULT_VALUE, get_login_status())
        (content::field_search::command_t::COMMAND_SAVE, "desc[type=login_status]/data")

        // generate!
        ;
}


/** \brief Whenever a permissions link changes we reset the caches.
 *
 * To make sure that the permissions caches are up to date we have
 * to update the cache time stamp. This signal let us know whether
 * a permissions link was modified and if so we update the
 * timestamp. To make sure that we get the correct date we get
 * the real current date here (opposed to the usual get_start_time()
 * call.) This means that the cache will remain invalid throughout
 * this request...
 *
 * \param[in] link  The link information being modified.
 * \param[in] created  Whether this was a new link (true) or not.
 */
void permissions::on_modified_link(links::link_info const & link, bool const created)
{
    NOTUSED(created);

    if(!link.name().startsWith(QString("%1::").arg(get_name(name_t::SNAP_NAME_PERMISSIONS_NAMESPACE))))
    {
        // not a permission link, who cares
        return;
    }

    // a permissions link got modified, reset the timestamp date and time
    // so any existing caches are reset
    //
    reset_permissions_cache();
}


/** \brief Reset last updated time for permissions cache.
 *
 * This function saves 'now' as the current permission time threshold.
 * This is important because certain things (such as changing a link)
 * requires the permissions currently cached to be ignored. This
 * function updates that threshold.
 */
void permissions::reset_permissions_cache()
{
    // we use 'last_updated + EXPECTED_TIME_ACCURACY_EPSILON' so that all
    // caches in this session will be ignored which is important; it will
    // also re-generate the permissions the next time the user access the
    // website; this also affects backend processes for that long (10ms
    // at time of writing)
    //
    // TODO: if the accuracy epsilon is too small, we get problems...
    //       if it is "too large", we get slowness which people should
    //       be able to survive... we would need to have a strong PTP
    //       service running and see what kind of accuracy we can get
    //       on a "large" network
    //
    int64_t const last_updated(f_snap->get_current_date());
    libdbproxy::value value;
    value.setInt64Value(last_updated + EXPECTED_TIME_ACCURACY_EPSILON);
    f_snap->set_site_parameter(get_name(name_t::SNAP_NAME_PERMISSIONS_LAST_UPDATED), value);
}


/** \brief Repair the permission links.
 *
 * When cloning a page, permissions will disappear. This function restores
 * them from the source page.
 *
 * Later we may avoid copying certain permissions. At this point all the
 * permissions get copied.
 */
void permissions::repair_link_of_cloned_page(QString const & clone, snap_version::version_number_t branch_number, links::link_info const & source, links::link_info const & destination, bool const cloning)
{
    NOTUSED(cloning);

    // permission links are never unique
    links::link_info src(source.name(), false, clone, branch_number);
    links::links::instance()->create_link(src, destination);
}




SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
