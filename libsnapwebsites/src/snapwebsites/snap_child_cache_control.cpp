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
#include "snapwebsites/snap_child.h"


// snapwebsites lib
//
#include "snapwebsites/log.h"
#include "snapwebsites/snapwebsites.h"


// snapdev lib
//
#include <snapdev/not_reached.h>


// Qt lib
//
#include <QLocale>


// boost lib
//
#include <boost/algorithm/string/join.hpp>


// last include
//
#include <snapdev/poison.h>



namespace snap
{


/** \brief Retrieve a reference to the Cache-Control data from the client.
 *
 * The client has the possibility to send the server a Cache-Control
 * field. This function can be used to retrieve a reference to that
 * data. It is somewhat complicated to convert all the fields of
 * the Cache-Control field so it is done by the snap_child object
 * in cache_control_settings objects.
 *
 * In most cases, plugins are only interested by 'max-stale' and
 * 'min-fresh' if they deal with the cache. The 'no-transform'
 * may be useful to download the original of a document, that
 * being said, I don't see how the user could tweak the browser
 * to do such a thing.
 *
 * \note
 * This returns a constant cache_control_settings object because there
 * is no reason for anyone to modify this value.
 *
 * \return A constant reference to the client cache control info.
 *
 * \sa server_cache_control()
 * \sa page_cache_control()
 */
cache_control_settings const & snap_child::client_cache_control() const
{
    return f_client_cache_control;
}


/** \brief Retrieve a reference to the Cache-Control data from the server.
 *
 * The server and all its plugins are expected to make changes to the
 * server cache control information obtained through this function.
 *
 * In most cases, functions should be called to switch the value from
 * the default to whatever value you can use in that field. That way
 * you do not override another's plugin settings. For fields that
 * include values, the smallest value should be kept. You can do so
 * using the update_...() functions.
 *
 * \todo
 * See: SNAP-650
 * Right now there is no real priority between the server and page
 * settings. I think the page should have priority, but that's
 * complicated to know what that means... unless we add a flag for
 * each field to know whether someone changed that field or not
 * (if changed in the page, use that field and totally ignore the
 * server field.) The server field is then checked and used no matter
 * what.
 *
 * See the attachment.cpp and path.cpp files (plugins) for use examples.
 *
 * \return A reference to the server cache control info.
 *
 * \sa page_cache_control()
 * \sa client_cache_control()
 */
cache_control_settings & snap_child::server_cache_control()
{
    return f_server_cache_control;
}


/** \brief Retrieve a reference to the Cache-Control data from the page.
 *
 * The page has the ability to have its own Cache-Control settings. This
 * function is used to retrieve a reference to that cache data and tweak it.
 *
 * It is rather complicated to properly handle all the Cache-Control fields
 * so it is done by the snap_child and cache_control_settings objects.
 *
 * This control settings are usually given priority over the server cache
 * control settings since they are specific to a given page.
 *
 * If you are programming a plugin that control caches server wide,
 * then you want to use the server_cache_control() instead.
 *
 * \todo
 * Right now there is no real priority between the server and page
 * settings. I think the page should have priority, but that's
 * complicated to know what that means... unless we add a flag for
 * each field to know whether someone changed that field or not
 * (if changed in the page, use that field and totally ignore the
 * server field.) The server field is then checked and used no matter
 * what.
 *
 * See the attachment.cpp and path.cpp files (plugins) for use examples.
 *
 * \return A reference to the page cache control info.
 *
 * \sa server_cache_control()
 * \sa client_cache_control()
 */
cache_control_settings & snap_child::page_cache_control()
{
    return f_page_cache_control;
}


/** \brief Check the current cache settings to know whether caching is turned on.
 *
 * By default caching is turned ON for the page and server, but the
 * client may request for caches to not be used.
 *
 * Also the page "content::cache_control" field may include parameters
 * that require caching to be turned on.
 *
 * Finally, the server caching parameters are set by various plugins
 * which may also turn on or off various caching features.
 *
 * This function checks whether the cache is to be turned on for this request.
 */
bool snap_child::no_caching() const
{
    // IMPORTANT NOTE: A 'max-age' value of 0 means 'do not cache', also
    //                 the value may be set to IGNORE_VALUE (-1).
    //
    // Note: The client may send us a "Cache-Control: no-cache" request,
    //       which means we do not want to return data from any cache,
    //       however, that does not mean we cannot send a cached reply!
    //
    return f_page_cache_control.get_no_cache()
        || f_page_cache_control.get_max_age() <= 0
        || f_server_cache_control.get_no_store()
        || f_server_cache_control.get_max_age() <= 0;
}


/** \brief Check the request ETag and eventually generate an HTTP 304 reply.
 *
 * First this function checks whether the ETag of the client request
 * is the same as what the server is about to send back to the client.
 * If the ETag values are not equal, then the function returns
 * immediately.
 *
 * When the ETag values are equal, this function kills the child process
 * after sending an HTTP 304 reply to the user and to the logger.
 *
 * The reply does not include any HTML because it is not allowed by the
 * specification (and there is no point since the client will reuse its
 * cache anyway.)
 *
 * \note
 * The header fields must include the following if they were
 * there with the 200 reply:
 *
 * \li Cache-Control
 * \li Content-Location
 * \li Date
 * \li ETag
 * \li Expires
 * \li Vary
 *
 * \warning
 * This function does not return when the 304 reply is sent.
 *
 * \see
 * https://tools.ietf.org/html/rfc7232#section-4.1
 */
void snap_child::not_modified()
{
    try
    {
        // if the "Cache-Control" header was specified with "no-cache",
        // then we have to re-send the data no matter what
        //
        if(no_caching())
        {
            // never caching this data, never send the 304.
            //
            return;
        }
        if(f_page_cache_control.get_public()
        || f_server_cache_control.get_public())
        {
            // See SNAP-650
            //
            // let snap.cgi cache this one for us, right now we don't yet
            // have snap.cgi converted to a real proxy so we have to reply
            // with a full 200 OK response to make that cache work... once
            // we have a correct proxy, we will re-enable a 304 Not Modified
            // response even for snap.cgi which can then update its caches
            // accordingly (i.e. if snap.cgi does not have a cache, it can
            // send a request without conditionals and save the new response
            // and if it already has a file, it can include conditionals
            // that are defined from that file.)
            //
            return;
        }

        // if the data was cached, including an ETag parameter, we may
        // receive this request even though the browser has a version
        // cached but it asks the server whether it changed so we have
        // to return a 304;
        //
        // this has to be checked before the If-Modified-Since
        //
        QString const if_none_match(snapenv("HTTP_IF_NONE_MATCH"));
        if(!if_none_match.isEmpty()
        && if_none_match == get_header("ETag"))
        {
            // this or die() was already called, forget it
            //
            if(f_died)
            {
                // avoid loops
                return;
            }
            f_died = true;

            // define a default error name if undefined
            //
            QString err_name;
            define_http_name(http_code_t::HTTP_CODE_NOT_MODIFIED, err_name);

            // log the fact we are sending a 304
            //
            SNAP_LOG_INFO("snap_child_cache_control.cpp:not_modified(): replying with HTTP 304 for ")(f_uri.path())(" (1)");

            if(f_is_being_initialized)
            {
                // send initialization process the info about the fact
                // (this should never occur, we may instead want to call die()?)
                //
                trace(QString("Error: not_modified() called: %1\n").arg(f_uri.path()));
                trace("#END\n");
            }
            else
            {
                // On error we do not return the HTTP protocol, only the Status field
                // it just needs to be first to make sure it works right
                set_header("Status",
                           QString("%1 %2\n")
                                .arg(static_cast<int>(http_code_t::HTTP_CODE_NOT_MODIFIED))
                                .arg(err_name),
                           HEADER_MODE_EVERYWHERE);

                // Remove the Content-Type header, this is simpler than requiring
                // the correct content type information
                //
                set_header(get_name(name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER),
                           "",
                           HEADER_MODE_EVERYWHERE);

                // since we are going to exit without calling the attach_to_session()
                //
                server::pointer_t server( f_server.lock() );
                if(!server)
                {
                    throw snap_logic_exception("server pointer is nullptr");
                }
                server->attach_to_session();

                // in case there are any cookies, send them along too
                //
                output_headers(HEADER_MODE_NO_ERROR);

                // no data to output with 304 (it's forbidden)
            }

            // the cache worked as expected
            //
            exit(0);
        }

        // No "If-None-Match" header found, so check fo the next
        // possible modification check which is the "If-Modified-Since"
        //
        QString const if_modified_since(snapenv("HTTP_IF_MODIFIED_SINCE"));
        QString const last_modified_str(get_header("Last-Modified"));
        if(!if_modified_since.isEmpty()
        && !last_modified_str.isEmpty())
        {
            time_t const modified_since(string_to_date(if_modified_since));
            time_t const last_modified(string_to_date(last_modified_str));

            // TBD: should we use >= instead of == here?
            // (see in snapcgi/src/snap.cpp too)
            //
            if(modified_since == last_modified
            && modified_since != -1)
            {
                // this or die() was already called, forget it
                //
                if(f_died)
                {
                    // avoid loops
                    //
                    return;
                }
                f_died = true;

                // define a default error name if undefined
                //
                QString err_name;
                define_http_name(http_code_t::HTTP_CODE_NOT_MODIFIED, err_name);

                // log the fact we are sending a 304
                //
                SNAP_LOG_INFO("snap_child_cache_control.cpp:not_modified(): replying with HTTP 304 for ")(f_uri.path())(" (2)");

                if(f_is_being_initialized)
                {
                    // send initialization process the info about the fact
                    // (this should never occur, we may instead want to call die()?)
                    //
                    trace(QString("Error: not_modified() called: %1\n").arg(f_uri.path()));
                    trace("#END\n");
                }
                else
                {
                    // On error we do not return the HTTP protocol, only the Status field
                    // it just needs to be first to make sure it works right
                    //
                    set_header("Status",
                               QString("%1 %2\n")
                                    .arg(static_cast<int>(http_code_t::HTTP_CODE_NOT_MODIFIED))
                                    .arg(err_name),
                               HEADER_MODE_EVERYWHERE);

                    // Remove the Content-Type header, this is simpler than requiring
                    // the correct content type information
                    //
                    set_header(get_name(name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER),
                               "",
                               HEADER_MODE_EVERYWHERE);

                    // since we are going to exit without calling the attach_to_session()
                    //
                    server::pointer_t server( f_server.lock() );
                    if(!server)
                    {
                        throw snap_logic_exception("server pointer is nullptr");
                    }
                    server->attach_to_session();

                    // in case there are any cookies, send them along too
                    //
                    output_headers(HEADER_MODE_NO_ERROR);

                    // no data to output with 304 (it's forbidden)
                }

                // the cache worked as expected
                //
                exit(0);
            }
        }

        // No match from client, must return normally
        //
        return;
    }
    catch(...)
    {
        // ignore all errors because at this point we must die quickly.
        //
        SNAP_LOG_FATAL("snap_child_cache_control.cpp:not_modified(): try/catch caught an exception");
    }

    // exit with an error
    exit(1);
}



/** \brief Setup the headers in link with caching.
 *
 * This function takes the f_server_cache_control and f_page_cache_control
 * information and generates the corresponding HTTP headers. This funciton
 * is called just before we output the HTTP headers in the output buffer.
 *
 * The HTTP headers generated by this function are:
 *
 * \li Cache-Control
 * \li Pragma
 * \li Expires
 * \li Cache-Tag (see snapserver.conf and add_tags() for details)
 *
 * See the cache_control_settings class for details about all the possible
 * cache options.
 *
 * By default the cache controls are not modified meaning that the page is
 * marked as 'no-cache'. In other words, it won't be cached at all. The
 * Cache-Control field may also receive 'no-store' in that case.
 *
 * HTTP Cache-Control Reference:
 *
 * http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.9
 *
 * \note
 * This function gives us one single point where the Cache-Control field
 * (and equivalent HTTP/1.0) are set so it makes it a lot easier to make
 * sure that the fields are set appropriately in all cases.
 *
 * \note
 * The Cache-Tag field may be renamed in the snapserver.conf file. If no
 * cache tags were specified with the add_tag() function, then the field
 * doesn't get generated.
 */
void snap_child::set_cache_control()
{
    // the Cache-Control is composed of multiple sub-fields
    snap_string_list cache_control_fields;

    // if the client requested "no-cache" or "no-store" we return a
    // cache control header which bypasses all caches, very important!
    //
//SNAP_LOG_WARNING("no caching? ")(no_caching() ? "no-caching" : "CACHING!")
//    (" - ")(f_client_cache_control.get_no_cache() ? "client no-caching" : "client CACHING!")
//    (" - ")(f_client_cache_control.get_no_store() ? "client no-store" : "client STORE!")
//    (" - ")(f_page_cache_control.get_no_cache() ? "page no-cache" : "page CACHING!")
//    (" - ")(f_server_cache_control.get_no_store() ? "server no-store" : "server STORE!")
//    (" - ")(f_page_cache_control.get_max_age() <= 0 ? QString("page max-age=%1").arg(f_page_cache_control.get_max_age()) : "page NO-MAX-AGE!")
//    (" - ")(f_server_cache_control.get_max_age() <= 0 ? "server max-age" : "server NO-MAX-AGE!")
//;

    // make sure this data never gets transformed
    //
    // in our case, it can be very important for OAuth2 answers and
    // other similar data... although OAuth2 replies should not be
    // cached!
    //
    if(f_client_cache_control.get_no_transform()
    || f_page_cache_control.get_no_transform()
    || f_server_cache_control.get_no_transform())
    {
        cache_control_fields << "no-transform";
    }

    if(no_caching())
    {
        // using Pragma for older browsers, although from what I have read
        // it is probably never used by any live browser
        //
        set_header("Pragma", "no-cache", HEADER_MODE_EVERYWHERE);

        // use a date in the past so nothing gets cached
        //
        set_header("Expires", "Sat, 01 Jan 2000 00:00:00 GMT", HEADER_MODE_EVERYWHERE);

        // I put all the possible "do not cache anything" in this case
        //
        cache_control_fields << "no-cache";

        // put no-store only if specified somewhere (client, page, plugins)
        //
        if(f_client_cache_control.get_no_store()
        || f_page_cache_control.get_no_store()
        || f_server_cache_control.get_no_store())
        {
            cache_control_fields << "no-store";
        }

        // put must-revalidate if specified by page or plugins
        //
        if(f_page_cache_control.get_must_revalidate()
        || f_server_cache_control.get_must_revalidate())
        {
            cache_control_fields << "must-revalidate";
        }

        // this is to make sure IE understands that it is not to cache anything
        //
        cache_control_fields << "post-check=0"; // IE special background processing
        cache_control_fields << "pre-check=0";  // IE special "really too late" flag

        // non-cached data is also marked private since intermediate
        // shared proxy caches should not cache this data at all
        // (the specs says you should not have public or private when
        // specifying no-cache, but it looks like it works better in
        // some cases for some browsers; if they just ignore that entry
        // as expected, it will not hurt)
        //
        cache_control_fields << "private";
    }
    else
    {
        // HTTP/1.0 can only be sent public data; we use this
        // flag to know whether any Cache-Control settings are
        // contradictory and prevents us from asking anyone to
        // cache anything if using HTTP/1.0
        //
        bool public_data(true);

        // get the smallest max_age specified
        //
        // IMPORTANT: unless no_caching() fails, one of the get_max_age()
        //            function will not return 0 or -1
        //
        int64_t const max_age(f_page_cache_control.minimum(f_page_cache_control.get_max_age(), f_server_cache_control.get_max_age()));
        cache_control_fields << QString("max-age=%1").arg(max_age);

        // This is the default when only max-age is specified
        //cache_control_fields << QString("post-check=%1").arg(max_age);
        //cache_control_fields << QString("pre-check=%1").arg(max_age);

        // any 's-maxage' info?
        //
        // IMPORTANT NOTE: here max_age cannot be 0 or -1
        //
        int64_t const s_maxage(f_page_cache_control.minimum(f_page_cache_control.get_s_maxage(), f_server_cache_control.get_s_maxage()));
        if(s_maxage != cache_control_settings::IGNORE_VALUE
        && s_maxage < max_age)
        {
            // request for intermediate proxies to not cache data for more
            // than the specified value; we do not send this header if
            // larger than max_age since caches should respect 'max-age'
            // too so there would be no need to have a larger 's-maxage'
            //
            cache_control_fields << QString("s-maxage=%1").arg(s_maxage);
        }

        // although we specify max-age in case a browser doesn't understand
        // immutable, we want the immutable flag as well if true in the
        // page or server; this means the data never dies out (we set the
        // CSS and JS files to immutable because their version has to be
        // changed whenever you make changes to those files.)
        //
        if(f_page_cache_control.get_immutable()
        || f_server_cache_control.get_immutable())
        {
            cache_control_fields << "immutable";
        }

        // choose between public and private (or "neither"--the default is
        // private, really, but we leave that out if none was set to true)
        //
        // private has priority over public
        //
        if(f_page_cache_control.get_private()
        || f_server_cache_control.get_private())
        {
            cache_control_fields << "private";
            public_data = false;
        }
        else if(f_page_cache_control.get_public()
             || f_server_cache_control.get_public())
        {
            cache_control_fields << "public";

            // when the cache is made public, we may need to output
            // a no-cache and private fields with lists of field names
            //
            cache_control_settings::fields_t all_revalidate_fields(f_page_cache_control.get_revalidate_field_names());
            cache_control_settings::fields_t const server_revalidate_fields(f_server_cache_control.get_revalidate_field_names());
            all_revalidate_fields.insert(server_revalidate_fields.begin(), server_revalidate_fields.end());
            if(!all_revalidate_fields.empty())
            {
                // note that this value must be quoted in part because it
                // can include commas
                //
                bool add_comma(false);
                QString fields_to_revalidate("no-cache=\"");
                for(auto t : all_revalidate_fields)
                {
                    if(add_comma)
                    {
                        fields_to_revalidate += ',';
                    }
                    // field name must be quoted
                    //
                    fields_to_revalidate += QString::fromUtf8(t.c_str());
                    add_comma = true;
                }

                // although thelist may not be empty, each string within
                // the list may be empty and in that case we do not want
                // to have a "no-cache" field by itself
                //
                if(add_comma)
                {
                    // close the quotation mark
                    //
                    fields_to_revalidate += "\"";
                    cache_control_fields << fields_to_revalidate;
                }
            }

            cache_control_settings::fields_t all_private_fields(f_page_cache_control.get_private_field_names());
            cache_control_settings::fields_t const server_private_fields(f_server_cache_control.get_private_field_names());
            all_private_fields.insert(server_private_fields.begin(), server_private_fields.end());
            if(!all_private_fields.empty())
            {
                // note that this value must be quoted in part because it
                // can include commas
                //
                bool add_comma(false);
                QString fields_to_never_cache("private=\"");
                for(auto t : all_private_fields)
                {
                    if(add_comma)
                    {
                        fields_to_never_cache += ',';
                    }
                    // field name must be quoted
                    //
                    fields_to_never_cache += QString::fromUtf8(t.c_str());
                    add_comma = true;
                }
                // although thelist may not be empty, each string within
                // the list may be empty and in that case we do not want
                // to have a "no-cache" field by itself
                //
                if(add_comma)
                {
                    // close the quotation mark
                    //
                    fields_to_never_cache += "\"";
                    cache_control_fields << fields_to_never_cache;
                }
            }
        }
        else
        {
            // remember, the default is "private"
            //
            public_data = false;
        }

        // whether the client should always revalidate with the server
        // (which means we get a hit, so try not to use that option!)
        //
        if(f_page_cache_control.get_must_revalidate()
        || f_server_cache_control.get_must_revalidate())
        {
            cache_control_fields << "must-revalidate";
        }
        else if(f_page_cache_control.get_proxy_revalidate()
             || f_server_cache_control.get_proxy_revalidate())
        {
            // if we don't add must-revalidate, we may instead
            // add proxy-revalidate which asks the proxy cache
            // to always revalidate
            //
            cache_control_fields << "proxy-revalidate";
        }

        // so is the data considered public for HTTP/1.0?
        //
        if(public_data)
        {
            // make sure that the Pragma is not defined
            //
            set_header("Pragma", "", HEADER_MODE_EVERYWHERE);

            // use our start date (which is converted from micro-seconds
            // to seconds) plus the max_age value for Expires
            //
            // note that we force the date to appear in English as the
            // HTTP spec. tells us to do
            //
            QDateTime expires(QDateTime().toUTC());
            expires.setTime_t(f_start_date / 1000000 + max_age - 1);
            QLocale us_locale(QLocale::English, QLocale::UnitedStates);
            set_header("Expires", us_locale.toString(expires, "ddd, dd MMM yyyy hh:mm:ss' GMT'"), HEADER_MODE_EVERYWHERE);
        }
        else
        {
            // HTTP/1.0 will not understand the "private" properly so we
            // have to make sure no caching happens in this case
            // (we could check the protocol to make sure we have HTTP/1.0
            // but HTTP/1.1 is expected to ignore these two headers when
            // Cache-Control is defined)
            //
            set_header("Pragma", "no-cache", HEADER_MODE_EVERYWHERE);
            set_header("Expires", "Sat,  1 Jan 2000 00:00:00 GMT", HEADER_MODE_EVERYWHERE);
        }

        // check whether there are cache tags to add here
        //
        cache_control_settings::tags_t all_tags(f_page_cache_control.get_tags());
        cache_control_settings::tags_t const server_tags(f_server_cache_control.get_tags());
        all_tags.insert(server_tags.begin(), server_tags.end());
        if(!all_tags.empty())
        {
            // we have tags, let's see whether we have HTTP field names
            //
            QString const cache_tags_param(get_server_parameter("cache_tags"));
            if(!cache_tags_param.isEmpty())
            {
                QString cache_tags;
                for(auto t : all_tags)
                {
                    if(!t.empty())
                    {
                        if(!cache_tags.isEmpty())
                        {
                            cache_tags += ',';
                        }
                        cache_tags += QString::fromUtf8(t.c_str());
                    }
                }
                snap_string_list const cache_tag_names(cache_tags_param.split(","));
                for(auto name : cache_tag_names)
                {
                    QString const n(name.trimmed());
                    if(!n.isEmpty())
                    {
                        set_header(n, cache_tags);
                    }
                }
            }
        }
    }

    set_header("Cache-Control", cache_control_fields.join(","), HEADER_MODE_EVERYWHERE);
}


} // namespace snap
// vim: ts=4 sw=4 et
