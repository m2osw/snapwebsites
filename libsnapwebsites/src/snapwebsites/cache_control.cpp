// Snap Websites Server -- handle Cache-Control settings
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include "snapwebsites/cache_control.h"

// snapwebsites lib
//
#include "snapwebsites/http_strings.h"
#include "snapwebsites/log.h"

// snapdev lib
//
#include <snapdev/not_reached.h>

// boost lib
//
#include <boost/algorithm/string.hpp>


// last include
//
#include <snapdev/poison.h>


namespace snap
{


/** \brief Initialize a cache control object with defaults.
 *
 * This function initialize this cache control object with the
 * defaults and returns.
 *
 * You may later apply various changes to the cache control data
 * using the set_...() functions and the set_cache_info() if you
 * have cache control data in the form of a standard HTTP string.
 */
cache_control_settings::cache_control_settings()
{
}


/** \brief Initializes a cache control object with the specified info.
 *
 * This function initialize this cache control object with the defaults
 * and then applies the \p info parameters to the controls if \p info is
 * not an empty string.
 *
 * \param[in] info  The new cache control information to assign to this object.
 * \param[in] internal_setup  Whether to allow our special '!' syntax.
 *
 * \sa set_cache_info()
 */
cache_control_settings::cache_control_settings(QString const & info, bool const internal_setup)
{
    set_cache_info(info, internal_setup);
}


/** \brief Reset all the cache information to their defaults.
 *
 * This function resets all the cache information to their defaults so
 * the object looks as if it just got initialized.
 */
void cache_control_settings::reset_cache_info()
{
    f_max_age = 0;
    f_max_stale = IGNORE_VALUE;
    f_min_fresh = IGNORE_VALUE;
    f_must_revalidate = true;
    f_no_cache = false;
    f_no_store = true;
    f_no_transform = false;
    f_only_if_cached = false;
    f_private = false;
    f_public = false;
    f_s_maxage = IGNORE_VALUE;
}


/** \brief Set the cache information parsed from the \p info parameter.
 *
 * This function parses the \p info string for new cache definitions.
 * The \p info string may be empty in which case nothing is modified.
 * It is expected to be the string found in a Cache-Control header
 * sent by the client.
 *
 * If you want to start from scratch, you may call reset_cache_info()
 * first. You can also use a brand new object and then copy it since
 * the copy operator is available.
 *
 * The 'must-revalidate' and 'no-store' are set by default. Unfortunately
 * that would mean the page setup capability would not be able to ever
 * clear those two flags. That would mean you could never use a full
 * permanent cache definition. Instead we offer an extension to the
 * flags and allow one to add a '!' in front of the names as in:
 * '!no-cache'. This way you can force the 'no-cache' flag to false
 * instead of the default of true.
 *
 * \todo
 * Determine whether any error in the field should be considered fatal and
 * thus react by ignoring the entire \p info parameter. It seems that the
 * HTTP specification asks us to do so... (i.e. ignore all when any one
 * flag is not understood.) However, it seems that most browsers implement
 * such things the other way around: try to retrieve the maximum amount of
 * information as possible and use whatever they understand from that.
 *
 * \todo
 * Determine whether we should accept certain parameters only once.
 * Especially those that include values (i.e. max-age=123) because
 * the current implementation only takes the last one in account when
 * we probably should remember the first one (within one \p info string).
 *
 * \todo
 * Add support for private and no-cache parameters (server side only).
 *
 * \param[in] info  The new cache control information to assign to this object.
 * \param[in] internal_setup  Whether to allow our special '!' syntax.
 */
void cache_control_settings::set_cache_info(QString const & info, bool const internal_setup)
{
    // TODO: we want the weighted HTTP strings to understand
    //       parameters with values assigned and more than the
    //       q=xxx then we can update this code to better test
    //       what we are dealing with...

    // parse the data with the weighted HTTP string implementation
    //
    http_strings::WeightedHttpString client_cache_control(info);

    // no go through the list of parts and handle them appropriately
    //
    http_strings::WeightedHttpString::part_t::vector_t const & cache_control_parts(client_cache_control.get_parts());
    for(auto const & c : cache_control_parts)
    {
        // get the part name
        //
        QString name(c.get_name());

        bool negate(false);
        if(internal_setup)
        {
            negate = name[0].unicode() == '!';
            if(negate)
            {
                // remove the negate flag
                //
                name = name.mid(1);
            }
        }

        if(name.isEmpty())
        {
            continue;
        }

        // TODO: add code to check whether 'negate' (!) was used with
        //       an item that does not support it

        // do a switch first to save a tad bit of time
        switch(name[0].unicode())
        {
        case 'i':
            if(name == "immutable")
            {
                set_immutable(!negate);
            }
            break;

        case 'm':
            if(name == "max-age")
            {
                set_max_age(c.get_value());
            }
            else if(name == "max-stale")
            {
                QString const & value(c.get_value());
                if(value.isEmpty())
                {
                    // any stale data can be returned
                    //
                    set_max_stale(0);
                }
                else
                {
                    set_max_stale(value);
                }
            }
            else if(name == "min-fresh")
            {
                set_min_fresh(c.get_value());
            }
            else if(name == "must-revalidate")
            {
                set_must_revalidate(!negate);
            }
            break;

        case 'n':
            if(name == "no-cache")
            {
                QString const & value(c.get_value());
                if(value.isEmpty())
                {
                    set_no_cache(!negate);
                }
                else
                {
                    // list of fields that require revalidation
                    //
                    snap_string_list const list(value.split(","));
                    for(auto const & l : list)
                    {
                        add_revalidate_field_name(l);
                    }
                }
            }
            else if(name == "no-store")
            {
                set_no_store(!negate);
            }
            else if(name == "no-transform")
            {
                set_no_transform(!negate);
            }
            break;

        case 'o':
            if(name == "only-if-cached")
            {
                set_only_if_cached(!negate);
            }
            break;

        case 'p':
            if(name == "private")
            {
                QString const & value(c.get_value());
                if(value.isEmpty())
                {
                    set_private(!negate);
                }
                else
                {
                    // list of fields that require revalidation
                    //
                    snap_string_list list(value.split(","));
                    for(auto l : list)
                    {
                        add_private_field_name(l);
                    }
                }
            }
            else if(name == "proxy-revalidate")
            {
                set_proxy_revalidate(!negate);
            }
            else if(name == "public")
            {
                set_public(!negate);
            }
            break;

        case 's':
            if(name == "s-maxage")
            {
                set_s_maxage(c.get_value());
            }
            break;

        }
    }
}


/** \brief Set the must-revalidate to true or false.
 *
 * This function should only be called with 'true' to request
 * that the client revalidate the data each time it wants to
 * access it.
 *
 * However, the flag is set to 'true' by default, so really it
 * is only useful if you want to set the parameter to 'false'.
 *
 * \note
 * This flag may appear in the server response.
 *
 * \param[in] must_revalidate  Whether the client should revalidate that
 *                             specific content on each page load.
 *
 * \sa get_must_revalidate()
 */
void cache_control_settings::set_must_revalidate(bool const must_revalidate)
{
    f_must_revalidate = must_revalidate;
}


/** \brief Get the current value of the must-revalidate flag.
 *
 * This function returns the current value of the must-revalidate
 * flag.
 *
 * \note
 * This flag may appear in the server response.
 *
 * \return true if the must-revalidate flag is to be specified in the
 *         Cache-Control field.
 *
 * \sa set_must_revalidate()
 */
bool cache_control_settings::get_must_revalidate() const
{
    return f_must_revalidate;
}


/** \brief Set the 'private' flag to true or false.
 *
 * Any page that is private, and thus should not be saved in a
 * shared cache (i.e. proxies), must be assigned the private
 * flag, so this function must be called with true.
 *
 * Note that does not encrypt the data in any way. It just adds
 * the private flag to the Cache-Control header. If you need to
 * encrypt the data, make sure to enforce HTTPS before returning
 * a reply with secret data.
 *
 * \note
 * This flag may appear in the server response.
 *
 * \param[in] private_cache  Whether intermediate, shared, proxy caches
 *                           are allowed to cache this data.
 *
 * \sa get_private()
 */
void cache_control_settings::set_private(bool const private_cache)
{
    f_private = private_cache;
}


/** \brief Get the current value of the 'private' flag.
 *
 * This function returns the current value of the 'private'
 * flag.
 *
 * Note that 'private' has priority over 'public'. So if 'private'
 * is true, 'public' is ignored. For this reason you should only
 * set those flags to true and never attempt to reset them to
 * false. Similarly, the 'no-cache' and 'no-store' have priority
 * over the 'private' flag.
 *
 * \note
 * This flag may appear in the server response.
 *
 * \return true if the 'private' flag is to be specified in the
 *         Cache-Control field.
 *
 * \sa set_private()
 */
bool cache_control_settings::get_private() const
{
    return f_private;
}


/** \brief Set 'proxy-revalidate' to true or false.
 *
 * This function should only be called with 'true' to request
 * that proxy caches revalidate the data each time a client
 * asks for the data.
 *
 * You may instead want to use the 's-maxage' field.
 *
 * \note
 * This flag may appear in the server response.
 *
 * \param[in] proxy_revalidate  Whether proxy caches should revalidate that
 *                              specific content.
 *
 * \sa get_proxy_revalidate()
 */
void cache_control_settings::set_proxy_revalidate(bool const proxy_revalidate)
{
    f_proxy_revalidate = proxy_revalidate;
}


/** \brief Set whether the resource is immutable or not.
 *
 * This function sets whether the resource is immutable. Browsers that
 * understand this flag will never check the server again for that
 * specific resource as long as they keep it in their caches.
 *
 * Immutable means that it will never change. This is true for all
 * our CSS and JS files because these are versioned and any changes
 * to those files require a change in their version.
 *
 * \param[in] immutable  Whether the file is immutable or not.
 */
void cache_control_settings::set_immutable(bool const immutable)
{
    f_immutable = immutable;
}


/** \brief Get whether the data was marked immutable.
 *
 * This function returns true if the data attached to that resource
 * is considered immutable.
 *
 * For example, JS and CSS files are always considered immutable. This
 * is because we have a version and if you want to modify those files,
 * you must increase the version accordingly.
 *
 * Browsers that support the immutable flag never check the server
 * again because the file will be saved permanently in their caches.
 *
 * \return true if the resource is considered immutable.
 */
bool cache_control_settings::get_immutable() const
{
    return f_immutable;
}


/** \brief Get the current value of the 'proxy-revalidate' flag.
 *
 * This function returns the current value of the 'proxy-revalidate'
 * flag.
 *
 * Note that 'must-revalidate' has priority and if specified, the
 * 'proxy-revalidate' is ignored since the proxy-cache should
 * honor the 'must-revalidate' anyway. However, this function
 * directly returns the value of the 'proxy-revalidate' flag.
 *
 * \note
 * This flag may appear in the server response.
 *
 * \return true if the 'proxy-revalidate' flag is to be specified in the
 *         Cache-Control field.
 *
 * \sa set_proxy_revalidate()
 */
bool cache_control_settings::get_proxy_revalidate() const
{
    return f_proxy_revalidate;
}


/** \brief Set the 'public' flag to true or false.
 *
 * Any page that is public, and thus can be saved in a public shared
 * cache (i.e. proxy caches).
 *
 * Snap! detects whether a page is accessible by a visitor, if so
 * it sets the public flag automatically. So you should not have
 * to set this flag unless somehow your page is public and the Snap!
 * test could fail or you know that your pages are always public
 * and thus you could avoid having to check the permissions.
 *
 * Note that if the 'private' flag is set to true, then the 'public'
 * flag is ignored. Further, if the 'no-cache' or 'no-store' flags
 * are set, then 'public' and 'private' are ignored.
 *
 * \note
 * This flag may appear in the server response.
 *
 * \param[in] public_cache  Whether proxy caches can cache this content.
 *
 * \sa get_public()
 */
void cache_control_settings::set_public(bool const public_cache)
{
    f_public = public_cache;
}


/** \brief Get the current value of the 'public' flag.
 *
 * This function returns the current value of the 'public'
 * flag.
 *
 * Note that 'private' has priority over 'public'. So if 'private'
 * is true, 'public' is ignored. Similarly, the 'no-cache' and
 * 'no-store' have priority over the 'private' flag. However, this
 * function directly returns the 'public' flag.
 *
 * \note
 * This flag may appear in the server response.
 *
 * \return true if the 'public' flag is to be specified in the
 *         Cache-Control field.
 *
 * \sa set_public()
 */
bool cache_control_settings::get_public() const
{
    return f_public;
}


/** \brief Add a field name that needs to get revalidated by proxy caches.
 *
 * The Cache-Control header can include a no-cache parameter that includes
 * a list of field names that should not be cached and be revalidated
 * whenever a new client request is received.
 *
 * Note that means there are three possibilities with the no-cache parameters:
 *
 * \li the parameter is not present, caching may still not be allowed
 *     (i.e. max-age=0)
 * \li the parameter is present on its own, absolutely no caching is possible
 * \li the parameter is set to a list of field names, in which case caching
 *     is allowed, but the specified fields must be revalidated from the
 *     server (and should probably not be saved in the cache)
 *
 * \note
 * If the string is empty after left and right trimming the request to
 * add a field name is ignored.
 *
 * References:
 *
 * https://tools.ietf.org/html/rfc7234#section-5.2.2
 *
 * \param[in] field_name  The name of the HTTP header field which needs to
 *                        never be cached.
 *
 * \sa add_private_field_name()
 */
void cache_control_settings::add_revalidate_field_name(std::string const & field_name)
{
    // we just have to insert, it will be inserted just once
    //
    std::string name(boost::trim_copy(field_name));
    if(!name.empty())
    {
        f_revalidate_field_names.insert(name);
    }
}


/** \brief Add a field name that needs to never be cached.
 *
 * This function is an overload of the add_never_cache_field_name() with
 * an std::string parameter.
 *
 * \param[in] field_name  The name of the HTTP header field which needs to
 *                        never be cached.
 */
void cache_control_settings::add_revalidate_field_name(QString const & field_name)
{
    add_revalidate_field_name(std::string(field_name.toUtf8().data()));
}


/** \brief Add a field name that needs to never be cached.
 *
 * This function is an overload of the add_never_cache_field_name() with
 * an std::string parameter.
 *
 * \note
 * The function accepts a nullptr and ignores the request in that situation.
 *
 * \param[in] field_name  The name of the HTTP header field which needs to
 *                        never be cached.
 */
void cache_control_settings::add_revalidate_field_name(char const * field_name)
{
    if(field_name != nullptr)
    {
        add_revalidate_field_name(std::string(field_name));
    }
}


/** \brief Retrieve the existing list of field names to never cache.
 *
 * The Cache-Control field can include a no-cache="<list of field names>".
 * This is that list.
 *
 * By default this list is empty (Contrary to the private list which
 * includes the "Set-Cookie".)
 *
 * For certain pages that require the "Set-Cookie" or some other user
 * fields, such should be added to this list. That way the cache will
 * make sure to revalidate the page conditionally.
 *
 * Other fields that are specific to a user need to be added to this
 * list with one of the add_revalidate_field_name() functions.
 *
 * \return A list of field names.
 */
cache_control_settings::tags_t const & cache_control_settings::get_revalidate_field_names() const
{
    return f_revalidate_field_names;
}


/** \brief Add a field name that needs to remain private.
 *
 * The Cache-Control header can include a private parameter that includes
 * a list of field names that need to not be cached, except by private
 * caches (i.e. client browser).
 *
 * Note that means there are three possibilities with the private parameters:
 *
 * \li the parameter is not present, the data is considered private anyway
 * \li the parameter is present on its own, all data is considered private
 * \li the parameter is set to a list of field names, in which case caching
 *     is allowed, but the specified fields must be removed from the HTTP
 *     header before caching the header
 *
 * Note that this is different from the no-cache parameter which requires
 * a hit to the server to revalidate the header. In case of the private
 * parameter, no revalidation is required. We can simply send the cache
 * without the private fields (which with Snap! C++ is fine for pretty
 * much all our attachments.)
 *
 * \note
 * If the string is empty after left and right trimming the request to
 * add a field name is ignored.
 *
 * References:
 *
 * https://tools.ietf.org/html/rfc7234#section-5.2.2
 *
 * \param[in] field_name  The name of the HTTP header field which needs to
 *                        never be cached.
 *
 * \sa add_revalidate_field_name()
 */
void cache_control_settings::add_private_field_name(std::string const & field_name)
{
    // we just have to insert, it will be inserted just once
    //
    std::string name(boost::trim_copy(field_name));
    if(!name.empty())
    {
        f_private_field_names.insert(name);
    }
}


/** \brief Add a field name that needs to never be cached.
 *
 * This function is an overload of the add_private_field_name() with
 * an std::string parameter.
 *
 * \param[in] field_name  The name of the HTTP header field which needs to
 *                        never be cached.
 */
void cache_control_settings::add_private_field_name(QString const & field_name)
{
    add_private_field_name(std::string(field_name.toUtf8().data()));
}


/** \brief Add a field name that needs to never be cached.
 *
 * This function is an overload of the add_private_field_name() with
 * an std::string parameter.
 *
 * \note
 * The function accepts a nullptr and ignores the request in that situation.
 *
 * \param[in] field_name  The name of the HTTP header field which needs to
 *                        never be cached.
 */
void cache_control_settings::add_private_field_name(char const * field_name)
{
    if(field_name != nullptr)
    {
        add_private_field_name(std::string(field_name));
    }
}


/** \brief Retrieve the existing list of field names to never cache.
 *
 * The Cache-Control field can include a private="<list of field names>".
 * This is that list.
 *
 * By default, this list is set to "Set-Cookie" which is more than likely
 * a field that include many user specific data such as a session
 * identifier.
 *
 * Other fields that are specific to a user need to be added to this
 * list with one of the add_private_field_name() functions.
 *
 * \return A list of field names.
 */
cache_control_settings::tags_t const & cache_control_settings::get_private_field_names() const
{
    return f_private_field_names;
}


/** \brief Add a tag to the existing list of tags.
 *
 * This function adds the specified tag_name parameter to the list of
 * tags to send to the client. This is used by CDN systems to allow
 * for segregation of files to be cached.
 *
 * Note that a page which cache is turned off (i.e. Cache-Control has
 * the no-cache parameter set) does not get tagged. These tags will be
 * ignored in that situation.
 *
 * By default we use the "Cache-Tag" HTTP header as defined by CloudFlare.
 * The name can be changed in the snapserver.conf file. You may also
 * add multiple names. Drupal uses "X-Drupal-Cache-Tags". Akamai uses
 * "Edge-Cache-Tag".
 *
 * References:
 *
 * https://support.cloudflare.com/hc/en-us/articles/206596608-How-to-Purge-Cache-Using-Cache-Tags-Enterprise-only-
 *
 * https://www.drupal.org/docs/8/api/cache-api/cache-tags
 *
 * https://learn.akamai.com/en-us/webhelp/fast-purge/fast-purge/GUID-9AEF6978-697F-410C-A347-8155FDB535C8.html
 *
 * \param[in] tag_name  The name of the tag to add to the list.
 */
void cache_control_settings::add_tag(std::string const & tag_name)
{
    // we just have to insert, it will be inserted just once
    //
    f_tags.insert(tag_name);
}


/** \brief Add a tag to the existing list of tags.
 *
 * This function adds the specified tag_name parameter to the list
 * of tags to send to proxy caches.
 *
 * This is an overload version of the add_tag() with a std::string.
 *
 * \param[in] tag_name  The name of the tag to add to the list.
 */
void cache_control_settings::add_tag(QString const & tag_name)
{
    add_tag(std::string(tag_name.toUtf8().data()));
}


/** \brief Add a tag to the existing list of tags.
 *
 * This function adds the specified tag_name parameter to the list
 * of tags to send to proxy caches.
 *
 * This is an overload version of the add_tag() with a std::string.
 *
 * \param[in] tag_name  The name of the tag to add to the list.
 */
void cache_control_settings::add_tag(char const * tag_name)
{
    add_tag(std::string(tag_name));
}


/** \brief Retrieve the existing list of cache tags.
 *
 * Various CDN systems accept a cache tag. This function returns the
 * list of tags that will be used with that HTTP header.
 *
 * By default, this is expected to be empty and no tag will be added
 * anywhere.
 *
 * \return A list of tag names.
 */
cache_control_settings::tags_t const & cache_control_settings::get_tags() const
{
    return f_tags;
}


/** \brief Set the maximum number of seconds to cache this data.
 *
 * This function requests for the specified data to be cached
 * for that many seconds.
 *
 * By default the value of max-age is set to 0, meaning that the
 * data will not be cached.
 *
 * In order to create a cache on the client's side (and within proxies,)
 * the value can be set to a number of seconds between 1 and AGE_MAXIMUM.
 * Any value under 60 is probably not going to be very useful. Any
 * value larger than AGE_MAXIMUM (which is one year, as per HTTP/1.1)
 * is clamped to AGE_MAXIMUM.
 *
 * You may also set \p max_age to -1 in order for the system to ignore
 * the 'max-age' cache control parameter.
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \param[in] max_age  The maximum age the data can have before checking
 *                     the data again.
 *
 * \sa update_max_age()
 * \sa get_max_age()
 */
void cache_control_settings::set_max_age(int64_t const max_age)
{
    if(max_age >= AGE_MAXIMUM)
    {
        // 1 year in seconds
        f_max_age = AGE_MAXIMUM;
    }
    else if(max_age < 0)
    {
        f_max_age = IGNORE_VALUE;
    }
    else
    {
        f_max_age = max_age;
    }
}


/** \brief Set the 'max-age' field value from a string.
 *
 * This function accepts a string as input to setup the 'max-age'
 * field.
 *
 * The value may be set to IGNORE_VALUE if the string does not represent
 * a valid decimal number (no signs allowed.) It is also clamped to a
 * maximum of AGE_MAXIMUM.
 *
 * \param[in] max_age  The maximum age as a string.
 *
 * \sa update_max_age()
 * \sa get_max_age()
 */
void cache_control_settings::set_max_age(QString const & max_age)
{
    // in this case -1 is what we want in case of an error
    // and not 1 year in seconds... no other negative values
    // are possible so we are fine
    f_max_age = string_to_seconds(max_age);
}


/** \brief Update the maximum number of seconds to cache this data.
 *
 * This function keeps the smaller 'max-age' of the existing setup and
 * the new value specified to this function.
 *
 * Note that if the current value is IGNORE_VALUE, then the new maximum
 * is automatically used, whatever it is.
 *
 * Negative values are ignored.
 *
 * \param[in] max_age  The maximum age the data can have before the client
 *                     is expected to check the data again.
 *
 * \sa set_max_age()
 * \sa get_max_age()
 */
void cache_control_settings::update_max_age(int64_t max_age)
{
    if(max_age > AGE_MAXIMUM)
    {
        // 1 year in seconds
        //
        max_age = AGE_MAXIMUM;
    }
    f_max_age = minimum(f_max_age, max_age);
}


/** \brief Retrieve the current max-age field.
 *
 * The Cache-Control can specify how long the data being returned
 * can be cached for. The 'max-age' field defines that duration
 * in seconds.
 *
 * By default the data is marked as 'do not cache' (i.e. max-age
 * is set to zero.)
 *
 * This function may return IGNORE_VALUE in which case the
 * 'max-age' field is ignored.
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \return The number of seconds to cache the data or IGNORE_VALUE.
 *
 * \sa update_max_age()
 * \sa get_max_age()
 */
int64_t cache_control_settings::get_max_age() const
{
    return f_max_age;
}


/** \brief Set the 'no-cache' flag to true or false.
 *
 * This function should only be called with 'true' to request
 * that the client and intermediate caches do not cache any
 * of the data. This does not prevent the client to store
 * the data.
 *
 * When the client sets this field to true, it means that we
 * should regenerate the specified page data.
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \param[in] no_cache  Whether the client and intermediate caches
 *                      must not cache the data.
 *
 * \sa get_no_cache()
 */
void cache_control_settings::set_no_cache(bool const no_cache)
{
    f_no_cache = no_cache;
}


/** \brief Retrieve the 'no-cache' flag.
 *
 * This function returns the current value of the 'no-cache' flag.
 *
 * The system ignores the 'public' and 'private' flags when the
 * 'no-cache' flag is true.
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \return true if the 'no-cache' flag is to be specified in the
 *         Cache-Control field.
 *
 * \sa set_no_cache()
 */
bool cache_control_settings::get_no_cache() const
{
    return f_no_cache;
}


/** \brief Set the 'no-store' flag to true or false.
 *
 * This function sets the 'no-store' flag to true or false. This
 * flag means that any of the data in that request needs to be
 * transferred only and not stored anywhere except in 100%
 * temporary buffers on the client's machine.
 *
 * Further, shared/proxy caches should clear all the data bufferized
 * to process this request as soon as it is done with it.
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \param[in] no_store  Whether the data can be stored or not.
 *
 * \sa get_no_store()
 */
void cache_control_settings::set_no_store(bool const no_store)
{
    f_no_store = no_store;
}


/** \brief Retrieve the 'no-store' flag.
 *
 * This function returns the current 'no-store' flag status.
 *
 * In most cases, this flag is not required. It should be true
 * only on pages that include extremely secure content such
 * as a page recording the settings of an electronic payment
 * (i.e. the e-payment Paypal page allows you to enter your
 * Paypal identifiers and those should not be stored anywhere.)
 *
 * Since most of our HTML pages are already marked as 'no-cache',
 * the 'no-store' is generally not required.
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \return true if the data in this request should not be stored
 *         anywhere.
 *
 * \sa set_no_store()
 */
bool cache_control_settings::get_no_store() const
{
    return f_no_store;
}


/** \brief Set whether the data can be transformed.
 *
 * The 'no-transform' flag can be used to make sure that caches do
 * not transform the data. This can also appear in the request from
 * the client in which case an exact original is required.
 *
 * This is generally important only for document files that may be
 * converted to a lousy format such as images that could be saved
 * as JPEG images although we enforce it when the client sends us
 * an AJAX request.
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \param[in] no_transform  Set to true to retrieve the original document as is.
 *
 * \sa get_no_transform()
 */
void cache_control_settings::set_no_transform(bool const no_transform)
{
    f_no_transform = no_transform;
}


/** \brief Retrieve whether the data can be transformed.
 *
 * Check whether the client or the server are requesting that the data
 * not be transformed. If true, then the original data should be
 * transferred as is.
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \return true if the 'no-transform' flag is set.
 *
 * \sa set_no_transform()
 */
bool cache_control_settings::get_no_transform() const
{
    return f_no_transform;
}


/** \brief Set the number of seconds to cache this data in shared caches.
 *
 * This function requests for the specified data to be cached
 * for that many seconds in any shared caches between the client
 * and the server. The client ignores that information.
 *
 * To use the maximum, call this function with AGE_MAXIMUM.
 *
 * To ignore this value, call this function with IGNORE_VALUE. This is
 * the default value for this field.
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \param[in] s_maxage  The maximum age the data can have before checking
 *                      the source again.
 *
 * \sa update_s_maxage()
 * \sa get_s_maxage()
 */
void cache_control_settings::set_s_maxage(int64_t const s_maxage)
{
    if(s_maxage >= AGE_MAXIMUM)
    {
        f_s_maxage = AGE_MAXIMUM;
    }
    else if(s_maxage < 0)
    {
        f_s_maxage = IGNORE_VALUE;
    }
    else
    {
        f_s_maxage = s_maxage;
    }
}


/** \brief Set the 's-maxage' field value from a string.
 *
 * This function accepts a string as input to setup the 's-maxage'
 * field.
 *
 * The value may be set to -1 if the string does not represent a valid
 * decimal number (no signs allowed.) It will be clamped to a maximum
 * of AGE_MAXIMUM.
 *
 * \param[in] s_maxage  The new maximum age defined as a string.
 *
 * \sa update_s_maxage()
 * \sa get_s_maxage()
 */
void cache_control_settings::set_s_maxage(QString const & s_maxage)
{
    // in this case -1 is what we want in case of an error
    // and not 1 year in seconds... no other negative values
    // are possible so we are fine
    f_s_maxage = string_to_seconds(s_maxage);
}


/** \brief Update the maximum number of seconds to cache this data on proxies.
 *
 * This function keeps the smaller maximum of the existing setup and
 * the new value specified to this function.
 *
 * Note that if the current value is IGNORE_VALUE, then the new maximum
 * is always used.
 *
 * Negative values are ignored.
 *
 * \param[in] s_maxage  The maximum age the data can have before checking
 *                      the data again from shared proxy cache.
 *
 * \sa set_s_maxage()
 * \sa get_s_maxage()
 */
void cache_control_settings::update_s_maxage(int64_t s_maxage)
{
    if(s_maxage >= AGE_MAXIMUM)
    {
        // 1 year in seconds
        //
        s_maxage = AGE_MAXIMUM;
    }
    f_s_maxage = minimum(f_s_maxage, s_maxage);
}


/** \brief Retrieve the current 's-maxage' field.
 *
 * The 'Cache-Control' HTTP header can specify how long the data
 * being returned can be cached for in a shared cache. The
 * 's-maxage' field defines that duration in seconds.
 *
 * By default shared caches are expected to use the 'max-age' parameter
 * when the 's-maxage' parameter is not defined. So if the value is
 * the same, you do not have to specified 's-maxage'.
 *
 * The value of 0 means that shared caches will not cache anything.
 *
 * \note
 * This field may appear in the client request or the server response.
 *
 * \return The number of seconds to cache the data or IGNORE_VALUE.
 *
 * \sa set_s_maxage()
 * \sa update_s_maxage()
 */
int64_t cache_control_settings::get_s_maxage() const
{
    return f_s_maxage;
}


/** \brief How long of a 'stale' is accepted by the client.
 *
 * The client may asks for data that is stale. Assuming that
 * a cache may keep data after it became stale, the client may
 * retrieve that data if they specified the stale parameter.
 *
 * A value of zero means that any stale data is acceptable.
 * A greater value specifies the number of seconds after the
 * normal cache threshold the data can be to be considered
 * okay to be returned to the client.
 *
 * In general, this is for cache systems and not the server
 * so our server generally ignores that data.
 *
 * You may use the AGE_MAXIMUM parameter to set the 'stale'
 * parameter to the maximum possible.
 *
 * You may set the 'stale' parameter to IGNORE_VALUE in which
 * case any possible user of the parameter has to ignore the
 * value (do as if it was not specified).
 *
 * \note
 * This flag may appear in the client request.
 *
 * \param[in] max_stale  The maximum number of seconds for the stale
 *                       data to be before the request is forwarded
 *                       to the server.
 *
 * \sa get_max_stale()
 */
void cache_control_settings::set_max_stale(int64_t const max_stale)
{
    if(max_stale >= AGE_MAXIMUM)
    {
        f_max_stale = AGE_MAXIMUM;
    }
    else if(max_stale < 0)
    {
        f_max_stale = IGNORE_VALUE;
    }
    else
    {
        f_max_stale = max_stale;
    }
}


/** \brief Set the 'max-stale' field value from a string.
 *
 * This function accepts a string as input to setup the 'max-stale'
 * field.
 *
 * The value may be set to IGNORE_VALUE if the string does not
 * represent a valid decimal number (no signs allowed.)
 *
 * The value is clamped to a maximum of AGE_MAXIMUM.
 *
 * \param[in] max_stale  The 'max-stale' parameter as a string.
 *
 * \sa get_max_stale()
 */
void cache_control_settings::set_max_stale(QString const & max_stale)
{
    set_max_stale(string_to_seconds(max_stale));
}


/** \brief Retrieve the current maximum 'stale' value.
 *
 * This function returns the maximum number of seconds the client is
 * willing to accept after the cache expiration date.
 *
 * So if your cache expires at 14:30:00 and the user makes a new
 * request on 14:32:50 with a 'max-stale' value of 3600, then you
 * may return the stale cache instead of regenerating it.
 *
 * Note that only shared caches are expected to return stale data.
 * The server itself currently always regenerate the data as
 * required.
 *
 * The stale value may be set to zero in which case the cache data
 * is always returned if available.
 *
 * \note
 * This flag may appear in the client request.
 *
 * \return The number of seconds the cache may be stale by before
 *         regenerating it.
 *
 * \sa set_max_stale()
 */
int64_t cache_control_settings::get_max_stale() const
{
    return f_max_stale;
}


/** \brief Set the number of seconds of freshness required by the client.
 *
 * The freshness is the amount of seconds before the cache goes stale.
 * So if the cache goes stale in 60 seconds and the freshness query
 * is 3600, then the cache is ignored.
 *
 * Note that freshness cannot always be satisfied since a page cache
 * duration ('max-age') may always be smaller than the specified
 * freshness amount.
 *
 * You may set this parameter to its maximum using the AGE_MAXIMUM
 * value.
 *
 * You may set this parameter to IGNORE_VALUE to mark it as being
 * ignored (not set by client.)
 *
 * \note
 * This flag may appear in the client request.
 *
 * \param[in] min_fresh  The minimum freshness in seconds.
 *
 * \sa get_min_fresh()
 */
void cache_control_settings::set_min_fresh(int64_t const min_fresh)
{
    if(min_fresh >= AGE_MAXIMUM)
    {
        f_min_fresh = AGE_MAXIMUM;
    }
    else if(min_fresh < 0)
    {
        f_min_fresh = IGNORE_VALUE;
    }
    else
    {
        f_min_fresh = min_fresh;
    }
}


/** \brief Set the 'min-fresh' field value from a string.
 *
 * This function accepts a string as input to setup the 'min-fresh'
 * field.
 *
 * The value may be set to INGORE_VALUE if the string does not represent
 * a valid decimal number (no signs allowed.) It will be clamped to
 * a maximum of AGE_MAXIMUM.
 *
 * \param[in] min_fresh  The minimum freshness required by the client.
 *
 * \sa get_min_fresh()
 */
void cache_control_settings::set_min_fresh(QString const & min_fresh)
{
    set_min_fresh(string_to_seconds(min_fresh));
}


/** \brief Retrieve the 'min-fresh' value from the Cache-Control.
 *
 * If the cache is to get stale within less than 'min-fresh' then
 * the server is expected to recalculate the page.
 *
 * Pages that are given a 'max-age' of less than what 'min-fresh'
 * is set at will react as fully dynamic pages (i.e. as if no
 * caches were available.)
 *
 * \note
 * This flag may appear in the client request.
 *
 * \return The number of seconds of freshness that are required.
 *
 * \sa set_min_fresh()
 */
int64_t cache_control_settings::get_min_fresh() const
{
    return f_min_fresh;
}


/** \brief Set the 'only-if-cached' flag.
 *
 * The 'only-if-cached' flag is used by clients with poor network
 * connectivity to request any available data from any cache instead
 * of gettings newer data.
 *
 * The server ignores that flag since the user connected to the server
 * it would not make sense to not return a valid response from that
 * point.
 *
 * \note
 * This flag may appear in the client request.
 *
 * \param[in] only_if_cached  The new flag value.
 *
 * \sa get_only_if_cached()
 */
void cache_control_settings::set_only_if_cached(bool const only_if_cached)
{
    f_only_if_cached = only_if_cached;
}


/** \brief Retrieve the 'only-if-cached' flag.
 *
 * This function returns the current status of the 'only-if-cached' flag.
 *
 * The server ignores this flag since it is generally used so a client
 * can request one of the in between caches to return any data they have
 * available, instead of trying to reconnect to the server. However,
 * the server may still check the flag and if true change the behavior.
 * Yet, that would mean the cache behavior would change for all clients.
 *
 * Note that caches still do not return stale data unless the client
 * also specifies the max-stale parameter.
 *
 * \code
 *      Cache-Control: max-stale=0,only-if-cached
 * \endcode
 *
 * \return Return true if the only-if-cached flag was specified in the
 *         request.
 *
 * \sa set_only_if_cached()
 */
bool cache_control_settings::get_only_if_cached() const
{
    return f_only_if_cached;
}


/** \brief Convert a string to a number.
 *
 * This function returns a number as defined in a string. The input
 * string must exclusively be composed of decimal digits.
 *
 * No plus or minus signs are allowed. If any character is not valid,
 * then the function returns IGNORE_VALUE.
 *
 * If the value is larger than AGE_MAXIMUM, it will be clamped at
 * AGE_MAXIMUM.
 *
 * \note
 * The function only accepts decimal numbers.
 *
 * \param[in] max_age  The maximum age decimal number defined as a string.
 *
 * \return The number of seconds the string represents
 *         or IGNORE_VALUE on errors.
 */
int64_t cache_control_settings::string_to_seconds(QString const & max_age)
{
    if(max_age.isEmpty())
    {
        // undefined / invalid (not 0)
        //
        return IGNORE_VALUE;
    }

    QByteArray const utf8(max_age.toUtf8());
    char const * s(utf8.data());
    char const * const start(s);
    int64_t result(0);
    for(; *s != '\0'; ++s)
    {
        if(*s < '0'
        || *s > '9'
        || s - start > 9) // too large! (for 1 year in seconds we need 8 digits maximum)
        {
            // invalid
            //
            return IGNORE_VALUE;
        }
        result = result * 10 + *s - '0';
    }

    return result > AGE_MAXIMUM ? AGE_MAXIMUM : result;
}


/** \brief Retrieve the smallest value of two.
 *
 * This special minimum function returns the smallest of two values,
 * only if one of those values is IGNORE_VALUE, then it is ignored
 * and the other is returned. Of course, if both are IGNORE_VALUE,
 * you get IGNORE_VALUE as a result.
 *
 * \note
 * This function is expected to be used with the 'max-age' and
 * 's-maxage' numbers. These numbers are expected to be defined
 * between 0 and AGE_MAXIMUM, or set to IGNORE_VALUE.
 *
 * \param[in] a  The left hand side value.
 * \param[in] b  The right hand side value.
 *
 * \return The smallest of a and b, ignoring either if set to IGNORE_VALUE.
 */
int64_t cache_control_settings::minimum(int64_t const a, int64_t const b)
{
    // if a or b are -1, then return the other
    //
    if(a == IGNORE_VALUE)
    {
        // in this case the value may return -1
        //
        return b;
    }
    if(b == IGNORE_VALUE)
    {
        return a;
    }

    // normal std::min() otherwise
    //
    return std::min(a, b);
}


} // namespace snap

// vim: ts=4 sw=4 et
