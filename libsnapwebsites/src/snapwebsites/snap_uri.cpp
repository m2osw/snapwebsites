// Snap Websites Server -- URI canonicalization
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

#include "snapwebsites/snap_uri.h"

#include "snapwebsites/qstring_stream.h"
#include "snapwebsites/log.h"
#include "snapwebsites/not_used.h"

#include <QtSerialization/QSerializationComposite.h>
#include <QtSerialization/QSerializationFieldBasicTypes.h>
#include <QtSerialization/QSerializationFieldString.h>

#include <libtld/tld.h>
#include <netdb.h>
#include <QBuffer>

#include "snapwebsites/poison.h"




namespace snap
{

// needed to avoid some linkage problems when references to these are used
const domain_variable::domain_variable_type_t domain_variable::DOMAIN_VARIABLE_TYPE_STANDARD;
const domain_variable::domain_variable_type_t domain_variable::DOMAIN_VARIABLE_TYPE_WEBSITE;
const domain_variable::domain_variable_type_t domain_variable::DOMAIN_VARIABLE_TYPE_FLAG_WITH_DEFAULT;
const domain_variable::domain_variable_type_t domain_variable::DOMAIN_VARIABLE_TYPE_FLAG_NO_DEFAULT;

// needed to avoid some linkage problems when references to these are used
const website_variable::website_variable_type_t website_variable::WEBSITE_VARIABLE_TYPE_STANDARD;
const website_variable::website_variable_type_t website_variable::WEBSITE_VARIABLE_TYPE_WEBSITE;
const website_variable::website_variable_type_t website_variable::WEBSITE_VARIABLE_TYPE_FLAG_WITH_DEFAULT;
const website_variable::website_variable_type_t website_variable::WEBSITE_VARIABLE_TYPE_FLAG_NO_DEFAULT;


/** \brief This function intializes a default Snap URI object.
 *
 * Initialize a default Snap URI object.
 *
 * By default, the protocol is set to HTTP and everything else is set to
 * empty. This also means the original URI is set to empty (and stays that
 * way unless you later call set_uri() with a valid URI.)
 *
 * \sa set_uri()
 * \sa set_protocol()
 * \sa set_domain()
 * \sa set_path()
 * \sa set_option()
 * \sa set_query_string()
 * \sa set_anchor()
 */
snap_uri::snap_uri()
    //: f_original("") -- auto-init
    //, f_protocol("http") --auto-init
    //, f_username("") -- auto-init
    //, f_password("") -- auto-init
    //, f_port(80) -- auto-init
    //, f_domain("") -- auto-init
    //, f_top_level_domain("") -- auto-init
    //, f_sub_domains() -- auto-init
    //, f_path() -- auto-init
    //, f_options() -- auto-init
    //, f_query_strings() -- auto-init
    //, f_anchor("") -- auto-init
{
}

/** \brief Set the URI to the specified string.
 *
 * This function sets the URI to the specified string. The parsing
 * is the same as in the set_uri() function.
 *
 * \todo
 * Should this function throw if the URI is considered invalid?
 *
 * \param[in] uri  The URI to assign to this Snap URI object.
 *
 * \sa set_uri()
 */
snap_uri::snap_uri(QString const & uri)
{
    if(!set_uri(uri))
    {
        // TBD: should we throw if set_uri() returns false?
        SNAP_LOG_ERROR("URI \"")(uri)("\" is considered invalid.");
    }
}

/** \brief Replace the URI of this Snap URI object.
 *
 * This function replaces the current Snap URI object information
 * with the specified \p uri data.
 *
 * Before calling this function YOU must force a URI encoding if the
 * URI is not yet encoded.
 *
 * Anything wrong in the syntax and the function returns false. Wrong
 * means empty entries, invalid encoding sequence, etc.
 *
 * \param[in] uri  The new URI to replace all the current data of this Snap URI object.
 *
 * \return false if the URI could not be parsed (in which case nothing's changed in the object); true otherwise
 */
bool snap_uri::set_uri(QString const & uri)
{
    QChar const * u(uri.constData());

    // retrieve the protocol
    QChar const * s(u);
    while(!u->isNull() && u->unicode() != ':')
    {
        ++u;
    }
    if(u - s < 1 || u->isNull() || u[1].unicode() != '/' || u[2].unicode() != '/')
    {
        // protocol is not followed by :// or is an empty string
        return false;
    }
    QString uri_protocol(s, static_cast<int>(u - s));

    // skip the ://
    u += 3;

    // retrieve the sub-domains and domain parts
    // we may also discover a name, password, and port
    QChar const * colon1(nullptr);
    QChar const * colon2(nullptr);
    QChar const * at(nullptr);
    for(s = u; !u->isNull() && u->unicode() != '/'; ++u)
    {
        if(u->unicode() == ':')
        {
            if(colon1 == nullptr)
            {
                colon1 = u;
            }
            else
            {
                if(at != nullptr)
                {
                    if(colon2 != nullptr)
                    {
                        return false;
                    }
                    colon2 = u;
                }
                else
                {
                    return false;
                }
            }
        }
        if(u->unicode() == '@')
        {
            if(at != nullptr)
            {
                // we cannot have more than one @ character that wasn't escaped
                return false;
            }
            at = u;
        }
    }
    // without an at (@) colon1 indicates a port
    if(at == nullptr && colon1 != nullptr)
    {
        // colon2 is nullptr since otherwise we already returned with false
        colon2 = colon1;
        colon1 = nullptr;
    }

    QString username;
    QString password;
    QString full_domain_name;
    int port(protocol_to_port(uri_protocol));

    // retrieve the data
    if(colon1 != nullptr)
    {
        // if(at == nullptr) -- missing '@'? this is not possible since we just
        //                   turned colon1 to colon2 if no '@' was defined
        username.insert(0, s, static_cast<int>(colon1 - s));
        s = colon1 + 1;
    }
    if(at != nullptr)
    {
        password.insert(0, s, static_cast<int>(at - s));
        s = at + 1;
    }
    if(colon2 != nullptr)
    {
        full_domain_name.insert(0, s, static_cast<int>(colon2 - s));
        QChar const * p(colon2 + 1);
        if(p == u)
        {
            // empty port entries are considered invalid
            return false;
        }
        port = 0;  // Reset port.
        for(; p < u; ++p)
        {
            ushort d = p->unicode();
            if(d < '0' || d > '9')
            {
                // ports only accept digits
                return false;
            }
            port = port * 10 + d - '0';
            if(port > 65535)
            {
                // port overflow
                return false;
            }
        }
    }
    else
    {
        full_domain_name.insert(0, s, static_cast<int>(u - s));
    }

    // verify that there is a domain
    if(full_domain_name.isNull())
    {
        return false;
    }

    // force a username AND password or neither
    if(username.isNull() ^ password.isNull())
    {
        return false;
    }

    // break-up the domain in sub-domains, base domain, and TLD
    snap_string_list sub_domain_names;
    QString domain_name;
    QString tld;
    if(!process_domain(full_domain_name, sub_domain_names, domain_name, tld))
    {
        return false;
    }

    // now we are ready to parse further (i.e. path)
    snap_string_list uri_path;
    if(!u->isNull())
    {
        // skip the '/'
        ++u;
        for(s = u; !u->isNull() && u->unicode() != '?' && u->unicode() != '#'; ++u)
        {
            if(u->unicode() == '/')
            {
                if(s != u)
                {
                    // decode right here since we just separate one segment
                    uri_path << urldecode(QString(s, static_cast<int>(u - s)));
                }
                // skip the '/'
                s = u + 1;
            }
        }
        if(s != u)
        {
            // last segment when it does not end with '/'
            uri_path << urldecode(QString(s, static_cast<int>(u - s)));
        }
    }

    snap_uri_options_t query_strings;
    if(!u->isNull() && u->unicode() == '?')
    {
        // skip the '?' and then any (invalid?) introductory '&'
        do
        {
            ++u;
        }
        while(!u->isNull() && u->unicode() == '&');
        QChar const * e(nullptr);
        for(s = u;; ++u)
        {
            if(u->isNull() || u->unicode() == '&' || u->unicode() == '#')
            {
                if(e == nullptr)
                {
                    // special case when a parameter appears without value
                    // ...&name&...
                    e = u;
                }
                QString name(s, static_cast<int>(e - s));
                if(name.isEmpty())
                {
                    // this is a very special case!!!
                    // ...&=value&...
                    // so we use a "special" name, also even that name could be
                    // defined in the query string (with %2A=value)
                    name = "*";
                }

                // query strings are saved as options (name/value pairs)
                // although the value may not be defined at all (...&name&...)
                // query string names are case sensitive (as per 6.2.2.1 of RFC 3986)
                QString value;
                if(e != u)
                {
                    // note that we reach here if there is an equal sign,
                    // the value may still be empty (i.e. u - e - 1 == 0 is
                    // possible)
                    //
                    value = QString::fromRawData(e + 1, static_cast<int>(u - e - 1));
                }
                name = urldecode(name);
                if(query_strings.contains(name))
                {
                    // two parameters with the same name, refused
                    return false;
                }
                query_strings[name] = urldecode(value);

                // skip all the & and then reset s and e
                while(!u->isNull() && u->unicode() == '&')
                {
                    ++u;
                }
                if(u->isNull() || u->unicode() == '#')
                {
                    // reached the end of the query strings
                    break;
                }
                s = u;
                e = nullptr;
            }
            else if(e == nullptr && u->unicode() == '=')
            {
                e = u;
            }
        }
    }

    // finally check for an anchor
    // (note that browsers do not send us the anchor data, however, URIs
    // defined on the server side can very well include such.)
    //
    QString uri_anchor;
    if(!u->isNull() && u->unicode() == '#')
    {
        ++u;
        // we need to decode the string so we add the whole string here
        QString p(u);
        p = urldecode(p);
        if(!p.isEmpty() && p[0] == '!')
        {
            // what do we do here?!
            // it seems to me that we should not get those here, but that
            // could be from someone who wrote the URL in their document.
            // also, the ! should be encoded... (%21)
            u = p.constData();
            for(s = u; !u->isNull(); ++u)
            {
                if(u->unicode() == '/')
                {
                    // encode right here since we have separate strings
                    if(s != u)
                    {
                        uri_path << urldecode(QString(s, static_cast<int>(u - s)));
                    }
                    // skip the '/'
                    s = u + 1;
                }
            }
            if(s != u)
            {
                // last path that doesn't end with '/'
                uri_path << urldecode(QString(s, static_cast<int>(u - s)));
            }
        }
        else
        {
            uri_anchor = p;
        }
    }

    // the path may include some ".." which we want to eliminate
    // note that contrary to Unix we do not accept "/.." as an equivalent
    // to "/" and we do not verify that all the paths exist... (i.e.
    // if "c" does not exist under "/a/b" (folder /a/b/c), then it should
    // be an error to use "/a/b/c/.." since "/a/b/c" cannot be computed.)
    int max_path(uri_path.size());
    for(int i(0); i < max_path; ++i)
    {
        if(uri_path[i] == "..")
        {
            if(i == 0 || max_path < 2)
            {
                // the path starts with a ".." or has too many ".."
                return false;
            }
            uri_path.removeAt(i);
            uri_path.removeAt(--i);
            --i;
            max_path -= 2;
        }
    }

    // totally unchanged URI, but only if it is considered valid
    f_original = uri;

    // now decode all the entries that may be encoded
    f_protocol = uri_protocol;
    f_username = urldecode(username);
    f_password = urldecode(password);
    if(port != -1)
    {
        f_port = port;
    }
    f_domain = domain_name;
    f_top_level_domain = tld;
    f_sub_domains = sub_domain_names;
    f_path = uri_path;

    // options come from parsing the sub-domains, query strings and paths
    // and at this point we do not have that information...
    f_options.clear();

    f_query_strings = query_strings;
    f_anchor = uri_anchor;

    return true;
}


/** \brief Return the original URI used to define the Snap URI object.
 *
 * This function returns the original URI as defined when calling the
 * set_uri() or creating the Snap URI object with the snap_uri() constructor
 * accepting a string.
 *
 * Note that it is possible to use the snap_uri object without using the
 * set_uri() or a string in the constructor by calling the setters of
 * the different parts of a URI. This is actually how snap_child does it
 * because Apache does not give us one plane URI, instead we get pre
 * separated parts. Therefore the get_original_uri() is always empty when
 * called from that f_uri variable.
 *
 * Note that this URI may still include security issues, although if the
 * input was not considered valid (i.e. had a valid protocol, etc.) then
 * this function returns an empty string.
 *
 * \return A constant reference to the original Snap URI.
 */
QString const & snap_uri::get_original_uri() const
{
    return f_original;
}


/** \brief Return the current URI define in this Snap URI object.
 *
 * This function concatenate all the URI parts in a fully qualified URI
 * and returns the result.
 *
 * This function does NOT take the rules in account (since it does not
 * know anything about them.) So you may want to consider using the
 * snap_uri_rules::process_uri() function instead.
 *
 * \note
 * The returned URI is already encoded as required by HTTP and such.
 *
 * \param[in] use_hash_bang  When this flag is set to true the URI is returned
 * as a hash bang (i.e. domain/path becomes domain/#!path).
 *
 * \return The URI represented by this Snap URI object.
 */
QString snap_uri::get_uri(bool use_hash_bang) const
{
    QString uri(f_protocol);

    uri += "://";

    // username/password if defined
    if(!f_username.isEmpty())
    {
        uri += urlencode(f_username);
        if(!f_password.isEmpty())
        {
            uri += ":";
            uri += urlencode(f_password);
        }
        uri += "@";
    }

    // full domain
    // domains should rarely require encoding for special characters, however,
    // it often is for international domains that make use of UTF-8 characters
    // outside of the standard ASCII letters and those definitively require
    // URL encoding to work right.
    uri += urlencode(full_domain());
    if(f_port != protocol_to_port(f_protocol))
    {
        uri += QString(":%1").arg(f_port);
    }
    uri += "/";

    // path if no hash bang
    QString const p(path());
    if(!use_hash_bang && p.length() > 0)
    {
        // avoid a double slash if possible
        // XXX: should the path not have a starting slash?
        if(p[0] == '/')
        {
            uri += p.mid(1);
        }
        else
        {
            uri += p;
        }
    }

    // query string
    QString const q(query_string());
    if(!q.isEmpty())
    {
        uri += '?';
        uri += q;
    }

    // anchor
    if(!f_anchor.isEmpty())
    {
        if(use_hash_bang)
        {
            // hash bang and anchor are exclusive
            throw snap_uri_exception_exclusive_parameters("you cannot use the hash bang (#!) and an anchor (#) in the same URI");
        }
        uri += "#";
        uri += urlencode(f_anchor, "!/~");
    }

    // path when using the hash bang but only if not empty
    if(use_hash_bang && !p.isEmpty())
    {
        uri += "#!/";
        uri += p;
    }

    return uri;
}


/** \brief Retrieve the URI of the website.
 *
 * This function returns the URI of the website, without any path,
 * query string options, anchor. The port is included only if it
 * does not correspond to the protocol and the \p include_port flag
 * is set to true.
 *
 * \param[in] include_port  Whether the port should be included.
 *
 * \return The domain name with the protocol and optionally the port.
 */
QString snap_uri::get_website_uri(bool include_port) const
{
    QString result(f_protocol);

    result += "://";
    result += full_domain();

    // only include the port if the caller wants it and if it does not
    // match the default protocol port
    //
    if(include_port
    && protocol_to_port(f_protocol) != f_port)
    {
        result += QString(":%1").arg(f_port);
    }

    result += "/";

    return result;
}


/** \brief Retrieve a part by name.
 *
 * This function allows you to retrieve a part by name.
 *
 * The supported parts are:
 *
 * \li anchor -- The anchor
 * \li domain -- The domain name
 * \li full-domain -- The full domain: with sub-domains, domain, and TLD
 * \li option -- The option number \p part
 * \li option-count -- The number of options
 * \li original -- The original URI or ""
 * \li password -- The password
 * \li path -- The folder name number \p part
 * \li path-count -- the number of paths
 * \li protocol -- The protocol
 * \li query-string -- The query string number \p part
 * \li query-string-count -- The number of query strings
 * \li sub-domain -- The sub-domain name number \p part
 * \li sub-domain-count -- The number of sub-domains
 * \li tld or top-level-domain -- the top-level domain name
 * \li uri -- the full URI as you want it in an href="..." attribute
 * \li username -- The username
 *
 * \param[in] name  The named part to retrieve.
 * \param[in] part  The part number when required (i.e. sub-domains)
 *
 * \return The data representing this part as a string.
 */
QString snap_uri::get_part(QString const& name, int part) const
{
    if(name.isEmpty())
    {
        // should this be an error?
        return "";
    }
    switch(name[0].unicode())
    {
    case 'a':
        if(name == "anchor")
        {
            return f_anchor;
        }
        break;

    case 'd':
        if(name == "domain")
        {
            return f_domain;
        }
        break;

    case 'f':
        if(name == "full-domain")
        {
            return full_domain();
        }
        break;

    case 'o':
        if(name == "option")
        {
            if(part < 0 || part >= f_options.size())
            {
                throw snap_uri_exception_out_of_bounds(QString("option %1 does not exist (range is 0 to %2)").arg(part).arg(f_options.size()));
            }
            return (f_options.begin() + part).value();
        }
        if(name == "option-count")
        {
            QString count(QString("%1").arg(f_options.size()));
            return count;
        }
        if(name == "original")
        {
            return f_original;
        }
        break;

    case 'p':
        if(name == "password")
        {
            return f_password;
        }
        if(name == "path")
        {
            if(part < 0 || part >= f_path.size())
            {
                throw snap_uri_exception_out_of_bounds(QString("path %1 is not available (range 0 to %2)").arg(part).arg(f_path.size()));
            }
            return f_path[part];
        }
        if(name == "path-count")
        {
            QString count(QString("%1").arg(f_path.size()));
            return count;
        }
        if(name == "port")
        {
            QString port(QString("%1").arg(f_port));
            return port;
        }
        if(name == "protocol")
        {
            return f_protocol;
        }
        break;

    case 'q':
        if(name == "query-string")
        {
            if(part < 0 || part >= f_query_strings.size())
            {
                throw snap_uri_exception_out_of_bounds(QString("query-string %1 does not exist (range 0 to %2)").arg(part).arg(f_query_strings.size()));
            }
            return (f_query_strings.begin() + part).value();
        }
        if(name == "query-string-count")
        {
            QString const count(QString("%1").arg(f_query_strings.size()));
            return count;
        }
        break;

    case 's':
        if(name == "sub-domain")
        {
            if(part < 0 || part >= f_sub_domains.size())
            {
                throw snap_uri_exception_out_of_bounds(QString("sub-domain %1 does not exist (range 0 to %2)").arg(part).arg(f_sub_domains.size()));
            }
            return f_sub_domains[part];
        }
        if(name == "sub-domain-count")
        {
            QString const count(QString("%1").arg(f_sub_domains.size()));
            return count;
        }
        break;

    case 't':
        if(name == "tld" || name == "top-level-domain")
        {
            return f_top_level_domain;
        }
        break;

    case 'u':
        if(name == "uri")
        {
            return get_uri();
        }
        if(name == "username")
        {
            return f_username;
        }
        break;

    default:
        // no match for other characters
        break;

    }

    return "";
}


/** \brief Change the protocol.
 *
 * This function is called to set the protocol.
 *
 * The protocol is not checked since this can be used for any
 * URI, not just the HTTP and HTTPS protocols. The name is
 * expected to be all lowercase and lowercase letters [a-z].
 *
 * \param[in] uri_protocol  The name of the protocol.
 */
void snap_uri::set_protocol(QString const & uri_protocol)
{
    if(uri_protocol.isEmpty())
    {
        throw snap_uri_exception_invalid_parameter("the uri_protocol parameter cannot be an empty string");
    }
    f_protocol = uri_protocol;
}


/** \brief Retrieve a copy of the protocol.
 *
 * This value is the name that defines how messages are being
 * sent between the client and the server.
 *
 * The main interface only accepts "http" and "https", but the
 * snap_uri object accepts all protocols so one can write URIs
 * with protocols such as "ftp", "mail", and "gopher".
 *
 * \return A constant reference to the protocol of this URI.
 */
QString const& snap_uri::protocol() const
{
    return f_protocol;
}


/** \brief Process a domain name and break it up.
 *
 * This function processes a domain name and breaks it up in
 * the domain name, the sub-domains, and the TLD.
 *
 * \note
 * If the function returns false, then the out parameters may not
 * all be defined properly. None of them should be used in that
 * case anyway.
 *
 * \param[in] full_domain_name  The complete domain with sub-domains and TLD.
 * \param[out] sub_domain_names  An array of sub-domains, may be empty.
 * \param[out] domain_name  The domain by itself (no TLD and no sub-domain.)
 * \param[out] tld  The TLD part by itself.
 *
 * \return true if the function succeeds, false otherwise
 */
bool snap_uri::process_domain(QString const & full_domain_name,
            snap_string_list & sub_domain_names, QString & domain_name, QString & tld)
{
    // first we need to determine the TLD, we use the tld()
    // function from the libtld library for this purpose

    // (note that the URI is expected to be encoded so the UTF-8
    // encoding is the same as ASCII)
    QByteArray full_domain_utf8(full_domain_name.toUtf8());
    struct tld_info info;
    char const *fd(full_domain_utf8.data());
    tld_result r(::tld(fd, &info));
    if(r != TLD_RESULT_SUCCESS)
    {
        // (should we accept TLD_RESULT_INVALID URIs?)
        // the URI doesn't end with a known TLD
        return false;
    }

    // got the TLD, save it in the user's supplied variable
    tld = urldecode(info.f_tld);

    // search where the domain name starts
    char const *compute_domain_name(fd + info.f_offset);
    while(compute_domain_name > fd)
    {
        --compute_domain_name;
        if(*compute_domain_name == '.')
        {
            ++compute_domain_name;
            break;
        }
    }
    domain_name = urldecode(QString::fromUtf8(compute_domain_name, static_cast<int>(info.f_tld - compute_domain_name)));

    // now cut the remainder on each period, these are the sub-domains
    // there may be none if there are no other periods in the full name
    if(compute_domain_name > fd)
    {
        // forget the period
        --compute_domain_name;
    }
    QString all_sub_domains(QString::fromUtf8(fd, static_cast<int>(compute_domain_name - fd)));
    sub_domain_names = all_sub_domains.split('.');

    // verify that all the sub-domains are valid (i.e. no "..")
    if(!all_sub_domains.isEmpty())
    {
        int const max_sub_domain_names(sub_domain_names.size());
        for(int i(0); i < max_sub_domain_names; ++i)
        {
            if(sub_domain_names[i].isEmpty())
            {
                // sub-domains cannot be empty or the URI includes
                // two period one after another (this should actually
                // be caught by the tld() call.)
                return false;
            }
            sub_domain_names[i] = urldecode(sub_domain_names[i]);

            // TODO: look into whether we have to check for periods in the
            //       decoded sub-domain names (i.e. a %2E is probably not a
            //       valid character in a sub-domain name, at the same time
            //       if we reach here, there should not be such a DNS entry...
            //       but not automatically because a hacker can take an IP
            //       and use it with any URI and send an HTTP request that
            //       way... still, we would catch that in our domain/website
            //       canonicalization.) Maybe we should decode the domain part
            //       first, then parse it.
        }
    }

    return true;
}


/** \brief Set the domain to 'domain'.
 *
 * This function changes the Snap URI to the specified full domain.
 * This means changing the set of sub-domains, the TLD and the domain
 * it-self are updated with the corresponding data from the full domain.
 * The function takes care of breaking the input
 *
 * If any error is discovered in the full domain name, then the internal
 * variables do not get modified.
 *
 * Note that the domain is not expected to include a user name, password
 * and port information. You want to get rid of that information before
 * calling this function or consider calling set_uri() instead.
 *
 * \note
 * The only potential problem is when you get an out of memory error
 * while allocating a string.
 *
 * \todo
 * Check that the URL is not an IPv4 or IPv6 address. Such will always
 * fail and we should look into avoiding the use of an exception in
 * that circumstance.
 *
 * \exception snap_uri_exception_invalid_uri
 * If the domain cannot properly be broken up in sub-domains,
 * the doman name and the tld, then this exception is raised.
 *
 * \param[in] full_domain_name  A full domain name, without protocol, path,
 *                              query string or anchor.
 */
void snap_uri::set_domain(QString const & full_domain_name)
{
    snap_string_list sub_domain_names;
    QString domain_name;
    QString tld;
    if(!process_domain(full_domain_name, sub_domain_names, domain_name, tld))
    {
        throw snap_uri_exception_invalid_uri(QString("could not break up \"%1\" as a valid domain name").arg(full_domain_name));
    }

    f_domain = domain_name;
    f_top_level_domain = tld;
    f_sub_domains = sub_domain_names;
}


/** \brief Reconstruct the full domain from the broken down information
 *
 * This function rebuilds a full domain name from the broken down
 * data saved in the Snap URI: the sub-domains, the domain name,
 * and the TLD.
 *
 * \return The full domain name representation of this Snap URI.
 */
QString snap_uri::full_domain() const
{
    QString full_domains(f_sub_domains.join("."));
    if(!full_domains.isEmpty())
    {
        full_domains += '.';
    }
    full_domains += f_domain;
    full_domains += f_top_level_domain;
    return full_domains;
}

/** \brief Get the top level domain name.
 *
 * This function returns the top level domain name by itself.
 * For example, in "www.example.com", the top level domain name
 * is "com".
 *
 * \return The top level domain name of the Snap URI.
 */
QString const& snap_uri::top_level_domain() const
{
    return f_top_level_domain;
}


/** \brief Get the domain name by itself.
 *
 * This function returns the stripped down domain name. This name
 * has no period since it includes no sub-domains and no top level
 * domain names.
 *
 * \return The stripped down domain name.
 */
QString const& snap_uri::domain() const
{
    return f_domain;
}


/** \brief Return the concatenated list of sub-domains.
 *
 * This function returns the concatenated list of sub-domains
 * in one string.
 *
 * \return The concatenated sub-domains separated by periods.
 */
QString snap_uri::sub_domains() const
{
    return f_sub_domains.join(".");
}


/** \brief Return the number of sub-domains defined.
 *
 * This function defines a set of sub-domains.
 *
 * \return The number of sub-domains.
 */
int snap_uri::sub_domain_count() const
{
    return f_sub_domains.size();
}


/** \brief Return one of the sub-domain names.
 *
 * This function returns the specified domain name.
 *
 * \param[in] part  The sub-domain name index.
 *
 * \return The sub-domain corresponding to the specified index.
 */
QString snap_uri::sub_domain(int part) const
{
    if(part < 0 || part >= f_sub_domains.size())
    {
        throw snap_uri_exception_out_of_bounds(QString("sub-domain %1 does not exist (range 0 to %2)").arg(part).arg(f_sub_domains.size()));
    }
    return f_sub_domains[part];
}


/** \brief Return the array of sub-domains.
 *
 * This function gives you a constant reference to all the sub-domains
 * at once. You may use this function to make use of the list iterator,
 * for example.
 *
 * The strings are in order as in the first is the left-most sub-domain
 * (or the furthest away from the domain name.)
 *
 * \return A list of strings representing the sub-domains.
 */
snap_string_list const & snap_uri::sub_domains_list() const
{
    return f_sub_domains;
}


/** \brief Set the port to the specified string.
 *
 * This function changes the port of the URI from what it is now
 * to the specified value.
 *
 * The port value must be a positive number or zero.
 *
 * Negative values or other invalid numbers generate an error.
 *
 * You can retrieve the port number with the get_port() function.
 *
 * \exception snap_uri_exception_invalid_parameter
 * This function generates an exception if an invalid port is detected
 * (negative, larger than 65535, or characters other than 0-9).
 *
 * \param[in] port  The new port for this Snap URI object.
 */
void snap_uri::set_port(QString const & port)
{
    bool ok;
    int p = port.toInt(&ok);
    if(!ok || p < 0 || p > 65535)
    {
        throw snap_uri_exception_invalid_parameter(QString("\"%1\" is an invalid port number").arg(port));
    }
    f_port = p;
}


/** \brief Set the port to the specified string.
 *
 * This function changes the port of the URI from what it is now
 * to the specified value.
 *
 * The port value must be a positive number or zero.
 *
 * Negative values or invalid numbers generate an error.
 *
 * \exception snap_uri_exception_invalid_parameter
 * This function generates an exception if an invalid port is
 * detected (negative or characters other than 0-9).
 *
 * \param[in] port  The new port for this Snap URI object.
 */
void snap_uri::set_port(int port)
{
    if(port < 0 || port > 65535)
    {
        throw snap_uri_exception_invalid_parameter(QString("port \"%1\" is out of range (1 to 65535)").arg(port));
    }
    f_port = port;
}


/** \brief Retrieve the port number.
 *
 * This function returns the specific port used to access
 * the server. This parameter can be used as one of the
 * options used to select a specific website.
 *
 * \return The port as an integer.
 */
int snap_uri::get_port() const
{
    return f_port;
}


/** \brief Replace the current path.
 *
 * This function can be used to replace the entire path of
 * the URI by starting the new path with a slash (/something).
 * If the \p path parameter does not start with a slash, then
 * it is used as a relative path from the existing path.
 *
 * A path includes parts separated by one or more slashes (/).
 * The function removes parts that are just "." since these
 * mean "this directory" and they would not be valid in a
 * canonicalized path.
 *
 * A path may include one or more ".." as a path part. These
 * mean remove one part prior.
 *
 * The ".." are accepted in any path, however, it must be
 * correct in that it is not possible to use ".." without at
 * least one part just before that (i.e. "/this/one/../other/one" is
 * valid, but "/../that/one/is/not" since ".." from / does not
 * exist. This is not how Unix usually manages paths since
 * in Unix / and /.. are one and the same folder.)
 *
 * Note that if you wanted to make use of the hash bang feature,
 * you would still make use of this function to setup your path in
 * the Snap URI object. The hash bang feature determines how
 * the path is handled when you get the URI with get_uri().
 *
 * \exception snap_uri_exception_invalid_path
 * The function raises this exception if the path includes more
 * ".." than there are "normal" parts on the left side of the "..".
 *
 * \param[in] uri_path  The new path for this URI.
 *
 * \sa path()
 */
void snap_uri::set_path(QString uri_path)
{
    // check whether the path starts with a '/':
    // if so, then we replace the existing path;
    // if not, then we append uri_path to the existing path.
    //
    if((uri_path.isEmpty() || uri_path[0] != '/')
    && !f_path.empty())
    {
        // append unless the user passed a path starting with "/"
        // or the current path is empty
        uri_path = f_path.join("/") + "/" + uri_path;
    }

    // if the path starts with a '/' or includes a double '/'
    // within itself, it will be removed because of the SkipEmptyParts
    snap_string_list p(uri_path.split('/', QString::SkipEmptyParts));

    // next we remove all ".." (and the previous part); if ".." was
    // at the start of the path, then an exception is raised
    //
    int max_parts(p.size());
    for(int i(0); i < max_parts; ++i)
    {
        if(p[i] == ".")
        {
            // canonalization includes removing "." parts which are
            // viewed exactly as empty parts
            p.removeAt(i);
            --i;
            --max_parts;
        }
        else if(p[i] == "..")
        {
            // note: max should not be less than 2 if i != 0
            if(i == 0 || max_parts < 2)
            {
                throw snap_uri_exception_invalid_path(QString("path \"%1\" is not valid (it includes too many \"..\")").arg(uri_path));
            }
            p.removeAt(i);
            p.removeAt(--i);
            --i;
            max_parts -= 2;
        }
    }

    // the input was valid, save the new result
    f_path.swap(p);
}


/** \brief Return the full path.
 *
 * This function returns the full concatenated path of the URI.
 *
 * The function encodes the path appropriately. The path can thus be
 * used anywhere an encoded path is accepted. The encoding can be
 * avoided by setting the \p encoded flag to false.
 *
 * Note that a non encoded path may include / characters instead of
 * the %2F encoded character and thus not match the internal path.
 *
 * \note
 * The URL encode will not encode the ~ character which is at times
 * used for user references (~username/...).
 *
 * \warning
 * The result of the function returns what looks like a relative path.
 * This is useful since in many cases you need to remove the starting
 * slash, so we avoid adding it in the first place. If there is no path,
 * the function returns the empty string ("").
 *
 * \param[in] encoded  Should the resulting path be URL encoded already?
 * By default the path is URL encoded as expected by the HTTP protocol.
 *
 * \return The full path of the URI.
 */
QString snap_uri::path(bool encoded) const
{
    if(encoded)
    {
        QString output;
        bool first(true);
        for(snap_string_list::const_iterator it(f_path.begin());
                    it != f_path.end(); ++it)
        {
            if(first)
            {
                first = false;
            }
            else
            {
                output += '/';
            }
            output += urlencode(*it, "~");
        }
        return output;
    }
    return f_path.join("/");
}


/** \brief Retrieve the number of folder names defined in the path.
 *
 * This function returns the number of folder names defined in the
 * path. Each name can be retrieved with the path_folder() function.
 *
 * The function may return 0 if no folder name is available.
 *
 * \return The number of folder names available.
 *
 * \sa path_folder()
 */
int snap_uri::path_count() const
{
    return f_path.size();
}


/** \brief Get a folder name from the path.
 *
 * This function is used to retrieve the name of a specific folder.
 * This is useful when you make use of a folder name as a dynamic
 * name. For example with a path such as "journal/george",
 * path_folder_name(1); returns "george" which may be the name of
 * the journal owner.
 *
 * When you use this function to retrieve dynamic entries, it is
 * assumed that you do it after the path options were removed so a
 * path such as "en/journal/george" would be changed to
 * "journal/george" and path_folder_name(1); would still return
 * "george".
 *
 * \exception snap_uri_exception_out_of_bounds
 * This function raises this exception if the \p part parameter is
 * outside the range of folder names available. \p part should be
 * between 0 and path_count() - 1. If the path is empty, then this
 * function cannot be called.
 *
 * \param[in] part  The index of the folder to retrieve.
 *
 * \return The folder name.
 *
 * \sa path_count();
 */
QString snap_uri::path_folder_name(int part) const
{
    if(part < 0 || part >= f_path.size())
    {
        throw snap_uri_exception_out_of_bounds(QString("no path section %1 available (range 0 to %2)").arg(part).arg(f_path.size()));
    }
    return f_path[part];
}


/** \brief The array of folder names.
 *
 * This function returns a reference to the array used to hold the
 * folder names forming the URI path.
 *
 * \return A constant reference to the list of string forming the path.
 */
snap_string_list const & snap_uri::path_list() const
{
    return f_path;
}


/** \brief Set an option.
 *
 * This function is used to define the value of an option in a URI.
 * Remember that options only work for URIs that are clearly marked
 * as from this website.
 *
 * Setting the value to an empty string has the effect of deleting
 * the given option. You may also call the unset_option() function.
 *
 * \param[in] name  The name of the option to set.
 * \param[in] value  The new value for this option.
 *
 * \sa option();
 * \sa unset_option();
 */
void snap_uri::set_option(QString const& name, QString const& value)
{
    if(value.isEmpty())
    {
        f_options.remove(name);
    }
    else
    {
        f_options[name] = value;
    }
}

/** \brief Remove the specified option.
 *
 * This function is used to remove (delete) an option from the list
 * of options. For example, going to a page where the language is
 * neutral, you probably want to remove the language option.
 *
 * \param[in] name  The name of the option to remove.
 *
 * \sa set_option();
 */
void snap_uri::unset_option(QString const& name)
{
    f_options.remove(name);
}


/** \brief Retrieve the value of the named option.
 *
 * This function retrieves the current value of the named option.
 *
 * If the option is not defined, then the function returns an empty
 * string. The empty string always represents an undefined option.
 *
 * \param[in] name  The name of the option to retrieve.
 *
 * \return The value of the named option.
 *
 * \sa set_option();
 */
QString snap_uri::option(QString const& name) const
{
    if(f_options.contains(name))
    {
        return f_options[name];
    }
    return "";
}


/** \brief Retrieve the number of currently defined options.
 *
 * This function returns the number of options that can be retrieved
 * with the option() function using an index. If the function returns
 * zero, then no options are defined.
 *
 * \return The number of options defined in this URI.
 */
int snap_uri::option_count() const
{
    return f_options.size();
}


/** \brief Retrieve an option by index.
 *
 * This function allows you to retrieve the name and value of an option
 * using its index. The index (\p part) must be a number between 0 and
 * option_count() - 1.
 *
 * \param[in] part  The index of the option to retrieve.
 * \param[out] name  The name of the option being retrieved.
 *
 * \return The value of the option being retrieved.
 *
 * \sa option();
 * \sa option_count();
 */
QString snap_uri::option(int part, QString& name) const
{
    if(part < 0 || part >= f_options.size())
    {
        throw snap_uri_exception_out_of_bounds(QString("no option %1 available (range 0 to %2)").arg(part).arg(f_options.size()));
    }
    auto it(f_options.begin() + part);
    name = it.key();
    return it.value();
}


/** \brief Retrieve the map of options.
 *
 * This function returns the map of options so one can use the begin()
 * and end() functions to go through the entire list without having to
 * use the option() function.
 *
 * \return A constant reference to the map of options.
 *
 * \sa option();
 */
snap_uri::snap_uri_options_t const& snap_uri::options_list() const
{
    return f_options;
}


/** \brief Set a query string option.
 *
 * This function is used to change the named query string with the
 * specified value.
 *
 * A query string option with an empty string as a value is considered
 * undefined and is not shown on the final URI. So setting an option to
 * the empty string ("") is equivalent to unset_query_option().
 *
 * \param[in] name  The name of the query string option.
 * \param[in] value  The value of the query string option.
 */
void snap_uri::set_query_option(QString const& name, QString const& value)
{
    if(name.isEmpty())
    {
        // this happens if the name was not defined in the configuration file
        return;
    }

    // TODO: see whether we currently use this feature, because it is rather
    //       incorrect, it is possible to have an empty value in a query
    //       string (i.e. "...?logout")
    //
    //       we should use unset_query_option() instead
    //
    if(value.isEmpty())
    {
        f_query_strings.remove(name);
    }
    else
    {
        f_query_strings[name] = value;
    }
}


/** \brief Unset the named query string option.
 *
 * This function ensures that the named query string option is deleted
 * and thus will not appear in the URI.
 *
 * \param[in] name  The name of the option to delete.
 */
void snap_uri::unset_query_option(QString const& name)
{
    if(name.isEmpty())
    {
        // this happens if the name was not defined in the configuration file
        return;
    }

    f_query_strings.remove(name);
}


/** \brief Set the query string.
 *
 * This function can be used to reset the query string to the
 * parameters defined in this URI query string.
 *
 * The function does not clear all the existing query strings,
 * it only replaces existing entries. This means also means that
 * it does not detect whether the input includes the same option
 * more than once and only the last one sticks.
 *
 * The query string variable names and data gets URL decoded.
 *
 * \warning
 * This function does not clear the existing list of query
 * string options.
 *
 * \param[in] uri_query_string  The query string to add to the existing data.
 */
void snap_uri::set_query_string(QString const & uri_query_string)
{
    snap_string_list const value_pairs(uri_query_string.split('&', QString::SkipEmptyParts));
    for(snap_string_list::const_iterator it(value_pairs.begin());
                            it != value_pairs.end();
                            ++it)
    {
        int const pos(it->indexOf('='));
        if(pos == -1)
        {
            // no value
            f_query_strings[urldecode(*it)] = QString();
        }
        else if(pos == 0)
        {
            // name is missing, use "*" instead
            f_query_strings["*"] = urldecode(*it);
        }
        else
        {
            f_query_strings[urldecode(it->mid(0, pos))] = urldecode(it->mid(pos + 1));
        }
    }
}


/** \brief Clear all query option strings.
 *
 * This is useful if you want to "start fresh" with the base URI.
 */
void snap_uri::clear_query_options()
{
    f_query_strings.clear();
}


/** \brief Generate the query string.
 *
 * This function goes through the list of defined query string options
 * and builds the resulting query string to generate the final URI.
 *
 * The result is already URL ecoded since you would otherwise not know
 * where/which equal and ampersand are legal.
 *
 * \return The URI query string.
 */
QString snap_uri::query_string() const
{
    QString result;
    for(snap_uri_options_t::const_iterator it(f_query_strings.begin());
                                            it != f_query_strings.end();
                                            ++it)
    {
        if(!result.isEmpty())
        {
            result += '&';
        }
        result += urlencode(it.key());
        if(!it.value().isEmpty())
        {
            // add the value only if not empty
            result += "=";
            // we now support commas in URIs because... well... it is
            // common and it won't break anything
            //
            result += urlencode(it.value(), ",");
        }
    }
    return result;
}


/** \brief Retrieve whether a query option is defined.
 *
 * This function returns true if a query option is defined. Note that
 * an option may be the empty string ("") and that cannot be distinguish
 * from the empty string ("") returned when the query_option() function
 * is used against an undefined option.
 *
 * \param[in] name  The name of the option to query.
 *
 * \return true when the has_query_option() is defined.
 *
 * \sa query_option();
 */
bool snap_uri::has_query_option(const QString& name) const
{
    if(name.isEmpty())
    {
        // this happens if the name was not defined in the configuration file
        return false;
    }

    return f_query_strings.contains(name);
}

/** \brief Retrieve a query string option.
 *
 * This function can be used to retrieve the current value of a query
 * string option.
 *
 * Note that you cannot know whether an option is defined using this
 * function since the function returns an empty string whether it is
 * empty or undefined. Instead, use the has_query_option() function
 * to determine whether an option is defined.
 *
 * \param[in] name  Name of the query string option to return.
 *
 * \sa has_query_option();
 */
QString snap_uri::query_option(const QString & name) const
{
    if(name.isEmpty())
    {
        // this happens if the name was not defined in the configuration file
        return "";
    }

    if(f_query_strings.contains(name))
    {
        return f_query_strings[name];
    }
    return "";
}

/** \brief Return the number of options are defined in the query string.
 *
 * This function returns the number of options currently defined in the
 * query string. This is useful to go over the list of options with the
 * query_option(int part, QString& name) function.
 *
 * \return The number of query string options currently defined.
 */
int snap_uri::query_option_count() const
{
    return f_query_strings.size();
}

/** \brief Retrieve an option specifying its index.
 *
 * This function returns the name and value of the option defined at
 * index \p part.
 *
 * The index must be between 0 and the number of options available minus
 * 1 (i.e. query_options_count() - 1).
 *
 * \param[in] part  The index of the query string option to retrieve.
 * \param[out] name  The name of the option at that index.
 *
 * \return The value of the option at that index.
 *
 * \sa query_option_count();
 */
QString snap_uri::query_option(int part, QString& name) const
{
    if(part < 0 || part >= f_query_strings.size())
    {
        throw snap_uri_exception_out_of_bounds(QString("query-option %1 does not exist (range 0 to %2)").arg(part).arg(f_query_strings.size()));
    }
    auto it(f_query_strings.begin() + part);
    name = it.key();
    return it.value();
}

/** \brief Return the complete map of query strings.
 *
 * This function returns a reference to the internal map of query strings.
 * This is useful to use the begin()/end() and other functions to go through
 * the map.
 *
 * \return A constant reference to the internal query string map.
 */
const snap_uri::snap_uri_options_t& snap_uri::query_string_list() const
{
    return f_query_strings;
}

/** \brief Define the anchor for this URI.
 *
 * This function is used to setup the anchor used in this URI.
 *
 * An anchor can be defined only if you don't plan to make use of
 * the hash bang feature (see get_uri() for more info) since both
 * features make use of the same technical option.
 *
 * The \p anchor parameter cannot include a '#' character.
 *
 * \note
 * The anchor string can start with a bang (!) since it is legal
 * in an anchor. If you are not using the hash bang feature, it
 * is fine, although it may confuse some search engines.
 *
 * \param[in] uri_anchor  The new value for the anchor.
 *
 * \sa get_uri()
 */
void snap_uri::set_anchor(const QString& uri_anchor)
{
    if(uri_anchor.indexOf('#') != -1)
    {
        throw snap_uri_exception_invalid_parameter(QString("anchor string \"%1\" cannot include a '#' character").arg(uri_anchor));
    }
    f_anchor = uri_anchor;
}

/** \brief Retrieve the current anchor.
 *
 * This function returns a copy of the current anchor. The empty string
 * represents the fact that the anchor is not defined.
 *
 * \return A constant reference to the anchor.
 */
const QString& snap_uri::anchor() const
{
    return f_anchor;
}

/** \brief Compare two URIs against each other.
 *
 * This function compares two URIs and returns true if they are
 * equal. The URIs are tested using what the get_uri() function
 * generates which means not 100% of the information included
 * in the Snap URI object.
 *
 * \param[in] rhs  The right handside to compare this against.
 *
 * \return true when both URIs are equal.
 */
bool snap_uri::operator == (const snap_uri& rhs) const
{
    return get_uri() == rhs.get_uri();
}

/** \brief Compare two URIs against each other.
 *
 * This function compares two URIs and returns true if they are
 * not equal. The URIs are tested using what the get_uri() function
 * generates which means not 100% of the information included
 * in the Snap URI object.
 *
 * \param[in] rhs  The right handside to compare this against.
 *
 * \return true when both URIs differ.
 */
bool snap_uri::operator != (const snap_uri& rhs) const
{
    return !operator == (rhs);
}

/** \brief Compare two URIs against each other.
 *
 * This function compares two URIs and returns true if this is
 * smaller than the \p rhs parameter. The URIs are tested using
 * what the get_uri() function generates which means not 100% of
 * the information included in the Snap URI object.
 *
 * \param[in] rhs  The right handside to compare this against.
 *
 * \return true when this is smaller than rhs.
 */
bool snap_uri::operator < (const snap_uri& rhs) const
{
    return get_uri() < rhs.get_uri();
}

/** \brief Compare two URIs against each other.
 *
 * This function compares two URIs and returns true if this is
 * smaller or equal to \p rhs. The URIs are tested using
 * what the get_uri() function generates which means not 100% of
 * the information included in the Snap URI object.
 *
 * \param[in] rhs  The right handside to compare this against.
 *
 * \return true when this is smaller or equal to rhs.
 */
bool snap_uri::operator <= (const snap_uri& rhs) const
{
    return get_uri() <= rhs.get_uri();
}


/** \brief Compare two URIs against each other.
 *
 * This function compares two URIs and returns true if this is
 * larger than the \p rhs parameter. The URIs are tested using
 * what the get_uri() function generates which means not 100% of
 * the information included in the Snap URI object.
 *
 * \param[in] rhs  The right handside to compare this against.
 *
 * \return true when this is larger than rhs.
 */
bool snap_uri::operator > (const snap_uri& rhs) const
{
    return !operator <= (rhs);
}


/** \brief Compare two URIs against each other.
 *
 * This function compares two URIs and returns true if this is
 * larger or equal to \p rhs. The URIs are tested using
 * what the get_uri() function generates which means not 100% of
 * the information included in the Snap URI object.
 *
 * \param[in] rhs  The right handside to compare this against.
 *
 * \return true when this is larger or equal to rhs.
 */
bool snap_uri::operator >= (const snap_uri& rhs) const
{
    return !operator < (rhs);
}


/** \brief Encode a URI so it is valid for HTTP.
 *
 * This function encodes all the characters that need to be encoded
 * for a URI to be valid for the HTTP protocol.
 *
 * WARNING: This encodes the entire string. Remember that the string
 * cannot include characters such as :, /, @, ?, =, &, #, ~ which at
 * times appear in fully qualified URIs. Instead, it must be built
 * piece by piece.
 *
 * Note that we do not encode underscores.
 *
 * The \p accepted parameter can be used to avoid converting certain
 * characters (such as / in an anchor and ~ in a path).
 *
 * \param[in] uri  The URI to encode.
 * \param[in] accepted  Extra characters accepted and not encoded. This
 * parameter cannot be set to nullptr. Use "" instead if no extra characters
 * are accepted.
 *
 * \return The encoded URI, it may be equal to the input.
 */
QString snap_uri::urlencode(QString const & uri, char const * accepted)
{
    QString encoded;

    QByteArray utf8(uri.toUtf8());
    for(const char *u(utf8.data()); *u != '\0'; ++u)
    {
        if((*u >= 'A' && *u <= 'Z')
        || (*u >= 'a' && *u <= 'z')
        || (*u >= '0' && *u <= '9')
        || *u == '.' || *u == '-' || *u == '_'
        || strchr(accepted, *u) != nullptr)
        {
            encoded += *u;
        }
        else
        {
            // note that we are encoding space as %20 and not +
            // because the + should not be supported anymore
            encoded += '%';
            QString v(QString("%1").arg(*u & 255, 2, 16, QLatin1Char('0')));
            encoded += v;
        }
    }

    return encoded;
}


/** \brief Decode a URI so it can be used internally.
 *
 * This function decodes all the characters that need to be decoded
 * in a URI. In general, this is done to use URI components in a
 * query string, although it needs to be applied to the entire URI.
 *
 * The input is expected to be a valid ASCII string (i.e. A-Z,
 * 0-9, ., %, _, -, ~, and ! characters.) To enter UTF-8 characters,
 * use the % and UTF-8 encoded characters. At this point we do not
 * support the U+ syntax which MS Internet Explorer supports. It may
 * be necessary to add that support at some point.
 *
 * \exception snap_uri_exception_invalid_uri
 * This exception is raised if an invalid character is found in the
 * input URI. This means the URI includes a character that should
 * have been encoded or a %XX is not a valid hexadecimal number.
 *
 * \param[in] uri  The URI to encode.
 * \param[in] relax  Relax the syntax and accept otherwise invalid codes.
 *
 * \return The decoded URI, it may be equal to the input.
 */
QString snap_uri::urldecode(QString const & uri, bool relax)
{
    // Note that if the URI is properly encoded, then latin1 == UTF-8
    QByteArray input(uri.toUtf8());

    QByteArray utf8;
    for(char const * u(input.data()); *u != '\0'; ++u)
    {
        if(*u == '+')
        {
            utf8 += ' ';
        }
        else if(*u == '%')
        {
            ++u;
            char c;
            if(u[0] >= '0' && u[0] <= '9')
            {
                c = static_cast<char>((u[0] - '0') * 16);
            }
            else if(u[0] >= 'A' && u[0] <= 'F')
            {
                c = static_cast<char>((u[0] - ('A' - 10)) * 16);
            }
            else if(u[0] >= 'a' && u[0] <= 'f')
            {
                c = static_cast<char>((u[0] - ('a' - 10)) * 16);
            }
            else
            {
                if(!relax)
                {
//#ifdef DEBUG
//SNAP_LOG_TRACE() << "url decode?! [" << uri << "]";
//#endif
                    throw snap_uri_exception_invalid_uri(QString("urldecode(\"%1\", %2) failed because of an invalid %%xx character (digits are %3 / %4)")
                            .arg(uri)
                            .arg(relax ? "true" : "false")
                            .arg(static_cast<int>(u[0]))
                            .arg(static_cast<int>(u[1])));
                }
                // use the % as is
                utf8 += '%';
                --u;
                continue;
            }
            if(u[1] >= '0' && u[1] <= '9')
            {
                c = static_cast<char>(c + u[1] - '0');
            }
            else if(u[1] >= 'A' && u[1] <= 'F')
            {
                c = static_cast<char>(c + u[1] - ('A' - 10));
            }
            else if(u[1] >= 'a' && u[1] <= 'f')
            {
                c = static_cast<char>(c + u[1] - ('a' - 10));
            }
            else
            {
                if(!relax)
                {
//#ifdef DEBUG
//SNAP_LOG_TRACE() << "url decode?! [" << uri << "] (2)";
//#endif
                    throw snap_uri_exception_invalid_uri(QString("urldecode(\"%1\", %2) failed because of an invalid %%xx character (digits are %3 / %4)")
                                .arg(uri)
                                .arg(relax ? "true" : "false")
                                .arg(static_cast<int>(u[0]))
                                .arg(static_cast<int>(u[1])));
                }
                // use the % as is
                utf8 += c;
                --u;
                continue;
            }
            // skip one of the two characters here, the other
            // is skipped in the for() statement
            ++u;
            utf8 += c;
        }
        else if(relax

                // these are the only characters allowed by the RFC
                || (*u >= 'A' && *u <= 'Z')
                || (*u >= 'a' && *u <= 'z')
                || (*u >= '0' && *u <= '9')
                || *u == '.' || *u == '-'
                || *u == '/' || *u == '_'

                // not legal in a URI considered 100% valid but most
                // systems accept the following as is so we do too
                || *u == '~' || *u == '!'
                || *u == '@' || *u == ','
                || *u == ';' || *u == ':'
                || *u == '(' || *u == ')'
        )
        {
            // The tilde (~), when used, is often to indicate a user a la
            // Unix (~<name>/... or just ~/... for the current user.)
            //
            // The exclamation point (!) is most often used with the hash
            // bang; if that appears in a query string variable, then we
            // need to accept at least the exclamation point (the hash has
            // to be encoded no matter what.)
            //
            // The at sign (@) is used in email addresses.
            //
            // The comma (,) is often used to separate elements; for example
            // the paging support uses "page=p3,s30" for show page 3 with
            // 30 elements per page.
            //
            // The semi-colon (;) may appear if you have an HTML entity in
            // a query string (i.e. "...?value=this+%26amp;+that".)
            //
            // The colon (:) can be used to separate values within a
            // parameter when the comma is not appropriate.
            //
            utf8 += *u;
        }
        else
        {
//#ifdef DEBUG
//SNAP_LOG_TRACE() << "url decode?! found an invalid character [" << uri << "] (3)";
//#endif
            throw snap_uri_exception_invalid_uri(QString("urldecode(\"%1\", %2) failed because of an invalid character (%3)")
                            .arg(uri)
                            .arg(relax ? "true" : "false")
                            .arg(static_cast<int>(*u)));
        }
    }

    return QString::fromUtf8(utf8.data());
}


/** \brief Return the port corresponding to a protocol.
 *
 * This function determines what port corresponds to a given protocol
 * assuming that the default is being used.
 *
 * It will handle common protocols internally, others make use of the
 * /etc/services file via the services function calls.
 *
 * \param[in] protocol  The protocol to convert to a port number.
 *
 * \return The corresponding port number or -1 if the function cannot
 *         determine that number.
 */
int snap_uri::protocol_to_port(QString const & protocol)
{
    if(protocol == "http") // 99% so put it first
    {
        return 80;
    }
    if(protocol == "https") // 0.9% so put it next
    {
        return 443;
    }
    if(protocol == "ftp")
    {
        return 21;
    }
    if(protocol == "ssh")
    {
        return 22;
    }
    if(protocol == "telnet")
    {
        return 23;
    }
    if(protocol == "smtp")
    {
        return 25;
    }
    if(protocol == "gopher")
    {
        return 70;
    }

    // not a common service, ask the system... (probably less than 0.01%)
    QByteArray p(protocol.toUtf8());
    servent *s = getservbyname(p.data(), "tcp");
    if(s == nullptr)
    {
        s = getservbyname(p.data(), "udp");
        if(s == nullptr)
        {
            // we don't know...
            return -1;
        }
    }
    return s->s_port;
}




void domain_variable::read(QtSerialization::QReader& r)
{
    QtSerialization::QComposite comp;
    QtSerialization::QFieldInt32 type(comp, "domain_variable::type", f_type);
    QtSerialization::QFieldString name(comp, "domain_variable::name", f_name);
    QtSerialization::QFieldString value(comp, "domain_variable::value", f_value);
    QtSerialization::QFieldString default_value(comp, "domain_variable::default", f_default);
    QtSerialization::QFieldBasicType<bool> required(comp, "domain_variable::required", f_required);
    r.read(comp);
}


void domain_variable::write(QtSerialization::QWriter& w) const
{
    QtSerialization::QWriter::QTag tag(w, "domain_variable");
    QtSerialization::writeTag(w, "domain_variable::type", f_type);
    QtSerialization::writeTag(w, "domain_variable::name", f_name);
    QtSerialization::writeTag(w, "domain_variable::value", f_value);
    switch(f_type)
    {
    case DOMAIN_VARIABLE_TYPE_WEBSITE:
    case DOMAIN_VARIABLE_TYPE_FLAG_WITH_DEFAULT:
        QtSerialization::writeTag(w, "domain_variable::default", f_default);
        break;

    default:
        // no default value
        break;

    }
    if(f_required)
    {
        QtSerialization::writeTag(w, "domain_variable::required", f_required);
    }
}


void domain_info::read(QtSerialization::QReader& r)
{
    QtSerialization::QComposite comp;
    QtSerialization::QFieldString name(comp, "domain_info::name", f_name);
    QtSerialization::QFieldTag vars(comp, "domain_variable", this);
    r.read(comp);
}


void domain_info::readTag(const QString& name, QtSerialization::QReader& r)
{
    if(name == "domain_variable")
    {
        // create a variable with an invalid name
        QSharedPointer<domain_variable> var(new domain_variable(domain_variable::DOMAIN_VARIABLE_TYPE_STANDARD, "***", ""));
        // read the data from the reader
        var->read(r);
        // add to the variable vector
        add_var(var);
    }
}


void domain_info::write(QtSerialization::QWriter& w) const
{
    QtSerialization::QWriter::QTag tag(w, "domain_info");
    QtSerialization::writeTag(w, "domain_info::name", f_name);
    int max_vars(f_vars.size());
    for(int i(0); i < max_vars; ++i)
    {
        f_vars[i]->write(w);
    }
}


void domain_rules::read(QtSerialization::QReader& r)
{
    QtSerialization::QComposite comp;
    QtSerialization::QFieldTag rules(comp, "domain_rules", this);
    r.read(comp);
}


void domain_rules::readTag(const QString& name, QtSerialization::QReader& r)
{
    if(name == "domain_rules")
    {
        QtSerialization::QComposite comp;
        QtSerialization::QFieldTag info(comp, "domain_info", this);
        r.read(comp);
    }
    else if(name == "domain_info")
    {
        // create a variable with an invalid name
        QSharedPointer<domain_info> info(new domain_info);
        // read the data from the reader
        info->read(r);
        // add the info to the rules
        add_info(info);
    }
}


void domain_rules::write(QtSerialization::QWriter& w) const
{
    QtSerialization::QWriter::QTag tag(w, "domain_rules");
    int max_info(f_info.size());
    for(int i(0); i < max_info; ++i)
    {
        f_info[i]->write(w);
    }
}




// the following uses the parser very heavily
using namespace parser;


/** \brief Callback function executed when a qualified name is reduced.
 *
 * Concatenate the qualification and the remainder of the name and save
 * the result in the first token so it looks exactly like a non-qualified
 * name.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void domain_set_qualified_name(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<parser::token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    (*t)[0]->set_value((*n)[0]->get_value().toString() + "::" + (*t)[2]->get_value().toString());
}

/** \brief Callback function executed when a standard variable is reduced.
 *
 * This function creates a domain variable of type Standard
 * (DOMAIN_VARIABLE_TYPE_STANDARD) and save it's name and value
 * in the variable.
 *
 * The variable is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void domain_set_standard_var(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    // get the node where the qualified name is defined
    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));

    QSharedPointer<domain_variable> v(new domain_variable(domain_variable::DOMAIN_VARIABLE_TYPE_STANDARD, (*n)[0]->get_value().toString(), (*t)[2]->get_value().toString()));
    t->set_user_data(v);
}

/** \brief Callback function executed when a website variable is reduced.
 *
 * This function creates a domain variable of type Website
 * (DOMAIN_VARIABLE_TYPE_WEBSITE) and save it's name and value
 * in the variable.
 *
 * The variable is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void domain_set_website_var(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    QSharedPointer<domain_variable> v(new domain_variable(domain_variable::DOMAIN_VARIABLE_TYPE_WEBSITE, (*n)[0]->get_value().toString(), (*t)[4]->get_value().toString()));
    v->set_default((*t)[6]->get_value().toString());
    t->set_user_data(v);
}

/** \brief Callback function executed when a flag variable is reduced.
 *
 * This function creates a domain variable of type Flag
 * and save it's name and value in the variable.
 *
 * The type of the variable is set to DOMAIN_VARIABLE_TYPE_FLAG_NO_DEFAULT
 * if the flag definition does not have a second parameter.
 *
 * The type is set to DOMAIN_VARIABLE_TYPE_FLAG_WITH_DEFAULT when a default
 * is found.
 *
 * The variable is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void domain_set_flag_var(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    QSharedPointer<token_node> o(qSharedPointerDynamicCast<token_node, token>((*t)[5]));

    // if o starts with an empty token then it's the empty rule
    // (if not the empty rule, it is the comma)
    bool is_empty = (*o)[0]->get_id() == token_t::TOKEN_ID_EMPTY_ENUM;

    domain_variable::domain_variable_type_t type(is_empty ?
                  domain_variable::DOMAIN_VARIABLE_TYPE_FLAG_NO_DEFAULT
                : domain_variable::DOMAIN_VARIABLE_TYPE_FLAG_WITH_DEFAULT);
    QSharedPointer<domain_variable> v(new domain_variable(type,
            (*n)[0]->get_value().toString(), (*t)[4]->get_value().toString()));
    if(!is_empty)
    {
        // there is a default so we can access the next token
        v->set_default((*o)[1]->get_value().toString());
    }
    t->set_user_data(v);
}

/** \brief Callback function executed when a sub-domain is reduced.
 *
 * This function marks the sub-domain variable as required.
 *
 * The variable is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void domain_set_var_required(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[1]));
    QSharedPointer<domain_variable> v(qSharedPointerDynamicCast<domain_variable, parser_user_data>(n->get_user_data()));
    v->set_required();
    t->set_user_data(v);
}

/** \brief Callback function executed when a sub-domain is reduced.
 *
 * This function marks the sub-domain variable as optional.
 *
 * The variable is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void domain_set_var_optional(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[1]));
    QSharedPointer<domain_variable> v(qSharedPointerDynamicCast<domain_variable, parser_user_data>(n->get_user_data()));
    v->set_required(false);
    t->set_user_data(v);
}

/** \brief Callback function executed when a sub-domain list is reduced.
 *
 * This function creates a new domain information object and adds
 * the domain variable to it.
 *
 * The result is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void domain_set_new_domain_list(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    QSharedPointer<domain_variable> v(qSharedPointerDynamicCast<domain_variable, parser_user_data>(n->get_user_data()));
    QSharedPointer<domain_info> info(new domain_info);
    info->add_var(v);
    t->set_user_data(info);
}

/** \brief Callback function executed when a sub-domain list is reduced.
 *
 * This function adds the domain variable to an existing domain information
 * object.
 *
 * The result is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void domain_set_add_domain_list(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> nl(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    QSharedPointer<token_node> nr(qSharedPointerDynamicCast<token_node, token>((*t)[1]));
    QSharedPointer<domain_info> info(qSharedPointerDynamicCast<domain_info, parser_user_data>(nl->get_user_data()));
    QSharedPointer<domain_variable> v(qSharedPointerDynamicCast<domain_variable, parser_user_data>(nr->get_user_data()));
    info->add_var(v);
    t->set_user_data(info);
}

/** \brief Callback function executed when the rule is reduced.
 *
 * This function defines the name of the rule in the domain information
 * object.
 *
 * The result is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void domain_set_rule(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[2]));
    QSharedPointer<domain_info> info(qSharedPointerDynamicCast<domain_info, parser_user_data>(n->get_user_data()));
    info->set_name((*t)[0]->get_value().toString());
    t->set_user_data(info);
}

/** \brief Callback function executed when the rule list is reduced.
 *
 * This function creates a new domain rule object and adds the
 * domain information to it.
 *
 * The result is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void domain_set_new_rule_list(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    QSharedPointer<domain_info> info(qSharedPointerDynamicCast<domain_info, parser_user_data>(n->get_user_data()));
    QSharedPointer<domain_rules> rules(new domain_rules);
    rules->add_info(info);
    t->set_user_data(rules);
}

/** \brief Callback function executed when the rule list is reduced.
 *
 * This function adds the domain information to an existing rules
 * object.
 *
 * The result is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void domain_set_add_rule_list(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> nl(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    QSharedPointer<token_node> nr(qSharedPointerDynamicCast<token_node, token>((*t)[1]));
    QSharedPointer<domain_rules> rules(qSharedPointerDynamicCast<domain_rules, parser_user_data>(nl->get_user_data()));
    QSharedPointer<domain_info> info(qSharedPointerDynamicCast<domain_info, parser_user_data>(nr->get_user_data()));
    rules->add_info(info);
    t->set_user_data(rules);
}

/** \brief Callback function executed when the start rule is reduced.
 *
 * This function saves the result, domain_rules, in the start rule user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void domain_set_start_result(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    // we don't need the dynamic cast since we don't need to access the rules
    t->set_user_data(n->get_user_data());
}

/** \brief Parse a Domain Rule Script
 *
 * This function takes a script and parses it into a set of regular expressions
 * given a name and settings such as whether the expression is optional, has
 * a default value, etc.
 *
 * Details about the domain rule script are found here:
 * http://snapwebsites.org/implementation/basic-concept-url-website/url-test
 *
 * start: rule_list
 *
 * rule_list: rule
 *          | rule_list rule
 * rule: IDENTIFIER '{' sub_domain_list '}' ';'
 *
 * sub_domain_list: sub_domain
 *                | sub_domain_list sub_domain
 * sub_domain: OPTIONAL sub_domain_var ';'
 *           | REQUIRED sub_domain_var ';'
 * sub_domain_var: qualified_name '=' STRING
 *               | qualified_name '=' WEBSITE(STRING, STRING)
 *               | qualified_name '=' FLAG(STRING [, STRING] )
 *
 * qualified_name: IDENTIFIER
 *               | qualified_name '::' IDENTIFIER
 *
 * The following are all the post-parsing tests that we apply to the data to
 * make sure that it is valid.
 *
 * \li The variable qualified name must be "global::<name>", "domain::<name>",
 *     or "global::domain::<name>".
 * \li Each variable name must be unique within one rule, ignoring the
 *     qualification names.
 * \li A flag cannot have a default value when required.
 * \li Domain names, as defined in the rule, must be unique within the entire
 *     definition.
 * \li Variable values cannot include a regular expression that captures
 *     data or it will generate problems with our algorithms.
 * \li Variable values must be valid regular expressions (i.e. they must be
 *     compilable without errors.)
 *
 * \param[in] script  The script as written by an administrator.
 * \param[out] result  The resulting compressed script when the function
 *                     returns true.
 *
 * \return true if the parser succeeds, false otherwise.
 */
bool snap_uri_rules::parse_domain_rules(const QString& script, QByteArray& result)
{
    // LEXER

    // lexer object
    parser::lexer lexer;
    lexer.set_input(script);
    parser::keyword keyword_flag(lexer, "flag");
    parser::keyword keyword_optional(lexer, "optional");
    parser::keyword keyword_required(lexer, "required");
    parser::keyword keyword_website(lexer, "website");

    // GRAMMAR
    parser::grammar g;

    // qualified_name
    choices qualified_name(&g, "qualified_name");
    qualified_name >>= TOKEN_ID_IDENTIFIER // keep identifier as is
                     | qualified_name >> "::" >> TOKEN_ID_IDENTIFIER
                                                         >= domain_set_qualified_name
    ;

    // flag_opt_param
    choices flag_opt_param(&g, "flag_opt_param");
    flag_opt_param >>= TOKEN_ID_EMPTY // keep as is
                     | "," >> TOKEN_ID_STRING // keep as is
    ;

    // sub_domain_var
    parser::choices sub_domain_var(&g, "sub_domain_var");
    sub_domain_var >>= qualified_name >> "=" >> TOKEN_ID_STRING
                                    >= domain_set_standard_var
                     | qualified_name >> "=" >> keyword_website
                            >> "(" >> TOKEN_ID_STRING >> "," >> TOKEN_ID_STRING
                            >> ")" >= domain_set_website_var
                     | qualified_name >> "=" >> keyword_flag >>
                            "(" >> TOKEN_ID_STRING >> flag_opt_param >> ")"
                            >= domain_set_flag_var
    ;

    // sub_domain:
    parser::choices sub_domain(&g, "sub_domain");
    sub_domain >>= keyword_required >> sub_domain_var >> ";" >= domain_set_var_required
                 | keyword_optional >> sub_domain_var >> ";" >= domain_set_var_optional
    ;

    // sub_domain_list:
    parser::choices sub_domain_list(&g, "sub_domain_list");
    sub_domain_list >>= sub_domain >= domain_set_new_domain_list
                      | sub_domain_list >> sub_domain >= domain_set_add_domain_list
    ;

    // rule
    parser::choices rule(&g, "rule");
    rule >>= TOKEN_ID_IDENTIFIER >> "{" >> sub_domain_list >> "}"
                                                        >> ";" >= domain_set_rule
    ;

    // rule_list
    parser::choices rule_list(&g, "rule_list");
    rule_list >>= rule >= domain_set_new_rule_list
                | rule_list >> rule >= domain_set_add_rule_list
    ;

    // start
    parser::choices start(&g, "start");
    start >>= rule_list >= domain_set_start_result;

    if(!g.parse(lexer, start))
    {
        f_errmsg = "parsing error";
        return false;
    }

    // if we reach here, we've got a "parser valid result"
    // but there may be other problems that we check here
    //  . rule, domain name used twice
    //  . valid namespaces
    //  . flags with a default when marked as required
    //  . invalid, unacceptable regular expressions
    QSharedPointer<token_node> r(g.get_result());
    QSharedPointer<domain_rules> dr(qSharedPointerDynamicCast<domain_rules, parser_user_data>(r->get_user_data()));
    QMap<QString, int> rule_names;
    int domain_max(dr->size());
    for(int i = 0; i < domain_max; ++i)
    {
        QSharedPointer<domain_info> info((*dr)[i]);
        const QString& rule_name(info->get_name());
        if(rule_names.contains(rule_name))
        {
            // the same domain name was defined twice
            f_errmsg = "found two rules named \"" + rule_name + "\"";
            return false;
        }
        // the map value is ignored
        rule_names.insert(rule_name, 0);

        QMap<QString, int> var_names;
        int info_max(info->size());
        for(int j = 0; j < info_max; ++j)
        {
            QSharedPointer<domain_variable> var((*info)[j]);
            const QString& var_name(var->get_name());
            const snap_string_list var_qualified_names(var_name.split("::"));
            if(var_names.contains(var_qualified_names.last()))
            {
                // the same domain variable name was defined twice
                f_errmsg = "found two variables named \"" + var_name + "\"";
                return false;
            }
            var_names.insert(var_qualified_names.last(), 1);
            switch(var_qualified_names.size())
            {
            case 1:
                // just the name no extra test necessary
                break;

            case 2:
                // one qualification, can be global or domain
                if(var_qualified_names.first() != "global"
                && var_qualified_names.first() != "domain")
                {
                    // invalid qualification
                    f_errmsg = "incompatible qualified name \"" + var_qualified_names.first() + "\"";
                    return false;
                }
                break;

            case 3:
                // two qualification, must be global::domain
                if(var_qualified_names[0] != "global"
                || var_qualified_names[1] != "domain")
                {
                    // invalid qualification
                    f_errmsg = "incompatible qualified name \"" + var_qualified_names[0] + "::" + var_qualified_names[1] + "\"";
                    return false;
                }
                break;

            default:
                // invalid qualification, cannot be more than 2 names
                f_errmsg = "incompatible qualified names \"" + var_qualified_names[0] + "::" + var_qualified_names[1] + "::...\"";
                return false;

            }
            if(var->get_required()
            && var->get_type() == domain_variable::DOMAIN_VARIABLE_TYPE_FLAG_WITH_DEFAULT)
            {
                // if a flag is required, then no default can be specified
                f_errmsg = "a required flag cannot have a default value";
                return false;
            }
            // the values cannot include the delimiters since we are to
            // merge them to use them so here we take that in account
            const QString& value(var->get_value());
            QRegExp re(value);
            if(!re.isValid())
            {
                // the regular expression is not valid!?
                f_errmsg = "regular expression \"" + value + "\" is not valid";
                return false;
            }
            if(re.captureCount() != 0)
            {
                // we do not allow users to capture anything, they have to
                // use the Perl syntax: (?:<pattern>)
                f_errmsg = "regular expression \"" + value + "\" cannot include a capture (something between parenthesis)";
                return false;
            }
        }
    }

    // now we can generate the result
    //QDataStream archive(&result, QIODevice::WriteOnly);
    //archive << *dr;

    QBuffer archive(&result);
    archive.open(QIODevice::WriteOnly);
    QtSerialization::QWriter w(archive, "domain_rules", 1, 0);
    dr->write(w);

    return true;
}


void website_variable::read(QtSerialization::QReader& r)
{
    QtSerialization::QComposite comp;
    QtSerialization::QFieldInt32 type(comp, "website_variable::type", f_type);
    QtSerialization::QFieldInt32 part(comp, "website_variable::part", f_part);
    QtSerialization::QFieldString name(comp, "website_variable::name", f_name);
    QtSerialization::QFieldString value(comp, "website_variable::value", f_value);
    QtSerialization::QFieldString default_value(comp, "website_variable::default", f_default);
    QtSerialization::QFieldBasicType<bool> required(comp, "website_variable::required", f_required);
    r.read(comp);
}

void website_variable::write(QtSerialization::QWriter& w) const
{
    QtSerialization::QWriter::QTag tag(w, "website_variable");
    QtSerialization::writeTag(w, "website_variable::type", f_type);
    QtSerialization::writeTag(w, "website_variable::part", f_part);
    QtSerialization::writeTag(w, "website_variable::name", f_name);
    QtSerialization::writeTag(w, "website_variable::value", f_value);
    switch(f_type)
    {
    case WEBSITE_VARIABLE_TYPE_WEBSITE:
    case WEBSITE_VARIABLE_TYPE_FLAG_WITH_DEFAULT:
        QtSerialization::writeTag(w, "website_variable::default", f_default);
        break;

    default:
        // not default value
        break;

    }
    if(f_required)
    {
        QtSerialization::writeTag(w, "website_variable::required", f_required);
    }
}

void website_info::read(QtSerialization::QReader& r)
{
    QtSerialization::QComposite comp;
    QtSerialization::QFieldString name(comp, "website_info::name", f_name);
    QtSerialization::QFieldTag vars(comp, "website_variable", this);
    r.read(comp);
}

void website_info::readTag(const QString& name, QtSerialization::QReader& r)
{
    if(name == "website_variable")
    {
        // create a variable with an invalid name
        QSharedPointer<website_variable> var(new website_variable(website_variable::WEBSITE_VARIABLE_TYPE_STANDARD, "***", ""));
        // read the data from the reader
        var->read(r);
        // add to the variable vector
        add_var(var);
    }
}

void website_info::write(QtSerialization::QWriter& w) const
{
    QtSerialization::QWriter::QTag tag(w, "website_info");
    QtSerialization::writeTag(w, "website_info::name", f_name);
    int max_vars(f_vars.size());
    for(int i(0); i < max_vars; ++i)
    {
        f_vars[i]->write(w);
    }
}

void website_rules::read(QtSerialization::QReader& r)
{
    QtSerialization::QComposite comp;
    QtSerialization::QFieldTag info(comp, "website_rules", this);
    r.read(comp);
}

void website_rules::readTag(const QString& name, QtSerialization::QReader& r)
{
    if(name == "website_rules")
    {
        QtSerialization::QComposite comp;
        QtSerialization::QFieldTag info(comp, "website_info", this);
        r.read(comp);
    }
    else if(name == "website_info")
    {
        // create a variable with an invalid name
        QSharedPointer<website_info> info(new website_info);
        // read the data from the reader
        info->read(r);
        // add the info to the rules
        add_info(info);
    }
}

void website_rules::write(QtSerialization::QWriter& w) const
{
    QtSerialization::QWriter::QTag tag(w, "website_rules");
    int max_info(f_info.size());
    for(int i(0); i < max_info; ++i)
    {
        f_info[i]->write(w);
    }
}


/** \brief Callback function executed when a qualified name is reduced.
 *
 * Concatenate the qualification and the remainder of the name and save
 * the result in the first token so it looks exactly like a non-qualified
 * name.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_qualified_name(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<parser::token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    (*t)[0]->set_value((*n)[0]->get_value().toString() + "::" + (*t)[2]->get_value().toString());
}

/** \brief Callback function executed when a standard variable is reduced.
 *
 * This function creates a website variable of type Standard
 * (WEBSITE_VARIABLE_TYPE_STANDARD) and save it's name and value
 * in the variable.
 *
 * The variable is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_standard_var(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    // get the node where the qualified name is defined
    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));

    QSharedPointer<website_variable> v(new website_variable(website_variable::WEBSITE_VARIABLE_TYPE_STANDARD, (*n)[0]->get_value().toString(), (*t)[2]->get_value().toString()));
    t->set_user_data(v);
}

/** \brief Callback function executed when a website variable is reduced.
 *
 * This function creates a website variable of type Website
 * (WEBSITE_VARIABLE_TYPE_WEBSITE) and save it's name and value
 * in the variable.
 *
 * The variable is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_website_var(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    QSharedPointer<website_variable> v(new website_variable(website_variable::WEBSITE_VARIABLE_TYPE_WEBSITE, (*n)[0]->get_value().toString(), (*t)[4]->get_value().toString()));
    v->set_default((*t)[6]->get_value().toString());
    t->set_user_data(v);
}

/** \brief Callback function executed when a flag variable is reduced.
 *
 * This function creates a website variable of type Flag
 * and save it's name and value in the variable.
 *
 * The type of the variable is set to WEBSITE_VARIABLE_TYPE_FLAG_NO_DEFAULT
 * if the flag definition does not have a second parameter.
 *
 * The type is set to WEBSITE_VARIABLE_TYPE_FLAG_WITH_DEFAULT when a default
 * is found.
 *
 * The variable is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_flag_var(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    QSharedPointer<token_node> o(qSharedPointerDynamicCast<token_node, token>((*t)[5]));

    // if o starts with an empty token then it's the empty rule
    // (if not the empty rule, it is the comma)
    bool is_empty = (*o)[0]->get_id() == token_t::TOKEN_ID_EMPTY_ENUM;

    website_variable::website_variable_type_t type(is_empty ?
                  website_variable::WEBSITE_VARIABLE_TYPE_FLAG_NO_DEFAULT
                : website_variable::WEBSITE_VARIABLE_TYPE_FLAG_WITH_DEFAULT);
    QSharedPointer<website_variable> v(new website_variable(type,
            (*n)[0]->get_value().toString(), (*t)[4]->get_value().toString()));
    if(!is_empty)
    {
        // there is a default so we can access the next token
        v->set_default((*o)[1]->get_value().toString());
    }
    t->set_user_data(v);
}

/** \brief Callback function executed when a website is reduced.
 *
 * This function marks the website variable as required.
 *
 * The variable is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_var_required(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[1]));
    QSharedPointer<website_variable> v(qSharedPointerDynamicCast<website_variable, parser_user_data>(n->get_user_data()));
    v->set_required();
    t->set_user_data(v);
}

/** \brief Callback function executed when a website is reduced.
 *
 * This function marks the website variable as optional.
 *
 * The variable is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_var_optional(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[1]));
    QSharedPointer<website_variable> v(qSharedPointerDynamicCast<website_variable, parser_user_data>(n->get_user_data()));
    v->set_required(false);
    t->set_user_data(v);
}

/** \brief Callback function executed when a path rule is reduced.
 *
 * This function marks the website variable as a path part.
 *
 * The result is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_var_path(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    if(t->size() == 1)
    {
        QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
        QSharedPointer<website_variable> v(qSharedPointerDynamicCast<website_variable, parser_user_data>(n->get_user_data()));
        //v->set_part(website_variable::WEBSITE_VARIABLE_PART_PATH); -- this is the default anyway
        t->set_user_data(v);
    }
    else
    {
        QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[1]));
        QSharedPointer<website_variable> v(qSharedPointerDynamicCast<website_variable, parser_user_data>(n->get_user_data()));
        //v->set_part(website_variable::WEBSITE_VARIABLE_PART_PATH); -- this is the default anyway
        t->set_user_data(v);
    }
}

/** \brief Callback function executed when a port rule is reduced.
 *
 * This function marks the website variable as a port part.
 *
 * The result is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_var_port(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<website_variable> v(new website_variable(website_variable::WEBSITE_VARIABLE_TYPE_STANDARD,
            "port", (*t)[2]->get_value().toString()));
    v->set_part(website_variable::WEBSITE_VARIABLE_PART_PORT);
    v->set_required();
    t->set_user_data(v);
}

/** \brief Callback function executed when a query rule is reduced.
 *
 * This function marks the website variable as a query part.
 *
 * The result is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_var_query(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[1]));
    QSharedPointer<website_variable> v(qSharedPointerDynamicCast<website_variable, parser_user_data>(n->get_user_data()));
    v->set_part(website_variable::WEBSITE_VARIABLE_PART_QUERY);
    t->set_user_data(v);
}

/** \brief Callback function executed when a protocol rule is reduced.
 *
 * This function marks the website variable as a protocol part.
 *
 * The result is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_var_protocol(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    //QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[1]));
    QSharedPointer<website_variable> v(new website_variable(website_variable::WEBSITE_VARIABLE_TYPE_STANDARD,
            "protocol", (*t)[2]->get_value().toString()));
    v->set_part(website_variable::WEBSITE_VARIABLE_PART_PROTOCOL);
    v->set_required();
    t->set_user_data(v);
}

/** \brief Callback function executed when the website rule is reduced.
 *
 * This function saves the result, website_rule, in the token user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_variable_rule(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    // we don't need the dynamic cast since we don't need to access the variables
    t->set_user_data(n->get_user_data());
}

/** \brief Callback function executed when a website list is reduced.
 *
 * This function creates a new website information object and adds
 * the website variable to it.
 *
 * The result is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_new_website_list(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    QSharedPointer<website_variable> v(qSharedPointerDynamicCast<website_variable, parser_user_data>(n->get_user_data()));
    QSharedPointer<website_info> info(new website_info);
    info->add_var(v);
    t->set_user_data(info);
}

/** \brief Callback function executed when a website list is reduced.
 *
 * This function adds the website variable to an existing website information
 * object.
 *
 * The result is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_add_website_list(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> nl(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    QSharedPointer<token_node> nr(qSharedPointerDynamicCast<token_node, token>((*t)[1]));
    QSharedPointer<website_info> info(qSharedPointerDynamicCast<website_info, parser_user_data>(nl->get_user_data()));
    QSharedPointer<website_variable> v(qSharedPointerDynamicCast<website_variable, parser_user_data>(nr->get_user_data()));
    info->add_var(v);
    t->set_user_data(info);
}

/** \brief Callback function executed when the rule is reduced.
 *
 * This function defines the name of the rule in the website information
 * object.
 *
 * The result is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_rule(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[2]));
    QSharedPointer<website_info> info(qSharedPointerDynamicCast<website_info, parser_user_data>(n->get_user_data()));
    info->set_name((*t)[0]->get_value().toString());
    t->set_user_data(info);
}

/** \brief Callback function executed when the rule list is reduced.
 *
 * This function creates a new website rule object and adds the
 * website information to it.
 *
 * The result is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_new_rule_list(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    QSharedPointer<website_info> info(qSharedPointerDynamicCast<website_info, parser_user_data>(n->get_user_data()));
    QSharedPointer<website_rules> rules(new website_rules);
    rules->add_info(info);
    t->set_user_data(rules);
}

/** \brief Callback function executed when the rule list is reduced.
 *
 * This function adds the website information to an existing rules
 * object.
 *
 * The result is then saved in the node as user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_add_rule_list(const rule& r, QSharedPointer<token_node>& t)
{
    NOTUSED(r);

    QSharedPointer<token_node> nl(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    QSharedPointer<token_node> nr(qSharedPointerDynamicCast<token_node, token>((*t)[1]));
    QSharedPointer<website_rules> rules(qSharedPointerDynamicCast<website_rules, parser_user_data>(nl->get_user_data()));
    QSharedPointer<website_info> info(qSharedPointerDynamicCast<website_info, parser_user_data>(nr->get_user_data()));
    rules->add_info(info);
    t->set_user_data(rules);
}

/** \brief Callback function executed when the start rule is reduced.
 *
 * This function saves the result, website_rules, in the start rule user data.
 *
 * \param[in] r  The rule that generated the callback.
 * \param[in] t  The token node holding the data parsed so far.
 */
void website_set_start_result(rule const & r, QSharedPointer<token_node> & t)
{
    NOTUSED(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    // we don't need the dynamic cast since we don't need to access the rules
    t->set_user_data(n->get_user_data());
}


/** \brief Parse a Website Rule Script
 *
 * This function takes a script and parses it into a set of regular expressions
 * given a name and settings such as whether the expression is optional, has
 * a default value, etc.
 *
 * Details about the website rule script are found here:
 * http://snapwebsites.org/implementation/basic-concept-url-website/url-test
 *
 * \code
 * start: rule_list
 *
 * rule_list: rule
 *          | rule_list rule
 * rule: IDENTIFIER '{' website_rule_list '}' ';'
 *
 * website_rule_list: website_rule
 *                  | website_rule_list website_rule
 * website_rule: protocol_rule ';'
 *             | port_rule ';'
 *             | path_rule ';'
 *             | query_rule ';'
 *
 * protocol_rule: PROTOCOL '=' STRING
 *
 * port_rule: PORT '=' STRING
 *
 * path_rule: PATH website
 *          | website
 *
 * query_rule: QUERY website
 *
 * website: OPTIONAL website_var ';'
 *        | REQUIRED website_var ';'
 * website_var: qualified_name '=' STRING
 *            | qualified_name '=' WEBSITE(STRING, STRING)
 *            | qualified_name '=' FLAG(STRING [, STRING] )
 *
 * qualified_name: IDENTIFIER
 *               | qualified_name '::' IDENTIFIER
 * \endcode
 *
 * The following are all the post-parsing tests that we apply to the data to
 * make sure that it is valid.
 *
 * \li The variable qualified name must be "global::<name>", "site::<name>",
 *     or "global::site::<name>".
 * \li Each variable name must be unique within one rule, ignoring the
 *     qualification names.
 * \li A flag cannot have a default value when required.
 * \li Domain names, as defined in the rule, must be unique within the entire
 *     definition.
 * \li Variable values cannot include a regular expression that captures
 *     data or it will generate problems with our algorithms.
 * \li Variable values must be valid regular expressions (i.e. they must be
 *     compilable without errors.)
 *
 * \param[in] script  The script as written by an administrator.
 * \param[out] result  The resulting compressed script when the function
 *                     returns true.
 *
 * \return true if the parser succeeds, false otherwise.
 */
bool snap_uri_rules::parse_website_rules(QString const& script, QByteArray& result)
{
    // LEXER

    // lexer object
    parser::lexer lexer;
    lexer.set_input(script);
    parser::keyword keyword_flag(lexer, "flag");
    parser::keyword keyword_optional(lexer, "optional");
    parser::keyword keyword_path(lexer, "path");
    parser::keyword keyword_port(lexer, "port");
    parser::keyword keyword_protocol(lexer, "protocol");
    parser::keyword keyword_query(lexer, "query");
    parser::keyword keyword_required(lexer, "required");
    parser::keyword keyword_website(lexer, "website");

    // GRAMMAR
    parser::grammar g;

    // qualified_name
    choices qualified_name(&g, "qualified_name");
    qualified_name >>= TOKEN_ID_IDENTIFIER // keep identifier as is
                     | qualified_name >> "::" >> TOKEN_ID_IDENTIFIER
                                                         >= website_set_qualified_name
    ;

    // flag_opt_param
    choices flag_opt_param(&g, "flag_opt_param");
    flag_opt_param >>= TOKEN_ID_EMPTY // keep as is
                     | "," >> TOKEN_ID_STRING // keep as is
    ;

    // website_var
    parser::choices website_var(&g, "website_var");
    website_var >>= qualified_name >> "=" >> TOKEN_ID_STRING
                                    >= website_set_standard_var
                  | qualified_name >> "=" >> keyword_website
                            >> "(" >> TOKEN_ID_STRING >> "," >> TOKEN_ID_STRING
                            >> ")" >= website_set_website_var
                  | qualified_name >> "=" >> keyword_flag >>
                            "(" >> TOKEN_ID_STRING >> flag_opt_param >> ")"
                            >= website_set_flag_var
    ;

    // website:
    parser::choices website(&g, "website");
    website >>= keyword_required >> website_var >= website_set_var_required
              | keyword_optional >> website_var >= website_set_var_optional
    ;

    // query_rule:
    parser::choices query_rule(&g, "query_rule");
    query_rule >>= keyword_query >> website >= website_set_var_query
    ;

    // path_rule:
    parser::choices path_rule(&g, "path_rule");
    path_rule >>= keyword_path >> website >= website_set_var_path
                | website >= website_set_var_path
    ;

    // port_rule:
    parser::choices port_rule(&g, "port_rule");
    port_rule >>= keyword_port >> "=" >> TOKEN_ID_STRING >= website_set_var_port
    ;

    // protocol_rule:
    parser::choices protocol_rule(&g, "protocol_rule");
    protocol_rule >>= keyword_protocol >> "=" >> TOKEN_ID_STRING >= website_set_var_protocol
    ;

    // website_rule:
    parser::choices website_rule(&g, "website_rule");
    website_rule >>= protocol_rule >> ";" >= website_set_variable_rule
                   | port_rule >> ";" >= website_set_variable_rule
                   | path_rule >> ";" >= website_set_variable_rule
                   | query_rule >> ";" >= website_set_variable_rule
    ;

    // website_rule_list:
    parser::choices website_rule_list(&g, "website_rule_list");
    website_rule_list >>= website_rule >= website_set_new_website_list
                        | website_rule_list >> website_rule >= website_set_add_website_list
    ;

    // rule
    parser::choices rule(&g, "rule");
    rule >>= TOKEN_ID_IDENTIFIER >> "{" >> website_rule_list >> "}"
                                                        >> ";" >= website_set_rule
    ;

    // rule_list
    parser::choices rule_list(&g, "rule_list");
    rule_list >>= rule >= website_set_new_rule_list
                | rule_list >> rule >= website_set_add_rule_list
    ;

    // start
    parser::choices start(&g, "start");
    start >>= rule_list >= website_set_start_result;

    if(!g.parse(lexer, start))
    {
        f_errmsg = "parsing error";
        return false;
    }

    // if we reach here, we've got a "parser valid result"
    // but there may be other problems that we check here
    //  . rule, website name used twice
    //  . valid namespaces
    //  . flags with a default when marked as required
    //  . invalid, unacceptable regular expressions
    QSharedPointer<token_node> r(g.get_result());
    QSharedPointer<website_rules> ws(qSharedPointerDynamicCast<website_rules, parser_user_data>(r->get_user_data()));
    QMap<QString, int> rule_names;
    int website_max(ws->size());
    for(int i = 0; i < website_max; ++i)
    {
        QSharedPointer<website_info> info((*ws)[i]);
        const QString& rule_name(info->get_name());
        if(rule_names.contains(rule_name))
        {
            // the same website name was defined twice
            f_errmsg = "found two rules named \"" + rule_name + "\"";
            return false;
        }
        // the map value is ignored
        rule_names.insert(rule_name, 0);

        QMap<QString, int> var_names;
        int info_max(info->size());
        for(int j = 0; j < info_max; ++j) {
            QSharedPointer<website_variable> var((*info)[j]);
            const QString& var_name(var->get_name());
            const snap_string_list var_qualified_names(var_name.split("::"));
            if(var_names.contains(var_qualified_names.last()))
            {
                // the same website variable name was defined twice
                f_errmsg = "found two variables named \"" + var_name + "\"";
                return false;
            }
            var_names.insert(var_qualified_names.last(), 1);
            switch(var_qualified_names.size())
            {
            case 1:
                // just the name no extra test necessary
                break;

            case 2:
                // one qualification, can be global or site
                if(var_qualified_names.first() != "global"
                && var_qualified_names.first() != "site") {
                    // invalid qualification
                    f_errmsg = "incompatible qualified name \"" + var_qualified_names.first() + "\"";
                    return false;
                }
                break;

            case 3:
                // two qualification, must be global::site
                if(var_qualified_names[0] != "global"
                || var_qualified_names[1] != "site") {
                    // invalid qualification
                    f_errmsg = "incompatible qualified name \"" + var_qualified_names[0] + "::" + var_qualified_names[1] + "\"";
                    return false;
                }
                break;

            default:
                // invalid qualification, cannot be more than 2 names
                f_errmsg = "incompatible qualified names \"" + var_qualified_names[0] + "::" + var_qualified_names[1] + "::...\"";
                return false;

            }
            if(var->get_required()
            && var->get_type() == website_variable::WEBSITE_VARIABLE_TYPE_FLAG_WITH_DEFAULT)
            {
                // if a flag is required, then no default can be specified
                f_errmsg = "a required flag cannot have a default value";
                return false;
            }
            // the values cannot include the delimiters since we are to
            // merge them to use them so here we take that in account
            const QString& value(var->get_value());
            QRegExp re(value);
            if(!re.isValid())
            {
                // the regular expression is not valid!?
                f_errmsg = "regular expression \"" + value + "\" is not valid";
                return false;
            }
            if(re.captureCount() != 0)
            {
                // we do not allow users to capture anything, they have to
                // use the Perl syntax: (?:<pattern>)
                f_errmsg = "regular expression \"" + value + "\" cannot include a capture (something between parenthesis without the ?: at the start)";
                return false;
            }
        }
    }

    // now we can generate the result
    //QDataStream archive(&result, QIODevice::WriteOnly);
    //archive << *ws;

    QBuffer archive(&result);
    archive.open(QIODevice::WriteOnly);
    QtSerialization::QWriter w(archive, "website_rules", 1, 0);
    ws->write(w);

    return true;
}




} // namespace snap

// vim: ts=4 sw=4 et
